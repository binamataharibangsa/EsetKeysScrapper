#pragma once

#include <random>
#include <string>

/// Small wrappers around the OpenSSL primitives the PKCE flow needs.
///
/// This header used to be the project's de-facto "include everything" header:
/// every translation unit that touched a string ended up pulling in libcurl,
/// nlohmann/json, `<regex>`, `<thread>` and `using namespace std` along with
/// it. It now declares only what it owns, and includes only what it uses.
namespace Crypto {

/// Process-wide Mersenne Twister used by every generator in the project.
///
/// Exposed so TempMail draws its mailbox addresses from the same engine
/// instead of reseeding `rand()` on each call.
std::mt19937 &randomEngine();

/// Random string of `length` characters.
///
/// `specialChars` adds the two URL-safe punctuation characters the PKCE spec
/// allows ("-", "_"); the default alphabet is alphanumeric only.
std::string generateRandomString(int length = 56, bool specialChars = true);

/// Base64 encoding using the URL-safe alphabet, without padding:
/// '+' becomes '-', '/' becomes '_', and trailing '=' are stripped.
std::string base64UrlEncode(const std::string &input);

/// SHA-256 digest of `input`, base64url-encoded without padding.
std::string sha256Base64Url(const std::string &input);

} // namespace Crypto
