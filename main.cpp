#include "dependencies/cxxopts.hpp"

#include "Helpers/Console.h"
#include "src/Core/Config.h"
#include "src/I18n/I18n.h"
#include "src/LicenseManager.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace {

/// Derived from the central version string instead of a bare `#define version`,
/// which shadowed <version> and any variable of that name for every file that
/// included this translation unit.
constexpr const char *kVersion = config::kAppVersion;

#if defined(_WIN32)
/// Enables ANSI escape processing. Without this the colour codes print as
/// literal text in the legacy console host.
void enableAnsiColors() {
  HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
  if (console == INVALID_HANDLE_VALUE) {
    return;
  }

  DWORD mode = 0;
  if (!GetConsoleMode(console, &mode)) {
    return;
  }

  SetConsoleMode(console, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}
#endif

void printBanner() {
  std::cout << CYAN << R"(
  ______          _   _  __               _____
 |  ____|        | | | |/ /              / ____|
 | |__   ___  ___| |_| ' / ___ _   _ ___| (___   ___ _ __ __ _ _ __  _ __   ___ _ __
 |  __| / __|/ _ | __|  < / _ | | | / __|\___ \ / __| '__/ _` | '_ \| '_ \ / _ | '__|
 | |____\__ |  __| |_| . |  __| |_| \__ \____) | (__| | | (_| | |_) | |_) |  __| |
 |______|___/\___|\__|_|\_\___|\__, |___|_____/ \___|_|  \__,_| .__/| .__/ \___|_|
                                __/ |                         | |   | |
                               |___/                          |_|   |_|

)" << RESET
            << std::endl;
}

void printDisclaimer() {
  using i18n::Key;
  std::cout << RED << i18n::tr(Key::CliDisclaimerTitle) << std::endl
            << i18n::tr(Key::CliDisclaimerBody) << RESET << std::endl
            << GREEN << "\t" << config::kAppAuthor << "." << RESET << std::endl
            << std::endl;
}

/// Applies the language chosen on the command line, falling back to the
/// environment and then to English.
void applyLanguage(const std::string &requested) {
  using i18n::Lang;
  using i18n::Translator;

  const Lang lang =
      requested.empty()
          ? Translator::fromEnvironment()
          : Translator::fromCode(requested, Translator::instance().language());

  Translator::instance().setLanguage(lang);
  std::cout << GREEN << i18n::tr(i18n::Key::CliLanguageSelected) << RESET
            << Translator::displayName(lang) << std::endl;
}

} // namespace

int main(int argc, char *argv[]) {
#if defined(_WIN32)
  enableAnsiColors();
#endif

  printBanner();

  cxxopts::Options options(config::kAppName,
                           i18n::tr(i18n::Key::CliDescription));

  int numLicenses = config::kMinLicenseCount;
  int domainLength = config::kDefaultDomainLength;
  std::string proxyFile;
  std::string language;

  options.add_options()("h,help", i18n::tr(i18n::Key::CliOptHelp))(
      "v,version", i18n::tr(i18n::Key::CliOptVersion))(
      "n,number", i18n::tr(i18n::Key::CliOptNumber),
      cxxopts::value<int>(numLicenses)->default_value("1"))(
      "l,length", i18n::tr(i18n::Key::CliOptLength),
      cxxopts::value<int>(domainLength)->default_value("10"))(
      "p,proxy", i18n::tr(i18n::Key::CliOptProxy),
      cxxopts::value<std::string>(proxyFile))(
      "lang", i18n::tr(i18n::Key::CliOptLang),
      cxxopts::value<std::string>(language));

  try {
    const auto result = options.parse(argc, argv);

    if (result.count("help") > 0) {
      std::cout << options.help() << std::endl;
      return EXIT_SUCCESS;
    }

    if (result.count("version") > 0) {
      std::cout << config::kAppName << LGREEN << "v" << kVersion << std::endl;
      return EXIT_SUCCESS;
    }

    // `--lang` is applied after parsing but before anything user-facing, so
    // the disclaimer below already honours it. Help and version stay in the
    // detected language because they are printed during parsing.
    applyLanguage(language);
    printDisclaimer();

    LicenseManager manager(numLicenses, domainLength, proxyFile);
    manager.generateLicenses();

    std::cout << std::endl
              << GREEN << "---" << RESET << std::endl
              << GREEN << i18n::tr(i18n::Key::CliLicensesGenerated) << RESET
              << std::endl
              << std::endl;
    manager.showAllLicenses();

#if defined(_WIN32)
    // Keeps the console window open when the binary is double-clicked.
    std::getchar();
#endif
  } catch (const cxxopts::exceptions::exception &e) {
    // `bad_exception` was used before, which no option parser ever throws:
    // a mistyped option therefore escaped main() and terminated the process
    // with an unhandled exception instead of printing a message.
    std::cout << i18n::tr(i18n::Key::CliErrorParsingOptions) << e.what()
              << std::endl;
    return EXIT_FAILURE;
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
