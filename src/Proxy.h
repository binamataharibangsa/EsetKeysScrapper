#pragma once

#include <string>

/// A single `protocol://host:port` proxy entry.
///
/// Parsing and the liveness probe are separated on purpose: constructing a
/// Proxy never performs I/O, so a malformed line in the proxy file is a
/// reporting problem rather than a silent side effect of building a value.
class Proxy {
public:
  /// Parses `text` ("socks5://1.2.3.4:1080"). A malformed entry yields an
  /// object for which `isValid()` is false; it never throws.
  explicit Proxy(const std::string &text = std::string());

  /// True when a protocol, host and port were all parsed successfully.
  bool isValid() const noexcept;

  /// Sends a throwaway request through this proxy and reports whether it
  /// answered. Returns false for an invalid proxy.
  bool isWorking() const;

  /// Canonical "protocol://host:port" form.
  std::string toString() const;

  const std::string &protocol() const noexcept { return protocol_; }
  const std::string &host() const noexcept { return host_; }
  int port() const noexcept { return port_; }

private:
  std::string protocol_;
  std::string host_;
  int port_ = 0;
  bool valid_ = false;
};
