#pragma once

#include <cstdint>

enum class QclocksError : uint8_t
{
    OK = 0,
    FAIL = 1,
    TIMEOUT = 2,
    INVALID_ARG = 3,
    NOT_INITIALIZED = 4,
};
