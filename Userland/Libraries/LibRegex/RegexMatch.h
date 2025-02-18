/*
 * Copyright (c) 2020, Emanuel Sprung <emanuel.sprung@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "RegexOptions.h"

#include <AK/FlyString.h>
#include <AK/HashMap.h>
#include <AK/MemMem.h>
#include <AK/String.h>
#include <AK/StringBuilder.h>
#include <AK/StringView.h>
#include <AK/Utf32View.h>
#include <AK/Utf8View.h>
#include <AK/Variant.h>
#include <AK/Vector.h>

namespace regex {

class RegexStringView {
public:
    RegexStringView(const char* chars)
        : m_view(StringView { chars })
    {
    }

    RegexStringView(const String& string)
        : m_view(string.view())
    {
    }

    RegexStringView(const StringView view)
        : m_view(view)
    {
    }

    RegexStringView(Utf32View view)
        : m_view(view)
    {
    }

    RegexStringView(Utf8View view)
        : m_view(view)
    {
    }

    const StringView& string_view() const
    {
        return m_view.get<StringView>();
    }

    const Utf32View& u32_view() const
    {
        return m_view.get<Utf32View>();
    }

    const Utf8View& u8_view() const
    {
        return m_view.get<Utf8View>();
    }

    bool is_empty() const
    {
        return m_view.visit([](auto& view) { return view.is_empty(); });
    }

    bool is_null() const
    {
        return m_view.visit([](auto& view) { return view.is_null(); });
    }

    size_t length() const
    {
        return m_view.visit([](auto& view) { return view.length(); });
    }

    RegexStringView construct_as_same(Span<u32> data, Optional<String>& optional_string_storage) const
    {
        return m_view.visit(
            [&]<typename T>(T const&) {
                StringBuilder builder;
                for (auto ch : data)
                    builder.append(ch); // Note: The type conversion is intentional.
                optional_string_storage = builder.build();
                return RegexStringView { T { *optional_string_storage } };
            },
            [&](Utf32View) {
                return RegexStringView { Utf32View { data.data(), data.size() } };
            });
    }

    Vector<RegexStringView> lines() const
    {
        return m_view.visit(
            [](StringView view) {
                auto views = view.lines(false);
                Vector<RegexStringView> new_views;
                for (auto& view : views)
                    new_views.empend(view);
                return new_views;
            },
            [](Utf32View view) {
                Vector<RegexStringView> views;
                u32 newline = '\n';
                while (!view.is_empty()) {
                    auto position = AK::memmem_optional(view.code_points(), view.length() * sizeof(u32), &newline, sizeof(u32));
                    if (!position.has_value())
                        break;
                    auto offset = position.value() / sizeof(u32);
                    views.empend(view.substring_view(0, offset));
                    view = view.substring_view(offset + 1, view.length() - offset - 1);
                }
                if (!view.is_empty())
                    views.empend(view);
                return views;
            },
            [](Utf8View& view) {
                Vector<RegexStringView> views;
                auto it = view.begin();
                auto previous_newline_position_it = it;
                for (;;) {
                    if (*it == '\n') {
                        auto previous_offset = view.byte_offset_of(previous_newline_position_it);
                        auto new_offset = view.byte_offset_of(it);
                        auto slice = view.substring_view(previous_offset, new_offset - previous_offset);
                        views.empend(slice);
                        ++it;
                        previous_newline_position_it = it;
                    }
                    if (it.done())
                        break;
                    ++it;
                }
                if (it != previous_newline_position_it) {
                    auto previous_offset = view.byte_offset_of(previous_newline_position_it);
                    auto new_offset = view.byte_offset_of(it);
                    auto slice = view.substring_view(previous_offset, new_offset - previous_offset);
                    views.empend(slice);
                }
                return views;
            });
    }

    RegexStringView substring_view(size_t offset, size_t length) const
    {
        return m_view.visit(
            [&](auto view) { return RegexStringView { view.substring_view(offset, length) }; },
            [&](Utf8View const& view) { return RegexStringView { view.unicode_substring_view(offset, length) }; });
    }

    String to_string() const
    {
        return m_view.visit(
            [](StringView view) { return view.to_string(); },
            [](auto& view) {
                StringBuilder builder;
                for (auto it = view.begin(); it != view.end(); ++it)
                    builder.append_code_point(*it);
                return builder.to_string();
            });
    }

    u32 operator[](size_t index) const
    {
        return m_view.visit(
            [&](StringView view) -> u32 {
                auto ch = view[index];
                if (ch < 0)
                    return 256u + ch;
                return ch;
            },
            [&](auto view) -> u32 { return view[index]; },
            [&](Utf8View& view) -> u32 {
                size_t i = index;
                for (auto it = view.begin(); it != view.end(); ++it, --i) {
                    if (i == 0)
                        return *it;
                }
                VERIFY_NOT_REACHED();
            });
    }

    bool operator==(const char* cstring) const
    {
        return m_view.visit(
            [&](Utf32View) { return to_string() == cstring; },
            [&](Utf8View const& view) { return view.as_string() == cstring; },
            [&](StringView view) { return view == cstring; });
    }

    bool operator!=(const char* cstring) const
    {
        return !(*this == cstring);
    }

    bool operator==(const String& string) const
    {
        return m_view.visit(
            [&](Utf32View) { return to_string() == string; },
            [&](Utf8View const& view) { return view.as_string() == string; },
            [&](StringView view) { return view == string; });
    }

    bool operator==(const StringView& string) const
    {
        return m_view.visit(
            [&](Utf32View) { return to_string() == string; },
            [&](Utf8View const& view) { return view.as_string() == string; },
            [&](StringView view) { return view == string; });
    }

    bool operator!=(const StringView& other) const
    {
        return !(*this == other);
    }

    bool operator==(const Utf32View& other) const
    {
        return m_view.visit(
            [&](Utf32View view) {
                return view.length() == other.length() && __builtin_memcmp(view.code_points(), other.code_points(), view.length() * sizeof(u32)) == 0;
            },
            [&](Utf8View const& view) { return view.as_string() == RegexStringView { other }.to_string(); },
            [&](StringView view) { return view == RegexStringView { other }.to_string(); });
    }

    bool operator!=(const Utf32View& other) const
    {
        return !(*this == other);
    }

    bool operator==(const Utf8View& other) const
    {
        return m_view.visit(
            [&](Utf32View) {
                return to_string() == other.as_string();
            },
            [&](Utf8View const& view) { return view.as_string() == other.as_string(); },
            [&](StringView view) { return other.as_string() == view; });
    }

    bool operator!=(const Utf8View& other) const
    {
        return !(*this == other);
    }

    bool equals(const RegexStringView& other) const
    {
        return other.m_view.visit([&](auto const& view) { return operator==(view); });
    }

    bool equals_ignoring_case(const RegexStringView& other) const
    {
        // FIXME: Implement equals_ignoring_case() for unicode.
        return m_view.visit(
            [&](StringView view) {
                return other.m_view.visit(
                    [&](StringView other_view) { return view.equals_ignoring_case(other_view); },
                    [](auto&) -> bool { TODO(); });
            },
            [](auto&) -> bool { TODO(); });
    }

    bool starts_with(const StringView& str) const
    {
        return m_view.visit(
            [&](Utf32View) -> bool {
                TODO();
            },
            [&](Utf8View const& view) { return view.as_string().starts_with(str); },
            [&](StringView view) { return view.starts_with(str); });
    }

    bool starts_with(const Utf32View& str) const
    {
        return m_view.visit(
            [&](Utf32View view) -> bool {
                if (str.length() > view.length())
                    return false;
                if (str.length() == view.length())
                    return operator==(str);
                for (size_t i = 0; i < str.length(); ++i) {
                    if (str.at(i) != view.at(i))
                        return false;
                }
                return true;
            },
            [&](Utf8View const& view) {
                auto it = view.begin();
                for (auto code_point : str) {
                    if (it.done())
                        return false;
                    if (code_point != *it)
                        return false;
                    ++it;
                }
                return true;
            },
            [&](StringView) -> bool { TODO(); });
    }

private:
    Variant<StringView, Utf8View, Utf32View> m_view;
};

class Match final {
private:
    Optional<FlyString> string;

public:
    Match() = default;
    ~Match() = default;

    Match(const RegexStringView view_, const size_t line_, const size_t column_, const size_t global_offset_)
        : view(view_)
        , line(line_)
        , column(column_)
        , global_offset(global_offset_)
        , left_column(column_)
    {
    }

    Match(const String string_, const size_t line_, const size_t column_, const size_t global_offset_)
        : string(string_)
        , view(string.value().view())
        , line(line_)
        , column(column_)
        , global_offset(global_offset_)
        , left_column(column_)
    {
    }

    RegexStringView view { nullptr };
    size_t line { 0 };
    size_t column { 0 };
    size_t global_offset { 0 };

    // ugly, as not usable by user, but needed to prevent to create extra vectors that are
    // able to store the column when the left paren has been found
    size_t left_column { 0 };
};

struct MatchInput {
    RegexStringView view { nullptr };
    AllOptions regex_options {};
    size_t start_offset { 0 }; // For Stateful matches, saved and restored from Regex::start_offset.

    size_t match_index { 0 };
    size_t line { 0 };
    size_t column { 0 };

    size_t global_offset { 0 }; // For multiline matching, knowing the offset from start could be important

    mutable size_t fail_counter { 0 };
    mutable Vector<size_t> saved_positions;
};

struct MatchState {
    size_t string_position_before_match { 0 };
    size_t string_position { 0 };
    size_t instruction_position { 0 };
    size_t fork_at_position { 0 };
    Vector<Match> matches;
    Vector<Vector<Match>> capture_group_matches;
    Vector<HashMap<String, Match>> named_capture_group_matches;
};

struct MatchOutput {
    size_t operations;
    Vector<Match> matches;
    Vector<Vector<Match>> capture_group_matches;
    Vector<HashMap<String, Match>> named_capture_group_matches;
};

}

using regex::RegexStringView;

template<>
struct AK::Formatter<regex::RegexStringView> : Formatter<StringView> {
    void format(FormatBuilder& builder, const regex::RegexStringView& value)
    {
        auto string = value.to_string();
        return Formatter<StringView>::format(builder, string);
    }
};
