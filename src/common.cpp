//
// Copyright (C) Wojciech Jarosz <wjarosz@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE.txt file.
//

#include "common.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace std;

string to_lower(string str)
{
    transform(begin(str), end(str), begin(str), [](unsigned char c) { return (char)tolower(c); });
    return str;
}

string to_upper(string str)
{
    transform(begin(str), end(str), begin(str), [](unsigned char c) { return (char)toupper(c); });
    return str;
}

string add_line_numbers(string_view input)
{
    stringstream  result;
    istringstream input_stream(input.data());
    string        line;
    size_t        line_number = 1;

    // Calculate the number of digits in the total number of lines
    size_t totalLines  = std::count(input.begin(), input.end(), '\n') + 1;
    size_t line_digits = (totalLines == 0) ? 1 : static_cast<size_t>(std::log10(totalLines)) + 1;

    while (std::getline(input_stream, line))
    {
        // Prepend the line number with padding
        result << std::setw(line_digits) << std::setfill(' ') << line_number << ": " << line << '\n';
        line_number++;
    }

    return result.str();
}

const vector<string> &channel_names()
{
    static const vector<string> names{"RGB",
                                      "Red",
                                      "Green",
                                      "Blue",
                                      "Alpha",
                                      "Luminance",
                                      "Gray",
                                      "CIE L*",
                                      "CIE a*",
                                      "CIE b*",
                                      "CIE chromaticity",
                                      "False color",
                                      "Negative-positive"};
    return names;
}

const vector<string> &blend_mode_names()
{
    static const vector<string> names{
        "Normal", "Multiply", "Divide", "Add", "Average", "Subtract", "Difference", "Relative difference",
    };
    return names;
}

string channel_to_string(EChannel channel)
{
    return channel_names()[channel];
}

string blend_mode_to_string(EBlendMode mode)
{
    return blend_mode_names()[mode];
}
