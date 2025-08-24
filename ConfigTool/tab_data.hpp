// ============================================================================
// Project:   Universal Config Tool
// File:      tab_data.hpp
//
// Copyright (c) 2025 Afevis
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// ============================================================================
#pragma once

#include <vector>
#include <wx/string.h>
#include <optional>

struct Field
{
    wxString section;
    wxString key;

    wxString help;
    wxString tooltip;

    std::optional<std::pair<wxString, wxString>> prerequisite = std::nullopt;
    bool prerequisiteNegate = false;

    enum Type
    {
        Bool, Int, Float, Str, Choice, Hotkey, Spacer
    } type;

    int defaultInt = 0;
    int minInt = std::numeric_limits<int>::lowest();
    int maxInt = std::numeric_limits<int>::max();

    wxString defaultString;
    std::vector<wxString> choices;

    double defaultFloat = 0.0;
    double minFloat = std::numeric_limits<double>::lowest();
    double maxFloat = std::numeric_limits<double>::max();

};

extern const std::vector<std::pair<wxString, std::vector<Field>>> kTabs;
