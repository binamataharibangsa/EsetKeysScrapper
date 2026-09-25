#pragma once

#include <string>

/// Copies `text` to the system clipboard.
///
/// Returns false when no clipboard mechanism is available or the copy failed.
/// Never throws and never aborts the caller: failing to copy is not a reason to
/// lose the licence that was just generated.
bool copyToClipboard(const std::string &text);
