// Unit test for the translation tables and the language-code plumbing.
//
// Built only when -DBUILD_TESTS=ON is passed to CMake, so it never gets in the
// way of a normal build. Run the resulting `EsetKeysI18nTest` binary; it exits
// non-zero on the first failed assertion.
//
// The value of this test is mostly in catching table mis-ordering: the three
// translation tables are positional, so inserting a key in the wrong place
// silently shifts every string after it. The compile-time checks in I18n.cpp
// prove the tables are complete; this test proves they line up.

#include "I18n/I18n.h"

#include <cassert>
#include <iostream>
#include <string>

namespace {

using i18n::Key;
using i18n::Lang;
using i18n::Translator;

/// Number of entries actually present in a table, via the public lookup.
std::size_t countNonEmpty(Lang lang) {
  Translator::instance().setLanguage(lang);
  std::size_t count = 0;
  for (std::size_t i = 0; i < i18n::kKeyCount; ++i) {
    if (!i18n::tr(static_cast<Key>(i)).empty()) {
      ++count;
    }
  }
  return count;
}

void testEveryKeyTranslated() {
  for (Lang lang : i18n::kSupportedLanguages) {
    const std::size_t translated = countNonEmpty(lang);
    std::cout << "  " << Translator::toCode(lang) << ": " << translated << "/"
              << i18n::kKeyCount << " keys\n";
    assert(translated == i18n::kKeyCount);
  }
}

void testSpotChecks() {
  // Exact strings for a handful of keys, chosen to catch a shifted table.
  Translator::instance().setLanguage(Lang::English);
  assert(i18n::tr(Key::Yes) == "Yes");
  assert(i18n::tr(Key::LoggedIn) == "Logged in");
  assert(i18n::tr(Key::LanguageIndonesian) == "Bahasa Indonesia");

  Translator::instance().setLanguage(Lang::Spanish);
  assert(i18n::tr(Key::Yes) == "S\xC3\xAD");
  assert(i18n::tr(Key::LoggedIn) == "Sesi\xC3\xB3n iniciada");

  Translator::instance().setLanguage(Lang::Indonesian);
  assert(i18n::tr(Key::Yes) == "Ya");
  assert(i18n::tr(Key::LoggedIn) == "Berhasil masuk");
}

void testTranslationsActuallyDiffer() {
  // A key whose text must not be identical in all three languages, which would
  // mean two tables were accidentally filled with the same strings.
  const Key keys[] = {Key::LoggedIn, Key::ErrorCreatingAccount,
                      Key::NoWorkingProxies, Key::FailedToOpenFile,
                      Key::ErrorCreatingDataDirectory};

  for (Key key : keys) {
    Translator::instance().setLanguage(Lang::English);
    const std::string english = i18n::tr(key);
    Translator::instance().setLanguage(Lang::Spanish);
    const std::string spanish = i18n::tr(key);
    Translator::instance().setLanguage(Lang::Indonesian);
    const std::string indonesian = i18n::tr(key);

    assert(english != spanish);
    assert(english != indonesian);
    assert(spanish != indonesian);
  }
}

void testLanguageNamesAreNative() {
  // Language names are never translated: they read the same in every table.
  assert(Translator::displayName(Lang::English) == "English");
  assert(Translator::displayName(Lang::Spanish) == "Espa\xC3\xB1ol");
  assert(Translator::displayName(Lang::Indonesian) == "Bahasa Indonesia");
}

void testCodeParsing() {
  assert(Translator::fromCode("en") == Lang::English);
  assert(Translator::fromCode("EN") == Lang::English);
  assert(Translator::fromCode("es") == Lang::Spanish);
  assert(Translator::fromCode("es_ES.UTF-8") == Lang::Spanish);
  assert(Translator::fromCode("es-AR") == Lang::Spanish);
  assert(Translator::fromCode("id") == Lang::Indonesian);
  assert(Translator::fromCode("id-ID") == Lang::Indonesian);
  assert(Translator::fromCode("in") == Lang::Indonesian); // legacy code

  // Unknown codes fall back instead of throwing.
  assert(Translator::fromCode("fr") == Lang::English);
  assert(Translator::fromCode("fr", Lang::Spanish) == Lang::Spanish);
  assert(Translator::fromCode("") == Lang::English);

  assert(Translator::toCode(Lang::English) == "en");
  assert(Translator::toCode(Lang::Spanish) == "es");
  assert(Translator::toCode(Lang::Indonesian) == "id");

  // Round trip.
  for (Lang lang : i18n::kSupportedLanguages) {
    assert(Translator::fromCode(Translator::toCode(lang)) == lang);
  }
}

void testOutOfRangeKeyIsSafe() {
  assert(Translator::instance().tr(Key::Count).empty());
  assert(Translator::instance().tr(static_cast<Key>(9999)).empty());
}

} // namespace

int main() {
  std::cout << "testEveryKeyTranslated\n";
  testEveryKeyTranslated();
  std::cout << "testSpotChecks\n";
  testSpotChecks();
  std::cout << "testTranslationsActuallyDiffer\n";
  testTranslationsActuallyDiffer();
  std::cout << "testLanguageNamesAreNative\n";
  testLanguageNamesAreNative();
  std::cout << "testCodeParsing\n";
  testCodeParsing();
  std::cout << "testOutOfRangeKeyIsSafe\n";
  testOutOfRangeKeyIsSafe();

  std::cout << "\nALL I18N TESTS PASSED\n";
  return 0;
}
