#include <catch2/catch_test_macros.hpp>
#include "word_layout.hpp"
#include "led_index_map.hpp"
#include "app_types.hpp"

#include <algorithm>
#include <set>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// a) Every token has a non-empty cell list; TOKEN_INVALID returns empty
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("Every valid token returns a non-empty cell list", "[word_layout][a]") {
    for (uint8_t i = 0; i < static_cast<uint8_t>(TokenId::TOKEN_COUNT); ++i) {
        auto token = static_cast<TokenId>(i);
        CHECK(!token_to_cells(token).empty());
    }
}

TEST_CASE("TOKEN_INVALID returns an empty cell list", "[word_layout][a]") {
    REQUIRE(token_to_cells(TokenId::TOKEN_INVALID).empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// b) Every cell is within matrix bounds
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("Every cell coord is within matrix bounds", "[word_layout][b]") {
    for (uint8_t i = 0; i < static_cast<uint8_t>(TokenId::TOKEN_COUNT); ++i) {
        auto token = static_cast<TokenId>(i);
        for (const CellCoord& c : token_to_cells(token)) {
            CHECK(c.row < MATRIX_ROWS);
            CHECK(c.col < MATRIX_COLS);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// c) Exact position spot-checks (spec section 5)
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("Exact token positions match the spec", "[word_layout][c]") {

    SECTION("PREFIX_JE – row 0, cols 0..1 (2 cells)") {
        auto cells = token_to_cells(TokenId::PREFIX_JE);
        REQUIRE(cells.size() == 2);
        CHECK(cells[0].row == 0); CHECK(cells[0].col == 0);
        CHECK(cells[1].row == 0); CHECK(cells[1].col == 1);
    }

    SECTION("PREFIX_JSOU – row 0, cols 3..6 (4 cells)") {
        auto cells = token_to_cells(TokenId::PREFIX_JSOU);
        REQUIRE(cells.size() == 4);
        for (int i = 0; i < 4; ++i) {
            CHECK(cells[i].row == 0);
            CHECK(cells[i].col == static_cast<uint8_t>(3 + i));
        }
    }

    SECTION("PREFIX_BUDE – row 0, cols 8..11 (4 cells)") {
        auto cells = token_to_cells(TokenId::PREFIX_BUDE);
        REQUIRE(cells.size() == 4);
        for (int i = 0; i < 4; ++i) {
            CHECK(cells[i].row == 0);
            CHECK(cells[i].col == static_cast<uint8_t>(8 + i));
        }
    }

    SECTION("HOUR_7_SEDM – row 1, cols 0..3 (4 cells)") {
        auto cells = token_to_cells(TokenId::HOUR_7_SEDM);
        REQUIRE(cells.size() == 4);
        for (int i = 0; i < 4; ++i) {
            CHECK(cells[i].row == 1);
            CHECK(cells[i].col == static_cast<uint8_t>(i));
        }
    }

    SECTION("HOUR_12_DVANACT – row 2, cols 13..19 (7 cells)") {
        auto cells = token_to_cells(TokenId::HOUR_12_DVANACT);
        REQUIRE(cells.size() == 7);
        for (int i = 0; i < 7; ++i) {
            CHECK(cells[i].row == 2);
            CHECK(cells[i].col == static_cast<uint8_t>(13 + i));
        }
    }

    SECTION("HOUR_11_JEDENACT – row 3, cols 0..7 (8 cells)") {
        auto cells = token_to_cells(TokenId::HOUR_11_JEDENACT);
        REQUIRE(cells.size() == 8);
        for (int i = 0; i < 8; ++i) {
            CHECK(cells[i].row == 3);
            CHECK(cells[i].col == static_cast<uint8_t>(i));
        }
    }

    SECTION("MIN_15_PATNACT – row 4, cols 0..6 (7 cells)") {
        auto cells = token_to_cells(TokenId::MIN_15_PATNACT);
        REQUIRE(cells.size() == 7);
        for (int i = 0; i < 7; ++i) {
            CHECK(cells[i].row == 4);
            CHECK(cells[i].col == static_cast<uint8_t>(i));
        }
    }

    SECTION("MIN_40_CTYRICET – row 5, cols 0..7 (8 cells)") {
        auto cells = token_to_cells(TokenId::MIN_40_CTYRICET);
        REQUIRE(cells.size() == 8);
        for (int i = 0; i < 8; ++i) {
            CHECK(cells[i].row == 5);
            CHECK(cells[i].col == static_cast<uint8_t>(i));
        }
    }

    SECTION("UNIT_9_DEVET – row 7, cols 0..4 (5 cells)") {
        auto cells = token_to_cells(TokenId::UNIT_9_DEVET);
        REQUIRE(cells.size() == 5);
        for (int i = 0; i < 5; ++i) {
            CHECK(cells[i].row == 7);
            CHECK(cells[i].col == static_cast<uint8_t>(i));
        }
    }

    SECTION("UNIT_7_SEDM – row 7, cols 8..11 (4 cells)") {
        auto cells = token_to_cells(TokenId::UNIT_7_SEDM);
        REQUIRE(cells.size() == 4);
        for (int i = 0; i < 4; ++i) {
            CHECK(cells[i].row == 7);
            CHECK(cells[i].col == static_cast<uint8_t>(8 + i));
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// d) No duplicate LED indices within a single non-overlapping token
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("Non-overlapping tokens have unique LED indices", "[word_layout][d]") {
    // These two pairs intentionally share one cell each (tested in section e)
    const std::set<TokenId> overlapping_tokens = {
        TokenId::HOUR_3_TRI,
        TokenId::HOUR_6_SEST,
        TokenId::MIN_20_DVACET,
        TokenId::MIN_30_TRICET,
    };

    for (uint8_t i = 0; i < static_cast<uint8_t>(TokenId::TOKEN_COUNT); ++i) {
        auto token = static_cast<TokenId>(i);
        if (overlapping_tokens.count(token)) continue;

        auto cells = token_to_cells(token);
        std::vector<uint16_t> indices;
        indices.reserve(cells.size());
        for (const CellCoord& c : cells) {
            indices.push_back(matrix_to_led_index(c.row, c.col));
        }

        std::sort(indices.begin(), indices.end());
        auto unique_end = std::unique(indices.begin(), indices.end());
        CHECK(unique_end == indices.end());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// e) Allowed overlapping pairs share exactly 1 cell
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("Overlapping token pairs share exactly one LED index", "[word_layout][e]") {

    auto led_set = [](TokenId tok) {
        std::vector<uint16_t> v;
        for (const CellCoord& c : token_to_cells(tok)) {
            v.push_back(matrix_to_led_index(c.row, c.col));
        }
        std::sort(v.begin(), v.end());
        return v;
    };

    auto intersection = [](const std::vector<uint16_t>& a, const std::vector<uint16_t>& b) {
        std::vector<uint16_t> out;
        std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
                              std::back_inserter(out));
        return out;
    };

    SECTION("HOUR_3_TRI and HOUR_6_SEST share exactly cell (row=1, col=16)") {
        auto shared = intersection(led_set(TokenId::HOUR_3_TRI),
                                   led_set(TokenId::HOUR_6_SEST));
        REQUIRE(shared.size() == 1);
        CHECK(shared[0] == matrix_to_led_index(1, 16));
    }

    SECTION("MIN_20_DVACET and MIN_30_TRICET share exactly cell (row=4, col=13)") {
        auto shared = intersection(led_set(TokenId::MIN_20_DVACET),
                                   led_set(TokenId::MIN_30_TRICET));
        REQUIRE(shared.size() == 1);
        CHECK(shared[0] == matrix_to_led_index(4, 13));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// f) LED index map – serpentine wiring
//    even rows (0,2,4,6): left-to-right → row*20 + col
//    odd rows  (1,3,5,7): right-to-left → row*20 + (19-col)
// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("LED index map serpentine wiring is correct", "[led_index_map][f]") {

    SECTION("Row 0 – even row, left-to-right") {
        CHECK(matrix_to_led_index(0,  0) == 0);
        CHECK(matrix_to_led_index(0, 19) == 19);
    }

    SECTION("Row 1 – odd row, right-to-left") {
        CHECK(matrix_to_led_index(1,  0) == 39);   // 1*20 + (19-0)  = 39
        CHECK(matrix_to_led_index(1, 19) == 20);   // 1*20 + (19-19) = 20
    }

    SECTION("Row 7 – odd row, right-to-left") {
        CHECK(matrix_to_led_index(7,  0) == 159);  // 7*20 + (19-0)  = 159
        CHECK(matrix_to_led_index(7, 19) == 140);  // 7*20 + (19-19) = 140
    }
}
