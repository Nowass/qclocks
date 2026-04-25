#pragma once

#include "word_tokens.hpp"
#include "app_types.hpp"
#include <span>

// Returns the list of matrix cells occupied by a given token.
// Returns an empty span for TOKEN_INVALID or out-of-range values.
std::span<const CellCoord> token_to_cells(TokenId token);
