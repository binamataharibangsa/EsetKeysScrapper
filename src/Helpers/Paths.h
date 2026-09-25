#pragma once

#include <string>

/// Directory that holds generated artefacts: the licence CSV, the per-account
/// cookie jars and the list of mailbox addresses that were already taken.
///
/// Resolution order:
///   1. the `ESETKEYS_DATA_DIR` environment variable, when set and non-empty;
///   2. `data/` next to the build output (i.e. the parent of the current
///      working directory, which is where the console binary lives).
///
/// The directory is created on first use. The returned path has no trailing
/// separator; use `dataFile()` to build paths inside it.
std::string dataDirectory();

/// Absolute path of `fileName` inside `dataDirectory()`.
std::string dataFile(const std::string &fileName);
