/****************************************************************************

    File: ItemReaderBaseState.hpp
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

#include "ItemReader.hpp"
#include "../AppStateDefs.hpp"

class MemoryReader;

class InventoryViewState;
class FloorViewState;
class BankViewState;

class ItemReaderBaseState : public AppState {
public:
    void setup(std::shared_ptr<const MemoryReader>);

    void handle_event(const Event &) override;

    void handle_tick(double) override;

    void handle_resize(const GridSize &) override;

    UpdateStyle update_style() const noexcept override { return k_continuous_updates; }

    void setup_header_line
        (std::string &, const char * firstpart, int quantity, int padding, const char * lastpart);

protected:
    using ItemPtr = std::unique_ptr<Item>;
    using ReaderStates = TypeList<InventoryViewState, FloorViewState, BankViewState>;

    void update_item_list();

    virtual ItemList load_items    (const MemoryReader &, const AddressList &) = 0;
    virtual void     load_addresses(const MemoryReader &,       AddressList &) = 0;

    virtual std::size_t this_state_id() const noexcept = 0;

    void render_item_list(TargetGrid &, int start_line, int end_line) const;

    static bool lhs_code_lt_rhs(const ItemPtr & lhs, const ItemPtr & rhs)
        { return *lhs < *rhs; }

private:
    void update_item_strings();

    template <typename ... Types>
    ItemReaderBaseState & change_state_to_id(int id, TypeList<Types...>);

    template <typename ... Types>
    ItemReaderBaseState & change_state_to_id(int id);

    std::vector<std::string> m_item_strings;

    std::vector<Address> m_pointers;
    std::vector<Address> m_old_pointers;
    std::vector<ItemPtr> m_items;

    std::shared_ptr<const MemoryReader> m_reader = nullptr;

    int m_line_offset = 0;

    static constexpr const double k_max_delay = 0.5;
    double m_delay      = 0.;
    int m_delay_counter = 0;
    int m_page_step     = 0;
};

// ----------------------------------------------------------------------------

class SecondlyUpdatingItemReader : public ItemReaderBaseState {
    void handle_tick(double) final;

    static constexpr const double k_update_inv_delay = 1.;
    double m_delay = 0.;
};

// ----------------------------------------------------------------------------

template <typename ... Types>
ItemReaderBaseState & ItemReaderBaseState::change_state_to_id(int id, TypeList<Types...>) {
    using InheritedList = typename TypeList<Types...>::InheritedType;
    using HeadType      = typename TypeList<Types...>::HeadType;
    if (id < 0 || id >= int(ReaderStates::k_count)) {
        throw std::invalid_argument("Id value must be a non-negative integer less than the number of reader states.");
    }

    if constexpr (std::is_base_of_v<AppState, HeadType>) {
        if (id == int(TypeList<Types...>::k_count - 1)) {
            return switch_state<HeadType>();
        }
    }
    if constexpr (!std::is_same_v<InheritedList, TypeTag<void>>) {
        return change_state_to_id(id, InheritedList());
    }
    throw "impossible branch";
}

template <typename ... Types>
ItemReaderBaseState & ItemReaderBaseState::change_state_to_id(int id) {
    // wrap id
    /**/ if (id < 0) { id = ReaderStates::k_count - 1; }
    else if (id >= int(ReaderStates::k_count)) { id = 0; }

    return change_state_to_id(id, ReaderStates());
}
