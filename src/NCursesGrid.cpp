/****************************************************************************

    File: NCursesGrid.cpp
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

#include "NCursesGrid.hpp"

#include <cassert>

#include <thread>

#include <ncurses.h>

void CachedChangeGrid::set_cell(int x, int y, char chr, int cpair) {
    if (!m_character_grid.has_position(x, y)) {
        throw std::invalid_argument("CachedChangeGrid::set_cell: position not found in text grid.");
    }
    m_pressed(x, y) = true;
    if (m_character_grid(x, y) == chr   &&
        m_color_grid    (x, y) == cpair)
    { return; }
    m_character_grid(x, y) = chr  ;
    m_color_grid    (x, y) = cpair;
    m_changed       (x, y) = true ;
    ++m_num_changed;
}

bool CachedChangeGrid::update_size() {
    int width_ = width(), height_ = height();
    if (width_  == m_character_grid.width () &&
        height_ == m_character_grid.height()) return false;

    m_character_grid.set_size(width_, height_, ' ');
    m_color_grid    .set_size(width_, height_, [](){ return TargetGrid::k_normal_colors; }());
    m_changed       .set_size(width_, height_, true);
    m_pressed       .set_size(width_, height_, true);
    return true;
}

void CachedChangeGrid::fill_unpressed_space() {
    // set all unpressed to blank
    int pressed = 0;
    for (sf::Vector2i r; r != m_pressed.end_position(); r = m_pressed.next(r)) {
        if (m_pressed(r)) continue;
        assert(!m_changed(r));
        set_cell(r.x, r.y, ' ', k_normal_colors);
        ++pressed;
    }
    int k = 0;
    ++k;
}

void CachedChangeGrid::do_prerender() {
    using BoolRef = std::vector<bool>::reference;
    for (BoolRef b : m_pressed) b = false;
    for (BoolRef b : m_changed) b = false;
    m_num_changed = 0;
}

/* protected */ bool CachedChangeGrid::has_changed(int x, int y) const
    { return m_changed(x, y); }

/* protected */ std::pair<char, int> CachedChangeGrid::get_color_char_pair
    (int x, int y) const
{ return std::make_pair(m_character_grid(x, y), m_color_grid(x, y)); }

// ----------------------------------------------------------------------------

NCursesGrid::~NCursesGrid() {
    refresh();
    endwin();
}

void NCursesGrid::setup() {
    {
    Grid<char> g;
    g.set_size(2, 2);

    // assume memory is laid out into contiguous rows
    assert(&*(g.begin() + 1) == &g(1, 0));
    // if this assertion is false, that messes up the implementation!
    }
    initscr();
    if (!has_colors()) {
        throw std::runtime_error("Program requires color support.");
    }
    start_color();
    curs_set(0);
    keypad(stdscr, TRUE);

    init_pair(TargetGrid::k_normal_colors   , COLOR_WHITE, COLOR_BLACK);
    init_pair(TargetGrid::k_highlight_colors, COLOR_BLACK, COLOR_WHITE);

    init_pair(TargetGrid::k_dark_yellow_colors, COLOR_YELLOW, COLOR_BLACK);

    init_pair(TargetGrid::k_red_text    , COLOR_RED     | 0x8, COLOR_BLACK);
    init_pair(TargetGrid::k_green_text  , COLOR_GREEN   | 0x8, COLOR_BLACK);
    init_pair(TargetGrid::k_yellow_text , COLOR_YELLOW  | 0x8, COLOR_BLACK);
    init_pair(TargetGrid::k_blue_text   , COLOR_BLUE    | 0x8, COLOR_BLACK);
    init_pair(TargetGrid::k_magenta_text, COLOR_MAGENTA | 0x8, COLOR_BLACK);
    init_pair(TargetGrid::k_cyan_text   , COLOR_CYAN    | 0x8, COLOR_BLACK);

    update_size();
}

int NCursesGrid::width() const {
    return getmaxx(stdscr);
}

int NCursesGrid::height() const {
    return getmaxy(stdscr);
}

void NCursesGrid::render() const {
    auto height_ = height();
    auto width_  = width ();
    static constexpr const int k_no_color_pair = -1;
    int current_color_pair = k_no_color_pair;

    int writes = 0;
    for (int y = 0; y != height_; ++y) {
    for (int x = 0; x != width_ ; ++x) {
        auto [outc, cpair] = get_color_char_pair(x, y);
        if (cpair != current_color_pair) {
            if (current_color_pair != k_no_color_pair) {
                attroff(COLOR_PAIR(current_color_pair));
            }
            current_color_pair = cpair;
            attron(COLOR_PAIR(current_color_pair));
        }
        mvwinsnstr(stdscr, y, x, &outc, 1);
        ++writes;
    }}
    if (current_color_pair != k_no_color_pair) {
        attroff(COLOR_PAIR(current_color_pair));
    }
}

