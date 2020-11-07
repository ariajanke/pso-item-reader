/****************************************************************************

    File: ItemReader.cpp
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

#include "ItemReader.hpp"
#include "ItemDb.hpp"
#include "Item.hpp"

#include "../AppStateDefs.hpp"
#include "../MemoryReader.hpp"

#include <iostream>
#include <iomanip>

namespace {

using LoadItemFunc = void (Item::*)(Address, const MemoryReader &);

static constexpr const Address k_bank_ptr_addr     = 0x00A95DE0 + 0x18;
static constexpr const Address k_item_ptr_to_array = 0x00A8D81C;
static constexpr const Address k_item_array_size   = 0x00A8D820;
static constexpr const Address k_player_index      = 0x00A9C4F4;
static constexpr const Address k_item_owner_offset = 0xE4;
static constexpr const int     k_no_owner          = -1;

/** Loads the entire list of item addresses including floor and inventories.
 *  @param owner_id ID number of the owner, all other items not owned by this
 *                  ID are filtered out.
 *  @returns true if the address list has changed (is different from old)
 *  @warning
 *  refer to rule 6 on:
 *  https://www.pioneer2.net/community/threads/ephinea-forum-and-server-rules.2026/
 *  "thou shall not read other player's inventories"
 *  I cannot stop you from the breaking the rules, but you may not hide in
 *  ignorance from it.
 */
void update_item_list_for_owner(const MemoryReader &, AddressList &, int owner_id);

void clean(AddressList &);

uint32_t load_bank_ptr(const MemoryReader &);

template <LoadItemFunc loadf>
ItemList load_gen
    (const MemoryReader & memory, const AddressList & addresses,
     Address fullcode_offset);

} // end of <anonymous> namespace

namespace TextPalette {

/* free fn */ int to_grid_color(char c, int rot) {
    switch (c) {
    case k_plain   : return TargetGrid::k_normal_colors;
    case k_tool    : return TargetGrid::k_green_text;
    case k_gold    : return TargetGrid::k_yellow_text;
    case k_rare    : return TargetGrid::k_yellow_text;
    case k_esrank  : return TargetGrid::k_red_text;
    case k_untekked: return TargetGrid::k_cyan_text;
    case k_defense : return TargetGrid::k_blue_text;
    case k_interest: return TargetGrid::k_magenta_text;
    case k_weapon  : return TargetGrid::k_dark_yellow_colors;
    case k_uber    : switch (rot % 7) {
                     case 0: return TargetGrid::k_red_text;
                     case 1: return TargetGrid::k_dark_yellow_colors;
                     case 2: return TargetGrid::k_yellow_text;
                     case 3: return TargetGrid::k_green_text;
                     case 4: return TargetGrid::k_cyan_text;
                     case 5: return TargetGrid::k_blue_text;
                     case 6: return TargetGrid::k_magenta_text;
                     }
    default: throw std::invalid_argument("Not a color character");
    }
}

/* free fn */ char interpret_rarity(Rarity r, char default_) {
    switch (r) {
    case Rarity::common  : return default_;
    case Rarity::interest: return k_interest;
    case Rarity::uber    : return k_uber;
    case Rarity::rare    : return k_rare;
    case Rarity::esrank  : return k_esrank;
    }
    throw std::invalid_argument("interpret_rarity: rarity value not valid.");
}

} // end of namespace TextPalette

/* free fn */ void update_bank_pointers
    (const MemoryReader & memory, AddressList & addresses)
{
    // why was this read as an i32?
    auto bank_ptr = load_bank_ptr(memory);
    if (!bank_ptr) return;

    addresses.clear();
    int count = memory.read_u8(bank_ptr);
    for (int i = 0; i != count; ++i) {
        addresses.push_back(bank_ptr + 8 + 24*i);
    }

    clean(addresses);
}

/* free fn */ void update_inventory_pointers
    (const MemoryReader & memory, AddressList & addresses)
{
    auto player_index = memory.read_u32(k_player_index);
    update_item_list_for_owner(memory, addresses, player_index);
}

/* free fn */ void update_floor_pointers
    (const MemoryReader & memory, AddressList & addresses)
{ return update_item_list_for_owner(memory, addresses, k_no_owner); }

/* free fn */ ItemList load_bank
    (const MemoryReader & memory, const AddressList & addresses)
{
    auto rv = load_gen<&Item::load_from_bank>(memory, addresses, 0);

    auto bank_ptr = load_bank_ptr(memory);
    if (bank_ptr) {
        auto mes = std::make_unique<Meseta>();
        mes->set_quantity(memory.read_i32(bank_ptr + 4));
        rv.push_back(std::move(mes));
    }

    return rv;
}

/* free fn */ ItemList load_inventory
    (const MemoryReader & memory, const AddressList & addresses)
{ return load_gen<&Item::load_from>(memory, addresses, k_item_code_offset); }

/* free fn */ ItemList load_floor
    (const MemoryReader & memory, const AddressList & addresses)
{ return load_gen<&Item::load_from>(memory, addresses, k_item_code_offset); }

void Item::load_from(Address addr, const MemoryReader & memory) {
    uint32_t fullcode = memory.read_u32(addr + k_item_code_offset) & 0xFF'FFFF;
    set_fullcode_and_kills(fullcode, addr, memory);
    load_from_(addr, memory);
}

void Item::load_from_bank(Address addr, const MemoryReader & memory) {
    set_fullcode_and_kills(memory.read_u32(addr) & 0xFF'FFFF, addr, memory);
    load_from_bank_(addr, memory);
}

/* protected */ std::ostream & Item::print_name(char default_, std::ostream & out) const {
    out << "[" << TextPalette::interpret_rarity(rarity, default_) << ":";
    print_name_min(out);
    return (out << "]");
}

/* protected */ std::ostream & Item::print_name_min(std::ostream & out) const {
    if (std::equal(name, name + strlen(name), ItemInfo::k_unknown_item)) {
        auto fc = fullcode;
        process_endian_u32(fc, k_big_endian);
        fc >>= 8;
        out << "[" << TextPalette::k_untekked << ":?]"
            << std::hex << std::uppercase << std::setw(6) << fc << "["
            << TextPalette::k_untekked << ":?]"
            << std::dec << std::nouppercase << std::setw(0);
    } else {
        out << name;
    }
    return out;
}

/* protected */ void Item::set_name(const char * name_) { name = name_; }

/* private */ void Item::set_fullcode_and_kills
    (uint32_t fullcode, Address addr, const MemoryReader & memory)
{
    // both inventory and bank
    static constexpr const Address k_kill_counter_offset = 0xE8;
    const auto & nfo = get_item_info(fullcode);
    name = nfo.name;
    if (nfo.has_kill_counter) {
        kills = memory.read_u16(addr + k_kill_counter_offset);
    }
    rarity = nfo.rarity;
    this->fullcode = fullcode;
}

namespace {

std::unique_ptr<Item> make_item(const MemoryReader &, Address);

template <LoadItemFunc loadf>
ItemList load_gen
    (const MemoryReader & memory, const AddressList & addresses,
     Address fullcode_offset)
{
    ItemList rv;
    rv.reserve(addresses.size());
    for (auto addr : addresses) {
        rv.push_back(make_item(memory, addr + fullcode_offset));
        ((*rv.back()).*loadf)(addr, memory);
    }
    return rv;
}

// refer to rule 6 on:
// https://www.pioneer2.net/community/threads/ephinea-forum-and-server-rules.2026/
// "thou shall not read other player's inventories"
// I cannot stop you from the breaking the rules, but you may not hide in
// ignorance from it.
void update_item_list_for_owner
    (const MemoryReader & memory, AddressList & addresses, int owner_id)
{
    addresses.clear();

    auto item_count = memory.read_u8(k_item_array_size);
    if (!item_count) return;

    auto item_array = memory.read_u32(k_item_ptr_to_array);
    addresses.reserve(item_count);

    std::array<uint32_t, 0xFF> rawptrs;
    memory.read(item_array, reinterpret_cast<uint8_t *>(rawptrs.data()),
                item_count*sizeof(uint32_t));
    while (item_count) {
        addresses.push_back(Address(rawptrs[--item_count]));
    }

    for (auto & addr : addresses) {
        auto item_owner_id = memory.read_i8(addr + k_item_owner_offset);
        if (item_owner_id != owner_id) addr = k_no_address;
    }

    clean(addresses);
}

void clean(AddressList & addresses) {
    addresses.erase(
        std::remove(addresses.begin(), addresses.end(), k_no_address),
        addresses.end());
}

uint32_t load_bank_ptr(const MemoryReader & memory) {
    // why was this read as an i32?
    auto bank_ptr = memory.read_u32(k_bank_ptr_addr) & 0x7FFF'FFFF;
    if (!bank_ptr) return 0;
    return bank_ptr + 0x021C;
}

// ----------------------------------------------------------------------------

std::unique_ptr<Item> make_item(const MemoryReader & memory, Address addr) {
    using std::make_unique;
    auto fullcode = memory.read_u32(addr) & 0xFFFFFF;
    auto low  = fullcode & 0xFF;
    auto high = (fullcode >> 8) & 0xFF;
    switch (low) {
    case 0:
        if (is_esrank(fullcode)) { return make_unique<EsWeapon>(); }
        else                     { return make_unique<  Weapon>(); }
    case 1:
        switch (high) {
        case 1 : return make_unique<Frame             >();
        case 2 : return make_unique<Barrier           >();
        case 3 : return make_unique<Unit              >();
        default: return make_unique<TotallyUnknownItem>();
        }
    case 2: return make_unique<Mag>();
    case 3:
        if (high == 2) { return make_unique<Tech>(); }
        else           { return make_unique<Tool>(); }
    case 4 : return make_unique<Meseta>();
    default: return make_unique<TotallyUnknownItem>();
    }
}

} // end of <anonymous> namespace
