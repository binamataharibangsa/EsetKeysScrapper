#include "Paths.h"

#include "../Core/Config.h"
#include "../Core/Environment.h"
#include "../I18n/I18n.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>

namespace {

/// Resolved once: the location cannot change while the process runs, and the
/// original implementation performed two filesystem round trips per call.
const std::string &resolveDataDirectory() {
  namespace fs = std::filesystem;

  static const std::string resolved = []() -> std::string {
    const std::string override = environment::get(config::kDataDirectoryEnvVar);
    fs::path directory;
    if (!override.empty()) {
      directory = override;
    } else {
      // The console binary is started from build/bin, so its parent is the
      // build directory -- the same place the original code resolved to.
      directory = fs::current_path().parent_path() / config::kDataDirectoryName;
    }

    std::error_code error;
    if (!fs::exists(directory, error)) {
      fs::create_directories(directory, error);
      if (error) {
        std::cerr << i18n::tr(i18n::Key::ErrorCreatingDataDirectory)
                  << error.message() << std::endl;
      }
    }

    return directory.string();
  }();

  return resolved;
}

} // namespace

std::string dataDirectory() { return resolveDataDirectory(); }

std::string dataFile(const std::string &fileName) {
  const std::string directory = dataDirectory();
  if (directory.empty()) {
    return fileName;
  }
  // '/' works as a separator on every platform we target, including Windows.
  return directory + "/" + fileName;
}
