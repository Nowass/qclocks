#include "clock_rules.hpp"

TokenId choose_prefix(uint8_t hour12, uint8_t minute)
{
    // BUDE for the "rounded up" minute ranges (spec section 6.3)
    if ((minute >= 11 && minute <= 14) || (minute >= 16 && minute <= 19)) {
        return TokenId::PREFIX_BUDE;
    }

    // JSOU for hours 2, 3, 4
    if (hour12 == 2 || hour12 == 3 || hour12 == 4) {
        return TokenId::PREFIX_JSOU;
    }

    return TokenId::PREFIX_JE;
}
