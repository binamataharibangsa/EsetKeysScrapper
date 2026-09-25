#pragma once

#include <string>

/// One message as returned by the mail.tm listing endpoint.
struct Message {
  std::string id;
  std::string from;
  std::string subject;
  std::string body;
  bool read = false;

  /// Dumps the message to stdout using the translated field labels.
  void print() const;
};
