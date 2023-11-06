#pragma once
/*
*	different defines for random usage and speed up writing
*/

#include <stdint.h>
#include <string>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

struct Vec2 {
    int x;
    int y;

    Vec2(int x, int y) : x(x), y(y) {};
};