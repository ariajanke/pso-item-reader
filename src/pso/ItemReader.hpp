/****************************************************************************

    File: ItemReader.hpp
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

#include "../Defs.hpp"

#include <memory>

enum class Rarity { uber, rare, interest, common, esrank };

namespace TextPalette {

constexpr const char k_plain    = 'a';
constexpr const char k_tool     = 'b';
constexpr const char k_gold     = 'c';
constexpr const char k_rare     = 'd';
constexpr const char k_untekked = 'e';
constexpr const char k_esrank   = 'f';
constexpr const char k_defense  = 'g';
constexpr const char k_interest = 'h';
constexpr const char k_weapon   = 'i';
constexpr const char k_uber     = 'j';

int to_grid_color(char, int rot);
char interpret_rarity(Rarity, char default_);

} // end of namespace TextPalette

class Item;
class MemoryReader;
using AddressList    = std::vector<Address>;
using ItemList       = std::vector<std::unique_ptr<Item>>;
using ItemLoader     = ItemList(*)(const MemoryReader &, const AddressList &);
using ItemPtrUpdater = void    (*)(const MemoryReader &, AddressList &);

void update_bank_pointers     (const MemoryReader &, AddressList &);
void update_inventory_pointers(const MemoryReader &, AddressList &);
void update_floor_pointers    (const MemoryReader &, AddressList &);

ItemList load_bank     (const MemoryReader &, const AddressList &);
ItemList load_inventory(const MemoryReader &, const AddressList &);
ItemList load_floor    (const MemoryReader &, const AddressList &);

class Item {
public:
    static constexpr const int          k_has_no_kill_counter = -1;
    static constexpr const char * const k_unknown_item        = "<unknown item>";

    virtual ~Item() {}
    virtual void print_to(std::ostream &) const = 0;

    void load_from     (Address, const MemoryReader &);
    void load_from_bank(Address, const MemoryReader &);

    bool operator < (const Item & rhs) const noexcept
        { return order_compare_to(rhs) < 0; }

protected:
    Rarity   rarity   = Rarity::common;
    int      kills    = k_has_no_kill_counter;
    uint32_t fullcode = 0;

    virtual void load_from_     (Address, const MemoryReader &) = 0;
    virtual void load_from_bank_(Address, const MemoryReader &) = 0;

    std::ostream & print_name(char default_, std::ostream &) const;
    std::ostream & print_name_min(std::ostream &) const;
    void set_name(const char *);

private:
    int order_compare_to(const Item & rhs) const noexcept {
        auto fc  = fullcode;
        auto ofc = rhs.fullcode;
        process_endian_u32( fc, k_big_endian);
        process_endian_u32(ofc, k_big_endian);
        return int(fc) - int(ofc);
    }

    const char * name = k_unknown_item;

    void set_fullcode_and_kills(uint32_t, Address, const MemoryReader &);
};

inline bool is_rare_tier(Rarity r)
    { return r == Rarity::uber || r == Rarity::rare; }
