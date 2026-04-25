#include "word_layout.hpp"
#include "led_index_map.hpp"

// ---------------------------------------------------------------------------
// LED index map – serpentine layout, source of truth for LED wiring.
// Generated from the serpentine rule (see matrix_to_led_index).
// Modify individual entries here to correct mechanical wiring mistakes.
// ---------------------------------------------------------------------------

const uint16_t led_index_map[MATRIX_ROWS][MATRIX_COLS] = {
    // Row 0 – left to right
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 },
    // Row 1 – right to left
    { 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20 },
    // Row 2 – left to right
    { 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59 },
    // Row 3 – right to left
    { 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60 },
    // Row 4 – left to right
    { 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99 },
    // Row 5 – right to left
    { 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100 },
    // Row 6 – left to right
    { 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139 },
    // Row 7 – right to left
    { 159, 158, 157, 156, 155, 154, 153, 152, 151, 150, 149, 148, 147, 146, 145, 144, 143, 142, 141, 140 },
};

uint16_t matrix_to_led_index(uint8_t row, uint8_t col)
{
    return led_index_map[row][col];
}

// ---------------------------------------------------------------------------
// Word-to-cell mapping (spec section 5)
// ---------------------------------------------------------------------------

// Each entry holds the cells for one token.
// Cells are listed in left-to-right order within the row.

static constexpr CellCoord cells_PREFIX_JE[]    = { {0,0},{0,1} };
static constexpr CellCoord cells_PREFIX_JSOU[]  = { {0,3},{0,4},{0,5},{0,6} };
static constexpr CellCoord cells_PREFIX_BUDE[]  = { {0,8},{0,9},{0,10},{0,11} };

static constexpr CellCoord cells_HOUR_1_JEDNA[]    = { {0,12},{0,13},{0,14},{0,15},{0,16} };
static constexpr CellCoord cells_HOUR_2_DVE[]       = { {0,17},{0,18},{0,19} };
static constexpr CellCoord cells_HOUR_7_SEDM[]      = { {1,0},{1,1},{1,2},{1,3} };
static constexpr CellCoord cells_HOUR_4_CTYRI[]     = { {1,4},{1,5},{1,6},{1,7},{1,8} };
static constexpr CellCoord cells_HOUR_5_PET[]       = { {1,10},{1,11},{1,12} };
static constexpr CellCoord cells_HOUR_6_SEST[]      = { {1,13},{1,14},{1,15},{1,16} };
static constexpr CellCoord cells_HOUR_3_TRI[]       = { {1,16},{1,17},{1,18} };  // shares 'T' with SEST
static constexpr CellCoord cells_HOUR_8_OSM[]       = { {2,0},{2,1},{2,2} };
static constexpr CellCoord cells_HOUR_9_DEVET[]     = { {2,3},{2,4},{2,5},{2,6},{2,7} };
static constexpr CellCoord cells_HOUR_10_DESET[]    = { {2,8},{2,9},{2,10},{2,11},{2,12} };
static constexpr CellCoord cells_HOUR_12_DVANACT[]  = { {2,13},{2,14},{2,15},{2,16},{2,17},{2,18},{2,19} };
static constexpr CellCoord cells_HOUR_11_JEDENACT[] = { {3,0},{3,1},{3,2},{3,3},{3,4},{3,5},{3,6},{3,7} };

static constexpr CellCoord cells_MIN_0_NULA[]      = { {3,9},{3,10},{3,11},{3,12} };
static constexpr CellCoord cells_MIN_10_DESET[]    = { {3,14},{3,15},{3,16},{3,17},{3,18} };
static constexpr CellCoord cells_MIN_15_PATNACT[]  = { {4,0},{4,1},{4,2},{4,3},{4,4},{4,5},{4,6} };
static constexpr CellCoord cells_MIN_20_DVACET[]   = { {4,8},{4,9},{4,10},{4,11},{4,12},{4,13} };
static constexpr CellCoord cells_MIN_30_TRICET[]   = { {4,13},{4,14},{4,15},{4,16},{4,17},{4,18} }; // shares 'T' with DVACET
static constexpr CellCoord cells_MIN_40_CTYRICET[] = { {5,0},{5,1},{5,2},{5,3},{5,4},{5,5},{5,6},{5,7} };
static constexpr CellCoord cells_MIN_50_PADESAT[]  = { {5,8},{5,9},{5,10},{5,11},{5,12},{5,13},{5,14} };

static constexpr CellCoord cells_UNIT_1_JEDNA[] = { {5,15},{5,16},{5,17},{5,18},{5,19} };
static constexpr CellCoord cells_UNIT_2_DVE[]   = { {6,0},{6,1},{6,2} };
static constexpr CellCoord cells_UNIT_3_TRI[]   = { {6,4},{6,5},{6,6} };
static constexpr CellCoord cells_UNIT_4_CTYRI[] = { {6,8},{6,9},{6,10},{6,11},{6,12} };
static constexpr CellCoord cells_UNIT_5_PET[]   = { {6,13},{6,14},{6,15} };
static constexpr CellCoord cells_UNIT_6_SEST[]  = { {6,16},{6,17},{6,18},{6,19} };
static constexpr CellCoord cells_UNIT_9_DEVET[] = { {7,0},{7,1},{7,2},{7,3},{7,4} };
static constexpr CellCoord cells_UNIT_7_SEDM[]  = { {7,8},{7,9},{7,10},{7,11} };
static constexpr CellCoord cells_UNIT_8_OSM[]   = { {7,14},{7,15},{7,16} };

// Dispatch table indexed by TokenId
struct TokenEntry {
    const CellCoord *cells;
    uint8_t          count;
};

#define ENTRY(arr) { arr, static_cast<uint8_t>(sizeof(arr) / sizeof(arr[0])) }

static constexpr TokenEntry token_table[] = {
    ENTRY(cells_PREFIX_JE),         // 0  PREFIX_JE
    ENTRY(cells_PREFIX_JSOU),       // 1  PREFIX_JSOU
    ENTRY(cells_PREFIX_BUDE),       // 2  PREFIX_BUDE
    ENTRY(cells_HOUR_1_JEDNA),      // 3  HOUR_1_JEDNA
    ENTRY(cells_HOUR_2_DVE),        // 4  HOUR_2_DVE
    ENTRY(cells_HOUR_3_TRI),        // 5  HOUR_3_TRI
    ENTRY(cells_HOUR_4_CTYRI),      // 6  HOUR_4_CTYRI
    ENTRY(cells_HOUR_5_PET),        // 7  HOUR_5_PET
    ENTRY(cells_HOUR_6_SEST),       // 8  HOUR_6_SEST
    ENTRY(cells_HOUR_7_SEDM),       // 9  HOUR_7_SEDM
    ENTRY(cells_HOUR_8_OSM),        // 10 HOUR_8_OSM
    ENTRY(cells_HOUR_9_DEVET),      // 11 HOUR_9_DEVET
    ENTRY(cells_HOUR_10_DESET),     // 12 HOUR_10_DESET
    ENTRY(cells_HOUR_11_JEDENACT),  // 13 HOUR_11_JEDENACT
    ENTRY(cells_HOUR_12_DVANACT),   // 14 HOUR_12_DVANACT
    ENTRY(cells_MIN_0_NULA),        // 15 MIN_0_NULA
    ENTRY(cells_MIN_10_DESET),      // 16 MIN_10_DESET
    ENTRY(cells_MIN_15_PATNACT),    // 17 MIN_15_PATNACT
    ENTRY(cells_MIN_20_DVACET),     // 18 MIN_20_DVACET
    ENTRY(cells_MIN_30_TRICET),     // 19 MIN_30_TRICET
    ENTRY(cells_MIN_40_CTYRICET),   // 20 MIN_40_CTYRICET
    ENTRY(cells_MIN_50_PADESAT),    // 21 MIN_50_PADESAT
    ENTRY(cells_UNIT_1_JEDNA),      // 22 UNIT_1_JEDNA
    ENTRY(cells_UNIT_2_DVE),        // 23 UNIT_2_DVE
    ENTRY(cells_UNIT_3_TRI),        // 24 UNIT_3_TRI
    ENTRY(cells_UNIT_4_CTYRI),      // 25 UNIT_4_CTYRI
    ENTRY(cells_UNIT_5_PET),        // 26 UNIT_5_PET
    ENTRY(cells_UNIT_6_SEST),       // 27 UNIT_6_SEST
    ENTRY(cells_UNIT_7_SEDM),       // 28 UNIT_7_SEDM
    ENTRY(cells_UNIT_8_OSM),        // 29 UNIT_8_OSM
    ENTRY(cells_UNIT_9_DEVET),      // 30 UNIT_9_DEVET
};

#undef ENTRY

std::span<const CellCoord> token_to_cells(TokenId token)
{
    auto idx = static_cast<uint8_t>(token);
    if (idx >= static_cast<uint8_t>(TokenId::TOKEN_COUNT)) {
        return {};
    }
    return { token_table[idx].cells, token_table[idx].count };
}
