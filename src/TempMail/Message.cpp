#include "Message.h"

#include "../Helpers/Console.h"
#include "../I18n/I18n.h"

#include <iostream>
#include <string>

void Message::print() const {
  using i18n::Key;
  using i18n::tr;

  std::cout << YELLOW << tr(Key::LabelId) << RESET << id << std::endl
            << YELLOW << tr(Key::LabelRead) << RESET
            << tr(read ? Key::Yes : Key::No) << std::endl
            << YELLOW << tr(Key::LabelFrom) << RESET << from << std::endl
            << YELLOW << tr(Key::LabelSubject) << RESET << subject << std::endl
            << YELLOW << tr(Key::LabelBody) << RESET << body << std::endl;
}
