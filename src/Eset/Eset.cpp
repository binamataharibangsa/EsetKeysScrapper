#include "Eset.h"

#include "../Core/Config.h"
#include "../Helpers/Paths.h"
#include "../I18n/I18n.h"

#include <cctype>
#include <cstddef>
#include <regex>
#include <string>
#include <vector>

namespace {

/// The ESET account API is happiest when the request advertises itself as the
/// registered web client, so these string literals are protocol, not style.
const char *kSecChUa = "'Chromium';v='121', 'Not A(Brand';v='99'";
const char *kAcceptJson = "application/json, text/plain, */*";
const char *kAcceptHtml =
    "text/html,application/xhtml+xml,application/xml;q=0.9,image/"
    "avif,image/webp,image/apng,*/*;q=0.8,application/"
    "signed-exchange;v=b3;q=0.7";

/// Hosts this class talks to, as they must appear in the Host: header.
std::string hostHeader(const char *host) { return std::string("Host: ") + host; }

} // namespace

Eset::Eset(const std::string &mail, const Proxy *proxy)
    : mail_(mail), proxy_(proxy) {
  // One cookie jar per account. The original used a single shared
  // "eset_cookies.txt" for every instance, so concurrent or sequential
  // accounts overwrote each other's session cookies.
  const std::string cookieJar =
      dataFile(std::string(config::kCookieFilePrefix) + mail_ +
               config::kCookieFileSuffix);
  curl_easy_setopt(curl_, CURLOPT_COOKIEJAR, cookieJar.c_str());
  curl_easy_setopt(curl_, CURLOPT_COOKIEFILE, cookieJar.c_str());

  if (proxy_ != nullptr && proxy_->isValid()) {
    std::cout << std::endl
              << GREEN << i18n::tr(i18n::Key::UsingProxy) << YELLOW
              << proxy_->toString() << RESET << std::endl;

    curl_easy_setopt(curl_, CURLOPT_PROXYTYPE,
                     proxy_->protocol() == "socks5"   ? CURLPROXY_SOCKS5
                     : proxy_->protocol() == "socks4" ? CURLPROXY_SOCKS4
                                                      : CURLPROXY_HTTP);
    curl_easy_setopt(curl_, CURLOPT_PROXY, proxy_->host().c_str());
    curl_easy_setopt(curl_, CURLOPT_PROXYPORT,
                     static_cast<long>(proxy_->port()));
  }

  useLoginHeaders(false);
}

bool Eset::createAccount() {
  response.clear();
  useLoginHeaders(true);

  // Built with the JSON API so the "selectedCountry is a string" quirk below
  // is expressed as a type rather than a comment.
  const json payload = {
      {"wantReceiveNews", false},
      {"password", config::kAccountPassword},
      {"email", mail_},
      {"selectedCountry", config::kSelectedCountry},
      {"agreeWithTerms", true},
      {"taskId", ""},
      {"returnUrl", pkce_.getAuthorizationUrl()},
      {"browserFingerprint", config::kBrowserFingerprint}};
  const std::string body = payload.dump();

  curl_easy_setopt(curl_, CURLOPT_URL, config::kCreateAccountUrl);
  curl_easy_setopt(curl_, CURLOPT_POST, 1L);
  curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, body.c_str());

  const CURLcode result = perform();
  if (result != CURLE_OK || response.empty()) {
    return false;
  }

  std::cout << std::endl
            << YELLOW << i18n::tr(i18n::Key::AccountCreated) << std::endl
            << GREEN << i18n::tr(i18n::Key::LabelEmail) << RESET << mail_
            << std::endl
            << GREEN << i18n::tr(i18n::Key::LabelPassword) << RESET
            << config::kAccountPassword << std::endl;

  useLoginHeaders(false);
  return true;
}

bool Eset::confirmRegistration(const std::string &body) {
  response.clear();

  const std::string url = extractVerificationLink(body);
  if (url.empty()) {
    return false;
  }

  useLoginHeaders(false);
  curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);

  const CURLcode result = perform();

  if (response.find("We are sorry") != std::string::npos) {
    std::cout << RED << i18n::tr(i18n::Key::VerificationFailed) << std::endl
              << i18n::tr(i18n::Key::Retrying) << std::endl;
    return false;
  }

  if (result == CURLE_OK) {
    std::cout << GREEN << i18n::tr(i18n::Key::VerificationSuccessful)
              << std::endl
              << std::endl;
    return true;
  }
  return false;
}

bool Eset::getLicense() {
  if (token.empty() && !login()) {
    return false;
  }

  std::this_thread::sleep_for(
      std::chrono::milliseconds(config::kActivationPollDelayMs));

  if (!activateLicense()) {
    std::cout << RED << i18n::tr(i18n::Key::ErrorActivatingLicense)
              << std::endl;
    return false;
  }

  return fetchLicenseKey();
}

bool Eset::login() {
  response.clear();

  const json payload = {{"email", mail_},
                        {"password", config::kAccountPassword},
                        {"returnUrl", pkce_.getAuthorizationUrl()},
                        {"browserFingerprint", config::kBrowserFingerprint}};

  curl_easy_setopt(curl_, CURLOPT_URL, config::kLoginAccountUrl);
  curl_easy_setopt(curl_, CURLOPT_POST, 1L);
  curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, payload.dump().c_str());

  const CURLcode result = perform();
  if (result != CURLE_OK ||
      response.find("redirectUrl") == std::string::npos) {
    return false;
  }

  const json answer = json::parse(response);
  if (answer.value("result", 0) == 1) {
    std::cout << RED << i18n::tr(i18n::Key::AccountDoesNotExist) << RESET
              << std::endl;
    return false;
  }
  if (response.find("\"https://home.eset.com\"") != std::string::npos) {
    std::cout << RED << i18n::tr(i18n::Key::ErrorInPkceCallback) << RESET
              << std::endl;
    return false;
  }

  if (!resolveTokenFromRedirect(response)) {
    std::cout << RED << i18n::tr(i18n::Key::ErrorGettingJwtToken) << RESET
              << std::endl;
    return false;
  }

  std::cout << GREEN << i18n::tr(i18n::Key::LoggedIn) << RESET << std::endl;
  return true;
}

bool Eset::resolveTokenFromRedirect(const std::string &loginResponse) {
  const std::string redirectHeaders = followPkceRedirect(loginResponse);

  const std::string location = locationHeader(redirectHeaders);
  if (location.empty()) {
    return false;
  }

  const std::string code = queryParameter(location, "code");
  if (code.empty()) {
    return false;
  }

  response.clear();
  responseHeaders.clear();

  useHeaders({"Content-Type: application/x-www-form-urlencoded"});

  const std::string tokenBody =
      std::string("client_id=") + config::kPkceClientId + "&code=" + code +
      "&redirect_uri=https%3A%2F%2Fhome.eset.com%2Fcallback&code_verifier=" +
      pkce_.codeVerifier() + "&grant_type=authorization_code";

  curl_easy_setopt(curl_, CURLOPT_URL, config::kTokenUrl);
  curl_easy_setopt(curl_, CURLOPT_POST, 1L);
  curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, tokenBody.c_str());

  if (perform() != CURLE_OK) {
    return false;
  }

  token = json::parse(response).value("access_token", std::string());
  if (token.empty()) {
    return false;
  }

  setHeaders();
  return true;
}

bool Eset::activateLicense() {
  response.clear();
  responseHeaders.clear();

  useHeaders({hostHeader(config::kEsetHomeHost),
              "Content-type: application/json", "Accept: */*",
              "x-eset-client-type: browser_desktop",
              "x-eset-client-device-language: en-US",
              "Referer: https://home.eset.com/onboarding/trial-subscription",
              "Authorization: Bearer " + token});

  const std::string payload =
      std::string("{\"productCode\":\"") + config::kTrialProductCode + "\"}";

  curl_easy_setopt(curl_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
  curl_easy_setopt(curl_, CURLOPT_URL, config::kActivateTrialUrl);
  curl_easy_setopt(curl_, CURLOPT_POST, 1L);
  curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, payload.c_str());

  const CURLcode result = perform();

  // A straight JSON answer means the trial was granted; an HTML error page
  // means it was not.
  return result == CURLE_OK && response.find("!DOCTYPE") == std::string::npos;
}

bool Eset::fetchLicenseKey() {
  response.clear();
  responseHeaders.clear();

  useHeaders({hostHeader(config::kEsetHomeHost),
              "Authorization: Bearer " + token});

  curl_easy_setopt(curl_, CURLOPT_URL, config::kGetAllLicensesUrl);
  curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);

  if (perform() != CURLE_OK) {
    return false;
  }

  if (response.length() < 3) {
    std::cout << RED << i18n::tr(i18n::Key::NoLicenseFound) << std::endl;
    return false;
  }

  // The endpoint answers with an array; an empty one has no key to read.
  const json licenses = json::parse(response, nullptr, false);
  if (licenses.is_discarded() || !licenses.is_array() || licenses.empty()) {
    std::cout << RED << i18n::tr(i18n::Key::NoLicenseFound) << std::endl;
    return false;
  }

  license_ = licenses[0].value("licenseKey", std::string());
  if (license_.empty()) {
    std::cout << RED << i18n::tr(i18n::Key::NoLicenseFound) << std::endl;
    return false;
  }

  std::cout << GREEN << i18n::tr(i18n::Key::LabelLicense) << RESET << license_
            << std::endl;
  return true;
}

void Eset::useLoginHeaders(bool forAccountCreation) {
  const std::string referer =
      forAccountCreation ? "Referer: https://login.eset.com/register/final-step"
                         : "Referer: https://login.eset.com/login";

  useHeaders({
      hostHeader(config::kEsetLoginHost),
      referer,
      std::string("Accept: ") + kAcceptJson,
      "Accept-Language: en-US,en;q=0.5",
      "Accept-Encoding: gzip, deflate",
      "X-Security-Request: required",
      "Content-Type: application/json",
      "Origin: https://login.eset.com",
      "Connection: keep-alive",
      "Upgrade-Insecure-Requests: 1",
      std::string("Sec-Ch-Ua: ") + kSecChUa,
      "Cache-Control: no-cache",
      "TE: trailers",
  });
}

void Eset::useHeaders(const std::vector<std::string> &lines) {
  // Replace the whole list rather than appending to the installed one: the
  // original reassigned `headers = NULL` without freeing, leaking every list
  // it built, and then added the shared headers through Scrapper, which
  // duplicated the user-agent on each call.
  clearHeaders();
  for (const std::string &line : lines) {
    appendHeader(line);
  }
  Scrapper::setHeaders();
  curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
}

std::string Eset::followPkceRedirect(const std::string &loginResponse) {
  response.clear();

  const json answer = json::parse(loginResponse);
  std::string redirectUrl = answer.value("redirectUrl", std::string());
  if (redirectUrl.empty()) {
    return {};
  }

  redirectUrl = std::string(config::kPkceRedirectPrefix) +
                std::regex_replace(redirectUrl, std::regex(" "), "%20");

  useHeaders({std::string("Accept: ") + kAcceptHtml});
  curl_easy_setopt(curl_, CURLOPT_URL, redirectUrl.c_str());
  curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);

  perform();
  return responseHeaders;
}

std::string Eset::extractVerificationLink(const std::string &body) {
  // The mail client renders the link as "[url]".
  static const std::regex linkPattern(R"(\[([^\]]+)\])");
  std::smatch match;
  if (std::regex_search(body, match, linkPattern) && match.size() > 1) {
    return match.str(1);
  }
  return {};
}

std::string Eset::locationHeader(const std::string &headers) {
  // Scanned line by line rather than with a single case-insensitive regex.
  //
  // Two reasons: header names are case-insensitive, and the inline `(?i)` flag
  // that would express that is not usable here -- libstdc++'s std::regex
  // rejects or crashes on it depending on the toolchain, which took down the
  // whole program at startup when it was used.
  std::size_t lineStart = 0;
  while (lineStart < headers.size()) {
    std::size_t lineEnd = headers.find('\n', lineStart);
    if (lineEnd == std::string::npos) {
      lineEnd = headers.size();
    }

    std::string line = headers.substr(lineStart, lineEnd - lineStart);
    // Tolerate CRLF.
    while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
      line.pop_back();
    }

    const std::size_t colon = line.find(':');
    if (colon != std::string::npos) {
      std::string name = line.substr(0, colon);
      for (char &character : name) {
        character = static_cast<char>(
            std::tolower(static_cast<unsigned char>(character)));
      }
      if (name == "location") {
        std::size_t valueStart = line.find_first_not_of(" \t", colon + 1);
        if (valueStart != std::string::npos) {
          return line.substr(valueStart);
        }
      }
    }

    lineStart = lineEnd + 1;
  }
  return {};
}

std::string Eset::queryParameter(const std::string &url,
                                 const std::string &parameter) {
  const std::regex pattern(parameter + "=([^&]+)");
  std::smatch match;
  if (std::regex_search(url, match, pattern)) {
    return match[1];
  }
  return {};
}
