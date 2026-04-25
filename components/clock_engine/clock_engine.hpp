#pragma once

#include "word_tokens.hpp"
#include <cstdint>
#include <vector>

// Convert a 24-hour time value into the set of display tokens (spec section 6.2).
// hour24 : 0-23
// minute : 0-59
// Returns tokens in reading order: prefix, hour, [minute word], [unit].
std::vector<TokenId> render_time_tokens(uint8_t hour24, uint8_t minute);
