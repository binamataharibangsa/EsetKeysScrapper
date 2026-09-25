#pragma once

#include "Eset/Eset.h"
#include "LicenseRecord.h"
#include "ProxyReader.h"
#include "TempMail/TempMail.h"

#include <memory>
#include <string>
#include <vector>

/// Orchestrates a run: creates the throwaway mailboxes, registers one ESET
/// account per mailbox, waits for the confirmation mail and collects the
/// resulting licence keys.
///
/// Results are exposed as `LicenseRecord` values rather than `Eset` objects,
/// because an `Eset` owns a libcurl handle: the original returned
/// `vector<Eset>` by value, so every element copy produced a second owner of
/// the same handle and a double free at exit.
class LicenseManager {
public:
  /// `domainLength` is clamped to the bounds in `config`.
  LicenseManager(int numLicenses, int domainLength, const std::string &proxyFile);

  /// Runs the whole pipeline. Never throws for an individual account: a
  /// failure is logged and the next account is attempted.
  void generateLicenses();

  /// Successful results, in the order they were produced.
  const std::vector<LicenseRecord> &licenses() const noexcept {
    return licenses_;
  }

  /// Prints every result and copies the first licence key to the clipboard.
  void showAllLicenses() const;

private:
  /// Creates `numLicenses_` mailboxes, skipping any that cannot be registered.
  void createMailboxes();

  /// Registers, confirms and mints a licence for one mailbox.
  /// Returns true when a licence key was obtained.
  bool processMailbox(TempMail &mailbox);

  /// Polls the mailbox until the ESET confirmation mail arrives and has been
  /// followed. Returns false once the attempt budget is exhausted.
  bool waitForAccountActivation(TempMail &mailbox, Eset &eset);

  /// True when `message` is the ESET confirmation mail and the confirmation
  /// succeeded.
  bool confirmIfEsetMail(TempMail &mailbox, Eset &eset,
                         const Message &message);

  /// Picks a working proxy, or nullptr to go direct.
  ///
  /// Returns a pointer into `proxyReader_`, so the result is valid for the
  /// lifetime of this manager.
  const Proxy *acquireProxy();

  /// Rewrites `config::kDataFileName` with one "mail,license" line per result.
  void saveResults() const;

  int numLicenses_ = 1;
  int domainLength_ = 15;
  bool useProxies_ = false;

  ProxyReader proxyReader_;
  std::vector<std::unique_ptr<TempMail>> mailboxes_;
  std::vector<LicenseRecord> licenses_;
};
