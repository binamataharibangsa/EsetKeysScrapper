#pragma once

#include <string>

/// One PKCE code pair plus the authorization URL built from it.
///
/// The flow (RFC 7636) is: generate a random `code_verifier`, derive
/// `code_challenge = BASE64URL(SHA256(verifier))`, send the challenge when
/// asking for the authorization code, and send the verifier when exchanging
/// that code for a token.
///
/// Each Eset account needs its own instance: `getAuthorizationUrl()` mints a
/// fresh verifier on every call, and the verifier that produced a given
/// authorization code is the only one the token endpoint will accept.
class Pkce {
public:
  Pkce();

  /// Mints a new verifier/challenge pair and returns the authorization URL.
  ///
  /// Regenerating here is deliberate -- the URL and the challenge must change
  /// together -- but callers must therefore call it exactly once per login
  /// attempt and reuse the same instance for the subsequent
  /// `getCodeVerifier()`.
  std::string getAuthorizationUrl();

  const std::string &state() const noexcept { return state_; }
  const std::string &codeVerifier() const noexcept { return codeVerifier_; }
  const std::string &codeChallenge() const noexcept { return codeChallenge_; }

private:
  void regenerate();

  std::string state_;
  std::string codeVerifier_;
  std::string codeChallenge_;
};
