#pragma once

#include <array>
#include <cstddef>
#include <string>

namespace i18n {

/// Every user-facing string in the program.
///
/// Adding a string means adding one enumerator here and one entry to each
/// table in I18n.cpp. Forgetting a translation is a compile error (see the
/// static_asserts in I18n.cpp) rather than a silent fallback to English.
enum class Key {
  // --- Generic labels ------------------------------------------------------
  Yes,
  No,
  LabelMail,
  LabelEmail,
  LabelPassword,
  LabelLicense,
  LabelId,
  LabelFrom,
  LabelSubject,
  LabelBody,
  LabelRead,

  // --- Command line interface ---------------------------------------------
  CliDescription,
  CliOptHelp,
  CliOptVersion,
  CliOptNumber,
  CliOptLength,
  CliOptProxy,
  CliOptLang,
  CliErrorParsingOptions,
  CliLicensesGenerated,
  CliDisclaimerTitle,
  CliDisclaimerBody,
  CliLanguageSelected,

  // --- LicenseManager ------------------------------------------------------
  LoadingProxyFile,
  UsingProxies,
  UsingProxiesLatencyWarning,
  RetryingCreateAccount,
  ErrorCreatingAccount,
  WaitingForVerificationCode,
  RetryingGetLicense,
  ErrorGettingLicense,
  NoLicenseWasGenerated,
  DataSavedTo,

  // --- Eset ----------------------------------------------------------------
  UsingProxy,
  AccountCreated,
  VerificationFailed,
  Retrying,
  VerificationSuccessful,
  LoggedIn,
  LoginFailed,
  ErrorActivatingLicense,
  NoLicenseFound,
  AccountDoesNotExist,
  ErrorInPkceCallback,
  ErrorGettingJwtToken,

  // --- TempMail ------------------------------------------------------------
  TempMailCreated,

  // --- Proxy / ProxyReader -------------------------------------------------
  ErrorInitializingProxy,
  FailedToOpenFile,
  NoWorkingProxies,
  LeavingProgram,

  // --- Helpers -------------------------------------------------------------
  ErrorCreatingDataDirectory,
  ErrorInitializingCurl,
  CurlHandleIsNull,

  // --- Qt GUI --------------------------------------------------------------
  QtCopy,
  QtGenerate,
  QtGenerating,
  QtNoLicenseGenerated,
  QtLanguage,

  // --- Language names (always shown in their own language) -----------------
  LanguageEnglish,
  LanguageSpanish,
  LanguageIndonesian,

  Count
};

/// Number of translatable strings, kept in sync with Key automatically.
inline constexpr std::size_t kKeyCount = static_cast<std::size_t>(Key::Count);

/// Supported interface languages.
enum class Lang { English, Spanish, Indonesian };

/// All languages, in the order they are offered to the user.
inline constexpr std::array<Lang, 3> kSupportedLanguages{
    Lang::English, Lang::Spanish, Lang::Indonesian};

/// Process-wide language holder.
///
/// Output happens from a single thread, so one shared instance is enough and
/// keeps every call site short (`i18n::tr(Key::X)`).
class Translator {
public:
  static Translator &instance();

  Translator(const Translator &) = delete;
  Translator &operator=(const Translator &) = delete;

  void setLanguage(Lang lang) noexcept { language_ = lang; }
  Lang language() const noexcept { return language_; }

  /// Translation of `key` in the currently selected language.
  std::string tr(Key key) const;

  /// "en" / "es" / "id" -> Lang. Unknown codes yield `fallback`.
  static Lang fromCode(const std::string &code,
                       Lang fallback = Lang::English) noexcept;

  /// Lang -> "en" / "es" / "id".
  static std::string toCode(Lang lang);

  /// Best-effort detection from LANGUAGE / LC_ALL / LC_MESSAGES / LANG
  /// (e.g. "es_ES.UTF-8" -> Spanish).
  static Lang fromEnvironment(Lang fallback = Lang::English) noexcept;

  /// Name of the language written in that language ("Español", "Bahasa
  /// Indonesia", ...). Language names are never translated.
  static std::string displayName(Lang lang);

private:
  Translator() = default;

  Lang language_ = Lang::English;
};

/// Shorthand for `Translator::instance().tr(key)`.
std::string tr(Key key);

} // namespace i18n
