/*
 * File: types.hpp
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-11-02
 *
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>
#include <array>
#include <string>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

struct Entity
{
    uint32_t id;
    uint8_t gen;
};