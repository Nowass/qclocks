#pragma once

#include "word_tokens.hpp"
#include <cstdint>

// Select the appropriate prefix token for a given (hour12, minute) pair.
// Rules (spec section 6.3):
//   minute in 11-14 or 16-19  -> PREFIX_BUDE
//   hour12 in {2, 3, 4}       -> PREFIX_JSOU
//   otherwise                 -> PREFIX_JE
TokenId choose_prefix(uint8_t hour12, uint8_t minute);
