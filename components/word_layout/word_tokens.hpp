#pragma once

#include <cstdint>

// All display tokens as defined in SW spec section 4.
// Enum values are stable – do not reorder.
enum class TokenId : uint8_t
{
    // --- Prefixes (section 4.1) ---
    PREFIX_JE = 0,
    PREFIX_JSOU = 1,
    PREFIX_BUDE = 2,

    // --- Hours (section 4.2) ---
    HOUR_1_JEDNA = 3,
    HOUR_2_DVE = 4,
    HOUR_3_TRI = 5,
    HOUR_4_CTYRI = 6,
    HOUR_5_PET = 7,
    HOUR_6_SEST = 8,
    HOUR_7_SEDM = 9,
    HOUR_8_OSM = 10,
    HOUR_9_DEVET = 11,
    HOUR_10_DESET = 12,
    HOUR_11_JEDENACT = 13,
    HOUR_12_DVANACT = 14,

    // --- Minute decades / special words (section 4.3) ---
    MIN_0_NULA = 15,
    MIN_10_DESET = 16,
    MIN_15_PATNACT = 17,
    MIN_20_DVACET = 18,
    MIN_30_TRICET = 19,
    MIN_40_CTYRICET = 20,
    MIN_50_PADESAT = 21,

    // --- Minute units (section 4.4) ---
    UNIT_1_JEDNA = 22,
    UNIT_2_DVE = 23,
    UNIT_3_TRI = 24,
    UNIT_4_CTYRI = 25,
    UNIT_5_PET = 26,
    UNIT_6_SEST = 27,
    UNIT_7_SEDM = 28,
    UNIT_8_OSM = 29,
    UNIT_9_DEVET = 30,

    TOKEN_COUNT = 31,
    TOKEN_INVALID = 0xFF,
};
