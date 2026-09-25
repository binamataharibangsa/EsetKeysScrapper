#pragma once

#include "Proxy.h"

#include <cstddef>
#include <queue>
#include <string>

/// Reads a proxy list file and hands the entries out one per account.
class ProxyReader {
public:
  /// Loads one proxy per non-empty, non-comment line of `path`.
  ///
  /// Lines starting with '#' are ignored so a list can be annotated. Returns
  /// the number of usable proxies loaded; a missing or unreadable file leaves
  /// the queue empty and prints a diagnostic rather than throwing.
  std::size_t readProxies(const std::string &path);

  /// Number of proxies still available.
  std::size_t remaining() const noexcept { return proxies_.size(); }

  /// True when at least one proxy was loaded and none has been consumed yet.
  bool empty() const noexcept { return proxies_.empty(); }

  /// Next proxy, or an invalid Proxy when the list is exhausted.
  ///
  /// Returning an invalid value instead of calling `front()` on an empty queue
  /// matters: the original dereferenced an empty `std::queue`, which is
  /// undefined behaviour and crashed precisely when the proxy list ran out
  /// mid-run.
  Proxy giveNext();

private:
  std::queue<Proxy> proxies_;
};
