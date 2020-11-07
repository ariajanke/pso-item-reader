/****************************************************************************

    File: Item.cpp
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

#include "Item.hpp"
#include "ItemDb.hpp"
#include "../AppStateDefs.hpp"
#include "../MemoryReader.hpp"

#include <numeric>
#include <iostream>
#include <iomanip>

#include <cmath>

namespace {

enum class ItemType {
    weapon, frame, barrier, unit, mag, tool, tech, meseta,
    invalid
};

using EsRankName = EsWeapon::NameArray;

WeaponSpecial to_weapon_special(uint8_t);

EsRankName parse_esrank_name(Address addr, const MemoryReader &);

} // end of <anonymous> namespace

namespace TextPalette {

char interpret_special(WeaponSpecial spec) {
    using Ws = WeaponSpecial;
    switch (spec) {
    case Ws::none:
        return k_plain;
    case Ws::draw: case Ws::drain: case Ws::fill: case Ws::gush:
        return k_tool;
    case Ws::blizzard: case Ws::spirit:
        return k_untekked;
    case Ws::geist: case Ws::devils: case Ws::demons:
        return k_defense;
    case Ws::heart  : case Ws::mind   : case Ws::soul  :
    case Ws::masters: case Ws::lords  : case Ws::kings :
    case Ws::heat   : case Ws::fire   : case Ws::flame :
    case Ws::ice    : case Ws::frost  : case Ws::freeze:
    case Ws::shock  : case Ws::thunder: case Ws::storm :
    case Ws::bind   : case Ws::hold   : case Ws::seize :
    case Ws::dim    : case Ws::shadow : case Ws::dark  :
    case Ws::panic  : case Ws::riot   : case Ws::havoc :
        return k_plain;
    case Ws::arrest :                   return k_weapon;
    case Ws::burning: case Ws::berserk: return k_esrank;
    case Ws::tempest: case Ws::charge : return k_gold;
    case Ws::hell   : case Ws::chaos  : return k_interest;
    }
    return k_plain;
}

} // end of TextPalette namespace

// ----------------------------------------------------------------------------

void WeaponBase::load_from_(Address addr, const MemoryReader & memory) {
    static constexpr const int k_special = 0x1F6;
    static constexpr const int k_grind   = 0x1F5;
    load_grind_and_special(addr + k_grind, addr + k_special, memory);
}

void WeaponBase::load_from_bank_(Address addr, const MemoryReader & memory) {
    static constexpr const int k_special = 0x4;
    static constexpr const int k_grind   = 0x3;
    load_grind_and_special(addr + k_grind, addr + k_special, memory);
}

/* private */ void WeaponBase::load_grind_and_special
    (Address grindaddr, Address specaddr, const MemoryReader & memory)
{
    uint8_t wraptekspec_datum = memory.read_u8(specaddr);
    special = to_weapon_special(wraptekspec_datum % 64);
    if (wraptekspec_datum > 0xBF) {
        wrapped = true ;
        tekked  = false;
    } else if (wraptekspec_datum > 0x7F) {
        tekked = false;
    } else if (wraptekspec_datum > 0x3F) {
        wrapped = true;
    }

    grind = memory.read_u8(grindaddr);
}

// ----------------------------------------------------------------------------

void DefenseItem::print_def_stats(std::ostream & out) const {
    bool dfp_varies = mins_maxes->min_dfp != mins_maxes->max_dfp;
    bool evp_varies = mins_maxes->min_evp != mins_maxes->max_evp;
    if (!dfp_varies && !evp_varies) return;

    out << "\\[";
    if (dfp_varies) {
        out << "[" << TextPalette::k_defense << ":";
        if (!evp_varies) { out << "DFP "; }
        out << (dfp + mins_maxes->min_dfp) << " (" << mins_maxes->max_dfp << ")]";
    }
    if (dfp_varies && evp_varies) { out << " - "; }
    if (evp_varies) {
        out << "[" << TextPalette::k_tool << ":";
        if (!evp_varies) { out << "EVP "; }
        out << (evp + mins_maxes->min_evp) << " (" << mins_maxes->max_evp << ")]";
    }
    out << "\\]";
}

void DefenseItem::load_def_stats
    (Address evp_addr, Address dfp_addr, const MemoryReader & memory)
{
    dfp = memory.read_u8(evp_addr);
    evp = memory.read_u8(dfp_addr);
    mins_maxes = &get_defense_item_info(fullcode);
}

// ----------------------------------------------------------------------------

void Tool::print_to(std::ostream & out) const {
    print_name(TextPalette::k_tool, out);
    if (quantity > 1)
        out << " x" << quantity;
}

void Tool::load_from_(Address addr, const MemoryReader & memory) {
    auto count = memory.read_u32(addr + k_count_offset);
    quantity = count ^ (addr + k_count_offset);
}

void Tool::load_from_bank_(Address addr, const MemoryReader & memory) {
    quantity = memory.read_u8(addr + 20);
}

// ----------------------------------------------------------------------------

void Tech::print_to(std::ostream & out) const {
    auto color = TextPalette::interpret_rarity(get_tech_rarity(type, level), TextPalette::k_tool);
    out << "[" << color << ":";
    print_name_min(out);
    if (!tech_has_only_one_level(type))
        out << " Lv " << level;
    out << "]";
}

void Tech::load_from_(Address addr, const MemoryReader & memory) {
    static constexpr const int k_tech_type_offset = 0x108;
    load_level_and_type(memory, addr + k_item_code_offset, addr + k_tech_type_offset);
}

void Tech::load_from_bank_(Address addr, const MemoryReader & memory)
    { load_level_and_type(memory, addr, addr + 4); }

void Tech::load_level_and_type
    (const MemoryReader & memory, Address leveladdr, Address typeaddr)
{
    level = ((memory.read_u32(leveladdr) >> 16) & 0xFF) + 1;
    type  = get_tech_type(memory.read_u8(typeaddr));
    set_name(to_string(type));
}

// ----------------------------------------------------------------------------

void Meseta::set_quantity(int i) {
    quantity = i;
}

/* private */ void Meseta::print_to(std::ostream & out) const {
    out << "[" << TextPalette::k_gold << ":" << quantity << " Meseta]";
}

/* private */ void Meseta::load_from_(Address addr, const MemoryReader & memory) {
    static constexpr const int k_meseta_offset = 0x100;
    quantity = memory.read_u32(addr + k_meseta_offset);
}

/* private */ void Meseta::load_from_bank_(Address addr, const MemoryReader & memory) {
    // sorry... what?
    quantity = memory.read_u32(addr + 12);
}

// ----------------------------------------------------------------------------

void Weapon::print_to(std::ostream & out) const {
    auto defwep_color = TextPalette::k_weapon;
    if ((!tekked || wrapped) && rarity != Rarity::uber) {
        defwep_color = TextPalette::k_untekked;
        out << "\\[[" << TextPalette::k_untekked << ":"
            << (tekked ? "" : "U") << (wrapped ? "W" : "") << "]\\] ";
    }
    if (special != WeaponSpecial::none && !is_rare_tier(rarity)) {
        out << "[" << TextPalette::interpret_special(special) << ":"
            << to_string(special) << "] ";
    }
    print_name(defwep_color, out);

    if (grind != 0) {
        out << " +" << grind;
    }
    if (kills != k_has_no_kill_counter) {
        out << " (" << kills << " kills)";
    }

    if (m_attr_count == 1) {
        print_single_attribute(out);
    } else if (m_attr_count > 1) {
        print_multiple_attributes(out);
    }
}

void Weapon::load_from_(Address addr, const MemoryReader & memory) {
    WeaponBase::load_from_(addr, memory);
    load_attributes(addr + WeaponBase::k_stats_offset, memory);
}

void Weapon::load_from_bank_(Address addr, const MemoryReader & memory) {
    WeaponBase::load_from_bank_(addr, memory);
    load_attributes(addr + 6, memory);
}

void Weapon::load_attributes(Address attraddr, const MemoryReader & memory) {
    m_attr_count = 0;
    // on Solybum's [7 12]
    using AttrData = std::array<uint8_t, 3/* attributes */*2/* id + %s */>;
    AttrData attributes;
    memory.read(attraddr, attributes.data(), attributes.size());
    for (auto idx : { 0, 2, 4 }) {
        if (attributes[idx] >= 6) continue;
        // I'll need to test negative percentages too
        auto attr = int8_t(attributes[idx + 1]);
        m_attributes[attributes[idx] - 1] = attr;
        if (attr) ++m_attr_count;
    }
}

/* private */ void Weapon::print_single_attribute(std::ostream & out) const {
    for (const auto & perc : m_attributes) {
        if (perc == 0) continue;
        out << " [" << get_attribute_color(perc, &perc == &m_attributes.back())
            << ":" << int(perc) << "]";
        switch (&perc - &m_attributes.front()) {
        case 0: out << "n" ; break;
        case 1: out << "ab"; break;
        case 2: out << "m" ; break;
        case 3: out << "d" ; break;
        case 4: out << "h" ; break;
        default: throw "impossible branch";
        }
    }
}

/* private */ void Weapon::print_multiple_attributes(std::ostream & out) const {
    out << " \\[";
    bool has_hit = m_attributes.back() != 0;
    const auto * last = &m_attributes.back() - (has_hit ? 0 : 1);
    for (const auto & perc : m_attributes) {
        if (perc != 0) {
            out << "[" << get_attribute_color(perc, &perc == &m_attributes.back())
                << ":" << std::setw(3) << int(perc) << std::setw(0) << "]";
        } else {
            out << " - ";
        }
        if (&perc == last) { break; }
        // have to escape for both C++ and my string colorizer
        else { out << "\\\\"; }
    }
    out << "\\]";
}

/* static */ char Weapon::get_attribute_color(int8_t val, bool hit) {
    using namespace TextPalette;
    if (val < 0) return k_interest;
    if (hit ? val > 35 : val > 75) return k_esrank;
    if (hit ? val >  1 : val > 50) return k_rare  ;
    return k_plain;
}

// ----------------------------------------------------------------------------

void EsWeapon::print_to(std::ostream & out) const {
    out << "[" << TextPalette::k_esrank << ":" << custom_name.data()
        << " ES] ";
    print_name(TextPalette::k_esrank, out);
    if (grind != 0) { out << " +" << grind; }
}

void EsWeapon::load_from_(Address addr, const MemoryReader & memory) {
    WeaponBase::load_from_(addr, memory);
    custom_name = parse_esrank_name(addr + k_name_offset, memory);
}

void EsWeapon::load_from_bank_(Address addr, const MemoryReader & memory) {
    WeaponBase::load_from_bank_(addr, memory);
    custom_name = parse_esrank_name(addr + 6, memory);
}

// ----------------------------------------------------------------------------

void Frame::print_to(std::ostream & out) const {
    print_name(TextPalette::k_defense, out) << " (";
    if (slot_count != 0) {
        out << slot_count << " slot" << (slot_count != 1 ? "s" : "");
    } else {
        out << "no slots";
    }
    out << ") ";
    print_def_stats(out);
}

void Frame::load_from_(Address addr, const MemoryReader & memory) {
    slot_count = memory.read_u8(addr + k_slots_offset);
    static constexpr const int k_dfp_offset = 0x1B9;
    static constexpr const int k_evp_offset = 0x1BA;
    load_def_stats(addr + k_evp_offset, addr + k_dfp_offset, memory);
}

void Frame::load_from_bank_(Address addr, const MemoryReader & memory) {
    slot_count = memory.read_u8(addr + 5);
    load_def_stats(addr + 8, addr + 6, memory);
}

// ----------------------------------------------------------------------------

void Unit::print_to(std::ostream & out) const {
    print_name(TextPalette::k_defense, out);
    if (kills != k_has_no_kill_counter) {
        out << " (" << kills << " kills)";
    }
}

void Unit::load_from_(Address, const MemoryReader &) {}

void Unit::load_from_bank_(Address, const MemoryReader &) {}

// ----------------------------------------------------------------------------

void Barrier::print_to(std::ostream & out) const {
    print_name(TextPalette::k_defense, out) << " ";
    print_def_stats(out);
}

void Barrier::load_from_(Address addr, const MemoryReader & memory) {
    static constexpr const int k_dfp_offset = 0x1E4;
    static constexpr const int k_evp_offset = 0x1E5;
    load_def_stats(addr + k_evp_offset, addr + k_dfp_offset, memory);
}

void Barrier::load_from_bank_(Address addr, const MemoryReader & memory) {
    load_def_stats(addr + 8, addr + 6, memory);
}

// ----------------------------------------------------------------------------

void Mag::print_to(std::ostream & out) const {
    int level = 0;
    for (int l : levels) level += l;
    out << "Lv " << level << " ";
    print_name(TextPalette::k_plain, out) << " ";
    if (!in_bank) {
        if (seconds_until_feeding > 90) {
            int mins = seconds_until_feeding / 60;
            int secs = seconds_until_feeding % 60;
            const char * min_str = mins != 1 ? "mins" : "min";
            out << "feed in " << mins << min_str << " " << secs << "s ";
        } else if (seconds_until_feeding == 0) {
            out << "[" << TextPalette::k_gold << ":READY!] ";
        } else if (seconds_until_feeding <= 90) {
            out << "feed in " << seconds_until_feeding << "s ";
        }
    }

    bool feed = seconds_until_feeding == 0;
    out << "\\[DEF " << int(levels[k_def ]) << " ";
    if (feed && !in_bank) { out << "(" << int(percentages[k_def ]) << "%) "; }
    out << "POW " << int(levels[k_pow ]) << " ";
    if (feed && !in_bank) { out << "(" << int(percentages[k_pow ]) << "%) "; }
    out << "DEX " << int(levels[k_dex ]) << " ";
    if (feed && !in_bank) { out << "(" << int(percentages[k_dex ]) << "%) "; }
    out << "MND " << int(levels[k_mind]);
    if (feed && !in_bank) { out << " (" << int(percentages[k_mind]) << "%)"; }
    out << "\\]";
}

void Mag::load_from_(Address addr, const MemoryReader & memory) {
    static constexpr const int k_stat_offset  = 0x1C0;
    static constexpr const int k_timer_offset = 0x1B4;
    load_stats(addr + k_stat_offset, memory);
    seconds_until_feeding = int(std::round(memory.read_f32(addr + k_timer_offset) / 30.f));
}

void Mag::load_from_bank_(Address addr, const MemoryReader & memory) {
    load_stats(addr + 4, memory);
    in_bank = true;
}

void Mag::load_stats(Address addr, const MemoryReader & memory) {
    std::array<uint16_t, k_stat_count> rawstats;
    memory.read(addr, reinterpret_cast<uint8_t *>(rawstats.data()),
                rawstats.size()*sizeof(uint16_t));

    levels[k_def ] = rawstats[k_def ] / 100;
    levels[k_pow ] = rawstats[k_pow ] / 100;
    levels[k_dex ] = rawstats[k_dex ] / 100;
    levels[k_mind] = rawstats[k_mind] / 100;

    percentages[k_def ] = rawstats[k_def ] % 100;
    percentages[k_pow ] = rawstats[k_pow ] % 100;
    percentages[k_dex ] = rawstats[k_dex ] % 100;
    percentages[k_mind] = rawstats[k_mind] % 100;
}

// ----------------------------------------------------------------------------

void TotallyUnknownItem::print_to(std::ostream & out) const {
    out << "[" << TextPalette::k_untekked << ":?";
    print_name_min(out);
    out << "?]";
}

namespace {

WeaponSpecial to_weapon_special(uint8_t code) {
    if (code > uint8_t(WeaponSpecial::demons)) {
        throw std::invalid_argument("Cannot convert code to weapon special. (perhaps a different set of parsing rules apply?)");
    }
    return static_cast<WeaponSpecial>(code);
}


EsRankName parse_esrank_name(Address addr, const MemoryReader & memory) {
    using NameData = std::array<uint16_t, 6 / sizeof(uint16_t)>;

    NameData namedata = zero_initialized<NameData>();
    memory.read(addr,
                reinterpret_cast<uint8_t *>(namedata.data()),
                namedata.size()*sizeof(uint16_t));

    EsRankName rv = zero_initialized<EsRankName>();
    auto itr = rv.begin();
    auto add_char = [&itr] (int t) {
        t |= ('A' - 1);
        if (t >= 'A' && t <= 'Z') { *itr++ = char(t); }
    };

    // there is absolutely no documentation found in hell/heaven/earth
    // on how this works

    // each character is 5 bits
    // values 1-26 map to A-Z

    bool first_time = true;
    for (auto n : namedata) {
        n = ((n & 0xFF) << 8) | (n >> 8);

        if (!first_time) add_char((n >> 10) & 0x1F);
        else first_time = false;

        add_char((n >> 5) & 0x1F);
        add_char(n & 0x1F);
    }
    return rv;
}

} // end of <anonymous> namespace
