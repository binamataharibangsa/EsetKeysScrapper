#include "I18n.h"

#include "../Core/Environment.h"

#include <algorithm>
#include <cctype>

namespace i18n {
namespace {

using Table = std::array<const char *, kKeyCount>;

// ---------------------------------------------------------------------------
// English (reference language)
// ---------------------------------------------------------------------------
constexpr Table kEnglish{
    /* Yes                       */ "Yes",
    /* No                        */ "No",
    /* LabelMail                 */ "Mail: ",
    /* LabelEmail                */ "Email: ",
    /* LabelPassword             */ "Password: ",
    /* LabelLicense              */ "License: ",
    /* LabelId                   */ "ID: ",
    /* LabelFrom                 */ "From: ",
    /* LabelSubject              */ "Subject: ",
    /* LabelBody                 */ "Body: ",
    /* LabelRead                 */ "Read: ",

    /* CliDescription            */ "A tool for extracting free licenses from "
                                   "Eset NOD32 antivirus accounts through web "
                                   "scraping",
    /* CliOptHelp                */ "Show help",
    /* CliOptVersion             */ "Show version",
    /* CliOptNumber              */ "Number of licenses",
    /* CliOptLength              */ "Domain length for temporal mails",
    /* CliOptProxy               */ "Proxy list file (protocol://ip:port)",
    /* CliOptLang                */ "Interface language: en, es, id",
    /* CliErrorParsingOptions    */ "Error parsing options: ",
    /* CliLicensesGenerated      */ "Licenses generated successfully",
    /* CliDisclaimerTitle        */ "DISCLAIMER:",
    /* CliDisclaimerBody         */ "This tool is for educational purposes "
                                   "only. Use at your own risk. I am not "
                                   "responsible for any damage caused by the "
                                   "use of this tool.",
    /* CliLanguageSelected       */ "Language: ",

    /* LoadingProxyFile          */ "Loading proxies file...",
    /* UsingProxies              */ "--- Using proxies ---",
    /* UsingProxiesLatencyWarning*/ "This can take a while depending on the "
                                   "latency of the proxy",
    /* RetryingCreateAccount     */ "Retrying to create account...",
    /* ErrorCreatingAccount      */ "Error creating account",
    /* WaitingForVerificationCode*/ "Waiting for verification code...",
    /* RetryingGetLicense        */ "Retrying to get license...",
    /* ErrorGettingLicense       */ "Error getting license",
    /* NoLicenseWasGenerated     */ "No license was generated",
    /* DataSavedTo               */ "Data saved to: ",

    /* UsingProxy                */ "Using proxy: ",
    /* AccountCreated            */ "--- ESET ACCOUNT CREATED ---",
    /* VerificationFailed        */ "Verification failed",
    /* Retrying                  */ "Retrying...",
    /* VerificationSuccessful    */ "Verification successful",
    /* LoggedIn                  */ "Logged in",
    /* LoginFailed               */ "Login failed",
    /* ErrorActivatingLicense    */ "Error activating license",
    /* NoLicenseFound            */ "No license found",
    /* AccountDoesNotExist       */ "Account does not exist",
    /* ErrorInPkceCallback       */ "Error in PKCE callback",
    /* ErrorGettingJwtToken      */ "Error getting JWT token",

    /* TempMailCreated           */ "--- TEMP MAIL CREATED ---",

    /* ErrorInitializingProxy    */ "Error initializing proxy: ",
    /* FailedToOpenFile          */ "Failed to open file: ",
    /* NoWorkingProxies          */ "No working proxies available...",
    /* LeavingProgram            */ "Leaving the program",

    /* ErrorCreatingDataDirectory*/ "Error creating data directory: ",
    /* ErrorInitializingCurl     */ "Error initializing CURL",
    /* CurlHandleIsNull          */ "curl handle is null!",

    /* QtCopy                    */ "\xF0\x9F\x93\x8B Copy",
    /* QtGenerate                */ "\xE2\x9A\x99\xEF\xB8\x8F Generate",
    /* QtGenerating              */ "\xE2\x8F\xB3 Generating license...",
    /* QtNoLicenseGenerated      */ "\xE2\x9D\x8C No license generated.",
    /* QtLanguage                */ "Language",

    /* LanguageEnglish           */ "English",
    /* LanguageSpanish           */ "Espa\xC3\xB1ol",
    /* LanguageIndonesian        */ "Bahasa Indonesia",
};

// ---------------------------------------------------------------------------
// Español
// ---------------------------------------------------------------------------
constexpr Table kSpanish{
    /* Yes                       */ "S\xC3\xAD",
    /* No                        */ "No",
    /* LabelMail                 */ "Correo: ",
    /* LabelEmail                */ "Correo: ",
    /* LabelPassword             */ "Contrase\xC3\xB1" "a: ",
    /* LabelLicense              */ "Licencia: ",
    /* LabelId                   */ "ID: ",
    /* LabelFrom                 */ "De: ",
    /* LabelSubject              */ "Asunto: ",
    /* LabelBody                 */ "Cuerpo: ",
    /* LabelRead                 */ "Le\xC3\xAD" "do: ",

    /* CliDescription            */ "Herramienta para extraer licencias "
                                   "gratuitas de cuentas de Eset NOD32 "
                                   "mediante web scraping",
    /* CliOptHelp                */ "Mostrar ayuda",
    /* CliOptVersion             */ "Mostrar versi\xC3\xB3n",
    /* CliOptNumber              */ "Cantidad de licencias",
    /* CliOptLength              */ "Longitud del dominio para correos "
                                   "temporales",
    /* CliOptProxy               */ "Archivo de lista de proxies "
                                   "(protocolo://ip:puerto)",
    /* CliOptLang                */ "Idioma de la interfaz: en, es, id",
    /* CliErrorParsingOptions    */ "Error al interpretar las opciones: ",
    /* CliLicensesGenerated      */ "Licencias generadas correctamente",
    /* CliDisclaimerTitle        */ "AVISO LEGAL:",
    /* CliDisclaimerBody         */ "Esta herramienta es solo para fines "
                                   "educativos. \xC3\x9A" "sela bajo su propio "
                                   "riesgo. No soy responsable de ning\xC3\xBAn "
                                   "da\xC3\xB1o causado por el uso de esta "
                                   "herramienta.",
    /* CliLanguageSelected       */ "Idioma: ",

    /* LoadingProxyFile          */ "Cargando archivo de proxies...",
    /* UsingProxies              */ "--- Usando proxies ---",
    /* UsingProxiesLatencyWarning*/ "Esto puede tardar un rato seg\xC3\xBAn la "
                                   "latencia del proxy",
    /* RetryingCreateAccount     */ "Reintentando crear la cuenta...",
    /* ErrorCreatingAccount      */ "Error al crear la cuenta",
    /* WaitingForVerificationCode*/ "Esperando el c\xC3\xB3" "digo de "
                                   "verificaci\xC3\xB3n...",
    /* RetryingGetLicense        */ "Reintentando obtener la licencia...",
    /* ErrorGettingLicense       */ "Error al obtener la licencia",
    /* NoLicenseWasGenerated     */ "No se gener\xC3\xB3 ninguna licencia",
    /* DataSavedTo               */ "Datos guardados en: ",

    /* UsingProxy                */ "Usando proxy: ",
    /* AccountCreated            */ "--- CUENTA ESET CREADA ---",
    /* VerificationFailed        */ "Verificaci\xC3\xB3n fallida",
    /* Retrying                  */ "Reintentando...",
    /* VerificationSuccessful    */ "Verificaci\xC3\xB3n correcta",
    /* LoggedIn                  */ "Sesi\xC3\xB3n iniciada",
    /* LoginFailed               */ "Error al iniciar sesi\xC3\xB3n",
    /* ErrorActivatingLicense    */ "Error al activar la licencia",
    /* NoLicenseFound            */ "No se encontr\xC3\xB3 ninguna licencia",
    /* AccountDoesNotExist       */ "La cuenta no existe",
    /* ErrorInPkceCallback       */ "Error en la respuesta PKCE",
    /* ErrorGettingJwtToken      */ "Error al obtener el token JWT",

    /* TempMailCreated           */ "--- CORREO TEMPORAL CREADO ---",

    /* ErrorInitializingProxy    */ "Error al inicializar el proxy: ",
    /* FailedToOpenFile          */ "No se pudo abrir el archivo: ",
    /* NoWorkingProxies          */ "No hay proxies funcionales disponibles...",
    /* LeavingProgram            */ "Saliendo del programa",

    /* ErrorCreatingDataDirectory*/ "Error al crear el directorio de datos: ",
    /* ErrorInitializingCurl     */ "Error al inicializar CURL",
    /* CurlHandleIsNull          */ "\xC2\xA1" "El identificador de curl es "
                                   "nulo!",

    /* QtCopy                    */ "\xF0\x9F\x93\x8B Copiar",
    /* QtGenerate                */ "\xE2\x9A\x99\xEF\xB8\x8F Generar",
    /* QtGenerating              */ "\xE2\x8F\xB3 Generando licencia...",
    /* QtNoLicenseGenerated      */ "\xE2\x9D\x8C No se gener\xC3\xB3 ninguna "
                                   "licencia.",
    /* QtLanguage                */ "Idioma",

    /* LanguageEnglish           */ "English",
    /* LanguageSpanish           */ "Espa\xC3\xB1ol",
    /* LanguageIndonesian        */ "Bahasa Indonesia",
};

// ---------------------------------------------------------------------------
// Bahasa Indonesia
// ---------------------------------------------------------------------------
constexpr Table kIndonesian{
    /* Yes                       */ "Ya",
    /* No                        */ "Tidak",
    /* LabelMail                 */ "Surel: ",
    /* LabelEmail                */ "Surel: ",
    /* LabelPassword             */ "Kata sandi: ",
    /* LabelLicense              */ "Lisensi: ",
    /* LabelId                   */ "ID: ",
    /* LabelFrom                 */ "Dari: ",
    /* LabelSubject              */ "Subjek: ",
    /* LabelBody                 */ "Isi: ",
    /* LabelRead                 */ "Dibaca: ",

    /* CliDescription            */ "Alat untuk mengambil lisensi gratis dari "
                                   "akun antivirus Eset NOD32 melalui web "
                                   "scraping",
    /* CliOptHelp                */ "Tampilkan bantuan",
    /* CliOptVersion             */ "Tampilkan versi",
    /* CliOptNumber              */ "Jumlah lisensi",
    /* CliOptLength              */ "Panjang domain untuk surel sementara",
    /* CliOptProxy               */ "Berkas daftar proxy (protokol://ip:port)",
    /* CliOptLang                */ "Bahasa antarmuka: en, es, id",
    /* CliErrorParsingOptions    */ "Gagal membaca opsi: ",
    /* CliLicensesGenerated      */ "Lisensi berhasil dibuat",
    /* CliDisclaimerTitle        */ "DISCLAIMER:",
    /* CliDisclaimerBody         */ "Alat ini hanya untuk tujuan edukasi. "
                                   "Gunakan dengan risiko Anda sendiri. Saya "
                                   "tidak bertanggung jawab atas kerusakan apa "
                                   "pun akibat penggunaan alat ini.",
    /* CliLanguageSelected       */ "Bahasa: ",

    /* LoadingProxyFile          */ "Memuat berkas proxy...",
    /* UsingProxies              */ "--- Menggunakan proxy ---",
    /* UsingProxiesLatencyWarning*/ "Proses ini bisa memakan waktu tergantung "
                                   "latensi proxy",
    /* RetryingCreateAccount     */ "Mencoba membuat akun lagi...",
    /* ErrorCreatingAccount      */ "Gagal membuat akun",
    /* WaitingForVerificationCode*/ "Menunggu kode verifikasi...",
    /* RetryingGetLicense        */ "Mencoba mengambil lisensi lagi...",
    /* ErrorGettingLicense       */ "Gagal mengambil lisensi",
    /* NoLicenseWasGenerated     */ "Tidak ada lisensi yang dihasilkan",
    /* DataSavedTo               */ "Data disimpan di: ",

    /* UsingProxy                */ "Menggunakan proxy: ",
    /* AccountCreated            */ "--- AKUN ESET DIBUAT ---",
    /* VerificationFailed        */ "Verifikasi gagal",
    /* Retrying                  */ "Mencoba lagi...",
    /* VerificationSuccessful    */ "Verifikasi berhasil",
    /* LoggedIn                  */ "Berhasil masuk",
    /* LoginFailed               */ "Gagal masuk",
    /* ErrorActivatingLicense    */ "Gagal mengaktifkan lisensi",
    /* NoLicenseFound            */ "Lisensi tidak ditemukan",
    /* AccountDoesNotExist       */ "Akun tidak ada",
    /* ErrorInPkceCallback       */ "Gagal pada callback PKCE",
    /* ErrorGettingJwtToken      */ "Gagal mendapatkan token JWT",

    /* TempMailCreated           */ "--- SUREL SEMENTARA DIBUAT ---",

    /* ErrorInitializingProxy    */ "Gagal menginisialisasi proxy: ",
    /* FailedToOpenFile          */ "Gagal membuka berkas: ",
    /* NoWorkingProxies          */ "Tidak ada proxy yang berfungsi...",
    /* LeavingProgram            */ "Keluar dari program",

    /* ErrorCreatingDataDirectory*/ "Gagal membuat direktori data: ",
    /* ErrorInitializingCurl     */ "Gagal menginisialisasi CURL",
    /* CurlHandleIsNull          */ "handle curl bernilai null!",

    /* QtCopy                    */ "\xF0\x9F\x93\x8B Salin",
    /* QtGenerate                */ "\xE2\x9A\x99\xEF\xB8\x8F Buat",
    /* QtGenerating              */ "\xE2\x8F\xB3 Membuat lisensi...",
    /* QtNoLicenseGenerated      */ "\xE2\x9D\x8C Tidak ada lisensi yang "
                                   "dihasilkan.",
    /* QtLanguage                */ "Bahasa",

    /* LanguageEnglish           */ "English",
    /* LanguageSpanish           */ "Espa\xC3\xB1ol",
    /* LanguageIndonesian        */ "Bahasa Indonesia",
};

// Compile-time guard: every table must be fully populated and all entries
// non-empty. A missing translation is therefore a build error, never a silent
// English string in the middle of a Spanish session.
constexpr bool isComplete(const Table &table) {
  for (const char *entry : table) {
    if (entry == nullptr || entry[0] == '\0') {
      return false;
    }
  }
  return true;
}

static_assert(isComplete(kEnglish), "English table is incomplete");
static_assert(isComplete(kSpanish), "Spanish table is incomplete");
static_assert(isComplete(kIndonesian), "Indonesian table is incomplete");

const Table &tableFor(Lang lang) {
  switch (lang) {
  case Lang::Spanish:
    return kSpanish;
  case Lang::Indonesian:
    return kIndonesian;
  case Lang::English:
  default:
    return kEnglish;
  }
}

/// Lower-cases ASCII letters so "ES_es" and "es" compare equal.
std::string toLowerAscii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

/// Parses a locale-ish code such as "es", "es_ES" or "id-ID.UTF-8" into a
/// Lang. Returns false when the code is not one we ship translations for.
bool parseCode(const std::string &code, Lang &out) {
  std::string normalized = toLowerAscii(code);

  // Drop any encoding suffix ("es_ES.UTF-8" -> "es_ES").
  const std::size_t suffix = normalized.find_first_of(".-");
  if (suffix != std::string::npos) {
    normalized = normalized.substr(0, suffix);
  }

  // Keep only the language part ("es_ES" -> "es").
  const std::size_t separator = normalized.find_first_of("-_");
  if (separator != std::string::npos) {
    normalized = normalized.substr(0, separator);
  }

  if (normalized == "en") {
    out = Lang::English;
    return true;
  }
  if (normalized == "es") {
    out = Lang::Spanish;
    return true;
  }
  if (normalized == "id" || normalized == "in") {
    // "in" is the legacy ISO 639 code for Indonesian.
    out = Lang::Indonesian;
    return true;
  }
  return false;
}

/// Reads an environment variable, returning "" when unset.
std::string readEnv(const char *name) { return environment::get(name); }

} // namespace

Translator &Translator::instance() {
  static Translator translator;
  return translator;
}

std::string Translator::tr(Key key) const {
  const std::size_t index = static_cast<std::size_t>(key);
  if (index >= kKeyCount) {
    return {};
  }
  return tableFor(language_)[index];
}

Lang Translator::fromCode(const std::string &code, Lang fallback) noexcept {
  Lang parsed = fallback;
  return parseCode(code, parsed) ? parsed : fallback;
}

std::string Translator::toCode(Lang lang) {
  switch (lang) {
  case Lang::Spanish:
    return "es";
  case Lang::Indonesian:
    return "id";
  case Lang::English:
  default:
    return "en";
  }
}

Lang Translator::fromEnvironment(Lang fallback) noexcept {
  for (const char *name : {"LANGUAGE", "LC_ALL", "LC_MESSAGES", "LANG"}) {
    const std::string value = readEnv(name);
    if (value.empty()) {
      continue;
    }

    // LANGUAGE may hold a priority list such as "es:en".
    const std::size_t listEnd = value.find(':');
    const std::string first =
        listEnd == std::string::npos ? value : value.substr(0, listEnd);

    Lang detected = fallback;
    if (parseCode(first, detected)) {
      return detected;
    }
  }
  return fallback;
}

std::string Translator::displayName(Lang lang) {
  // Language names are never translated, so the entry is read from the
  // language's own table: every table carries the same three native names.
  Key key = Key::LanguageEnglish;
  switch (lang) {
  case Lang::Spanish:
    key = Key::LanguageSpanish;
    break;
  case Lang::Indonesian:
    key = Key::LanguageIndonesian;
    break;
  case Lang::English:
  default:
    key = Key::LanguageEnglish;
    break;
  }
  return tableFor(lang)[static_cast<std::size_t>(key)];
}

std::string tr(Key key) { return Translator::instance().tr(key); }

} // namespace i18n
