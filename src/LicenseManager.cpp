#include "LicenseManager.h"

#include "Core/Config.h"
#include "Helpers/Clipboard.h"
#include "Helpers/Console.h"
#include "Helpers/Paths.h"
#include "I18n/I18n.h"

#include <chrono>
#include <fstream>
#include <memory>
#include <string>
#include <thread>

namespace {

/// Clamps `value` into [low, high].
int clamp(int value, int low, int high) {
  if (value < low) {
    return low;
  }
  return value > high ? high : value;
}

} // namespace

LicenseManager::LicenseManager(int numLicenses, int domainLength,
                               const std::string &proxyFile)
    : numLicenses_(clamp(numLicenses, config::kMinLicenseCount, 1'000'000)),
      domainLength_(clamp(domainLength, config::kMinDomainLength,
                          config::kMaxDomainLength)) {
  if (proxyFile.empty()) {
    return;
  }

  std::cout << GREEN << "---" << RESET << std::endl
            << GREEN << i18n::tr(i18n::Key::LoadingProxyFile) << RESET
            << std::endl
            << YELLOW << proxyFile << RESET << std::endl
            << GREEN << "---" << RESET << std::endl
            << std::endl;

  useProxies_ = proxyReader_.readProxies(proxyFile) > 0;
}

void LicenseManager::generateLicenses() {
  createMailboxes();

  if (useProxies_) {
    std::cout << std::endl
              << YELLOW << i18n::tr(i18n::Key::UsingProxies) << std::endl
              << i18n::tr(i18n::Key::UsingProxiesLatencyWarning) << std::endl
              << "--- ---" << std::endl;
  }

  for (std::unique_ptr<TempMail> &mailbox : mailboxes_) {
    if (!mailbox->isValid()) {
      std::cout << RED << i18n::tr(i18n::Key::ErrorCreatingAccount) << RESET
                << std::endl;
      continue;
    }
    processMailbox(*mailbox);
  }

  saveResults();
}

void LicenseManager::createMailboxes() {
  mailboxes_.reserve(static_cast<std::size_t>(numLicenses_));
  for (int i = 0; i < numLicenses_; ++i) {
    // Registration performs network I/O and retries internally, so a mailbox
    // that cannot be created is reported by an invalid object rather than an
    // exception; processMailbox() skips those.
    mailboxes_.push_back(std::make_unique<TempMail>(domainLength_));
  }
}

bool LicenseManager::processMailbox(TempMail &mailbox) {
  const Proxy *proxy = acquireProxy();
  Eset eset(mailbox.email(), proxy);

  int attempts = 0;
  while (!eset.createAccount() &&
         ++attempts < config::kMaxAccountCreationAttempts) {
    std::cout << YELLOW << i18n::tr(i18n::Key::RetryingCreateAccount) << RESET
              << std::endl;
  }

  // The original compared the counter with `>=` against a `while (… < 5)`
  // loop, so a failure after exactly the budget was silently treated as a
  // success and the run continued with an unregistered account.
  if (attempts >= config::kMaxAccountCreationAttempts) {
    std::cout << RED << i18n::tr(i18n::Key::ErrorCreatingAccount) << RESET
              << std::endl;
    return false;
  }

  std::this_thread::sleep_for(
      std::chrono::milliseconds(config::kActivationPollDelayMs));

  std::cout << std::endl
            << YELLOW << i18n::tr(i18n::Key::WaitingForVerificationCode)
            << std::endl;

  if (!waitForAccountActivation(mailbox, eset)) {
    return false;
  }

  attempts = 0;
  while (!eset.getLicense() &&
         ++attempts < config::kMaxLicenseAttempts) {
    std::cout << YELLOW << i18n::tr(i18n::Key::RetryingGetLicense)
              << std::endl;
  }

  if (attempts >= config::kMaxLicenseAttempts) {
    std::cout << RED << i18n::tr(i18n::Key::ErrorGettingLicense) << RESET
              << std::endl;
    return false;
  }

  licenses_.push_back({eset.mail(), eset.license()});
  return true;
}

bool LicenseManager::waitForAccountActivation(TempMail &mailbox, Eset &eset) {
  // Bounded, unlike the original `do { … } while (!activated)`, which spun
  // forever whenever the confirmation mail never arrived.
  for (int attempt = 0; attempt < config::kMaxActivationAttempts; ++attempt) {
    mailbox.getMessages();
    for (const Message &message : mailbox.messages()) {
      if (confirmIfEsetMail(mailbox, eset, message)) {
        return true;
      }
    }
    std::this_thread::sleep_for(
        std::chrono::milliseconds(config::kActivationPollDelayMs));
  }

  std::cout << RED << i18n::tr(i18n::Key::ErrorCreatingAccount) << RESET
            << std::endl;
  return false;
}

bool LicenseManager::confirmIfEsetMail(TempMail &mailbox, Eset &eset,
                                       const Message &message) {
  if (message.from != config::kConfirmationSender ||
      message.subject != config::kConfirmationSubject) {
    return false;
  }

  const Message full = mailbox.readMessage(message.id);
  if (full.body.empty()) {
    return false;
  }

  return eset.confirmRegistration(full.body);
}

const Proxy *LicenseManager::acquireProxy() {
  if (!useProxies_) {
    return nullptr;
  }

  // Hand out proxies until one answers the liveness probe.
  //
  // The original loop ran `do { proxy = giveNext(); } while (proxy.isWorking())`
  // -- it discarded the proxies that worked and stopped at the first that did
  // not.
  static Proxy candidate;
  for (int attempt = 0; attempt < config::kMaxProxyAttempts; ++attempt) {
    candidate = proxyReader_.giveNext();
    if (!candidate.isValid()) {
      std::cout << YELLOW << i18n::tr(i18n::Key::LeavingProgram) << RESET
                << std::endl;
      return nullptr;
    }
    if (candidate.isWorking()) {
      return &candidate;
    }
  }

  std::cout << RED << i18n::tr(i18n::Key::NoWorkingProxies) << RESET
            << std::endl;
  return nullptr;
}

void LicenseManager::saveResults() const {
  const std::string path = dataFile(config::kDataFileName);
  std::ofstream file(path);

  if (!file.is_open()) {
    std::cout << RED << i18n::tr(i18n::Key::FailedToOpenFile) << YELLOW << path
              << RESET << std::endl;
    return;
  }

  for (const LicenseRecord &record : licenses_) {
    // Skip incomplete accounts. The original used `||` here, so it wrote a
    // line only for the records that had *both* fields empty and dropped
    // every real result.
    if (record.isEmpty()) {
      continue;
    }
    file << record.mail << "," << record.license << std::endl;
  }

  std::cout << std::endl
            << GREEN << i18n::tr(i18n::Key::DataSavedTo) << RESET << path
            << std::endl;
}

void LicenseManager::showAllLicenses() const {
  if (licenses_.empty()) {
    std::cout << RED << i18n::tr(i18n::Key::NoLicenseWasGenerated) << RESET
              << std::endl;
    return;
  }

  for (const LicenseRecord &record : licenses_) {
    std::cout << GREEN << i18n::tr(i18n::Key::LabelMail) << RESET << record.mail
              << std::endl
              << GREEN << i18n::tr(i18n::Key::LabelLicense) << RESET
              << record.license << std::endl
              << std::endl;
  }

  copyToClipboard(licenses_.front().license);
}
