#pragma once

#include "Helpers/Console.h"

#include <cstddef>
#include <string>
#include <vector>

/// Base class for every component that talks HTTP: owns one libcurl easy
/// handle and the plumbing shared by all of them (response buffering, header
/// capture, user-agent rotation, bearer token).
///
/// Lifetime rule: one instance owns exactly one `CURL *`. Subclasses must not
/// be copied (see the deleted copy operations below) -- handing these objects
/// around by value was how the original code ended up with two owners for the
/// same handle.
class Scrapper {
public:
  Scrapper();
  virtual ~Scrapper();

  Scrapper(const Scrapper &) = delete;
  Scrapper &operator=(const Scrapper &) = delete;

protected:
  /// The owned libcurl easy handle. Subclasses configure it directly before
  /// calling `perform()`; they must never take ownership of it.
  CURL *curl_ = nullptr;

  /// The installed header list. Rebuilt by `setHeaders()`; subclasses that
  /// install their own set use `clearHeaders()` / `appendHeader()` and must
  /// leave this pointing at whatever is currently installed.
  struct curl_slist *headers_ = nullptr;

  /// Performs the request configured so far.
  ///
  /// Returns the cURL status code so callers can distinguish "the request was
  /// answered" from "the request never completed", which the original code
  /// conflated by inspecting `response` while ignoring `code`.
  CURLcode perform();

  /// Body of the last response. Cleared by `perform()`.
  std::string response;

  /// Raw headers of the last response, in arrival order.
  std::string responseHeaders;

  /// Bearer token sent on every request once it is non-empty.
  std::string token;

  /// Rebuilds the header list and installs it on the handle.
  ///
  /// Subclasses override this to add their own headers, then call
  /// `Scrapper::setHeaders()` to append the common ones. Note that the header
  /// list is *replaced*, not appended to: the previous list is freed first.
  virtual void setHeaders();

  /// Adds `header` to the installed list. Assumes the list was cleared or
  /// belongs to this object.
  void appendHeader(const std::string &header);

  /// Frees the installed header list, if any. Safe to call repeatedly.
  void clearHeaders() noexcept;

private:
  static std::size_t writeCallback(void *contents, std::size_t size,
                                   std::size_t nmemb, void *userp);
  static std::size_t headerCallback(char *buffer, std::size_t size,
                                    std::size_t nitems, void *userp);

  /// Picks one of `kUserAgents` at random.
  std::string pickUserAgent() const;
};
