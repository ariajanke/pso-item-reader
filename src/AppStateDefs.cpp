/****************************************************************************

    File: AppStateDefs.cpp
    Author: Aria Janke
    License: GPLv3

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*****************************************************************************/

#include "AppStateDefs.hpp"

#include <cassert>

namespace {

template <typename IterType>
IterType constrain_offset(IterType itr, IterType end, int offset);

template <typename IterType>
IterType find_last_ws_from(IterType beg, IterType itr, IterType end);

template <typename T, typename IterType>
void insert_trimmed(std::vector<T> &, IterType beg, IterType end);

} // end of <anonymous> namespace

AppStateChanger::AppStateChanger
    (std::shared_ptr<AppState> & state_ptr, AppStateMap & state_map):
    m_target_pointer(&state_ptr),
    m_state_map     (&state_map)
{}

/* vtable anchor */ AppState::~AppState() {}

AppStateChanger AppState::make_state_changer() {
    return AppStateChanger(m_new_state, *m_state_map);
}

/* vtable anchor */ GridSize::~GridSize() {}

void render_string_centered
    (TargetGrid & target, const std::string & str, int line, int color)
{
    if (int(str.size()) > target.width()) {
        throw std::invalid_argument("render_string_centered: given string may "
                                    "not be longer than the render target.");
    }
    int x = (target.width() - str.size()) / 2;
    for (int ox = 0; ox != x; ++ox) {
        target.set_cell(ox, line, ' ', color);
    }
    {
    auto itr = str.begin();
    for (; itr != str.end() && x != target.width(); ++x, ++itr) {
        target.set_cell(x, line, *itr, color);
    }
    }
    for (; x != target.width(); ++x) {
        target.set_cell(x, line, ' ', color);
    }
}

void render_wrapped_lines_to
    (const std::vector<std::string> & lines, int max_width, int max_height,
     std::vector<std::string> & display_lines)
{
    display_lines.clear();
    if (max_height == 0 || max_width == 0) { return; }

    using CIter = std::string::const_iterator;
    // std::bind doesn't like me
    using namespace std::placeholders;
    auto insert_trimmed_ = [&display_lines] (CIter beg, CIter end)
        { insert_trimmed(display_lines, beg, end); };

    for (auto itr = lines.rbegin();
         int(display_lines.size()) <= max_height && itr != lines.rend(); ++itr)
    {
        auto old_size = display_lines.size();
        auto jtr_last = itr->begin();
        auto jtr      = constrain_offset(itr->begin(), itr->end(), max_width);
        while (true) {
            if (jtr == itr->end()) {
                insert_trimmed_(jtr_last, jtr);
                break;
            }
            auto gv = find_last_ws_from(jtr_last, jtr, itr->end());

            if (gv != itr->end()) ++gv;
            if (gv == itr->end()) gv = jtr;
            insert_trimmed_(jtr_last, gv);
            jtr_last = gv;
            jtr      = constrain_offset(gv, itr->end(), max_width);
            assert(jtr_last <= jtr);
        }
        std::reverse(display_lines.begin() + old_size, display_lines.end());
    }
    std::reverse(display_lines.begin(), display_lines.end());
    display_lines.erase(
        constrain_offset(display_lines.begin(), display_lines.end(), max_height),
        display_lines.end());
}

namespace {

template <typename IterType>
IterType constrain_offset(IterType itr, IterType end, int offset) {
    return (offset > end - itr) ? end : itr + offset;
}

template <typename IterType>
IterType find_last_ws_from(IterType beg, IterType itr, IterType end) {
    assert(beg <= itr);
    assert(itr < end);
    while (!is_whitespace(*itr)) {
        if (itr == beg) return end;
        --itr;
    }
    return itr;
}

template <typename T, typename IterType>
void insert_trimmed(std::vector<T> & cont, IterType beg, IterType end) {
    assert(beg < end);
    auto nend = &*beg + (end - beg);
    auto nbeg = &*beg;
    trim<is_whitespace>(nbeg, nend);
    cont.emplace_back(T { nbeg, nend });
}

} // end of <anonymous> namespace
