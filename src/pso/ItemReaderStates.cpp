/****************************************************************************

    File: ItemReaderStates.cpp
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

#include "ItemReaderStates.hpp"

void InventoryViewState::render_to(TargetGrid & target) const {
    if (target.width() >= int(m_header_string.size())) {
        render_string_centered(target, m_header_string, 0, TargetGrid::k_highlight_colors);
    }
    render_item_list(target, 1, target.height());
}

/* private */ ItemList InventoryViewState::load_items
    (const MemoryReader & memory, const AddressList & addresses)
{
    auto rv = load_inventory(memory, addresses);
    std::sort(rv.begin(), rv.end(), lhs_code_lt_rhs);

    setup_header_line(m_header_string, "--- Inventory ", rv.size(), 2, " / 30 ---");
    return rv;
}

// ----------------------------------------------------------------------------

void BankViewState::render_to(TargetGrid & target) const {
    if (target.width() >= int(m_header_string.size())) {
        render_string_centered(target, m_header_string, 0, TargetGrid::k_highlight_colors);
    }
    render_item_list(target, 1, target.height());
}

/* private */ ItemList BankViewState::load_items
    (const MemoryReader & memory, const AddressList & addresses)
{
    auto rv = load_bank(memory, addresses);
    std::sort(rv.begin(), rv.end(), lhs_code_lt_rhs);

    // there is always one item for meseta, but does not count toward the
    // bank's capacity
    setup_header_line(m_header_string, "--- Bank ", rv.size() - 1, 3, " / 200 ---");
    return rv;
}

// ----------------------------------------------------------------------------

void FloorViewState::render_to(TargetGrid & target) const {
    if (target.width() >= int(m_header_string.size())) {
        render_string_centered(target, m_header_string, 0, TargetGrid::k_highlight_colors);
    }
    render_item_list(target, 1, target.height());
}

/* private */ ItemList FloorViewState::load_items
    (const MemoryReader & memory, const AddressList & addresses)
{
    auto rv = load_floor(memory, addresses);
    std::reverse(rv.begin(), rv.end());

    setup_header_line(m_header_string, "--- Floor ", rv.size(), 3,
                      rv.size() == 1 ? " item ---" : " items ---");
    return rv;
}

/* private */ void FloorViewState::load_addresses
    (const MemoryReader & memory, AddressList & addresses)
{
    update_floor_pointers(memory, addresses);
    std::reverse(addresses.begin(), addresses.end());
}
