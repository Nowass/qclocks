#include "clock_engine.hpp"
#include "clock_rules.hpp"

// Map hour token: hour12 (1-12) -> TokenId
static TokenId hour_token(uint8_t hour12)
{
    static constexpr TokenId hour_map[] = {
        TokenId::TOKEN_INVALID,      // 0 – unused
        TokenId::HOUR_1_JEDNA,       // 1
        TokenId::HOUR_2_DVE,         // 2
        TokenId::HOUR_3_TRI,         // 3
        TokenId::HOUR_4_CTYRI,       // 4
        TokenId::HOUR_5_PET,         // 5
        TokenId::HOUR_6_SEST,        // 6
        TokenId::HOUR_7_SEDM,        // 7
        TokenId::HOUR_8_OSM,         // 8
        TokenId::HOUR_9_DEVET,       // 9
        TokenId::HOUR_10_DESET,      // 10
        TokenId::HOUR_11_JEDENACT,   // 11
        TokenId::HOUR_12_DVANACT,    // 12
    };
    if (hour12 < 1 || hour12 > 12) return TokenId::TOKEN_INVALID;
    return hour_map[hour12];
}

// Map unit token: unit (1-9) -> TokenId
static TokenId unit_token(uint8_t unit)
{
    static constexpr TokenId unit_map[] = {
        TokenId::TOKEN_INVALID,  // 0 – unused
        TokenId::UNIT_1_JEDNA,   // 1
        TokenId::UNIT_2_DVE,     // 2
        TokenId::UNIT_3_TRI,     // 3
        TokenId::UNIT_4_CTYRI,   // 4
        TokenId::UNIT_5_PET,     // 5
        TokenId::UNIT_6_SEST,    // 6
        TokenId::UNIT_7_SEDM,    // 7
        TokenId::UNIT_8_OSM,     // 8
        TokenId::UNIT_9_DEVET,   // 9
    };
    if (unit < 1 || unit > 9) return TokenId::TOKEN_INVALID;
    return unit_map[unit];
}

std::vector<TokenId> render_time_tokens(uint8_t hour24, uint8_t minute)
{
    // Convert to 12-hour format (1-12)
    uint8_t hour12 = hour24 % 12;
    if (hour12 == 0) hour12 = 12;

    std::vector<TokenId> tokens;
    tokens.reserve(4);

    tokens.push_back(choose_prefix(hour12, minute));
    tokens.push_back(hour_token(hour12));

    if (minute == 0) {
        // exact hour – no minute tokens

    } else if (minute >= 1 && minute <= 9) {
        tokens.push_back(TokenId::MIN_0_NULA);
        tokens.push_back(unit_token(minute));

    } else if (minute == 10) {
        tokens.push_back(TokenId::MIN_10_DESET);

    } else if (minute >= 11 && minute <= 14) {
        // BUDE + next quarter (already set by choose_prefix)
        tokens.push_back(TokenId::MIN_15_PATNACT);

    } else if (minute == 15) {
        tokens.push_back(TokenId::MIN_15_PATNACT);

    } else if (minute >= 16 && minute <= 19) {
        // BUDE + next fifth (already set by choose_prefix)
        tokens.push_back(TokenId::MIN_20_DVACET);

    } else if (minute == 20) {
        tokens.push_back(TokenId::MIN_20_DVACET);

    } else if (minute >= 21 && minute <= 29) {
        tokens.push_back(TokenId::MIN_20_DVACET);
        tokens.push_back(unit_token(minute - 20));

    } else if (minute == 30) {
        tokens.push_back(TokenId::MIN_30_TRICET);

    } else if (minute >= 31 && minute <= 39) {
        tokens.push_back(TokenId::MIN_30_TRICET);
        tokens.push_back(unit_token(minute - 30));

    } else if (minute == 40) {
        tokens.push_back(TokenId::MIN_40_CTYRICET);

    } else if (minute >= 41 && minute <= 49) {
        tokens.push_back(TokenId::MIN_40_CTYRICET);
        tokens.push_back(unit_token(minute - 40));

    } else if (minute == 50) {
        tokens.push_back(TokenId::MIN_50_PADESAT);

    } else if (minute >= 51 && minute <= 59) {
        tokens.push_back(TokenId::MIN_50_PADESAT);
        tokens.push_back(unit_token(minute - 50));
    }

    return tokens;
}
