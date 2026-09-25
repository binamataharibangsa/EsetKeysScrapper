#include "Pkce.h"

#include "Core/Config.h"
#include "Helpers/Crypto.h"

#include <string>

namespace {

/// Lengths from RFC 7636: the verifier must be 43-128 characters, and the
/// state is only required to be unguessable.
constexpr int kStateLength = 32;
constexpr int kRandomStringLength = 56;

} // namespace

Pkce::Pkce() { regenerate(); }

std::string Pkce::getAuthorizationUrl() {
  regenerate();

  // Assembled from a single concatenation so the query string reads the way it
  // appears on the wire; the original split it mid-parameter ("code_" +
  // "challenge"), which made the URL impossible to check by eye.
  return std::string("/connect/authorize/callback") +
         "?client_id=" + config::kPkceClientId +
         "&redirect_uri=" + config::kRegistrationCallbackUrl +
         "&response_type=code" +
         "&scope=openid mecac myesetapi" +
         "&state=" + state_ +
         "&code_challenge=" + codeChallenge_ +
         "&code_challenge_method=S256" +
         "&response_mode=query";
}

void Pkce::regenerate() {
  // A fresh random string per pair. The original generated one string in the
  // constructor and re-encoded that same string on every regenerate(), so the
  // verifier never actually changed between logins.
  const std::string verifierSource =
      Crypto::generateRandomString(kRandomStringLength);

  state_ = Crypto::generateRandomString(kStateLength, false);
  codeVerifier_ = Crypto::base64UrlEncode(verifierSource);
  codeChallenge_ = Crypto::sha256Base64Url(codeVerifier_);
}
