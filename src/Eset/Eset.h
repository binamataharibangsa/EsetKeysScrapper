#pragma once

#include "../Helpers/Console.h"
#include "../Proxy.h"
#include "../Scrapper.h"
#include "Pkce.h"

#include <string>

/// Drives one ESET account end to end: register it, confirm the e-mail, log
/// in, activate a trial and read back the licence key.
///
/// One instance owns one libcurl handle and one cookie jar, and is not
/// copyable -- copying it duplicated the handle and produced a double free.
class Eset : public Scrapper {
public:
  /// `proxy` is borrowed and must outlive this object; pass nullptr for a
  /// direct connection.
  explicit Eset(const std::string &mail, const Proxy *proxy = nullptr);

  /// Registers the account and triggers the confirmation e-mail.
  bool createAccount();

  /// Follows the verification link found in `body` (the raw e-mail text).
  bool confirmRegistration(const std::string &body);

  /// Logs in (if needed), activates the trial and stores the licence key.
  bool getLicense();

  const std::string &mail() const noexcept { return mail_; }
  const std::string &license() const noexcept { return license_; }

private:
  // --- Session steps -------------------------------------------------------

  /// Exchanges credentials for a bearer token via the PKCE flow.
  bool login();
  /// Requests a trial licence for `config::kTrialProductCode`.
  bool activateLicense();
  /// Reads back the licence key minted by `activateLicense()`.
  bool fetchLicenseKey();
  /// Fetches the authorization code and, with it, the access token.
  bool resolveTokenFromRedirect(const std::string &loginResponse);

  // --- HTTP helpers --------------------------------------------------------

  /// Adds the *standard* login.eset.com header set.
  ///
  /// `forAccountCreation` selects the Referer that the registration flow
  /// expects; the two flows are otherwise identical.
  void useLoginHeaders(bool forAccountCreation);

  /// Replaces the header list with exactly the given entries.
  ///
  /// Taking the whole set at once -- rather than mutating a shared list and
  /// hiding it inside a function like the original did -- is what makes the
  /// "did we free the previous list?" question disappear.
  void useHeaders(const std::vector<std::string> &lines);

  // --- Parsing -------------------------------------------------------------

  /// Extracts the bracketed URL from a verification e-mail body.
  static std::string extractVerificationLink(const std::string &body);
  /// Value of `name` in a `location:` response header.
  static std::string locationHeader(const std::string &headers);
  /// Value of the `parameter` query-string entry of `url`.
  static std::string queryParameter(const std::string &url,
                                    const std::string &parameter);

  /// Runs the redirect URL from a login response and returns the headers of
  /// the final response (which carry the `location:` with the auth code).
  std::string followPkceRedirect(const std::string &loginResponse);

  std::string mail_;
  std::string license_;
  const Proxy *proxy_ = nullptr;
  Pkce pkce_;
};
