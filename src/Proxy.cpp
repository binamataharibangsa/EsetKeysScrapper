#include "Proxy.h"

#include "Core/Config.h"
#include "Helpers/Console.h"
#include "I18n/I18n.h"

#include <cstddef>
#include <string>

namespace {

/// cURL proxy type for the scheme in `protocol`.
///
/// Unknown schemes fall back to HTTP, which is what the original code did by
/// setting CURLPROXY_HTTP first and only overriding it for socks4/socks5.
long curlProxyType(const std::string &protocol) {
  if (protocol == "socks5") {
    return CURLPROXY_SOCKS5;
  }
  if (protocol == "socks4") {
    return CURLPROXY_SOCKS4;
  }
  return CURLPROXY_HTTP;
}

std::size_t writeCallback(void *contents, std::size_t size, std::size_t nmemb,
                          void *userp) {
  const std::size_t total = size * nmemb;
  static_cast<std::string *>(userp)->append(static_cast<char *>(contents),
                                            total);
  return total;
}

} // namespace

Proxy::Proxy(const std::string &text) {
  const std::size_t protocolEnd = text.find("://");
  if (protocolEnd == std::string::npos) {
    std::cerr << i18n::tr(i18n::Key::ErrorInitializingProxy) << YELLOW << text
              << RESET << std::endl;
    return;
  }

  const std::size_t hostStart = protocolEnd + 3;
  const std::size_t portSeparator = text.find(':', hostStart);
  if (portSeparator == std::string::npos || portSeparator == hostStart) {
    std::cerr << i18n::tr(i18n::Key::ErrorInitializingProxy) << YELLOW << text
              << RESET << std::endl;
    return;
  }

  const std::string portText = text.substr(portSeparator + 1);

  // std::stoi throws on a non-numeric port, and the original let that
  // exception escape from a constructor called inside a loop over a
  // user-supplied file: one bad line aborted the whole run.
  try {
    std::size_t consumed = 0;
    const int parsedPort = std::stoi(portText, &consumed);
    if (consumed != portText.size() || parsedPort <= 0 || parsedPort > 65535) {
      throw std::invalid_argument("port out of range");
    }
    port_ = parsedPort;
  } catch (const std::exception &) {
    std::cerr << i18n::tr(i18n::Key::ErrorInitializingProxy) << YELLOW << text
              << RESET << std::endl;
    return;
  }

  protocol_ = text.substr(0, protocolEnd);
  host_ = text.substr(hostStart, portSeparator - hostStart);
  valid_ = !protocol_.empty() && !host_.empty();
}

bool Proxy::isValid() const noexcept { return valid_; }

bool Proxy::isWorking() const {
  if (!valid_) {
    return false;
  }

  CURL *handle = curl_easy_init();
  if (handle == nullptr) {
    return false;
  }

  std::string body;

  curl_easy_setopt(handle, CURLOPT_URL, config::kProxyProbeUrl);
  curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(handle, CURLOPT_TIMEOUT, config::kProxyProbeTimeoutSeconds);
  curl_easy_setopt(handle, CURLOPT_PROXYTYPE, curlProxyType(protocol_));
  curl_easy_setopt(handle, CURLOPT_PROXY, host_.c_str());
  curl_easy_setopt(handle, CURLOPT_PROXYPORT, static_cast<long>(port_));
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeCallback);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, &body);

  const CURLcode result = curl_easy_perform(handle);
  curl_easy_cleanup(handle);

  if (result != CURLE_OK) {
    return false;
  }

  // The original tested `sizeof(buffer) > 0`, which is the size of the
  // std::string object itself and therefore always true -- every proxy that
  // completed the request counted as working regardless of the answer. Testing
  // the body length is what was meant.
  return !body.empty();
}

std::string Proxy::toString() const {
  return protocol_ + "://" + host_ + ":" + std::to_string(port_);
}
