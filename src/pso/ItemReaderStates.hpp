/****************************************************************************

    File: ItemReaderStates.hpp
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

#pragma once

#include "ItemReaderBaseState.hpp"

#include <unordered_set>

class InventoryViewState final : public SecondlyUpdatingItemReader {
public:
    void render_to(TargetGrid &) const override;

private:
    ItemList load_items
        (const MemoryReader & memory, const AddressList & addresses) override;

    void load_addresses(const MemoryReader & memory, AddressList & addresses) override
        { update_inventory_pointers(memory, addresses); }

    std::size_t this_state_id() const noexcept override
        { return ReaderStates::GetTypeId<InventoryViewState>::k_value; }

    std::string m_header_string;
};

class BankViewState final : public SecondlyUpdatingItemReader {
public:
    void render_to(TargetGrid &) const override;

private:
    ItemList load_items
        (const MemoryReader & memory, const AddressList & addresses) override;

    void load_addresses(const MemoryReader & memory, AddressList & addresses) override
        { update_bank_pointers(memory, addresses); }

    std::size_t this_state_id() const noexcept override
        { return ReaderStates::GetTypeId<BankViewState>::k_value; }

    std::string m_header_string;
};

class FloorViewState final : public ItemReaderBaseState {
public:
    void render_to(TargetGrid &) const override;

private:
    ItemList load_items
        (const MemoryReader & memory, const AddressList & addresses) override;

    void load_addresses(const MemoryReader & memory, AddressList & addresses) override;

    std::size_t this_state_id() const noexcept override
        { return ReaderStates::GetTypeId<FloorViewState>::k_value; }

    std::string m_header_string;
};
