#pragma once

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

using debug_instr_data = std::pair<std::string, std::string>;

struct Vec2 {
    int x;
    int y;

    Vec2(int _x, int _y) : x(_x), y(_y) {};

    bool operator==(const Vec2& n) const {
        return (n.x == x) && (n.y == y);
    }

    bool operator>(const Vec2& n) {
        return (x >= n.x) && (y > n.y);
    }
    bool operator<(const Vec2& n) {
        return (x < n.x) && (y < n.y);
    }
};