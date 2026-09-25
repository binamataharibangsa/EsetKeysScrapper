#include "Clipboard.h"

#include <cstddef>
#include <cstring>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#else
#include <cstdio>
#endif

#if !defined(_WIN32)
namespace {

/// Feeds `text` to `command` on its standard input.
///
/// The text is written through a pipe rather than interpolated into a shell
/// command line, so a licence key containing shell metacharacters cannot be
/// interpreted as commands.
bool pipeTo(const char *command, const std::string &text) {
  FILE *pipe = popen(command, "w");
  if (pipe == nullptr) {
    return false;
  }
  const std::size_t written = std::fwrite(text.data(), 1, text.size(), pipe);
  const int status = pclose(pipe);
  return written == text.size() && status == 0;
}

} // namespace
#endif

bool copyToClipboard(const std::string &text) {
  if (text.empty()) {
    return false;
  }

#if defined(_WIN32)
  if (OpenClipboard(nullptr) == 0) {
    return false;
  }
  EmptyClipboard();

  const std::size_t bytes = text.size() + 1;
  HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, bytes);
  if (handle == nullptr) {
    CloseClipboard();
    return false;
  }

  void *target = GlobalLock(handle);
  if (target == nullptr) {
    GlobalFree(handle);
    CloseClipboard();
    return false;
  }
  std::memcpy(target, text.c_str(), bytes);
  GlobalUnlock(handle);

  // Ownership of `handle` passes to the clipboard; it must not be freed here.
  if (SetClipboardData(CF_TEXT, handle) == nullptr) {
    GlobalFree(handle);
    CloseClipboard();
    return false;
  }

  CloseClipboard();
  return true;
#else
  // Wayland first, then X11.
  return pipeTo("wl-copy", text) ||
         pipeTo("xclip -selection clipboard", text);
#endif
}
