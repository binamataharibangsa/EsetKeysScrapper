#include "Scrapper.h"

#include "Core/Config.h"
#include "I18n/I18n.h"

#include <cstddef>
#include <random>
#include <stdexcept>
#include <string>

namespace {

/// Rotated per request so a burst of requests from one process does not look
/// like a single client. Kept here rather than as a member so every instance
/// shares the same pool.
const std::vector<std::string> kUserAgents{
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, "
    "like Gecko) Chrome/74.0.3729.169 Safari/537.36",
    "Mozilla/5.0 (iPhone; CPU iPhone OS 12_2 like Mac OS X) "
    "AppleWebKit/605.1.15 (KHTML, like Gecko) Mobile/15E148",
    "Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/115.0"};

const std::string &randomUserAgent() {
  // Same engine as Crypto: seeded from random_device once per process. The
  // original used rand(), which the standard says is not required to be
  // thread-safe or well-distributed, and which TempMail additionally reseeded
  // with srand(time(0)) on every call.
  static std::mt19937 engine{std::random_device{}()};
  std::uniform_int_distribution<std::size_t> pick(0, kUserAgents.size() - 1);
  return kUserAgents[pick(engine)];
}

} // namespace

Scrapper::Scrapper() {
  curl_ = curl_easy_init();
  if (curl_ == nullptr) {
    // A type-free `throw "string"` was the original: nothing could catch it
    // meaningfully and it aborted the process if it ever escaped.
    throw std::runtime_error(i18n::tr(i18n::Key::ErrorInitializingCurl));
  }

  curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
  curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
  curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, headerCallback);
  curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &responseHeaders);

  // The original set CURLOPT_ENCODING to "UTF-8", which is not a supported
  // content encoding -- cURL silently fell back to no compression. Asking for
  // everything and letting libcurl negotiate is both correct and faster.
  curl_easy_setopt(curl_, CURLOPT_ACCEPT_ENCODING, "");
  curl_easy_setopt(curl_, CURLOPT_TIMEOUT, config::kRequestTimeoutSeconds);
}

Scrapper::~Scrapper() {
  clearHeaders();
  if (curl_ != nullptr) {
    curl_easy_cleanup(curl_);
  }
}

void Scrapper::clearHeaders() noexcept {
  if (headers_ != nullptr) {
    curl_slist_free_all(headers_);
    headers_ = nullptr;
  }
}

void Scrapper::appendHeader(const std::string &header) {
  headers_ = curl_slist_append(headers_, header.c_str());
  if (headers_ == nullptr) {
    throw std::runtime_error(i18n::tr(i18n::Key::CurlHandleIsNull));
  }
}

CURLcode Scrapper::perform() {
  if (curl_ == nullptr) {
    return CURLE_FAILED_INIT;
  }
  return curl_easy_perform(curl_);
}

std::string Scrapper::pickUserAgent() const {
  return "User-Agent: " + randomUserAgent();
}

void Scrapper::setHeaders() {
  if (curl_ == nullptr) {
    std::cerr << i18n::tr(i18n::Key::CurlHandleIsNull) << std::endl;
    return;
  }

  // Rebuild from scratch. The original appended to the list that was still
  // installed from the previous request, so every call leaked the old list and
  // duplicated every header that had been added before.
  clearHeaders();

  appendHeader(pickUserAgent());
  if (!token.empty()) {
    appendHeader("Authorization: Bearer " + token);
  }

  curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
}

std::size_t Scrapper::writeCallback(void *contents, std::size_t size,
                                    std::size_t nmemb, void *userp) {
  const std::size_t total = size * nmemb;
  static_cast<std::string *>(userp)->append(static_cast<char *>(contents),
                                            total);
  return total;
}

std::size_t Scrapper::headerCallback(char *buffer, std::size_t size,
                                     std::size_t nitems, void *userp) {
  const std::size_t total = size * nitems;
  static_cast<std::string *>(userp)->append(buffer, total);
  return total;
}
