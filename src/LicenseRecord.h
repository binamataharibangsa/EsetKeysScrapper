#pragma once

#include <string>

/// One successfully generated account: the mailbox it was registered with and
/// the trial licence key minted from it.
///
/// This is deliberately a plain value type rather than the scraper object that
/// produced it. The original code handed its `Eset` objects (which each own a
/// libcurl handle) straight to the UI inside a `std::vector`, so every copy --
/// including the ones `std::vector` makes when it grows -- produced a second
/// owner of the same handle and a double free on destruction.
struct LicenseRecord {
  std::string mail;
  std::string license;

  /// True when neither field carries data, i.e. the account never completed.
  bool isEmpty() const noexcept {
    return mail.empty() && license.empty();
  }
};
