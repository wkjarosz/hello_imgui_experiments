//
// Copyright (C) Wojciech Jarosz <wjarosz@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE.txt file.
//

#pragma once

#if defined(_MSC_VER)
// Make MS cmath define M_PI but not the min/max macros
#define _USE_MATH_DEFINES
#define NOMINMAX
#endif

#include <string>

/// Linearly interpolates between `a` and `b`, using parameter `t`.
/**
    \param a    A value.
    \param b    Another value.
    \param t    A blending factor of `a` and `b`.
    \return     Linear interpolation of `a` and `b`:
                a value between `a` and `b` if `t` is between `0` and `1`.
*/
template <typename T, typename S>
inline T lerp(T a, T b, S t)
{
    return T((S(1) - t) * a + t * b);
}

/// Smoothly interpolates between a and b.
/**
    Performs a smooth s-curve (Hermite) interpolation between two values.

    \tparam T       A floating-point type
    \param a        A value.
    \param b        Another value.
    \param x        A number between `a` and `b`.
    \return         A value between `0.0` and `1.0`.
*/
template <typename T>
inline T smoothstep(T a, T b, T x)
{
    T t = std::min(std::max(T(x - a) / (b - a), T(0)), T(1));
    return t * t * (T(3) - T(2) * t);
}

std::string to_lower(std::string str);
std::string to_upper(std::string str);
std::string add_line_numbers(std::string_view in);

int         version_major();
int         version_minor();
int         version_patch();
int         version_combined();
std::string version();
std::string git_hash();
std::string git_describe();
std::string build_timestamp();
std::string backend();