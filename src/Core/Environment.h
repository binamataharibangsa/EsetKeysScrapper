#pragma once

#include <cstdlib>
#include <string>

namespace environment {

/// Value of the environment variable `name`, or "" when it is not set.
///
/// `std::getenv` is used on every platform. The MSVC-specific `_dupenv_s` was
/// tried first but is a UCRT-only symbol: it links against MSVC and UCRT
/// MinGW-w64 builds, yet not against the classic-msvcrt MinGW-w64 runtime, so
/// using it made the project silently unbuildable for some toolchains.
/// `_CRT_SECURE_NO_WARNINGS` is set for MSVC in CMakeLists.txt to keep the
/// deprecation warning away.
inline std::string get(const char *name) {
  const char *value = std::getenv(name);
  return value != nullptr ? std::string(value) : std::string();
}

} // namespace environment
