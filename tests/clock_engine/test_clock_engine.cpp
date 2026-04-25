#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include "clock_engine.hpp"
#include "clock_rules.hpp"
#include "word_tokens.hpp"

#include <vector>
#include <algorithm>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool is_valid_prefix(TokenId t)
{
    return t == TokenId::PREFIX_JE || t == TokenId::PREFIX_JSOU || t == TokenId::PREFIX_BUDE;
}

static bool is_valid_hour(TokenId t)
{
    return t >= TokenId::HOUR_1_JEDNA && t <= TokenId::HOUR_12_DVANACT;
}

static bool contains_invalid(const std::vector<TokenId>& v)
{
    return std::any_of(v.begin(), v.end(),
                       [](TokenId t) { return t == TokenId::TOKEN_INVALID; });
}

// ---------------------------------------------------------------------------
// a) choose_prefix
// ---------------------------------------------------------------------------

TEST_CASE("choose_prefix - minutes 11-14 always return PREFIX_BUDE", "[prefix]")
{
    for (uint8_t min = 11; min <= 14; ++min) {
        for (uint8_t h = 1; h <= 12; ++h) {
            INFO("hour=" << (int)h << " minute=" << (int)min);
            REQUIRE(choose_prefix(h, min) == TokenId::PREFIX_BUDE);
        }
    }
}

TEST_CASE("choose_prefix - minutes 16-19 always return PREFIX_BUDE", "[prefix]")
{
    for (uint8_t min = 16; min <= 19; ++min) {
        for (uint8_t h = 1; h <= 12; ++h) {
            INFO("hour=" << (int)h << " minute=" << (int)min);
            REQUIRE(choose_prefix(h, min) == TokenId::PREFIX_BUDE);
        }
    }
}

TEST_CASE("choose_prefix - hours 2,3,4 return PREFIX_JSOU outside BUDE ranges", "[prefix]")
{
    // All minutes NOT in 11-14 or 16-19
    for (uint8_t min : {0, 1, 5, 10, 15, 20, 21, 29, 30, 31, 39, 40, 41, 49, 50, 51, 59}) {
        for (uint8_t h : {2, 3, 4}) {
            INFO("hour=" << (int)h << " minute=" << (int)min);
            REQUIRE(choose_prefix(h, min) == TokenId::PREFIX_JSOU);
        }
    }
}

TEST_CASE("choose_prefix - hours 1,5-12 return PREFIX_JE outside BUDE ranges", "[prefix]")
{
    for (uint8_t min : {0, 1, 5, 10, 15, 20, 21, 29, 30, 31, 39, 40, 41, 49, 50, 51, 59}) {
        for (uint8_t h : {1, 5, 6, 7, 8, 9, 10, 11, 12}) {
            INFO("hour=" << (int)h << " minute=" << (int)min);
            REQUIRE(choose_prefix(h, min) == TokenId::PREFIX_JE);
        }
    }
}

TEST_CASE("choose_prefix - boundary: minute 10 hour 2 -> PREFIX_JSOU (not BUDE)", "[prefix]")
{
    REQUIRE(choose_prefix(2, 10) == TokenId::PREFIX_JSOU);
}

TEST_CASE("choose_prefix - boundary: minute 15 hour 3 -> PREFIX_JSOU (not BUDE)", "[prefix]")
{
    REQUIRE(choose_prefix(3, 15) == TokenId::PREFIX_JSOU);
}

TEST_CASE("choose_prefix - boundary: minute 20 hour 4 -> PREFIX_JSOU (not BUDE)", "[prefix]")
{
    REQUIRE(choose_prefix(4, 20) == TokenId::PREFIX_JSOU);
}

TEST_CASE("choose_prefix - boundary: minute 10 hour 7 -> PREFIX_JE (not BUDE)", "[prefix]")
{
    REQUIRE(choose_prefix(7, 10) == TokenId::PREFIX_JE);
}

// ---------------------------------------------------------------------------
// b) render_time_tokens – structural tests
// ---------------------------------------------------------------------------

TEST_CASE("render_time_tokens - minute 0: exactly 2 tokens, no minute/unit tokens", "[structure]")
{
    auto tokens = render_time_tokens(7, 0);
    REQUIRE(tokens.size() == 2);
    REQUIRE(is_valid_prefix(tokens[0]));
    REQUIRE(is_valid_hour(tokens[1]));
}

TEST_CASE("render_time_tokens - minute 5: [prefix, hour, MIN_0_NULA, UNIT_5_PET]", "[structure]")
{
    auto tokens = render_time_tokens(7, 5);
    REQUIRE(tokens.size() == 4);
    CHECK(is_valid_prefix(tokens[0]));
    CHECK(is_valid_hour(tokens[1]));
    CHECK(tokens[2] == TokenId::MIN_0_NULA);
    CHECK(tokens[3] == TokenId::UNIT_5_PET);
}

TEST_CASE("render_time_tokens - minute 10: exactly 3 tokens, third is MIN_10_DESET", "[structure]")
{
    auto tokens = render_time_tokens(7, 10);
    REQUIRE(tokens.size() == 3);
    CHECK(is_valid_prefix(tokens[0]));
    CHECK(is_valid_hour(tokens[1]));
    CHECK(tokens[2] == TokenId::MIN_10_DESET);
}

TEST_CASE("render_time_tokens - minutes 11-14: prefix=BUDE, third=MIN_15_PATNACT", "[structure]")
{
    for (uint8_t min = 11; min <= 14; ++min) {
        INFO("minute=" << (int)min);
        auto tokens = render_time_tokens(7, min);
        REQUIRE(tokens.size() == 3);
        CHECK(tokens[0] == TokenId::PREFIX_BUDE);
        CHECK(is_valid_hour(tokens[1]));
        CHECK(tokens[2] == TokenId::MIN_15_PATNACT);
    }
}

TEST_CASE("render_time_tokens - minute 15: normal prefix (not BUDE for hour 7), third=MIN_15_PATNACT", "[structure]")
{
    auto tokens = render_time_tokens(7, 15);
    REQUIRE(tokens.size() == 3);
    CHECK(tokens[0] == TokenId::PREFIX_JE);  // hour 7 → JE
    CHECK(is_valid_hour(tokens[1]));
    CHECK(tokens[2] == TokenId::MIN_15_PATNACT);
}

TEST_CASE("render_time_tokens - minute 15 with hour 2: PREFIX_JSOU (not BUDE)", "[structure]")
{
    auto tokens = render_time_tokens(2, 15);
    REQUIRE(tokens.size() == 3);
    CHECK(tokens[0] == TokenId::PREFIX_JSOU);
    CHECK(tokens[2] == TokenId::MIN_15_PATNACT);
}

TEST_CASE("render_time_tokens - minutes 16-19: prefix=BUDE, third=MIN_20_DVACET", "[structure]")
{
    for (uint8_t min = 16; min <= 19; ++min) {
        INFO("minute=" << (int)min);
        auto tokens = render_time_tokens(7, min);
        REQUIRE(tokens.size() == 3);
        CHECK(tokens[0] == TokenId::PREFIX_BUDE);
        CHECK(is_valid_hour(tokens[1]));
        CHECK(tokens[2] == TokenId::MIN_20_DVACET);
    }
}

TEST_CASE("render_time_tokens - minute 20: exactly 3 tokens, third=MIN_20_DVACET", "[structure]")
{
    auto tokens = render_time_tokens(7, 20);
    REQUIRE(tokens.size() == 3);
    CHECK(is_valid_prefix(tokens[0]));
    CHECK(is_valid_hour(tokens[1]));
    CHECK(tokens[2] == TokenId::MIN_20_DVACET);
}

TEST_CASE("render_time_tokens - minute 23: [prefix, hour, MIN_20_DVACET, UNIT_3_TRI]", "[structure]")
{
    auto tokens = render_time_tokens(7, 23);
    REQUIRE(tokens.size() == 4);
    CHECK(is_valid_prefix(tokens[0]));
    CHECK(is_valid_hour(tokens[1]));
    CHECK(tokens[2] == TokenId::MIN_20_DVACET);
    CHECK(tokens[3] == TokenId::UNIT_3_TRI);
}

TEST_CASE("render_time_tokens - minute 30: exactly 3 tokens, third=MIN_30_TRICET", "[structure]")
{
    auto tokens = render_time_tokens(7, 30);
    REQUIRE(tokens.size() == 3);
    CHECK(is_valid_prefix(tokens[0]));
    CHECK(is_valid_hour(tokens[1]));
    CHECK(tokens[2] == TokenId::MIN_30_TRICET);
}

TEST_CASE("render_time_tokens - minute 45: [prefix, hour, MIN_40_CTYRICET, UNIT_5_PET]", "[structure]")
{
    auto tokens = render_time_tokens(7, 45);
    REQUIRE(tokens.size() == 4);
    CHECK(is_valid_prefix(tokens[0]));
    CHECK(is_valid_hour(tokens[1]));
    CHECK(tokens[2] == TokenId::MIN_40_CTYRICET);
    CHECK(tokens[3] == TokenId::UNIT_5_PET);
}

TEST_CASE("render_time_tokens - minute 59: [prefix, hour, MIN_50_PADESAT, UNIT_9_DEVET]", "[structure]")
{
    auto tokens = render_time_tokens(7, 59);
    REQUIRE(tokens.size() == 4);
    CHECK(is_valid_prefix(tokens[0]));
    CHECK(is_valid_hour(tokens[1]));
    CHECK(tokens[2] == TokenId::MIN_50_PADESAT);
    CHECK(tokens[3] == TokenId::UNIT_9_DEVET);
}

// ---------------------------------------------------------------------------
// c) render_time_tokens – hour mapping (all 24 hour24 values at minute 0)
// ---------------------------------------------------------------------------

TEST_CASE("render_time_tokens - hour24=0 maps to HOUR_12_DVANACT", "[hour_mapping]")
{
    auto tokens = render_time_tokens(0, 0);
    REQUIRE(tokens.size() == 2);
    CHECK(tokens[1] == TokenId::HOUR_12_DVANACT);
}

TEST_CASE("render_time_tokens - hour24=1 maps to HOUR_1_JEDNA", "[hour_mapping]")
{
    auto tokens = render_time_tokens(1, 0);
    REQUIRE(tokens.size() == 2);
    CHECK(tokens[1] == TokenId::HOUR_1_JEDNA);
}

TEST_CASE("render_time_tokens - hour24=12 maps to HOUR_12_DVANACT", "[hour_mapping]")
{
    auto tokens = render_time_tokens(12, 0);
    REQUIRE(tokens.size() == 2);
    CHECK(tokens[1] == TokenId::HOUR_12_DVANACT);
}

TEST_CASE("render_time_tokens - hour24=13 maps to HOUR_1_JEDNA", "[hour_mapping]")
{
    auto tokens = render_time_tokens(13, 0);
    REQUIRE(tokens.size() == 2);
    CHECK(tokens[1] == TokenId::HOUR_1_JEDNA);
}

TEST_CASE("render_time_tokens - all 24 hour24 values at minute 0 map to valid hours", "[hour_mapping]")
{
    // Expected hour12 for each hour24 (0..23)
    static constexpr TokenId expected_hour_token[24] = {
        TokenId::HOUR_12_DVANACT,  //  0 → 12
        TokenId::HOUR_1_JEDNA,     //  1 → 1
        TokenId::HOUR_2_DVE,       //  2 → 2
        TokenId::HOUR_3_TRI,       //  3 → 3
        TokenId::HOUR_4_CTYRI,     //  4 → 4
        TokenId::HOUR_5_PET,       //  5 → 5
        TokenId::HOUR_6_SEST,      //  6 → 6
        TokenId::HOUR_7_SEDM,      //  7 → 7
        TokenId::HOUR_8_OSM,       //  8 → 8
        TokenId::HOUR_9_DEVET,     //  9 → 9
        TokenId::HOUR_10_DESET,    // 10 → 10
        TokenId::HOUR_11_JEDENACT, // 11 → 11
        TokenId::HOUR_12_DVANACT,  // 12 → 12
        TokenId::HOUR_1_JEDNA,     // 13 → 1
        TokenId::HOUR_2_DVE,       // 14 → 2
        TokenId::HOUR_3_TRI,       // 15 → 3
        TokenId::HOUR_4_CTYRI,     // 16 → 4
        TokenId::HOUR_5_PET,       // 17 → 5
        TokenId::HOUR_6_SEST,      // 18 → 6
        TokenId::HOUR_7_SEDM,      // 19 → 7
        TokenId::HOUR_8_OSM,       // 20 → 8
        TokenId::HOUR_9_DEVET,     // 21 → 9
        TokenId::HOUR_10_DESET,    // 22 → 10
        TokenId::HOUR_11_JEDENACT, // 23 → 11
    };

    for (uint8_t h24 = 0; h24 < 24; ++h24) {
        INFO("hour24=" << (int)h24);
        auto tokens = render_time_tokens(h24, 0);
        REQUIRE(tokens.size() == 2);
        CHECK(tokens[1] == expected_hour_token[h24]);
    }
}

// ---------------------------------------------------------------------------
// d) render_time_tokens – exhaustive sweep (all 720 combinations)
// ---------------------------------------------------------------------------

TEST_CASE("render_time_tokens - exhaustive sweep: structural invariants", "[exhaustive]")
{
    for (uint8_t h24 = 0; h24 < 24; ++h24) {
        uint8_t hour12 = h24 % 12;
        if (hour12 == 0) hour12 = 12;
        bool jsou_hour = (hour12 == 2 || hour12 == 3 || hour12 == 4);

        for (uint8_t min = 0; min <= 59; ++min) {
            INFO("hour24=" << (int)h24 << " minute=" << (int)min);

            auto tokens = render_time_tokens(h24, min);

            // Must not be empty
            REQUIRE(!tokens.empty());

            // First token must be a valid prefix
            REQUIRE(is_valid_prefix(tokens[0]));

            // Second token must be a valid hour
            REQUIRE(tokens.size() >= 2);
            REQUIRE(is_valid_hour(tokens[1]));

            // Prefix rules
            bool bude_range = (min >= 11 && min <= 14) || (min >= 16 && min <= 19);
            if (bude_range) {
                CHECK(tokens[0] == TokenId::PREFIX_BUDE);
            } else if (jsou_hour) {
                CHECK(tokens[0] == TokenId::PREFIX_JSOU);
            } else {
                CHECK(tokens[0] == TokenId::PREFIX_JE);
            }

            // Size must be 2, 3, or 4
            CHECK(tokens.size() >= 2);
            CHECK(tokens.size() <= 4);

            // No TOKEN_INVALID anywhere
            CHECK(!contains_invalid(tokens));
        }
    }
}

// ---------------------------------------------------------------------------
// e) Spec examples from section 2.2
// ---------------------------------------------------------------------------

TEST_CASE("Spec examples - section 2.2", "[spec]")
{
    using TId = TokenId;

    SECTION("07:00 -> [PREFIX_JE, HOUR_7_SEDM]")
    {
        auto t = render_time_tokens(7, 0);
        REQUIRE(t == std::vector<TId>{TId::PREFIX_JE, TId::HOUR_7_SEDM});
    }

    SECTION("07:05 -> [PREFIX_JE, HOUR_7_SEDM, MIN_0_NULA, UNIT_5_PET]")
    {
        auto t = render_time_tokens(7, 5);
        REQUIRE(t == (std::vector<TId>{TId::PREFIX_JE, TId::HOUR_7_SEDM,
                                        TId::MIN_0_NULA, TId::UNIT_5_PET}));
    }

    SECTION("07:10 -> [PREFIX_JE, HOUR_7_SEDM, MIN_10_DESET]")
    {
        auto t = render_time_tokens(7, 10);
        REQUIRE(t == (std::vector<TId>{TId::PREFIX_JE, TId::HOUR_7_SEDM,
                                        TId::MIN_10_DESET}));
    }

    SECTION("07:11 -> [PREFIX_BUDE, HOUR_7_SEDM, MIN_15_PATNACT]")
    {
        auto t = render_time_tokens(7, 11);
        REQUIRE(t == (std::vector<TId>{TId::PREFIX_BUDE, TId::HOUR_7_SEDM,
                                        TId::MIN_15_PATNACT}));
    }

    SECTION("07:15 -> [PREFIX_JE, HOUR_7_SEDM, MIN_15_PATNACT]")
    {
        auto t = render_time_tokens(7, 15);
        REQUIRE(t == (std::vector<TId>{TId::PREFIX_JE, TId::HOUR_7_SEDM,
                                        TId::MIN_15_PATNACT}));
    }

    SECTION("07:16 -> [PREFIX_BUDE, HOUR_7_SEDM, MIN_20_DVACET]")
    {
        auto t = render_time_tokens(7, 16);
        REQUIRE(t == (std::vector<TId>{TId::PREFIX_BUDE, TId::HOUR_7_SEDM,
                                        TId::MIN_20_DVACET}));
    }

    SECTION("07:20 -> [PREFIX_JE, HOUR_7_SEDM, MIN_20_DVACET]")
    {
        auto t = render_time_tokens(7, 20);
        REQUIRE(t == (std::vector<TId>{TId::PREFIX_JE, TId::HOUR_7_SEDM,
                                        TId::MIN_20_DVACET}));
    }

    SECTION("07:23 -> [PREFIX_JE, HOUR_7_SEDM, MIN_20_DVACET, UNIT_3_TRI]")
    {
        auto t = render_time_tokens(7, 23);
        REQUIRE(t == (std::vector<TId>{TId::PREFIX_JE, TId::HOUR_7_SEDM,
                                        TId::MIN_20_DVACET, TId::UNIT_3_TRI}));
    }

    SECTION("07:30 -> [PREFIX_JE, HOUR_7_SEDM, MIN_30_TRICET]")
    {
        auto t = render_time_tokens(7, 30);
        REQUIRE(t == (std::vector<TId>{TId::PREFIX_JE, TId::HOUR_7_SEDM,
                                        TId::MIN_30_TRICET}));
    }

    SECTION("07:45 -> [PREFIX_JE, HOUR_7_SEDM, MIN_40_CTYRICET, UNIT_5_PET]")
    {
        auto t = render_time_tokens(7, 45);
        REQUIRE(t == (std::vector<TId>{TId::PREFIX_JE, TId::HOUR_7_SEDM,
                                        TId::MIN_40_CTYRICET, TId::UNIT_5_PET}));
    }

    SECTION("07:50 -> [PREFIX_JE, HOUR_7_SEDM, MIN_50_PADESAT]")
    {
        auto t = render_time_tokens(7, 50);
        REQUIRE(t == (std::vector<TId>{TId::PREFIX_JE, TId::HOUR_7_SEDM,
                                        TId::MIN_50_PADESAT}));
    }

    SECTION("07:59 -> [PREFIX_JE, HOUR_7_SEDM, MIN_50_PADESAT, UNIT_9_DEVET]")
    {
        auto t = render_time_tokens(7, 59);
        REQUIRE(t == (std::vector<TId>{TId::PREFIX_JE, TId::HOUR_7_SEDM,
                                        TId::MIN_50_PADESAT, TId::UNIT_9_DEVET}));
    }

    SECTION("12:00 -> [PREFIX_JE, HOUR_12_DVANACT]")
    {
        auto t = render_time_tokens(12, 0);
        REQUIRE(t == (std::vector<TId>{TId::PREFIX_JE, TId::HOUR_12_DVANACT}));
    }

    SECTION("02:00 -> [PREFIX_JSOU, HOUR_2_DVE]")
    {
        auto t = render_time_tokens(2, 0);
        REQUIRE(t == (std::vector<TId>{TId::PREFIX_JSOU, TId::HOUR_2_DVE}));
    }

    SECTION("03:23 -> [PREFIX_JSOU, HOUR_3_TRI, MIN_20_DVACET, UNIT_3_TRI]")
    {
        auto t = render_time_tokens(3, 23);
        REQUIRE(t == (std::vector<TId>{TId::PREFIX_JSOU, TId::HOUR_3_TRI,
                                        TId::MIN_20_DVACET, TId::UNIT_3_TRI}));
    }

    SECTION("04:11 -> [PREFIX_BUDE, HOUR_4_CTYRI, MIN_15_PATNACT]")
    {
        auto t = render_time_tokens(4, 11);
        REQUIRE(t == (std::vector<TId>{TId::PREFIX_BUDE, TId::HOUR_4_CTYRI,
                                        TId::MIN_15_PATNACT}));
    }
}
