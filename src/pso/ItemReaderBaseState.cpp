/****************************************************************************

    File: ItemReaderBaseState.cpp
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

#include "ItemReaderBaseState.hpp"
#include "ItemReaderStates.hpp"
#include "ProcessWatcher.hpp"

#include <cmath>
#include <cassert>

#include <sstream>

namespace {

template <typename IterType>
IterType render_line
    (std::string & target, std::string & colors, char color,
     IterType beg, IterType end);


} // end of <anonymous> namespace

void ItemReaderBaseState::setup(std::shared_ptr<const MemoryReader> source) {
    m_reader = source;
    update_item_list();
}

void ItemReaderBaseState::handle_event(const Event & event) {
    auto scroll = [this](int step) {
        m_line_offset += step;
        if (m_line_offset < 0)
            { m_line_offset = 0; }
        else if (m_line_offset >= int(m_item_strings.size()))
            { m_line_offset = int(m_item_strings.size()) - 1; }
    };
    if (auto * sp = event.as_pointer<SpecialKey>()) {
        switch (*sp) {
        case SpecialKey::escape   : throw QuitAppException();
        case SpecialKey::up       : scroll(-1); break;
        case SpecialKey::down     : scroll( 1); break;
        case SpecialKey::page_up  : scroll(-m_page_step); break;
        case SpecialKey::page_down: scroll( m_page_step); break;
        case SpecialKey::left:
            change_state_to_id(this_state_id() - 1).setup(m_reader);
            break;
        case SpecialKey::right:
            change_state_to_id(this_state_id() + 1).setup(m_reader);
            break;
        default: break;
        }
    }
}

void ItemReaderBaseState::handle_tick(double et) {
    if ( (m_delay += et) >= k_max_delay ) {
        m_delay = std::fmod(m_delay, k_max_delay);
        m_delay_counter = (m_delay_counter + 1) % 8;
        assert(m_delay_counter >= 0);

        update_item_strings();
    }

    m_pointers.clear();
    try {
        load_addresses(*m_reader, m_pointers);
        if (!std::equal(m_pointers    .begin(), m_pointers    .end(),
                        m_old_pointers.begin(), m_old_pointers.end()))
        {
            m_items = load_items(*m_reader, m_pointers);
            update_item_strings();
            m_old_pointers = m_pointers;
        }
    }  catch (...) {
        switch_state<PsobbProcessWatcher>();
    }
}

void ItemReaderBaseState::handle_resize(const GridSize & gsize) {
    m_page_step = (gsize.height() * 2) / 5;
}

/* static */ void ItemReaderBaseState::setup_header_line
    (std::string & header_line, const char * firstpart,
     int quantity, int padding, const char * lastpart)
{

    auto sizestr = std::to_string(quantity);
    if (int(sizestr.size()) > int(padding)) {
        throw std::invalid_argument("ItemReaderBaseState::setup_header_line: "
            "padding must be equal to or greater than the "
            "ceil(log_base10(quantity)) [number of characters for the integer "
            "as a string].");
    }
    std::size_t pad = padding - sizestr.size();

    header_line = firstpart;
    header_line.append(pad, ' ');
    header_line += sizestr + lastpart;
}

/* protected */ void ItemReaderBaseState::update_item_list() {
    if (!m_reader) return;

    try {
        m_old_pointers = m_pointers;
        m_pointers.clear();
        load_addresses(*m_reader, m_pointers);
        m_items = load_items(*m_reader, m_pointers);

        update_item_strings();
    } catch (PermissionError &) {
        throw;
    } catch (...) {
        switch_state<PsobbProcessWatcher>();
    }
}

/* protected */ void ItemReaderBaseState::render_item_list
    (TargetGrid & target, int start_line, int end_line) const
{
    if (start_line < 0 || end_line < 0) {
        throw std::invalid_argument("ItemReaderBaseState::render_item_list: start_line, and end_line must be non-negative integers.");
    }
    if (end_line < start_line) {
        throw std::invalid_argument("ItemReaderBaseState::render_item_list: end_line must be less than or equal to start_line.");
    }
    if (start_line == end_line) return;
    int line = start_line;
    std::string outs, colors;
    auto itr = m_item_strings.begin() + m_line_offset;
    for (; itr != m_item_strings.end(); ++itr) {
        if (line >= end_line) break;
        int x = 0;
        outs.clear();
        colors.clear();
        render_line(outs, colors, TextPalette::k_plain, itr->begin(), itr->end());
        assert(outs.size() == colors.size());
        for (int i = 0; i != int(outs.size()); ++i) {
            if (x >= target.width()) break;
            assert(x >= 0);
            target.set_cell(x, line, outs[i], TextPalette::to_grid_color(colors[i], m_delay_counter + x));
            x++;
        }
        line++;
    }
}

/* private */ void ItemReaderBaseState::update_item_strings() {
    std::stringstream ssout;
    for (auto & item : m_items) {
        item->print_to(ssout);
        ssout << "\n";
    }

    auto output = ssout.str();
    m_item_strings.clear();
    for_split<is_newline>(output.data(), output.data() + output.size(),
        [this](const char * beg, const char * end)
    { m_item_strings.push_back(std::string(beg, end)); });

    m_line_offset = std::min(int(m_item_strings.size()), m_line_offset);
}

// ----------------------------------------------------------------------------

/* private */ void SecondlyUpdatingItemReader::handle_tick(double et) {
    ItemReaderBaseState::handle_tick(et);
    if ((m_delay += et) > k_update_inv_delay) {
        m_delay = std::fmod(m_delay, k_update_inv_delay);
        update_item_list();
    }
}

namespace {

template <typename IterType>
IterType render_line
    (std::string & target, std::string & colors, char color,
     IterType beg, IterType end)
{
    using Error = std::runtime_error;
    enum { reg, choose_color, escaped, look_for_colon };
    auto phase = reg;
    char color_char = 0;
    auto push = [&target, &colors](char c, char color) {
        target.push_back(c);
        colors.push_back(color);
    };
    for (auto itr = beg; itr != end; ++itr) {
        switch (phase) {
        case reg:
            switch (*itr) {
            case '\\': phase = escaped; break;
            // next character is the color character
            case '[': phase = choose_color; break;
            case ']': return itr + 1;
            default:
                push(*itr, color);
                break;
            }
            break;
        case choose_color:
            if (is_alphanumeric(*itr)) {
                color_char = *itr;
                phase = look_for_colon;
            } else {
                throw Error("Color character must be alphanumeric.");
            }
            break;
        case look_for_colon:
            if (*itr == ':') {
                itr = render_line(target, colors, color_char, itr + 1, end) - 1;
                phase = reg;
            } else {
                throw Error("Colon must come immediately after color character.");
            }
            break;
        case escaped:
            push(*itr, color);
            phase = reg;
            break;
        }
    }
    return end;
}

} // end of <anonymous> namespace
