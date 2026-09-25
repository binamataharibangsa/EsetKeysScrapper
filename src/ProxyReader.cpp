#include "ProxyReader.h"

#include "Helpers/Console.h"
#include "I18n/I18n.h"

#include <cstddef>
#include <fstream>
#include <string>

Proxy ProxyReader::giveNext() {
  if (proxies_.empty()) {
    std::cerr << RED << i18n::tr(i18n::Key::NoWorkingProxies) << RESET
              << std::endl;
    return Proxy();
  }

  Proxy proxy = proxies_.front();
  proxies_.pop();
  return proxy;
}

std::size_t ProxyReader::readProxies(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << RED << i18n::tr(i18n::Key::FailedToOpenFile) << YELLOW << path
              << RESET << std::endl;
    return 0;
  }

  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line.front() == '#') {
      continue;
    }
    proxies_.emplace(line);
  }

  return proxies_.size();
}
