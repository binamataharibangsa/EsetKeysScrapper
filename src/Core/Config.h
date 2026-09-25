#pragma once

// ---------------------------------------------------------------------------
// Central configuration.
//
// Every compile-time constant that used to sit inline in the sources lives
// here, so a value only has to change in one place and the intent behind each
// literal is documented next to it.
// ---------------------------------------------------------------------------
namespace config {

// --- Application identity --------------------------------------------------
inline constexpr const char *kAppName = "EsetKeysScrapper";
inline constexpr const char *kAppVersion = "1.3";
inline constexpr const char *kAppAuthor = "Xooter";

// --- Eset account ----------------------------------------------------------
// Password given to every throwaway account. Not a secret: the account only
// has to live long enough to mint a trial licence.
inline constexpr const char *kAccountPassword = "rula123123123R";
inline constexpr const char *kBrowserFingerprint =
    "593b2e9b81d07f79a98bb130b99bdc80";
// Sent as a JSON string even though it is a number; the API expects that.
inline constexpr const char *kSelectedCountry = "12";
inline constexpr const char *kTrialProductCode = "148";

// --- Eset endpoints --------------------------------------------------------
inline constexpr const char *kEsetLoginHost = "login.eset.com";
inline constexpr const char *kEsetHomeHost = "home.eset.com";
inline constexpr const char *kCreateAccountUrl =
    "https://login.eset.com/api/Account/Create";
inline constexpr const char *kLoginAccountUrl =
    "https://login.eset.com/api/Account/Login";
inline constexpr const char *kTokenUrl =
    "https://login.eset.com/connect/token";
inline constexpr const char *kActivateTrialUrl =
    "https://home.eset.com/api/License/ActivateTrialLicense";
inline constexpr const char *kGetAllLicensesUrl =
    "https://home.eset.com/api/License/GetAll";
inline constexpr const char *kRegistrationCallbackUrl =
    "https://home.eset.com/callback";

// --- OAuth / PKCE ----------------------------------------------------------
// Public client id of the ESET web app the browser flow impersonates.
inline constexpr const char *kPkceClientId = "myeset";
// Base the authorization URL is relative to.
inline constexpr const char *kLoginHostUrl = "https://login.eset.com";
// Prefix prepended to the `redirectUrl` returned by the login endpoint.
inline constexpr const char *kPkceRedirectPrefix = "https://login.eset.com";

// --- Activation e-mail -----------------------------------------------------
inline constexpr const char *kConfirmationSender = "info@product.eset.com";
inline constexpr const char *kConfirmationSubject = "Account confirmation";

// --- Temporary mailbox (mail.tm) -------------------------------------------
inline constexpr const char *kMailApiBaseUrl = "https://api.mail.tm";
inline constexpr const char *kMailApiHost = "api.mail.tm";
inline constexpr const char *kMailPassword = "EsetKeys";
inline constexpr const char *kExistingAddressesFile = "existing_addresses.txt";
inline constexpr const char *kMailMessagesPath = "/messages?page=1";

// --- HTTP ------------------------------------------------------------------
inline constexpr long kRequestTimeoutSeconds = 15;
inline constexpr const char *kAcceptEncoding = "UTF-8";
// Timeout for the throwaway "is this proxy alive?" request. Shorter than a
// normal request because a dead proxy in a long list should be skipped fast.
inline constexpr long kProxyProbeTimeoutSeconds = 15;
inline constexpr const char *kProxyProbeUrl = "http://google.com/";

// --- Retry policy and pacing ----------------------------------------------
inline constexpr int kMaxAccountCreationAttempts = 5;
inline constexpr int kMaxLicenseAttempts = 5;
inline constexpr int kMaxAddressGenerationAttempts = 20;
inline constexpr int kMaxTokenAttempts = 5;
// Upper bound on how many proxies are scanned for a working one per account.
inline constexpr int kMaxProxyAttempts = 20;
// How many times to poll the mailbox for the confirmation mail before giving
// up on an account.
inline constexpr int kMaxActivationAttempts = 30;
// Pause between "has the activation mail arrived yet?" polls.
inline constexpr int kActivationPollDelayMs = 100;
// Pause after a rejected mailbox creation / token request.
inline constexpr int kRequestThrottleMs = 500;

// --- Input validation ------------------------------------------------------
inline constexpr int kMinLicenseCount = 1;
inline constexpr int kMinDomainLength = 5;
inline constexpr int kMaxDomainLength = 25;
inline constexpr int kDefaultDomainLength = 10;

// --- Storage ---------------------------------------------------------------
inline constexpr const char *kDataDirectoryName = "data";
inline constexpr const char *kDataFileName = "data.csv";
inline constexpr const char *kCookieFilePrefix = "eset_cookies_";
inline constexpr const char *kCookieFileSuffix = ".txt";
// Optional override for where generated data is written.
inline constexpr const char *kDataDirectoryEnvVar = "ESETKEYS_DATA_DIR";

} // namespace config
