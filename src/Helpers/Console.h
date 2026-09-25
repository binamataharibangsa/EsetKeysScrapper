#pragma once

// ---------------------------------------------------------------------------
// The one header allowed to pull in "everything the program prints with".
//
// Every source file in this project speaks to the terminal, so nearly all of
// them reach for colour, cURL, JSON and the standard library. Historically
// those includes were smuggled in through Helpers/Crypto.h, which turned a
// small PKCE helper into the project's de-facto precompiled header: changing
// anything about the Crypto API broke files that had nothing to do with it.
//
// Crypto.h now declares only Crypto. This header takes over the "ambient
// includes" role explicitly, so the dependency is visible at the call site
// instead of being an accident of include order.
//
// `using namespace std` deliberately is NOT re-exported: it is what let
// identifiers such as `string`, `vector` and `cout` be written unqualified and
// resolve to whatever the *last* included header happened to drag in, which is
// exactly the fragility this refactor removes. Every source file now spells
// `std::` out and includes the standard headers it actually uses.
// ---------------------------------------------------------------------------

#include <curl/curl.h>

#include "dependencies/json.hpp"

// The colour macros (RED / GREEN / YELLOW / CYAN / LGREEN / RESET) come from the
// vendored gist rather than being redefined here, so there is still a single
// source of truth for the escape sequences. This include is the only reason
// Console.h has to exist at all; it is kept as a real header so that the
// "screen output" concern has one obvious home.
#include "dependencies/colors.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using json = nlohmann::json;
