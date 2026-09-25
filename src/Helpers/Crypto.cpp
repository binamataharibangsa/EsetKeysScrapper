#include "Crypto.h"

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#include <cstddef>
#include <random>
#include <string>

namespace {

/// Raw bytes -> base64url text (no padding).
std::string toBase64Url(const unsigned char *data, std::size_t length) {
  BIO *memory = BIO_new(BIO_s_mem());
  BIO *encoder = BIO_new(BIO_f_base64());
  if (memory == nullptr || encoder == nullptr) {
    BIO_free(memory);
    BIO_free(encoder);
    return {};
  }

  // No line breaks, no trailing newline.
  BIO_set_flags(encoder, BIO_FLAGS_BASE64_NO_NL);
  BIO_push(encoder, memory);

  BIO_write(encoder, data, static_cast<int>(length));
  BIO_flush(encoder);

  BUF_MEM *buffer = nullptr;
  BIO_get_mem_ptr(encoder, &buffer);

  std::string encoded = buffer != nullptr
                            ? std::string(buffer->data, buffer->length)
                            : std::string();

  BIO_free_all(encoder);

  for (char &character : encoded) {
    if (character == '+') {
      character = '-';
    } else if (character == '/') {
      character = '_';
    }
  }
  while (!encoded.empty() && encoded.back() == '=') {
    encoded.pop_back();
  }

  return encoded;
}

} // namespace

std::mt19937 &Crypto::randomEngine() {
  // Kept alive across calls, seeded once from random_device.
  //
  // The original called `srand(time(0))` inside the generator, so every draw
  // made within the same second produced the same string -- which silently
  // broke the uniqueness of both the PKCE values and the mailbox addresses.
  static std::mt19937 engine{std::random_device{}()};
  return engine;
}

std::string Crypto::generateRandomString(int length, bool specialChars) {
  static const std::string kAlphanumeric =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  static const std::string kAlphanumericWithPunctuation = kAlphanumeric + "-_";

  const std::string &alphabet =
      specialChars ? kAlphanumericWithPunctuation : kAlphanumeric;

  if (length <= 0) {
    return {};
  }

  std::uniform_int_distribution<std::size_t> distribution(0,
                                                         alphabet.size() - 1);

  std::string result;
  result.reserve(static_cast<std::size_t>(length));
  for (int i = 0; i < length; ++i) {
    result += alphabet[distribution(randomEngine())];
  }
  return result;
}

std::string Crypto::base64UrlEncode(const std::string &input) {
  return toBase64Url(reinterpret_cast<const unsigned char *>(input.data()),
                     input.size());
}

std::string Crypto::sha256Base64Url(const std::string &input) {
  unsigned char digest[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char *>(input.data()), input.size(),
         digest);

  // The original hex-encoded the digest and then decoded it back to bytes
  // before base64ing it; that round trip is unnecessary, so it is gone.
  return toBase64Url(digest, SHA256_DIGEST_LENGTH);
}
