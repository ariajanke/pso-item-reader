/****************************************************************************

    File: ItemDb.hpp
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

#include <cstdint>

#include "ItemReader.hpp"

enum class TechType {
    foie   , zonde  , barta  ,
    gifioe , gizonde, gibarta,
    rafoie , razonde, rabarta,
    grants , megid  ,
    resta  , anti   , reverser,
    shifta , deband , jellen  , zalure, ryuker
};

enum class WeaponSpecial : uint8_t {
    none    ,
    draw   , drain  , fill   , gush ,
    heart  , mind   , soul   , geist,
    masters, lords  , kings  ,
    charge , spirit , berserk,
    ice    , frost  , freeze , blizzard,
    bind   , hold   , seize  , arrest  ,
    heat   , fire   , flame  , burning ,
    shock  , thunder, storm  , tempest ,
    dim    , shadow , dark   , hell    ,
    panic  , riot   , havoc  , chaos   ,
    devils , demons
};

struct ItemInfo {
    static constexpr const auto k_unknown_item = Item::k_unknown_item;

    const char * name     = k_unknown_item;
    Rarity rarity         = Rarity::common;
    bool has_kill_counter = false;
};

struct DefenseItemInfo {
    static constexpr const int k_uninit = -1;

    DefenseItemInfo() = default;

    DefenseItemInfo(int max_dfp_, int min_dfp_, int max_evp_, int min_evp_):
        max_dfp(max_dfp_), min_dfp(min_dfp_), max_evp(max_evp_), min_evp(min_evp_)
    {}

    int max_dfp = k_uninit, min_dfp = k_uninit;
    int max_evp = k_uninit, min_evp = k_uninit;
};

const ItemInfo & get_item_info(uint32_t fullcode);

const DefenseItemInfo & get_defense_item_info(uint32_t fullcode);

TechType get_tech_type(int tech_code);

Rarity get_tech_rarity(TechType, int level);

bool tech_has_only_one_level(TechType);

const char * to_string(TechType);

const char * to_string(WeaponSpecial);

bool is_esrank(uint32_t fullcode);
