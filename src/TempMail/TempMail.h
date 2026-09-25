#pragma once

#include "../Helpers/Console.h"
#include "../Scrapper.h"
#include "Message.h"

#include <string>
#include <vector>

/// A throwaway mailbox on mail.tm.
///
/// Construction registers the address immediately, so a successfully built
/// TempMail is always a usable mailbox; failure is reported by leaving the
/// address empty, which `isValid()` exposes.
class TempMail : public Scrapper {
public:
  /// Requests a new mailbox whose local part is `addressLength` characters.
  explicit TempMail(int addressLength);

  bool isValid() const noexcept { return !email_.empty(); }

  const std::string &email() const noexcept { return email_; }

  /// Refreshes `messages` from the server. Appends to the existing list.
  bool getMessages();

  /// Messages fetched so far, oldest first.
  const std::vector<Message> &messages() const noexcept { return messages_; }

  /// Fetches the full text of one message, marks it read and returns it.
  Message readMessage(const std::string &id);

private:
  // --- Registration --------------------------------------------------------

  /// Asks mail.tm for a free domain and builds a random address on it.
  /// Returns an empty string when no domain could be obtained.
  std::string generateAddress();

  /// Registers `address` with the mail.tm accounts endpoint.
  bool createAccount(const std::string &address);

  /// Exchanges the mailbox credentials for the API token used by the rest of
  /// the calls.
  bool requestToken();

  /// Bare domain ("example.com") to append addresses to, or "" on failure.
  std::string fetchDomain();

  // --- Local bookkeeping ---------------------------------------------------

  /// True when `address` appears in the rejected-addresses log.
  static bool isKnownRejectedAddress(const std::string &address);

  /// Appends `address` to the rejected-addresses log.
  static void rememberRejectedAddress(const std::string &address);

  void setHeaders() override;
  void waitForRequest() const;

  std::vector<Message> messages_;
  std::string email_;
  std::string id_;
  int addressLength_ = 15;
};
