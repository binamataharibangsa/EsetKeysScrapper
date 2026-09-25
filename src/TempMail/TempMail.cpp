#include "TempMail.h"

#include "../Core/Config.h"
#include "../Helpers/Crypto.h"
#include "../Helpers/Paths.h"
#include "../I18n/I18n.h"
#include "Message.h"

#include <fstream>
#include <random>
#include <string>

namespace {

/// Characters allowed in the local part of a generated address.
const char *kAddressAlphabet = "abcdefghijklmnopqrstuvwxyz0123456789";

std::string url(const std::string &path) {
  return std::string(config::kMailApiBaseUrl) + path;
}

} // namespace

TempMail::TempMail(int addressLength) : addressLength_(addressLength) {
  setHeaders();

  // The mailbox is registered eagerly, as before, but failures now surface as
  // an invalid object instead of an unbounded retry chain inside the
  // constructor.
  for (int attempt = 0; attempt < config::kMaxAddressGenerationAttempts;
       ++attempt) {
    const std::string address = generateAddress();
    if (address.empty()) {
      waitForRequest();
      continue;
    }
    if (createAccount(address) && requestToken()) {
      email_ = address;
      return;
    }
    waitForRequest();
  }
}

void TempMail::waitForRequest() const {
  std::this_thread::sleep_for(
      std::chrono::milliseconds(config::kRequestThrottleMs));
}

std::string TempMail::generateAddress() {
  const std::string domain = fetchDomain();
  if (domain.empty()) {
    return {};
  }

  // Drawn from the shared Crypto engine instead of a freshly seeded rand(),
  // which produced the same address for every mailbox created in the same
  // second.
  const std::string alphabet(kAddressAlphabet);
  std::uniform_int_distribution<std::size_t> pick(0, alphabet.size() - 1);

  std::string localPart;
  localPart.reserve(static_cast<std::size_t>(addressLength_));
  for (int i = 0; i < addressLength_; ++i) {
    localPart += alphabet[pick(Crypto::randomEngine())];
  }

  const std::string address = localPart + "@" + domain;
  if (isKnownRejectedAddress(address)) {
    return {};
  }
  return address;
}

bool TempMail::createAccount(const std::string &address) {
  response.clear();

  const json payload = {{"address", address},
                        {"password", config::kMailPassword}};

  curl_easy_setopt(curl_, CURLOPT_URL, url("/accounts").c_str());
  curl_easy_setopt(curl_, CURLOPT_POST, 1L);
  curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, payload.dump().c_str());

  if (perform() != CURLE_OK) {
    return false;
  }

  // A rejected address ("violations") is remembered so the next run does not
  // waste a request on it again.
  if (response.find("violations") != std::string::npos) {
    rememberRejectedAddress(address);
    return false;
  }

  const json answer = json::parse(response, nullptr, false);
  if (answer.is_discarded()) {
    return false;
  }

  email_ = answer.value("address", address);
  id_ = answer.value("id", std::string());

  std::cout << YELLOW << i18n::tr(i18n::Key::TempMailCreated) << std::endl
            << GREEN << i18n::tr(i18n::Key::LabelEmail) << RESET << email_
            << std::endl
            << GREEN << i18n::tr(i18n::Key::LabelId) << RESET << id_
            << std::endl;

  return true;
}

bool TempMail::requestToken() {
  response.clear();

  const json payload = {{"address", email_},
                        {"password", config::kMailPassword}};

  curl_easy_setopt(curl_, CURLOPT_URL, url("/token").c_str());
  curl_easy_setopt(curl_, CURLOPT_POST, 1L);
  curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, payload.dump().c_str());

  if (perform() != CURLE_OK || response.find("token") == std::string::npos) {
    return false;
  }

  token = json::parse(response, nullptr, false).value("token", std::string());
  if (token.empty()) {
    return false;
  }

  setHeaders();
  return true;
}

std::string TempMail::fetchDomain() {
  response.clear();
  responseHeaders.clear();

  curl_easy_setopt(curl_, CURLOPT_URL, url("/domains").c_str());
  curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);

  if (perform() != CURLE_OK || response.find("domain") == std::string::npos) {
    return {};
  }

  const json domains = json::parse(response, nullptr, false);
  if (domains.is_discarded() || !domains.is_array() || domains.empty()) {
    return {};
  }

  return domains[0].value("domain", std::string());
}

bool TempMail::getMessages() {
  response.clear();

  curl_easy_setopt(curl_, CURLOPT_URL,
                   url(config::kMailMessagesPath).c_str());
  curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);

  if (perform() != CURLE_OK) {
    return false;
  }

  const json answer = json::parse(response, nullptr, false);
  if (answer.is_discarded() || !answer.is_array()) {
    return false;
  }

  for (const json &item : answer) {
    messages_.push_back({item.value("id", std::string()),
                         item["from"].value("address", std::string()),
                         item.value("subject", std::string()),
                         item.value("intro", std::string()), false});
  }

  return true;
}

Message TempMail::readMessage(const std::string &id) {
  response.clear();

  curl_easy_setopt(curl_, CURLOPT_URL,
                   url(std::string("/messages/") + id).c_str());
  curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);

  if (perform() != CURLE_OK) {
    return {};
  }

  const json answer = json::parse(response, nullptr, false);
  if (answer.is_discarded()) {
    return {};
  }

  // Update the cached entry so a re-read does not refetch, then return the
  // full text. The original returned a *copy* while mutating the cache
  // separately, so callers had to be careful which one they read.
  for (Message &message : messages_) {
    if (message.id == id) {
      message.read = true;
      message.body = answer.value("text", std::string());
      return message;
    }
  }

  return {};
}

bool TempMail::isKnownRejectedAddress(const std::string &address) {
  std::ifstream file(dataFile(config::kExistingAddressesFile));
  if (!file.is_open()) {
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    if (line == address) {
      return true;
    }
  }
  return false;
}

void TempMail::rememberRejectedAddress(const std::string &address) {
  std::ofstream file(dataFile(config::kExistingAddressesFile),
                     std::ios::app);
  if (file.is_open()) {
    file << address << std::endl;
  }
}

void TempMail::setHeaders() {
  if (curl_ == nullptr) {
    return;
  }

  clearHeaders();
  appendHeader(std::string("Host: ") + config::kMailApiHost);
  appendHeader("Accept: application/json");
  appendHeader("Accept-Charset: UTF-8");
  appendHeader("Accept-Language: en-US,en;q=0.5");
  appendHeader("Referer: https://mail.tm/");
  appendHeader("Content-Type: application/json; charset=UTF-8");
  appendHeader("Origin: https://mail.tm");

  Scrapper::setHeaders();
  curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
}
