/****************************************************************************

    File: Item.hpp
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

#include "ItemDb.hpp"

#include <memory>
#include <array>

class MemoryReader;

template <typename ArrayType>
ArrayType zero_initialized() {
    using Element = typename ArrayType::value_type;
    ArrayType arr;
    arr.fill( Element(0) );
    return arr;
}

static constexpr const Address k_item_code_offset = 0xF2;

class WeaponBase : public Item {
protected:
    static constexpr const int k_stats_offset = 0x1C8;

    void load_from_(Address, const MemoryReader &) override;
    void load_from_bank_(Address, const MemoryReader &) override;

    int           grind   = 0;
    WeaponSpecial special = WeaponSpecial::none;
    bool          tekked  = true;
    bool          wrapped = false;

private:
    void load_grind_and_special(Address grindaddr, Address specaddr, const MemoryReader &);
};

class DefenseItem : public Item {
protected:
    void print_def_stats(std::ostream &) const;
    void load_def_stats(Address evp_addr, Address dfp_addr, const MemoryReader &);

private:
    int evp = 0, dfp = 0;
    const DefenseItemInfo * mins_maxes = nullptr;
};

// ----------------------------------------------------------------------------

class Meseta final : public Item {
public:
    void set_quantity(int);
private:
    void print_to(std::ostream &) const override;
    void load_from_(Address, const MemoryReader &) override;
    void load_from_bank_(Address, const MemoryReader &) override;

    int quantity = 0;
};

class Tool final : public Item {
    static constexpr const int k_count_offset = 0x104;
    void print_to(std::ostream &) const override;
    void load_from_(Address, const MemoryReader &) override;
    void load_from_bank_(Address, const MemoryReader &) override;
    int quantity = 0;
};

class Tech final : public Item {
    void print_to(std::ostream &) const override;
    void load_from_(Address, const MemoryReader &) override;
    void load_from_bank_(Address, const MemoryReader &) override;

    void load_level_and_type(const MemoryReader &, Address leveladdr, Address typeaddr);

    TechType type  = TechType::resta; // objectively the best tech
    int      level = 0;
};

class Weapon final : public WeaponBase {
    static constexpr const int k_num_attrs = 5;
    using AttrArray = std::array<int8_t, k_num_attrs>;

    void print_to(std::ostream &) const override;
    void load_from_(Address, const MemoryReader &) override;
    void load_from_bank_(Address, const MemoryReader &) override;

    void load_attributes(Address attraddr, const MemoryReader & memory);
    void print_single_attribute(std::ostream &) const;
    void print_multiple_attributes(std::ostream &) const;
    static char get_attribute_color(int8_t, bool hit);

    int m_attr_count = 0;
    AttrArray m_attributes = zero_initialized<AttrArray>();
};

class EsWeapon final : public WeaponBase {
    static constexpr const int k_max_name = 8 + 1; // null terminated
    static constexpr const int k_name_offset = WeaponBase::k_stats_offset;
public:
    using NameArray = std::array<char, k_max_name>;
private:
    void print_to(std::ostream &) const override;
    void load_from_(Address, const MemoryReader &) override;
    void load_from_bank_(Address, const MemoryReader &) override;

    NameArray custom_name = zero_initialized<NameArray>();
};

class Frame final : public DefenseItem {
    static constexpr const int k_slots_offset = 0x1B8;

    void print_to(std::ostream &) const override;
    void load_from_(Address, const MemoryReader &) override;
    void load_from_bank_(Address, const MemoryReader &) override;

    int slot_count = 0;
};

// barriers and units may have other stat boosts

class Barrier final : public DefenseItem {
    void print_to(std::ostream &) const override;
    void load_from_(Address, const MemoryReader &) override;
    void load_from_bank_(Address, const MemoryReader &) override;
};

class Unit final : public Item {
    void print_to(std::ostream &) const override;
    void load_from_(Address, const MemoryReader &) override;
    void load_from_bank_(Address, const MemoryReader &) override;
};

class Mag final : public Item {
    // need to double check!
    static constexpr const int k_def  = 0;
    static constexpr const int k_pow  = 1;
    static constexpr const int k_dex  = 2;
    static constexpr const int k_mind = 3;
    static constexpr const int k_stat_count = 4;

    using StatArray = std::array<uint8_t, k_stat_count>;

    void print_to(std::ostream &) const override;
    void load_from_(Address, const MemoryReader &) override;
    void load_from_bank_(Address, const MemoryReader &) override;

    void load_stats(Address, const MemoryReader &);

    StatArray levels = zero_initialized<StatArray>();
    StatArray percentages = zero_initialized<StatArray>();
    int seconds_until_feeding = 0;
    bool in_bank = false;
};

class TotallyUnknownItem final : public Item {
    void print_to(std::ostream &) const override;
    void load_from_(Address, const MemoryReader &) override {}
    void load_from_bank_(Address, const MemoryReader &) override {}
};
