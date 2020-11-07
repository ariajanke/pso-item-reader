/****************************************************************************

    File: ItemDb.cpp
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

#include "ItemDb.hpp"
#include "ItemReader.hpp"
#include "Item.hpp"
#include "../Defs.hpp"

#include <unordered_map>
#include <functional>

#include <cassert>

#ifdef MACRO_USE_MT_INITIALIZATION
#   include <memory>
#   include <atomic>
#   include <future>
#endif

namespace {

using ItemInfoMap = std::unordered_map<uint32_t, ItemInfo>;
using DefItemMap  = std::unordered_map<uint32_t, DefenseItemInfo>;

static constexpr const bool k_is_ephinea = true;

void add_weapons (ItemInfoMap &);
void add_frames  (ItemInfoMap &, DefItemMap &);
void add_barriers(ItemInfoMap &, DefItemMap &);
void add_units   (ItemInfoMap &);
void add_mags    (ItemInfoMap &);
void add_tools   (ItemInfoMap &);
void add_ephinea (ItemInfoMap &);
void add_esranks (ItemInfoMap &);

void prepare_code(uint32_t &);

#ifdef MACRO_USE_MT_INITIALIZATION

template <typename T>
using PtrMaker = T * (*)();

template <typename T>
PtrMaker<T> make_instance_maker();

#endif

class ItemDb {
public:
    static const ItemDb & instance();

    const ItemInfoMap & get_info_map() const { return m_info_map; }

    const DefItemMap  & get_def_map () const { return m_def_map ; }

private:
#   ifdef MACRO_USE_MT_INITIALIZATION
    template <typename T>
    friend PtrMaker<T> make_instance_maker();
#   endif
    ItemDb();

    ItemInfoMap m_info_map;
    DefItemMap m_def_map;
};

} // end of <anonymous> namespace

/* free fn */ const ItemInfo & get_item_info(uint32_t fullcode) {
    prepare_code(fullcode);

    const auto & info_map = ItemDb::instance().get_info_map();
    auto itr = info_map.find(fullcode);
    if (itr == info_map.end()) {
        static const ItemInfo k_unknown;
        return k_unknown;
    }
    return itr->second;
}

/* free fn */ const DefenseItemInfo & get_defense_item_info(uint32_t fullcode) {
    prepare_code(fullcode);

    const auto & def_map = ItemDb::instance().get_def_map();
    auto itr = def_map.find(fullcode);
    if (itr == def_map.end()) {
        static const DefenseItemInfo k_unknown;
        return k_unknown;
    }
    return itr->second;
}

/* free fn */ TechType get_tech_type(int tech_code) {
    using Tt = TechType;
    switch (tech_code) {
    case 0x00: return Tt::foie   ; case 0x01: return Tt::gifioe ; case 0x02: return Tt::rafoie ;
    case 0x03: return Tt::barta  ; case 0x04: return Tt::gibarta; case 0x05: return Tt::rabarta;
    case 0x06: return Tt::zonde  ; case 0x07: return Tt::gizonde; case 0x08: return Tt::razonde;
    case 0x09: return Tt::grants ; case 0x12: return Tt::megid  ;
    case 0x0A: return Tt::deband ; case 0x0D: return Tt::shifta ;
    case 0x0B: return Tt::jellen ; case 0x0C: return Tt::zalure ;
    case 0x0F: return Tt::resta  ; case 0x10: return Tt::anti   ;
    case 0x0E: return Tt::ryuker ;
    case 0x11: return Tt::reverser;
    default:
        throw std::runtime_error("get_tech_type: no tech associated with this code.");
    }
}

/* free fn */ Rarity get_tech_rarity(TechType tech, int level) {
    using Tt = TechType;
    switch (tech) {
    case Tt::foie    : case Tt::zonde   : case Tt::barta   :
    case Tt::gifioe  : case Tt::gizonde : case Tt::gibarta :
        switch (level) {
        case 20: case 15: return Rarity::interest;
        case 30: return Rarity::rare;
        default: return Rarity::common;
        }
    case Tt::rafoie  : case Tt::razonde : case Tt::rabarta :
        switch (level) {
        case 20: case 15: return Rarity::interest;
        case 29: case 30: return Rarity::rare;
        default: return Rarity::common;
        }
    case Tt::grants  : case Tt::megid   :
        switch (level) {
        case 26: case 27: return Rarity::interest;
        case 28: case 29: return Rarity::rare;
        case 30: return Rarity::uber;
        default: return Rarity::common;
        }
    case Tt::resta   : case Tt::shifta  : case Tt::deband  :
                       case Tt::jellen  : case Tt::zalure  :
        switch (level) {
        case 30: case 20: case 15: return Rarity::interest;
        default: return Rarity::common;
        }
    case Tt::anti    : return (level == 5 || level == 7) ? Rarity::interest : Rarity::common;
    case Tt::reverser: case Tt::ryuker  : return Rarity::interest;
    }
    throw std::invalid_argument("get_tech_rarity: tech type is not valid.");
}

/* free fn */ bool tech_has_only_one_level(TechType tech) {
    using Tt = TechType;
    switch (tech) {
    case Tt::foie    : case Tt::zonde   : case Tt::barta   :
    case Tt::gifioe  : case Tt::gizonde : case Tt::gibarta :
    case Tt::rafoie  : case Tt::razonde : case Tt::rabarta :
    case Tt::grants  : case Tt::megid   :
    case Tt::resta   : case Tt::anti    :
    case Tt::shifta  : case Tt::deband  :
    case Tt::jellen  : case Tt::zalure  : return false;
    case Tt::reverser: case Tt::ryuker  : return true;
    }
    throw std::invalid_argument("tech_has_only_one_level: tech type is not valid.");
}

/* free fn */ const char * to_string(TechType tech) {
    using Tt = TechType;
    switch (tech) {
    case Tt::foie    : return "Foie"    ;
    case Tt::zonde   : return "Zonde"   ;
    case Tt::barta   : return "Barta"   ;
    case Tt::gifioe  : return "Gifoie"  ;
    case Tt::gizonde : return "Gizonde" ;
    case Tt::gibarta : return "Gibarta" ;
    case Tt::rafoie  : return "Rafoie"  ;
    case Tt::razonde : return "Razonde" ;
    case Tt::rabarta : return "Rabarta" ;
    case Tt::grants  : return "Grants"  ;
    case Tt::megid   : return "Megid"   ;
    case Tt::resta   : return "Resta"   ;
    case Tt::anti    : return "Anti"    ;
    case Tt::reverser: return "Reverser";
    case Tt::shifta  : return "Shifta"  ;
    case Tt::deband  : return "Deband"  ;
    case Tt::jellen  : return "Jellen"  ;
    case Tt::zalure  : return "Zalure"  ;
    case Tt::ryuker  : return "Ryuker"  ;
    }
    throw std::invalid_argument("get_tech_name: tech type is not valid.");
}

/* free fn */ const char * to_string(WeaponSpecial special) {
    using Ws = WeaponSpecial;
    switch (special) {
    case Ws::none    : return "<none>";
    case Ws::draw    : return "Draw";
    case Ws::drain   : return "Drain";
    case Ws::fill    : return "Fill";
    case Ws::gush    : return "Gush";
    case Ws::heart   : return "Heart";
    case Ws::mind    : return "Mind";
    case Ws::soul    : return "Soul";
    case Ws::geist   : return "Geist";
    case Ws::masters : return "Master's";
    case Ws::lords   : return "Lord's";
    case Ws::kings   : return "King's";
    case Ws::charge  : return "Charge";
    case Ws::spirit  : return "Spirit";
    case Ws::berserk : return "Bersek";
    case Ws::ice     : return "Ice";
    case Ws::frost   : return "Frost";
    case Ws::freeze  : return "Freeze";
    case Ws::blizzard: return "Blizzard";
    case Ws::bind    : return "Bind";
    case Ws::hold    : return "Hold";
    case Ws::seize   : return "Seize";
    case Ws::arrest  : return "Arrest";
    case Ws::heat    : return "Heat";
    case Ws::fire    : return "Fire";
    case Ws::flame   : return "Flame";
    case Ws::burning : return "Burning";
    case Ws::shock   : return "Shock";
    case Ws::thunder : return "Thunder";
    case Ws::storm   : return "Storm";
    case Ws::tempest : return "Tempest";
    case Ws::dim     : return "Dim";
    case Ws::shadow  : return "Shadow";
    case Ws::dark    : return "Dark";
    case Ws::hell    : return "Hell";
    case Ws::panic   : return "Panic";
    case Ws::riot    : return "Riot";
    case Ws::havoc   : return "Havoc";
    case Ws::chaos   : return "Chaos";
    case Ws::devils  : return "Devil's";
    case Ws::demons  : return "Demon's";
    default: throw std::invalid_argument("to_string: cannot convert weapon special value to a string.");
    }
}

/* free fn */ bool is_esrank(uint32_t fullcode) {
    fullcode = (fullcode >> 8) & 0xFF;
    return (fullcode >= 0x70 && fullcode < 0x89) ||
           (fullcode >= 0xA5 && fullcode < 0xAA);
}

namespace {

using IntPair = std::pair<int, int>;
struct EvpPair : IntPair { EvpPair(int a, int b): IntPair(a, b) {} };
struct DfpPair : IntPair { DfpPair(int a, int b): IntPair(a, b) {} };
static_assert(!std::is_same_v<EvpPair, DfpPair>, "");

using namespace std::placeholders;
void add_item_only_once(ItemInfoMap &, uint32_t, const char *,
                        Rarity rarity, bool has_kill_counter);

void add_def_only_once
    (DefItemMap &, ItemInfoMap &,
     uint32_t fullcode, const char *, const DfpPair &, const EvpPair &,
     Rarity, bool has_kill_counter);

void add_weapons(ItemInfoMap & map) {
    using std::ref;
    auto add_generic = std::bind(add_item_only_once, ref(map), _1, _2, Rarity::common, false);
    auto add_rare    = std::bind(add_item_only_once, ref(map), _1, _2, Rarity::rare  , false);
#   if 0
    auto add_rare_wk = std::bind(add_item_only_once, ref(map), _1, _2, Rarity::rare  , true );
#   endif
    auto add_uber_wk = std::bind(add_item_only_once, ref(map), _1, _2, Rarity::uber  , true );
    auto add_uber    = std::bind(add_item_only_once, ref(map), _1, _2, Rarity::uber  , false);
    add_generic(0x000000, "Saber");
    add_generic(0x000100, "Saber");
    add_generic(0x000101, "Brand");
    add_generic(0x000102, "Buster");
    add_generic(0x000103, "Pallasch");
    add_generic(0x000104, "Gladius");
    add_rare   (0x000105, "DB'S SABER");
    add_rare   (0x000106, "KALADBOLG");
    add_rare   (0x000107, "DURANDAL");
    add_rare   (0x000108, "GALATINE");
    add_generic(0x000200, "Sword");
    add_generic(0x000201, "Gigush");
    add_generic(0x000202, "Breaker");
    add_generic(0x000203, "Claymore");
    add_generic(0x000204, "Calibur");
    add_rare   (0x000205, "FLOWEN'S SWORD");
    add_rare   (0x000206, "LAST SURVIVOR");
    add_rare   (0x000207, "DRAGON SLAYER");
    add_generic(0x000300, "Dagger");
    add_generic(0x000301, "Knife");
    add_generic(0x000302, "Blade");
    add_generic(0x000303, "Edge");
    add_generic(0x000304, "Ripper");
    add_rare   (0x000305, "BLADE DANCE");
    add_rare   (0x000306, "BLOODY ART");
    add_rare   (0x000307, "CROSS SCAR");
    add_rare   (0x000308, "ZERO DIVIDE");
    add_rare   (0x000309, "TWO KAMUI");
    add_generic(0x000400, "Partisan");
    add_generic(0x000401, "Halbert");
    add_generic(0x000402, "Glaive");
    add_generic(0x000403, "Berdys");
    add_generic(0x000404, "Gungnir");
    add_rare   (0x000405, "BRIONAC");
    add_rare   (0x000406, "VJAYA");
    add_rare   (0x000407, "GAE BOLG");
    add_rare   (0x000408, "ASTERON BELT");
    add_generic(0x000500, "Slicer");
    add_generic(0x000501, "Spinner");
    add_generic(0x000502, "Cutter");
    add_generic(0x000503, "Sawcer");
    add_generic(0x000504, "Diska");
    add_rare   (0x000505, "SLICER OF ASSASSIN");
    add_rare   (0x000506, "DISKA OF LIBERATOR");
    add_rare   (0x000507, "DISKA OF BRAVEMAN");
    add_rare   (0x000508, "IZMAELA");
    add_generic(0x000600, "Handgun");
    add_generic(0x000601, "Autogun");
    add_generic(0x000602, "Lockgun");
    add_generic(0x000603, "Railgun");
    add_generic(0x000604, "Raygun");
    add_rare   (0x000605, "VARISTA");
    add_rare   (0x000606, "CUSTOM RAY ver.OO");
    add_rare   (0x000607, "BRAVACE");
    add_rare   (0x000608, "TENSION BLASTER");
    add_generic(0x000700, "Rifle");
    add_generic(0x000701, "Sniper");
    add_generic(0x000702, "Blaster");
    add_generic(0x000703, "Beam");
    add_generic(0x000704, "Laser");
    add_rare   (0x000705, "VISK-235W");
    add_rare   (0x000706, "WALS-MK2");
    add_rare   (0x000707, "JUSTY-23ST");
    add_rare   (0x000708, "RIANOV 303SNR");
    add_rare   (0x000709, "RIANOV 303SNR-1");
    add_rare   (0x00070A, "RIANOV 303SNR-2");
    add_rare   (0x00070B, "RIANOV 303SNR-3");
    add_rare   (0x00070C, "RIANOV 303SNR-4");
    add_rare   (0x00070D, "RIANOV 303SNR-5");
    add_generic(0x000800, "Mechgun");
    add_generic(0x000801, "Assault");
    add_generic(0x000802, "Repeater");
    add_generic(0x000803, "Gatling");
    add_generic(0x000804, "Vulcan");
    add_rare   (0x000805, "M&A60 VISE");
    add_rare   (0x000806, "H&S25 JUSTICE");
    add_rare   (0x000807, "L&K14 COMBAT");
    add_generic(0x000900, "Shot");
    add_generic(0x000901, "Spread");
    add_generic(0x000902, "Cannon");
    add_generic(0x000903, "Launcher");
    add_generic(0x000904, "Arms");
    add_rare   (0x000905, "CRUSH BULLET");
    add_rare   (0x000906, "METEOR SMASH");
    add_rare   (0x000907, "FINAL IMPACT");
    add_generic(0x000A00, "Cane");
    add_generic(0x000A01, "Stick");
    add_generic(0x000A02, "Mace");
    add_generic(0x000A03, "Club");
    add_rare   (0x000A04, "CLUB OF LACONIUM");
    add_rare   (0x000A05, "MACE OF ADAMAN");
    add_rare   (0x000A06, "CLUB OF ZUMIURAN");
    add_rare   (0x000A07, "LOLLIPOP");
    add_generic(0x000B00, "Rod");
    add_generic(0x000B01, "Pole");
    add_generic(0x000B02, "Pillar");
    add_generic(0x000B03, "Striker");
    add_rare   (0x000B04, "BATTLE VERGE");
    add_rare   (0x000B05, "BRAVE HAMMER");
    add_rare   (0x000B06, "ALIVE AQHU");
    add_rare   (0x000B07, "VALKYRIE");
    add_generic(0x000C00, "Wand");
    add_generic(0x000C01, "Staff");
    add_generic(0x000C02, "Baton");
    add_generic(0x000C03, "Scepter");
    add_rare   (0x000C04, "FIRE SCEPTER:AGNI");
    add_rare   (0x000C05, "ICE STAFF:DAGON");
    add_rare   (0x000C06, "STORM WAND:INDRA");
    add_rare   (0x000C07, "EARTH WAND BROWNIE");
    add_rare   (0x000D00, "PHOTON CLAW");
    add_rare   (0x000D01, "SILENCE CLAW");
    add_rare   (0x000D02, "NEI'S CLAW");
    add_rare   (0x000D03, "PHOENIX CLAW");
    add_rare   (0x000E00, "DOUBLE SABER");
    add_rare   (0x000E01, "STAG CUTLERY");
    add_rare   (0x000E02, "TWIN BRAND");
    add_rare   (0x000F00, "BRAVE KNUCKLE");
    add_rare   (0x000F01, "ANGRY FIST");
    add_rare   (0x000F02, "GOD HAND");
    add_rare   (0x000F03, "SONIC KNUCKLE");
    add_rare   (0x000F04, "LOGiN");
    add_rare   (0x001000, "OROTIAGITO");
    add_rare   (0x001001, "AGITO 1975");
    add_rare   (0x001002, "AGITO 1983");
    add_rare   (0x001003, "AGITO 2001");
    add_rare   (0x001004, "AGITO 1991");
    add_rare   (0x001005, "AGITO 1977");
    add_rare   (0x001006, "AGITO 1980");
    add_rare   (0x001007, "RAIKIRI");
    add_rare   (0x001100, "SOUL EATER");
    add_rare   (0x001101, "SOUL BANISH");
    add_rare   (0x001200, "SPREAD NEEDLE");
    add_rare   (0x001300, "HOLY RAY");
    add_rare   (0x001400, "INFERNO BAZOOKA");
    add_rare   (0x001401, "RAMBLING MAY");
    add_rare   (0x001402, "L&K38 COMBAT");
    add_rare   (0x001500, "FLAME VISIT");
    add_rare   (0x001501, "BURNING VISIT");
    add_rare   (0x001600, "AKIKO'S FRYING PAN");
    add_rare   (0x001700, "SORCERER'S CANE");
    add_rare   (0x001800, "S-BEAT'S BLADE");
    add_rare   (0x001900, "P-ARMS'S BLADE");
    add_rare   (0x001A00, "DELSABER'S BUSTER");
    add_rare   (0x001B00, "BRINGER'S RIFLE");
    add_rare   (0x001C00, "EGG BLASTER");
    add_uber   (0x001D00, "PSYCHO WAND");
    add_uber   (0x001E00, "HEAVEN PUNISHER");
    add_uber   (0x001F00, "LAVIS CANNON");
    add_rare   (0x002000, "VICTOR AXE");
    add_rare   (0x002001, "LACONIUM AXE");
    add_rare   (0x002100, "CHAIN SAWD");
    add_rare   (0x002200, "CADUCEUS");
    add_rare   (0x002201, "MERCURIUS ROD");
    add_rare   (0x002300, "STING TIP");
    add_rare   (0x002400, "MAGICAL PIECE");
    add_rare   (0x002500, "TECHNICAL CROZIER");
    add_rare   (0x002600, "SUPPRESSED GUN");
    add_rare   (0x002700, "ANCIENT SABER");
    add_rare   (0x002800, "HARISEN BATTLE FAN");
    add_rare   (0x002900, "YAMIGARASU");
    add_rare   (0x002A00, "AKIKO'S WOK");
    add_rare   (0x002B00, "TOY HAMMER");
    add_rare   (0x002C00, "ELYSION");
    add_rare   (0x002D00, "RED SABER");
    add_rare   (0x002E00, "METEOR CUDGEL");
    add_rare   (0x002F00, "MONKEY KING BAR");
    add_rare   (0x002F01, "BLACK KING BAR");
    add_uber   (0x003000, "DOUBLE CANNON");
    add_rare   (0x003001, "GIRASOLE");
    add_rare   (0x003100, "HUGE BATTLE FAN");
    add_uber   (0x003200, "TSUMIKIRI J-SWORD");
    add_uber_wk(0x003300, "SEALED J-SWORD");
    add_rare   (0x003400, "RED SWORD");
    add_rare   (0x003500, "CRAZY TUNE");
    add_rare   (0x003600, "TWIN CHAKRAM");
    add_rare   (0x003700, "WOK OF AKIKO'S SHOP");
    add_uber   (0x003800, "LAVIS BLADE");
    add_rare   (0x003900, "RED DAGGER");
    add_rare   (0x003A00, "MADAM'S PARASOL");
    add_rare   (0x003B00, "MADAM'S UMBRELLA");
    add_rare   (0x003C00, "IMPERIAL PICK");
    add_rare   (0x003D00, "BERDYSH");
    add_rare   (0x003E00, "RED PARTISAN");
    add_rare   (0x003F00, "FLIGHT CUTTER");
    add_rare   (0x004000, "FLIGHT FAN");
    add_rare   (0x004100, "RED SLICER");
    add_uber   (0x004200, "HANDGUN:GULD");
    add_rare   (0x004201, "MASTER RAVEN");
    add_rare   (0x004300, "HANDGUN:MILLA");
    add_rare   (0x004301, "LAST SWAN");
    add_rare   (0x004400, "RED HANDGUN");
    add_rare   (0x004500, "FROZEN SHOOTER");
    add_rare   (0x004501, "SNOW QUEEN");
    add_rare   (0x004600, "ANTI ANDROID RIFLE");
    add_rare   (0x004700, "ROCKET PUNCH");
    add_rare   (0x004800, "SAMBA MARACAS");
    add_rare   (0x004900, "TWIN PSYCHOGUN");
    add_rare   (0x004A00, "DRILL LAUNCHER");
    add_uber   (0x004B00, "GULD MILLA");
    add_rare   (0x004B01, "DUAL BIRD");
    add_rare   (0x004C00, "RED MECHGUN");
    add_rare   (0x004D00, "BELRA CANNON");
    add_rare   (0x004E00, "PANZER FAUST");
    add_rare   (0x004E01, "IRON FAUST");
    add_rare   (0x004F00, "SUMMIT MOON");
    add_rare   (0x005000, "WINDMILL");
    add_rare   (0x005100, "EVIL CURST");
    add_rare   (0x005200, "FLOWER CANE");
    add_rare   (0x005300, "HILDEBEAR'S CANE");
    add_rare   (0x005400, "HILDEBLUE'S CANE");
    add_rare   (0x005500, "RABBIT WAND");
    add_rare   (0x005600, "PLANTAIN LEAF");
    add_rare   (0x005601, "FATSIA");
    add_rare   (0x005700, "DEMONIC FORK");
    add_rare   (0x005800, "STRIKER OF CHAO");
    add_rare   (0x005900, "BROOM");
    add_uber   (0x005A00, "PROPHETS OF MOTAV");
    add_rare   (0x005B00, "THE SIGH OF A GOD");
    add_rare   (0x005C00, "TWINKLE STAR");
    add_rare   (0x005D00, "PLANTAIN FAN");
    add_rare   (0x005E00, "TWIN BLAZE");
    add_rare   (0x005F00, "MARINA'S BAG");
    add_rare   (0x006000, "DRAGON'S CLAW");
    add_rare   (0x006100, "PANTHER'S CLAW");
    add_rare   (0x006200, "S-RED'S BLADE");
    add_rare   (0x006300, "PLANTAIN HUGE FAN");
    add_rare   (0x006400, "CHAMELEON SCYTHE");
    add_rare   (0x006500, "YASMINKOV 3000R");
    add_rare   (0x006600, "ANO RIFLE");
    add_rare   (0x006700, "BARANZ LAUNCHER");
    add_rare   (0x006800, "BRANCH OF PAKUPAKU");
    add_rare   (0x006900, "HEART OF POUMN");
    add_rare   (0x006A00, "YASMINKOV 2000H");
    add_rare   (0x006B00, "YASMINKOV 7000V");
    add_rare   (0x006C00, "YASMINKOV 9000M");
    add_rare   (0x006D00, "MASER BEAM");
    add_rare   (0x006D01, "POWER MASER");
    add_rare   (0x006E00, "GAME MAGAZNE");
    add_rare   (0x006E01, "LOGiN");
    add_rare   (0x006F00, "FLOWER BOUQUET");
    add_rare   (0x008900, "MUSASHI");
    add_rare   (0x008901, "YAMATO");
    add_rare   (0x008902, "ASUKA");
    add_rare   (0x008903, "SANGE & YASHA");
    add_rare   (0x008A00, "SANGE");
    add_rare   (0x008A01, "YASHA");
    add_rare   (0x008A02, "KAMUI");
    add_rare   (0x008B00, "PHOTON LAUNCHER");
    add_rare   (0x008B01, "GUILTY LIGHT");
    add_rare   (0x008B02, "RED SCORPIO");
    add_rare   (0x008B03, "PHONON MASER");
    add_rare   (0x008C00, "TALIS");
    add_rare   (0x008C01, "MAHU");
    add_rare   (0x008C02, "HITOGATA");
    add_rare   (0x008C03, "DANCING HITOGATA");
    add_rare   (0x008C04, "KUNAI");
    add_uber   (0x008D00, "NUG2000-BAZOOKA");
    add_rare   (0x008E00, "S-BERILL'S HANDS #0");
    add_rare   (0x008E01, "S-BERILL'S HANDS #1");
    add_rare   (0x008F00, "FLOWEN'S SWORD 3060");
    add_rare   (0x008F01, "FLOWEN'S SWORD 3064");
    add_rare   (0x008F02, "FLOWEN'S SWORD 3067");
    add_rare   (0x008F03, "FLOWEN'S SWORD 3073");
    add_rare   (0x008F04, "FLOWEN'S SWORD 3077");
    add_rare   (0x008F05, "FLOWEN'S SWORD 3082");
    add_rare   (0x008F06, "FLOWEN'S SWORD 3083");
    add_rare   (0x008F07, "FLOWEN'S SWORD 3084");
    add_rare   (0x008F08, "FLOWEN'S SWORD 3079");
    add_rare   (0x009000, "DB'S SABER 3062");
    add_rare   (0x009001, "DB'S SABER 3067");
    add_rare   (0x009002, "DB'S SABER 3069 Chris");
    add_rare   (0x009003, "DB'S SABER 3064");
    add_rare   (0x009004, "DB'S SABER 3069 Torato");
    add_rare   (0x009005, "DB'S SABER 3073");
    add_rare   (0x009006, "DB'S SABER 3070");
    add_rare   (0x009007, "DB'S SABER 3075");
    add_rare   (0x009008, "DB'S SABER 3077");
    add_rare   (0x009100, "GI GUE BAZOOKA");
    add_rare   (0x009200, "GUARDIANNA");
    add_rare   (0x009300, "VIRIDIA CARD");
    add_rare   (0x009301, "GREENILL CARD");
    add_rare   (0x009302, "SKYLY CARD");
    add_rare   (0x009303, "BLUEFULL CARD");
    add_rare   (0x009304, "PURPLENUM CARD");
    add_rare   (0x009305, "PINKAL CARD");
    add_rare   (0x009306, "REDRIA CARD");
    add_rare   (0x009307, "ORAN CARD");
    add_rare   (0x009308, "YELLOWBOZE CARD");
    add_rare   (0x009309, "WHITILL CARD");
    add_rare   (0x009400, "MORNING GLORY");
    add_rare   (0x009500, "PARTISAN of LIGHTNING");
    add_rare   (0x009600, "GAL WIND");
    add_rare   (0x009700, "ZANBA");
    add_rare   (0x009800, "RIKA'S CLAW");
    add_rare   (0x009900, "ANGEL HARP");
    add_rare   (0x009A00, "DEMOLITION COMET");
    add_uber   (0x009B00, "NEI'S CLAW");
    add_uber   (0x009C00, "RAINBOW BATON");
    add_uber   (0x009D00, "DARK FLOW");
    add_uber   (0x009E00, "DARK METEOR");
    add_uber   (0x009F00, "DARK BRIDGE");
    add_rare   (0x00A000, "G-ASSASSIN&'S SABERS");
    add_rare   (0x00A100, "RAPPY'S FAN");
    add_rare   (0x00A200, "BOOMA'S CLAW");
    add_rare   (0x00A201, "GOBOOMA'S CLAW");
    add_rare   (0x00A202, "GIGOBOOMA'S CLAW");
    add_rare   (0x00A300, "RUBY BULLET");
    add_rare   (0x00A400, "AMORE ROSE");
    add_rare   (0x00AA00, "SLICER OF FANATIC");
    add_uber_wk(0x00AB00, "LAME D'ARGENT");
    add_uber   (0x00AC00, "EXCALIBUR");
    add_rare   (0x00AD00, "RAGE DE FEU");
    add_rare   (0x00AD01, "RAGE DE FEU");
    add_rare   (0x00AD02, "RAGE DE FEU");
    add_rare   (0x00AD03, "RAGE DE FEU");
    add_rare   (0x00AE00, "DAISY CHAIN");
    add_rare   (0x00AF00, "OPHELIE SEIZE");
    add_uber   (0x00B000, "MILLE MARTEAUX");
    add_rare   (0x00B100, "LE COGNEUR");
    add_rare   (0x00B200, "COMMANDER BLADE");
    add_rare   (0x00B300, "VIVIENNE");
    add_rare   (0x00B400, "KUSANAGI");
    add_rare   (0x00B500, "SACRED DUSTER");
    add_rare   (0x00B600, "GUREN");
    add_rare   (0x00B700, "SHOUREN");
    add_rare   (0x00B800, "JIZAI");
    add_rare   (0x00B900, "FLAMBERGE");
    add_rare   (0x00BA00, "YUNCHANG");
    add_rare   (0x00BB00, "SNAKE SPIRE");
    add_rare   (0x00BC00, "FLAPJACK FLAPPER");
    add_rare   (0x00BD00, "GETSUGASAN");
    add_rare   (0x00BE00, "MAGUWA");
    add_rare   (0x00BF00, "HEAVEN STRIKER");
    add_rare   (0x00C000, "CANNON ROUGE");
    add_rare   (0x00C100, "METEOR ROUGE");
    add_rare   (0x00C200, "SOLFERINO");
    add_rare   (0x00C300, "CLIO");
    add_rare   (0x00C400, "SIREN GLASS HAMMER");
    add_rare   (0x00C500, "GLIDE DIVINE");
    add_rare   (0x00C600, "SHICHISHITO");
    add_rare   (0x00C700, "MURASAME");
    add_uber   (0x00C800, "DAYLIGHT SCAR");
    add_rare   (0x00C900, "DECALOG");
    add_rare   (0x00CA00, "5TH ANNIV. BLADE");
    add_rare   (0x00CB00, "TYRELL'S PARASOL");
    add_rare   (0x00CC00, "AKIKO'S CLEAVER");
    add_rare   (0x00CD00, "TANEGASHIMA");
    add_rare   (0x00CE00, "TREE CLIPPERS");
    add_rare   (0x00CF00, "NICE SHOT");
    add_rare   (0x00D000, "UNKNOWN3");
    add_rare   (0x00D100, "UNKNOWN4");
    add_rare   (0x00D200, "ANO BAZOOKA");
    add_rare   (0x00D300, "SYNTHESIZER");
    add_rare   (0x00D400, "BAMBOO SPEAR");
    add_rare   (0x00D500, "KAN'EI TSUHO");
    add_rare   (0x00D600, "JITTE");
    add_rare   (0x00D700, "BUTTERFLY NET");
    add_rare   (0x00D800, "SYRINGE");
    add_rare   (0x00D900, "BATTLEDORE");
    add_rare   (0x00DA00, "RACKET");
    add_rare   (0x00DB00, "HAMMER");
    add_rare   (0x00DC00, "GREAT BOUQUET");
    add_rare   (0x00DD00, "TypeSA/SABER");
    add_rare   (0x00DE00, "TypeSL/SABER");
    add_rare   (0x00DE01, "TypeSL/SLICER");
    add_rare   (0x00DE02, "TypeSL/CLAW");
    add_rare   (0x00DE03, "TypeSL/KATANA");
    add_rare   (0x00DF00, "TypeJS/SABER");
    add_rare   (0x00DF01, "TypeJS/SLICER");
    add_rare   (0x00DF02, "TypeJS/J-SWORD");
    add_rare   (0x00E000, "TypeSW/SWORD");
    add_rare   (0x00E001, "TypeSW/SLICER");
    add_rare   (0x00E002, "TypeSW/J-SWORD");
    add_rare   (0x00E100, "TypeRO/SWORD");
    add_rare   (0x00E101, "TypeRO/HALBERT");
    add_rare   (0x00E102, "TypeRO/ROD");
    add_rare   (0x00E200, "TypeBL/BLADE");
    add_rare   (0x00E300, "TypeKN/BLADE");
    add_rare   (0x00E301, "TypeKN/CLAW");
    add_rare   (0x00E400, "TypeHA/HALBERT");
    add_rare   (0x00E401, "TypeHA/ROD");
    add_rare   (0x00E500, "TypeDS/D.SABER");
    add_rare   (0x00E501, "TypeDS/ROD");
    add_rare   (0x00E502, "TypeDS");
    add_rare   (0x00E600, "TypeCL/CLAW");
    add_rare   (0x00E700, "TypeSS/SW");
    add_rare   (0x00E800, "TypeGU/HAND");
    add_rare   (0x00E801, "TypeGU/MECHGUN");
    add_rare   (0x00E900, "TypeRI/RIFLE");
    add_rare   (0x00EA00, "TypeME/MECHGUN");
    add_rare   (0x00EB00, "TypeSH/SHOT");
    add_rare   (0x00EC00, "TypeWA/WAND");
    add_rare   (0x00ED00, "????");
}

// ----------------------- inside <anonymous> namespace -----------------------

void add_frames(ItemInfoMap & map, DefItemMap & def_map) {
    using std::ref;
    using E = EvpPair;
    using D = DfpPair;

    auto add_frame = std::bind(
        add_def_only_once, ref(def_map), ref(map), _1, _2, _3, _4,
        Rarity::common, false);

    // source: https://wiki.pioneer2.net/index.php?title=Frames
    add_frame(0x010100, "Frame"          , D(  5,   7), E( 5,  7));
    add_frame(0x010103, "Giga Frame"     , D( 15,  19), E(12, 14));
    add_frame(0x010104, "Soul Frame"     , D( 20,  24), E(15, 17));
    add_frame(0x010106, "Solid Frame"    , D( 30,  34), E(20, 22));
    add_frame(0x010108, "Hyper Frame"    , D( 40,  44), E(25, 27));
    add_frame(0x01010A, "Shock Frame"    , D( 50,  54), E(30, 32));
    add_frame(0x01010B, "King's Frame"   , D( 55,  59), E(32, 34));
    add_frame(0x01010C, "Dragon Frame"   , D( 60,  64), E(35, 37));
    add_frame(0x01010E, "Protect Frame"  , D( 70,  74), E(40, 42));
    add_frame(0x010110, "Perfect Frame"  , D( 80,  84), E(45, 47));
    add_frame(0x010111, "Valiant Frame"  , D( 85,  89), E(47, 49));
    add_frame(0x010116, "Ultimate Frame" , D(110, 114), E(60, 62));

    add_frame(0x010101, "Armor"          , D(  7,   9), E( 7,  9));
    add_frame(0x010102, "Psy Armor"      , D( 10,  13), E(10, 12));
    add_frame(0x010105, "Cross Armor"    , D( 25,  29), E(17, 19));
    add_frame(0x010107, "Brave Armor"    , D( 35,  39), E(22, 24));
    add_frame(0x010109, "Grand Armor"    , D( 45,  49), E(27, 29));
    add_frame(0x01010D, "Absorb Armor"   , D( 65,  69), E(37, 39));
    add_frame(0x01010F, "General Armor"  , D( 75,  79), E(72, 82));
    add_frame(0x010112, "Imperial Armor" , D( 90,  94), E(50, 52));
    add_frame(0x010113, "Holiness Armor" , D( 95,  99), E(52, 54));
    add_frame(0x010114, "Guardian Armor" , D(100, 104), E(55, 57));
    add_frame(0x010115, "Divinity Armor" , D(105, 109), E(57, 59));
    add_frame(0x010117, "Celestial Armor", D(120, 130), E(72, 82));

    // --------------------- inside add_frames function -----------------------

    auto add_rare_frame = std::bind(
        add_def_only_once, ref(def_map), ref(map), _1, _2, _3, _4,
        Rarity::rare, false);
    add_rare_frame(0x010118, "HUNTER FIELD"               , D( 60, 68), E( 80, 88));
    add_rare_frame(0x010119, "RANGER FIELD"               , D( 50, 58), E( 80, 88));
    add_rare_frame(0x01011A, "FORCE FIELD"                , D( 40, 48), E( 80, 88));
    add_rare_frame(0x01011B, "REVIVAL GARMENT"            , D( 85, 90), E( 60, 70));
    add_rare_frame(0x01011C, "SPIRIT GARMENT"             , D(100,107), E( 92, 97));
    add_rare_frame(0x01011D, "STINK FRAME"                , D( 40,125), E( 15,100));
    add_rare_frame(0x01011E, "D-PARTS ver1.01"            , D(115,125), E( 85, 92));
    add_rare_frame(0x01011F, "D-PARTS ver2.10"            , D(125,135), E( 90, 98));
    add_rare_frame(0x010120, "PARASITE WEAR:De Rol"       , D(120,120), E(100,100));
    add_rare_frame(0x010121, "PARASITE WEAR:Nelgal"       , D(145,145), E( 85, 85));
    add_rare_frame(0x010122, "PARASITE WEAR:Vajulla"      , D(155,155), E(100,100));
    add_rare_frame(0x010123, "SENSE PLATE"                , D( 25, 32), E( 30, 38));
    add_rare_frame(0x010124, "GRAVITON PLATE"             , D(125,133), E(  0,  0));
    add_rare_frame(0x010125, "ATTRIBUTE PLATE"            , D(105,113), E( 85, 93));
    add_rare_frame(0x010126, "FLOWEN'S FRAME"             , D( 82, 92), E( 72, 82));
    add_rare_frame(0x010127, "CUSTOM FRAME ver.OO"        , D( 80, 90), E( 85, 95));
    add_rare_frame(0x010128, "DB'S ARMOR"                 , D( 85, 95), E( 80, 90));
    add_rare_frame(0x010129, "GUARD WAVE"                 , D(173,223), E(110,130));
    add_rare_frame(0x01012A, "DF FIELD"                   , D(203,253), E(116,136));
    add_rare_frame(0x01012B, "LUMINOUS FIELD"             , D(206,256), E(124,144));
    add_rare_frame(0x01012C, "CHU CHU FEVER"              , D(  5,  5), E(  5,  5));
    add_rare_frame(0x01012D, "LOVE HEART"                 , D(196,246), E(140,160));
    add_rare_frame(0x01012E, "FLAME GARMENT"              , D(180,230), E(114,134));
    add_rare_frame(0x01012F, "VIRUS ARMOR:Lafuteria"      , D(240,290), E( 90,110));
    add_rare_frame(0x010130, "BRIGHTNESS CIRCLE"          , D(190,240), E(116,136));
    add_rare_frame(0x010131, "AURA FIELD"                 , D(235,285), E(134,154));
    add_rare_frame(0x010132, "ELECTRO FRAME"              , D(196,246), E(120,140));
    add_rare_frame(0x010133, "SACRED CLOTH"               , D(100,150), E( 50, 70));
    add_rare_frame(0x010134, "SMOKING PLATE"              , D(223,273), E(122,142));
    add_rare_frame(0x010135, "STAR CUIRASS"               , D(250,280), E(  0,  0));
    add_rare_frame(0x010136, "BLACK HOUND CUIRASS"        , D(300,330), E(-200,-200));
    add_rare_frame(0x010137, "MORNING PRAYER"             , D(120,130), E(140,160));
    add_rare_frame(0x010138, "BLACK ODOSHI DOMARU"        , D(124,134), E( 82, 92));
    add_rare_frame(0x010139, "RED ODOSHI DOMARU"          , D(112,122), E(108,118));
    add_rare_frame(0x01013A, "BLACK ODOSHI RED NIMAIDOU"  , D(128,138), E(143,153));
    add_rare_frame(0x01013B, "BLUE ODOSHI VIOLET NIMAIDOU", D(156,166), E(181,191));
    add_rare_frame(0x01013C, "DIRTY LIFEJACKET"           , D(  5,  5), E(  5,  5));
    add_rare_frame(0x01013D, "KROE'S SWEATER"             , D(  1,  1), E(  1,  1));
    add_rare_frame(0x01013E, "WEDDING DRESS"              , D( 30, 30), E( 30, 30));
    add_rare_frame(0x01013F, "SONICTEAM ARMOR"            , D(500,500), E(500,500));
    add_rare_frame(0x010140, "RED COAT"                   , D(152,162), E(131,141));
    add_rare_frame(0x010141, "THIRTEEN"                   , D(113,121), E(136,144));
    add_rare_frame(0x010142, "MOTHER GARB"                , D(165,180), E( 85, 90));
    add_rare_frame(0x010143, "MOTHER GARB+"               , D(175,190), E( 95,100));
    add_rare_frame(0x010144, "DRESS PLATE"                , D( 30, 30), E( 30, 30));
    add_rare_frame(0x010145, "SWEETHEART"                 , D(176,226), E(164,184));
    add_rare_frame(0x010146, "IGNITION CLOAK"             , D(168,176), E(143,151));
    add_rare_frame(0x010147, "CONGEAL CLOAK"              , D(168,176), E(143,151));
    add_rare_frame(0x010148, "TEMPEST CLOAK"              , D(168,176), E(143,151));
    add_rare_frame(0x010149, "CURSED CLOAK"               , D(172,180), E(146,154));
    add_rare_frame(0x01014A, "SELECT CLOAK"               , D(172,180), E(146,154));
    add_rare_frame(0x01014B, "SPIRIT CUIRASS"             , D(122,129), E(116,121));
    add_rare_frame(0x01014C, "REVIVAL CURIASS"            , D(134,139), E( 94,104));
    add_rare_frame(0x01014D, "ALLIANCE UNIFORM"           , D( 88,100), E(  0,  0));
    add_rare_frame(0x01014E, "OFFICER UNIFORM"            , D(114,128), E(  0,  0));
    add_rare_frame(0x01014F, "COMMANDER UNIFORM"          , D(180,196), E( 85, 85));
    add_rare_frame(0x010150, "CRIMSON COAT"               , D(158,170), E(136,148));
    add_rare_frame(0x010151, "INFANTRY GEAR"              , D(118,130), E( 45, 53));
    add_rare_frame(0x010152, "LIEUTENANT GEAR"            , D(168,186), E(112,128));
    add_rare_frame(0x010153, "INFANTRY MANTLE"            , D( 92,102), E( 96,106));
    add_rare_frame(0x010154, "LIEUTENANT MANTLE"          , D(195,216), E(126,144));
    add_rare_frame(0x010155, "UNION FIELD"                , D(  0,  0), E( 50, 50));
    add_rare_frame(0x010156, "SAMURAI ARMOR"              , D(121,121), E(102,102));
    add_rare_frame(0x010157, "STEALTH SUIT"               , D(  1,  1), E(300,325));
    add_rare_frame(0x010158, "????"                       , D(  0,  0), E(  0,  0));
}

// ----------------------- inside <anonymous> namespace -----------------------

void add_barriers(ItemInfoMap & map, DefItemMap & defmap) {
    using std::ref;
    using E = EvpPair;
    using D = DfpPair;

    auto add_barrier = std::bind(
        add_def_only_once, ref(defmap), ref(map), _1, _2, _3, _4,
        Rarity::common, false);

    for (auto t : { 0x010234, 0x010236, 0x010237, 0x010238, 0x010239, 0x010200 }) {
        add_barrier(t, "Barrier", D(2, 7), E(25, 30));
    }

    add_barrier(0x010204, "Soul Barrier"    , D(10, 15), E( 55,  60));
    add_barrier(0x010206, "Brave Barrier"   , D(14, 19), E( 65,  70));
    add_barrier(0x010208, "Flame Barrier"   , D(19, 24), E( 85,  90));
    add_barrier(0x010209, "Plasma Barrier"  , D(21, 26), E( 92,  97));
    add_barrier(0x01020A, "Freeze Barrier"  , D(23, 28), E(100, 105));
    add_barrier(0x01020B, "Psychic Barrier" , D(26, 31), E(110, 115));
    add_barrier(0x01020D, "Protect Barrier" , D(32, 37), E(130, 135));
    add_barrier(0x01020F, "Imperial Barrier", D(38, 43), E(150, 155));
    add_barrier(0x010211, "Divinity Barrier", D(44, 49), E(170, 175));

    add_barrier(0x010201, "Shield"          , D( 4,  9), E( 32,  37));
    add_barrier(0x010202, "Core Shield"     , D( 6, 11), E( 40,  45));
    add_barrier(0x010203, "Giga Shield"     , D( 8, 13), E( 47,  52));
    add_barrier(0x010205, "Hard Shield"     , D(12, 17), E( 57,  62));
    add_barrier(0x010207, "Solid Shield"    , D(16, 21), E( 72,  77));
    add_barrier(0x01020C, "General Shield"  , D(29, 34), E(120, 125));
    add_barrier(0x01020E, "Glorious Shield" , D(35, 40), E(140, 145));
    add_barrier(0x010210, "Guardian Shield" , D(41, 46), E(160, 165));
    add_barrier(0x010212, "Ultimate Shield" , D(47, 52), E(180, 185));
    add_barrier(0x010213, "Spiritual Shield", D(50, 55), E(190, 195));
    add_barrier(0x010214, "Celestial Shield", D(52, 57), E(200, 205));

    // -------------------- inside add_barriers function ----------------------

    auto add_rare_barrier = std::bind(
        add_def_only_once, ref(defmap), ref(map), _1, _2, _3, _4, Rarity::rare, false);
    auto add_uber_barrier = std::bind(add_item_only_once, ref(map), _1, _2, Rarity::uber, false);

    add_rare_barrier(0x010215, "INVISIBLE GUARD"        , D( 15,  23), E( 70,  78));
    add_rare_barrier(0x010216, "SACRED GUARD"           , D(  5,  13), E( 15,  23));
    add_rare_barrier(0x010217, "S-PARTS ver1.16"        , D( 20,  28), E( 60,  68));
    add_uber_barrier(0x010218, "S-PARTS ver2.01"        , D( 25,  32), E( 65,  72));
    add_rare_barrier(0x010219, "LIGHT RELIEF"           , D( 20,  27), E( 70,  77));
    add_rare_barrier(0x01021A, "SHIELD OF DELSABER"     , D( 65,  72), E(115, 122));
    add_rare_barrier(0x01021B, "FORCE WALL"             , D( 65,  75), E(140, 150));
    add_rare_barrier(0x01021C, "RANGER WALL"            , D( 70,  80), E(145, 155));
    add_rare_barrier(0x01021D, "HUNTER WALL"            , D( 70,  80), E(135, 145));
    add_rare_barrier(0x01021E, "ATTRIBUTE WALL"         , D( 75,  85), E(100, 110));
    add_rare_barrier(0x01021F, "SECRET GEAR"            , D( 75,  85), E(105, 115));
    add_rare_barrier(0x010220, "COMBAT GEAR"            , D(  0,   0), E(  0,   0));
    add_rare_barrier(0x010221, "PROTO REGENE GEAR"      , D( 40,  47), E( 85,  92));
    add_rare_barrier(0x010222, "REGENERATE GEAR"        , D( 40,  47), E( 85,  92));
    add_rare_barrier(0x010223, "REGENE GEAR ADV."       , D( 45,  52), E( 90,  97));
    add_rare_barrier(0x010224, "FLOWEN'S SHIELD"        , D( 62,  72), E( 70,  80));
    add_rare_barrier(0x010225, "CUSTOM BARRIER ver.OO"  , D( 65,  75), E( 65,  75));
    add_rare_barrier(0x010226, "DB'S SHIELD"            , D( 67,  77), E( 67,  77));
    add_rare_barrier(0x010228, "TRIPOLIC SHIELD"        , D( 95, 145), E(231, 246));
    add_rare_barrier(0x010229, "STANDSTILL SHIELD"      , D(163, 213), E(175, 190));
    add_rare_barrier(0x01022A, "SAFETY HEART"           , D(106, 156), E(248, 263));
    add_rare_barrier(0x01022B, "KASAMI BRACER"          , D( 96, 146), E(235, 250));
    add_rare_barrier(0x01022C, "GODS SHIELD SUZAKU"     , D( 50,  50), E(100, 100));
    add_rare_barrier(0x01022D, "GODS SHIELD GENBU"      , D( 45,  45), E( 80,  80));
    add_rare_barrier(0x01022E, "GODS SHIELD BYAKKO"     , D( 45,  45), E( 80,  80));
    add_rare_barrier(0x01022F, "GODS SHIELD SEIRYU"     , D( 50,  50), E(100, 100));
    add_rare_barrier(0x010230, "HUNTER'S SHELL"         , D( 88, 138), E(222, 237));
    add_rare_barrier(0x010231, "RICO'S GLASSES"         , D(  1,   1), E(  1,   1));
    add_rare_barrier(0x010232, "RICO'S EARRING"         , D( 96, 181), E(237, 262));
    add_rare_barrier(0x010235, "SECURE FEET"            , D( 83, 133), E(230, 245));
    add_rare_barrier(0x010283, "WEAPONS SILVER SHIELD"  , D( 35,  35), E( 50,  50));
    add_rare_barrier(0x010284, "WEAPONS COPPER SHIELD"  , D( 24,  24), E( 25,  25));
    add_rare_barrier(0x010285, "GRATIA"                 , D(130, 150), E(200, 215));
    add_rare_barrier(0x010286, "TRIPOLIC REFLECTOR"     , D( 95, 145), E(235, 250));
    add_rare_barrier(0x010287, "STRIKER PLUS"           , D( 80,  90), E(200, 205));
    add_rare_barrier(0x010288, "REGENERATE GEAR B.P."   , D( 90,  97), E(180, 187));
    add_rare_barrier(0x010289, "RUPIKA"                 , D(120, 130), E(180, 200));
    add_rare_barrier(0x01028A, "YATA MIRROR"            , D( 40,  60), E(200, 225));
    add_rare_barrier(0x01028B, "BUNNY EARS"             , D(  2,   2), E( 25,  25));
    add_rare_barrier(0x01028C, "CAT EARS"               , D(  2,   2), E( 25,  25));
    add_rare_barrier(0x01028D, "THREE SEALS"            , D( 33,  36), E( 33,  36));
    add_rare_barrier(0x01028E, "GOD'S SHIELD \"KOURYU\"", D( 95,  95), E(180, 180));
    add_rare_barrier(0x01028F, "DF SHIELD"              , D( 60, 145), E(170, 195));
    add_uber_barrier(0x010290, "FROM THE DEPTHS"        , D(160, 160), E(240, 240));
    add_rare_barrier(0x010291, "DE ROL LE SHIELD"       , D(180, 255), E(120, 195));
    add_rare_barrier(0x010292, "HONEYCOMB REFLECTOR"    , D(110, 120), E(140, 150));
    add_rare_barrier(0x010293, "EPSIGUARD"              , D(120, 195), E(180, 255));
    add_rare_barrier(0x010294, "ANGEL RING"             , D( 40,  40), E( 60,  60));
    add_rare_barrier(0x010299, "STINK SHIELD"           , D( 50, 125), E( 55, 130));
    add_rare_barrier(0x01024F, "WEAPONS GOLD SHIELD"    , D( 41,  41), E(100, 100));
    add_rare_barrier(0x010250, "BLACK GEAR"             , D( 23,  28), E( 80,  85));
    add_rare_barrier(0x010251, "WORKS GUARD"            , D( 11,  16), E( 75,  80));
    add_rare_barrier(0x010252, "RAGOL RING"             , D(105, 105), E(130, 130));

    add_rare_barrier(0x010273, "Anti-Dark Ring" , D(20, 20), E(135, 135));
    add_rare_barrier(0x01027B, "Anti-Light Ring", D(90, 90), E( 80,  80));

    add_rare_barrier(0x01029A, "UNKNOWN_B", D(0, 0), E(0, 0)); // ???
    add_rare_barrier(0x0102A5, "????"     , D(0, 0), E(0, 0));

    auto make_barrier_maker = [&defmap, &map](const D & dfpp, const E & evpp) {
        return std::bind(
            add_def_only_once, ref(defmap), ref(map), _1, _2, dfpp, evpp, Rarity::rare, false);
    };
    {
    auto make_merge = make_barrier_maker(D(2, 7), E(25, 30));
    make_merge(0x01023A, "RESTA MERGE");
    make_merge(0x01023B, "ANTI MERGE");
    make_merge(0x01023C, "SHIFTA MERGE");
    make_merge(0x01023D, "DEBAND MERGE");
    make_merge(0x01023E, "FOIE MERGE");
    make_merge(0x01023F, "GIFOIE MERGE");
    make_merge(0x010240, "RAFOIE MERGE");
    make_merge(0x010241, "RED MERGE");
    make_merge(0x010242, "BARTA MERGE");
    make_merge(0x010243, "GIBARTA MERGE");
    make_merge(0x010244, "RABARTA MERGE");
    make_merge(0x010245, "BLUE MERGE");
    make_merge(0x010246, "ZONDE MERGE");
    make_merge(0x010247, "GIZONDE MERGE");
    make_merge(0x010248, "RAZONDE MERGE");
    make_merge(0x010249, "YELLOW MERGE");
    make_merge(0x01024A, "RECOVERY BARRIER");
    make_merge(0x01024B, "ASSIST BARRIER");
    make_merge(0x01024C, "RED BARRIER");
    make_merge(0x01024D, "BLUE BARRIER");
    make_merge(0x01024E, "YELLOW BARRIER");
    }

    {
    auto make_red_ring = make_barrier_maker(D(150, 235), E(232, 257));
    make_red_ring(0x010227, "RED RING"    );
    make_red_ring(0x010253, "Blue Ring*"  );
    make_red_ring(0x01025B, "Green Ring*" );
    make_red_ring(0x010263, "Yellow Ring*");
    make_red_ring(0x01026B, "Purple Ring*");
    make_red_ring(0x010274, "White Ring*" );
    make_red_ring(0x01027C, "Black Ring*" );
    }

    {
    auto add_ring = make_barrier_maker(D(35, 40), E(130, 135));
    add_ring(0x010233, "BLUE RING");
    add_ring(0x010254, "BLUE RING");
    add_ring(0x010255, "BLUE RING");
    add_ring(0x010256, "BLUE RING");
    add_ring(0x010257, "BLUE RING");
    add_ring(0x010258, "BLUE RING");
    add_ring(0x010259, "BLUE RING");
    add_ring(0x01025A, "BLUE RING");

    add_ring(0x01025C, "GREEN RING");
    add_ring(0x01025D, "GREEN RING");
    add_ring(0x01025E, "GREEN RING");
    add_ring(0x01025F, "GREEN RING");
    add_ring(0x010260, "GREEN RING");
    add_ring(0x010261, "GREEN RING");
    add_ring(0x010262, "GREEN RING");

    add_ring(0x010264, "YELLOW RING");
    add_ring(0x010265, "YELLOW RING");
    add_ring(0x010266, "YELLOW RING");
    add_ring(0x010267, "YELLOW RING");
    add_ring(0x010268, "YELLOW RING");
    add_ring(0x010269, "YELLOW RING");
    add_ring(0x01026A, "YELLOW RING");

    add_ring(0x01026C, "PURPLE RING");
    add_ring(0x01026D, "PURPLE RING");
    add_ring(0x01026E, "PURPLE RING");
    add_ring(0x01026F, "PURPLE RING");
    add_ring(0x010270, "PURPLE RING");
    add_ring(0x010271, "PURPLE RING");
    add_ring(0x010272, "PURPLE RING");

    add_ring(0x010275, "WHITE RING");
    add_ring(0x010276, "WHITE RING");
    add_ring(0x010277, "WHITE RING");
    add_ring(0x010278, "WHITE RING");
    add_ring(0x010279, "WHITE RING");
    add_ring(0x01027A, "WHITE RING");

    add_ring(0x01027D, "BLACK RING");
    add_ring(0x01027E, "BLACK RING");
    add_ring(0x01027F, "BLACK RING");
    add_ring(0x010280, "BLACK RING");
    add_ring(0x010281, "BLACK RING");
    add_ring(0x010282, "BLACK RING");
    }

    {
    auto make_union_guard = make_barrier_maker(D( 50,  50), E(  0,   0));
    make_union_guard(0x010295, "UNION GUARD");
    make_union_guard(0x010296, "UNION GUARD");
    make_union_guard(0x010297, "UNION GUARD");
    make_union_guard(0x010298, "UNION GUARD");
    }

    {
    auto make_genpei = make_barrier_maker(D(158, 158), E(237, 237));
    make_genpei(0x01029B, "GENPEI");
    make_genpei(0x01029C, "GENPEI");
    make_genpei(0x01029D, "GENPEI");
    make_genpei(0x01029E, "GENPEI");
    make_genpei(0x01029F, "GENPEI");
    make_genpei(0x0102A0, "GENPEI");
    make_genpei(0x0102A1, "GENPEI");
    make_genpei(0x0102A2, "GENPEI");
    make_genpei(0x0102A3, "GENPEI");
    make_genpei(0x0102A4, "GENPEI");
    }
}

// ----------------------- inside <anonymous> namespace -----------------------

void add_units(ItemInfoMap & map) {
    using std::ref;
    auto add_unit = std::bind(add_item_only_once, ref(map), _1, _2, Rarity::common, false);
    add_unit(0x010300, "Knight/Power");
    add_unit(0x010301, "General/Power");
    add_unit(0x010302, "Ogre/Power");
    add_unit(0x010304, "Priest/Mind");
    add_unit(0x010305, "General/Mind");
    add_unit(0x010306, "Angel/Mind");
    add_unit(0x010308, "Marksman/Arm");
    add_unit(0x010309, "General/Arm");
    add_unit(0x01030A, "Elf/Arm");
    add_unit(0x01030C, "Thief/Legs");
    add_unit(0x01030D, "General/Legs");
    add_unit(0x01030E, "Elf/Legs");
    add_unit(0x010310, "Digger/HP");
    add_unit(0x010311, "General/HP");
    add_unit(0x010312, "Dragon/HP");
    add_unit(0x010314, "Magician/TP");
    add_unit(0x010315, "General/TP");
    add_unit(0x010316, "Angel/TP");
    add_unit(0x010318, "Warrior/Body");
    add_unit(0x010319, "General/Body");
    add_unit(0x01031A, "Metal/Body");
    add_unit(0x01031C, "Angel/Luck");
    add_unit(0x01031E, "Master/Ability");
    add_unit(0x010321, "Resist/Fire");
    add_unit(0x010322, "Resist/Flame");
    add_unit(0x010323, "Resist/Burning");
    add_unit(0x010324, "Resist/Cold");
    add_unit(0x010325, "Resist/Freeze");
    add_unit(0x010326, "Resist/Blizzard");
    add_unit(0x010327, "Resist/Shock");
    add_unit(0x010328, "Resist/Thunder");
    add_unit(0x010329, "Resist/Storm");
    add_unit(0x01032A, "Resist/Light");
    add_unit(0x01032B, "Resist/Saint");
    add_unit(0x01032C, "Resist/Holy");
    add_unit(0x01032D, "Resist/Dark");
    add_unit(0x01032E, "Resist/Evil");
    add_unit(0x01032F, "Resist/Devil");
    add_unit(0x010330, "All/Resist");
    add_unit(0x010331, "Super/Resist");
    add_unit(0x010333, "HP/Restorate");
    add_unit(0x010334, "HP/Generate");
    add_unit(0x010335, "HP/Revival");
    add_unit(0x010336, "TP/Restorate");
    add_unit(0x010337, "TP/Generate");
    add_unit(0x010338, "TP/Revival");
    add_unit(0x010339, "PB/Amplifier");
    add_unit(0x01033A, "PB/Generate");
    add_unit(0x01033B, "PB/Create");
    add_unit(0x01033C, "Wizard/Technique");
    add_unit(0x01033D, "Devil/Technique");
    add_unit(0x01033F, "General/Battle");
    add_unit(0x010364, "????");

    // ---------------------- inside add_units function -----------------------

    auto add_rare_unit = std::bind(add_item_only_once, ref(map), _1, _2, Rarity::rare, false);
    auto add_rare_wkc  = std::bind(add_item_only_once, ref(map), _1, _2, Rarity::uber, true );

    add_rare_unit(0x010303, "God/Power");
    add_rare_unit(0x010307, "God/Mind");
    add_rare_unit(0x01030B, "God/Arm");
    add_rare_unit(0x01030F, "God/Legs");
    add_rare_unit(0x010313, "God/HP");
    add_rare_unit(0x010317, "God/TP");
    add_rare_unit(0x01031B, "God/Body");
    add_rare_unit(0x01031D, "God/Luck");
    add_rare_unit(0x01031F, "Hero/Ability");
    add_rare_unit(0x010320, "God/Ability");
    add_rare_unit(0x010332, "Perfect/Resist");
    add_rare_unit(0x01033E, "God/Technique");
    add_rare_unit(0x010340, "Devil/Battle");
    add_rare_unit(0x010341, "God/Battle");
    add_rare_unit(0x010342, "Cure/Poison");
    add_rare_unit(0x010343, "Cure/Paralysis");
    add_rare_unit(0x010344, "Cure/Slow");
    add_rare_unit(0x010345, "Cure/Confuse");
    add_rare_unit(0x010346, "Cure/Freeze");
    add_rare_unit(0x010347, "Cure/Shock");
    add_rare_unit(0x010348, "YASAKANI MAGATAMA");
    add_rare_unit(0x010349, "V101");
    add_rare_unit(0x01034A, "V501");
    add_rare_unit(0x01034B, "V502");
    add_rare_unit(0x01034C, "V801");
    add_rare_wkc (0x01034D, "LIMITER");
    add_rare_unit(0x01034E, "ADEPT");
    add_rare_wkc (0x01034F, "SWORDSMAN LORE");
    add_rare_unit(0x010350, "PROOF OF SWORD-SAINT");
    add_rare_unit(0x010351, "SMARTLINK");
    add_rare_unit(0x010352, "DIVINE PROTECTION");
    add_rare_unit(0x010353, "Heavenly/Battle");
    add_rare_unit(0x010354, "Heavenly/Power");
    add_rare_unit(0x010355, "Heavenly/Mind");
    add_rare_unit(0x010356, "Heavenly/Arms");
    add_rare_unit(0x010357, "Heavenly/Legs");
    add_rare_unit(0x010358, "Heavenly/Body");
    add_rare_unit(0x010359, "Heavenly/Luck");
    add_rare_unit(0x01035A, "Heavenly/Ability");
    add_rare_unit(0x01035B, "Centurion/Ability");
    add_rare_unit(0x01035C, "Friend Ring");
    add_rare_unit(0x01035D, "Heavenly/HP");
    add_rare_unit(0x01035E, "Heavenly/TP");
    add_rare_unit(0x01035F, "Heavenly/Resist");
    add_rare_unit(0x010360, "Heavenly/Technique");
    add_rare_unit(0x010361, "HP/Ressurection");
    add_rare_unit(0x010362, "TP/Ressurection");
    add_rare_unit(0x010363, "PB/trease");
}

// ----------------------- inside <anonymous> namespace -----------------------

void add_mags(ItemInfoMap & map) {
    using std::ref;
    auto add_mag = std::bind(add_item_only_once, ref(map), _1, _2, Rarity::common, false);
    add_mag(0x020000, "Mag");
    add_mag(0x020100, "Varuna");
    add_mag(0x020200, "Mitra");
    add_mag(0x020300, "Surya");
    add_mag(0x020400, "Vayu");
    add_mag(0x020500, "Varaha");
    add_mag(0x020600, "Kama");
    add_mag(0x020700, "Ushasu");
    add_mag(0x020800, "Apsaras");
    add_mag(0x020900, "Kumara");
    add_mag(0x020A00, "Kaitabha");
    add_mag(0x020B00, "Tapas");
    add_mag(0x020C00, "Bhirava");
    add_mag(0x020D00, "Kalki");
    add_mag(0x020E00, "Rudra");
    add_mag(0x020F00, "Marutah");
    add_mag(0x021000, "Yaksa");
    add_mag(0x021100, "Sita");
    add_mag(0x021200, "Garuda");
    add_mag(0x021300, "Nandin");
    add_mag(0x021400, "Ashvinau");
    add_mag(0x021500, "Ribhava");
    add_mag(0x021600, "Soma");
    add_mag(0x021700, "Ila");
    add_mag(0x021800, "Durga");
    add_mag(0x021900, "Vritra");
    add_mag(0x021A00, "Namuci");
    add_mag(0x021B00, "Sumba");
    add_mag(0x021C00, "Naga");
    add_mag(0x021D00, "Pitri");
    add_mag(0x021E00, "Kabanda");
    add_mag(0x021F00, "Ravana");
    add_mag(0x022000, "Marica");
    add_mag(0x022100, "Soniti");
    add_mag(0x022200, "Preta");
    add_mag(0x022300, "Andhaka");
    add_mag(0x022400, "Bana");
    add_mag(0x022500, "Naraka");
    add_mag(0x022600, "Madhu");
    add_mag(0x022700, "Churel");

    add_mag(0x024200, "Geung-si");
    add_mag(0x024300, "\\\\n");
    add_mag(0x025200, "????");

    auto add_4thev_mag  = std::bind(add_item_only_once, ref(map), _1, _2, Rarity::interest, false);
    add_4thev_mag(0x023900, "Deva");
    add_4thev_mag(0x023A00, "Rati");
    add_4thev_mag(0x023B00, "Savitri");
    add_4thev_mag(0x023C00, "Rukmin");
    add_4thev_mag(0x023D00, "Pushan");
    add_4thev_mag(0x023E00, "Diwari");
    add_4thev_mag(0x023F00, "Sato");
    add_4thev_mag(0x024000, "Bhima");
    add_4thev_mag(0x024100, "Nidra");

    auto add_rare_mag = std::bind(add_item_only_once, ref(map), _1, _2, Rarity::rare, false);
    add_rare_mag(0x022800, "ROBOCHAO");
    add_rare_mag(0x022900, "OPA-OPA");
    add_rare_mag(0x022A00, "PIAN");
    add_rare_mag(0x022B00, "CHAO");
    add_rare_mag(0x022C00, "CHU CHU");
    add_rare_mag(0x022D00, "KAPU KAPU");
    add_rare_mag(0x022E00, "ANGEL&'S WING");
    add_rare_mag(0x022F00, "DEVIL&'S WING");
    add_rare_mag(0x023000, "ELENOR");
    add_rare_mag(0x023100, "MARK3");
    add_rare_mag(0x023200, "MASTER SYSTEM");
    add_rare_mag(0x023300, "GENESIS");
    add_rare_mag(0x023400, "SEGA SATURN");
    add_rare_mag(0x023500, "DREAMCAST");
    add_rare_mag(0x023600, "HAMBURGER");
    add_rare_mag(0x023700, "PANZER'S TAIL");
    add_rare_mag(0x023800, "DEVIL'S TAIL");
    add_rare_mag(0x024400, "Tellusis");
    add_rare_mag(0x024500, "Striker Unit");
    add_rare_mag(0x024600, "Pioneer");
    add_rare_mag(0x024700, "Puyo");
    add_rare_mag(0x024800, "Moro");
    add_rare_mag(0x024900, "Rappy");
    add_rare_mag(0x024A00, "Yahoo!");
    add_rare_mag(0x024B00, "Gael Giel");
    add_rare_mag(0x024C00, "Agastya");

    if (!k_is_ephinea) {
        add_rare_mag(0x024D00, "Cell of MAG 0503");
    }

    add_rare_mag(0x024E00, "Cell of MAG 0504");
    add_rare_mag(0x024F00, "Cell of MAG 0505");
    add_rare_mag(0x025000, "Cell of MAG 0506");
    add_rare_mag(0x025100, "Cell of MAG 0507");

}

// ----------------------- inside <anonymous> namespace -----------------------

void add_tools(ItemInfoMap & map) {
    using std::ref;
    auto add_tool = std::bind(add_item_only_once, ref(map), _1, _2, Rarity::common, false);
    auto add_interst_tool = std::bind(add_item_only_once, ref(map), _1, _2, Rarity::interest, false);
    add_tool        (0x030000, "Monomate");
    add_tool        (0x030001, "Dimate");
    add_tool        (0x030002, "Trimate");
    add_tool        (0x030100, "Monofluid");
    add_tool        (0x030101, "Difluid");
    add_tool        (0x030102, "Trifluid");
    add_tool        (0x030300, "Sol Atomizer");
    add_tool        (0x030400, "Moon Atomizer");
    add_interst_tool(0x030500, "Star Atomizer");
    add_tool        (0x030600, "Antidote");
    add_tool        (0x030601, "Antiparalysis");
    add_tool        (0x030700, "Telepipe");
    add_tool        (0x030800, "Trap Vision");
    add_interst_tool(0x030900, "Scape Doll");
    add_tool        (0x030A00, "Monogrinder");
    add_tool        (0x030A01, "Digrinder");
    add_tool        (0x030A02, "Trigrinder");
    add_tool        (0x030B00, "Power Material");
    add_tool        (0x030B01, "Mind Material");
    add_tool        (0x030B02, "Evade Material");
    add_interst_tool(0x030B03, "HP Material");
    add_interst_tool(0x030B04, "TP Material");
    add_tool        (0x030B05, "Def Material");
    add_interst_tool(0x030B06, "Luck Material");
    add_tool        (0x031A00, "????");

    auto add_rare_tool = std::bind(add_item_only_once, ref(map), _1, _2, Rarity::rare, false);
    auto add_uber_tool = std::bind(add_item_only_once, ref(map), _1, _2, Rarity::uber, false);
    add_rare_tool(0x030C00, "Cell of MAG 502");
    add_rare_tool(0x030C01, "Cell of MAG 213");
    add_rare_tool(0x030C02, "Parts of RoboChao");
    add_rare_tool(0x030C03, "Heart of Opa Opa");
    add_rare_tool(0x030C04, "Heart of Pian");
    add_rare_tool(0x030C05, "Heart of Chao");
    add_rare_tool(0x030D00, "Sorcerer's Right Arm");
    add_rare_tool(0x030D01, "S-beat's Arms");
    add_rare_tool(0x030D02, "P-arm's Arms");
    add_rare_tool(0x030D03, "Delsaber's Right Arm");
    add_rare_tool(0x030D04, "Bringer's Right Arm");
    add_rare_tool(0x030D05, "Delsaber's Left Arm");
    add_rare_tool(0x030D06, "S-red's Arms");
    add_rare_tool(0x030D07, "Dragon's Claw");
    add_rare_tool(0x030D08, "Hildebear's Head");
    add_rare_tool(0x030D09, "Hildeblue's Head");
    add_rare_tool(0x030D0A, "Parts of Baranz");
    add_rare_tool(0x030D0B, "Belra's Right Arm");
    add_rare_tool(0x030D0C, "Gi Gue's body");
    add_rare_tool(0x030D0D, "Sinow Berill's Arms");
    add_rare_tool(0x030D0E, "Grass Assassin's Arms");
    add_rare_tool(0x030D0F, "Booma's Right Arm");
    add_rare_tool(0x030D10, "Gobooma's Right Arm");
    add_rare_tool(0x030D11, "Gigobooma's Right Arm");
    add_rare_tool(0x030D12, "Gal Gryphon's Wing");
    add_rare_tool(0x030D13, "Rappy's Wing");
    add_rare_tool(0x030D14, "Cladding of Epsilon");
    add_rare_tool(0x030D15, "De Rol Le Shell");
    add_rare_tool(0x030E00, "Berill Photon");
    add_uber_tool(0x030E01, "Parasitic gene \"Flow\"");
    add_uber_tool(0x030E02, "Magic Stone \"Iritista\"");
    add_rare_tool(0x030E03, "Blue-black stone");
    add_uber_tool(0x030E04, "Syncesta");
    add_rare_tool(0x030E05, "Magic Water");
    add_rare_tool(0x030E06, "Parasitic cell Type D");
    add_rare_tool(0x030E07, "magic rock \"Heart Key\"");
    add_rare_tool(0x030E08, "magic rock \"Moola\"");
    add_rare_tool(0x030E09, "Star Amplifier");
    add_rare_tool(0x030E0A, "Book of HITOGATA");
    add_rare_tool(0x030E0B, "Heart of Chu Chu");
    add_rare_tool(0x030E0C, "Parts of EGG BLASTER");
    add_rare_tool(0x030E0D, "Heart of Angel");
    add_rare_tool(0x030E0E, "Heart of Devil");
    add_rare_tool(0x030E0F, "Kit of Hamburger");
    add_rare_tool(0x030E10, "Panther&'s Spirit");
    add_rare_tool(0x030E11, "Kit of MARK3");
    add_rare_tool(0x030E12, "Kit of MASTER SYSTEM");
    add_rare_tool(0x030E13, "Kit of GENESIS");
    add_rare_tool(0x030E14, "Kit of SEGA SATURN");
    add_rare_tool(0x030E15, "Kit of DREAMCAST");
    add_rare_tool(0x030E16, "Amplifier of Resta");
    add_rare_tool(0x030E17, "Amplifier of Anti");
    add_rare_tool(0x030E18, "Amplifier of Shifta");
    add_rare_tool(0x030E19, "Amplifier of Deband");
    add_rare_tool(0x030E1A, "Amplifier of Foie");
    add_rare_tool(0x030E1B, "Amplifier of Gifoie");
    add_rare_tool(0x030E1C, "Amplifier of Rafoie");
    add_rare_tool(0x030E1D, "Amplifier of Barta");
    add_rare_tool(0x030E1E, "Amplifier of Gibarta");
    add_rare_tool(0x030E1F, "Amplifier of Rabarta");
    add_rare_tool(0x030E20, "Amplifier of Zonde");
    add_rare_tool(0x030E21, "Amplifier of Gizonde");
    add_rare_tool(0x030E22, "Amplifier of Razonde");
    add_rare_tool(0x030E23, "Amplifier of Red");
    add_rare_tool(0x030E24, "Amplifier of Blue");
    add_rare_tool(0x030E25, "Amplifier of Yellow");
    add_rare_tool(0x030E26, "Heart of KAPU KAPU");
    add_rare_tool(0x030E27, "Photon Booster");
    add_rare_tool(0x030F00, "AddSlot");
    add_rare_tool(0x031000, "Photon Drop");
    add_uber_tool(0x031001, "Photon Sphere");
    add_rare_tool(0x031002, "Photon Crystal");
    add_rare_tool(0x031003, "Secret Ticket");
    add_rare_tool(0x031004, "Photon Ticket");
    add_rare_tool(0x031100, "Book of KATANA1");
    add_rare_tool(0x031101, "Book of KATANA2");
    add_rare_tool(0x031102, "Book of KATANA3");
    add_rare_tool(0x031200, "Weapons Bronze Badge");
    add_rare_tool(0x031201, "Weapons Silver Badge");
    add_rare_tool(0x031202, "Weapons Gold Badge");
    add_rare_tool(0x031203, "Weapons Crystal Badge");
    add_rare_tool(0x031204, "Weapons Steel Badge");
    add_rare_tool(0x031205, "Weapons Aluminum Badge");
    add_rare_tool(0x031206, "Weapons Leather Badge");
    add_rare_tool(0x031207, "Weapons Bone Badge");
    add_rare_tool(0x031208, "Letter of appreciation");
    add_rare_tool(0x031209, "Item Ticket");
    add_rare_tool(0x03120A, "Valentine's Chocolate");
    add_rare_tool(0x03120B, "New Year's Card");
    add_rare_tool(0x03120C, "Christmas Card");
    add_rare_tool(0x03120D, "Birthday Card");
    add_rare_tool(0x03120E, "Proof of Sonic Team");
    add_rare_tool(0x03120F, "Special Event Ticket");
    add_rare_tool(0x031210, "Flower Bouquet");
    add_rare_tool(0x031211, "Cake");
    add_rare_tool(0x031212, "Accessories");
    add_rare_tool(0x031213, "Mr.Naka's Business Card");
    add_rare_tool(0x031300, "Present");
    add_rare_tool(0x031400, "Chocolate");
    add_rare_tool(0x031401, "Candy");
    add_rare_tool(0x031402, "Cake");
    add_rare_tool(0x031403, "Weapons Silver Badge");
    add_rare_tool(0x031404, "Weapons Gold Badge");
    add_rare_tool(0x031405, "Weapons Crystal Badge");
    add_rare_tool(0x031406, "Weapons Steel Badge");
    add_rare_tool(0x031407, "Weapons Aluminum Badge");
    add_rare_tool(0x031408, "Weapons Leather Badge");
    add_rare_tool(0x031409, "Weapons Bone Badge");
    add_rare_tool(0x03140A, "Bouquet");
    add_rare_tool(0x03140B, "Decoction");
    add_rare_tool(0x031500, "Christmas Present");
    add_rare_tool(0x031501, "Easter Egg");
    add_rare_tool(0x031502, "Jack-O'-Lantern");
    add_rare_tool(0x031600, "DISK Vol.1 \"Wedding March\"");
    add_rare_tool(0x031601, "DISK Vol.2 \"Day Light\"");
    add_rare_tool(0x031602, "DISK Vol.3 \"Burning Rangers\"");
    add_rare_tool(0x031603, "DISK Vol.4 \"Open Your Heart\"");
    add_rare_tool(0x031604, "DISK Vol.5 \"Live & Learn\"");
    add_rare_tool(0x031605, "DISK Vol.6 \"NiGHTS\"");
    add_rare_tool(0x031606, "DISK Vol.7 \"Ending Theme (Piano ver.)\"");
    add_rare_tool(0x031607, "DISK Vol.8 \"Heart to Heart\"");
    add_rare_tool(0x031608, "DISK Vol.9 \"Strange Blue\"");
    add_rare_tool(0x031609, "DISK Vol.10 \"Reunion System\"");
    add_rare_tool(0x03160A, "DISK Vol.11 \"Pinnacles\"");
    add_rare_tool(0x03160B, "DISK Vol.12 \"Fight inside the Spaceship\"");
    add_rare_tool(0x031700, "Hunters Report");
    add_rare_tool(0x031701, "Hunters Report");
    add_rare_tool(0x031702, "Hunters Report");
    add_rare_tool(0x031703, "Hunters Report");
    add_rare_tool(0x031704, "Hunters Report");
    add_rare_tool(0x031800, "Tablet");
    add_rare_tool(0x031801, "UNKNOWN2");
    add_rare_tool(0x031802, "Dragon Scale");
    add_rare_tool(0x031803, "Heaven Striker Coat");
    add_rare_tool(0x031804, "Pioneer Parts");
    add_rare_tool(0x031805, "Amitie's Memo");
    add_rare_tool(0x031806, "Heart of Morolian");
    add_rare_tool(0x031807, "Rappy's Beak");
    add_rare_tool(0x031808, "Yahoo!'s engine");
    add_rare_tool(0x031809, "D-Photon Core");
    add_rare_tool(0x03180A, "Liberta Kit");
    add_rare_tool(0x03180B, "Cell of MAG 0503");
    add_rare_tool(0x03180C, "Cell of MAG 0504");
    add_rare_tool(0x03180D, "Cell of MAG 0505");
    add_rare_tool(0x03180E, "Cell of MAG 0506");
    add_rare_tool(0x03180F, "Cell of MAG 0507");
    add_rare_tool(0x031900, "Team Points 500");
    add_rare_tool(0x031901, "Team Points 1000");
    add_rare_tool(0x031902, "Team Points 5000");
    add_rare_tool(0x031903, "Team Points 10000");
}

// ----------------------- inside <anonymous> namespace -----------------------

void add_ephinea(ItemInfoMap & map) {
    using std::ref;
    auto add_rare_item = std::bind(add_item_only_once, ref(map), _1, _2, Rarity::rare, false);
    auto add_uber_item = std::bind(add_item_only_once, ref(map), _1, _2, Rarity::uber, false);
    add_rare_item(0x031005, "Event Egg");
    add_rare_item(0x031006, "1st Anniv. Bronze Badge");
    add_rare_item(0x031007, "1st Anniv. Silver Badge");
    add_rare_item(0x031008, "1st Anniv. Gold Badge");
    add_uber_item(0x031009, "1st Anniv. Platinum Badge");
    add_rare_item(0x03100A, "2nd Anniv. Bronze Badge");
    add_rare_item(0x03100B, "2nd Anniv. Silver Badge");
    add_rare_item(0x03100C, "2nd Anniv. Gold Badge");
    add_uber_item(0x03100D, "2nd Anniv. Platinum Badge");
    add_rare_item(0x03100E, "Halloween Cookie");
    add_rare_item(0x03100F, "Coal");

    add_rare_item(0x031015, "4th Anniv. Bronze Badge");
    add_rare_item(0x031016, "4th Anniv. Silver Badge");
    add_rare_item(0x031017, "4th Anniv. Gold Badge");
    add_uber_item(0x031018, "4th Anniv. Platinum Badge");

    add_rare_item(0x031019, "5th Anniv. Bronze Badge");
    add_rare_item(0x03101A, "5th Anniv. Silver Badge");
    add_rare_item(0x03101B, "5th Anniv. Gold Badge");
    add_uber_item(0x03101C, "5th Anniv. Platinum Badge");

    add_rare_item(0x03160C, "Disk Vol.13 \"Get It Up\"");
    add_rare_item(0x03160D, "Disk Vol.14 \"Flight\"");
    add_rare_item(0x03160E, "Disk Vol.15 \"Space Harrier\"");
    add_rare_item(0x03160F, "Disk Vol.16 \"Deathwatch\"");
    add_rare_item(0x031610, "Disk Vol.17 \"Fly Me To The Moon\"");
    add_rare_item(0x031611, "Disk Vol.18 \"Puyo Puyo\"");
    add_rare_item(0x031612, "Disk Vol.19 \"Rhythm And Balance\"");
    add_rare_item(0x031613, "Disk Vol.20 \"The Party Must Go On\"");
    add_rare_item(0x031705, "Viridia Badge");
    add_rare_item(0x031706, "Greenill Badge");
    add_rare_item(0x031707, "Skyly Badge");
    add_rare_item(0x031708, "Bluefull Badge");
    add_rare_item(0x031709, "Purplenum Badge");
    add_rare_item(0x03170A, "Pinkal Badge");
    add_rare_item(0x03170B, "Redria Badge");
    add_rare_item(0x03170C, "Oran Badge");
    add_rare_item(0x03170D, "Yellowboze Badge");
    add_rare_item(0x03170E, "Whitill Badge");
    add_rare_item(0x031810, "Heart of YN-0117");

    add_rare_item(0x031614, "Stealth Kit");
    add_rare_item(0x024D00, "Stealth");
}

// ----------------------- inside <anonymous> namespace -----------------------

void add_esranks(ItemInfoMap & map) {
    auto add = std::bind(add_item_only_once, std::ref(map), _1, _2, Rarity::esrank, false);
    add(0x007000, "SABER");
    add(0x007100, "SWORD");
    add(0x007200, "BLADE");
    add(0x007300, "PARTISAN");
    add(0x007400, "SLICER");
    add(0x007500, "GUN");
    add(0x007600, "RIFLE");
    add(0x007700, "MECHGUN");
    add(0x007800, "SHOT");
    add(0x007900, "CANE");
    add(0x007A00, "ROD");
    add(0x007B00, "WAND");
    add(0x007C00, "TWIN");
    add(0x007D00, "CLAW");
    add(0x007E00, "BAZOOKA");
    add(0x007F00, "NEEDLE");
    add(0x008000, "SCYTHE");
    add(0x008100, "HAMMER");
    add(0x008200, "MOON");
    add(0x008300, "PSYCHOGUN");
    add(0x008400, "PUNCH");
    add(0x008500, "WINDMILL");
    add(0x008600, "HARISEN");
    add(0x008700, "KATANA");
    add(0x008800, "J-CUTTER");
}

void prepare_code(uint32_t & fullcode) {
    const uint8_t & lowu8 = *reinterpret_cast<const uint8_t *>(&fullcode);
    if (is_esrank(fullcode) || lowu8 == 0x02) { // is mag
        fullcode &= 0xFFFF;
    }

    // values were added to map in big endian
    process_endian_u32(fullcode, k_big_endian);
    fullcode >>= 8;
}

#ifdef MACRO_USE_MT_INITIALIZATION

template <typename T>
PtrMaker<T> make_instance_maker() {
    static PtrMaker<T> callme_once = []() {
        static std::once_flag flag;
        static std::atomic<T *> instance_ = nullptr;
        std::call_once(flag, []() {
            using Storage = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
            static Storage storage;
            instance_ = new (&storage) T();
        });
        return &*instance_;
    };

    static std::atomic<int> i = 0;
    if (i++) {
        throw std::runtime_error("make_instance_maker: this function should only be called once.");
    }

    std::thread(callme_once).detach();
    return callme_once;
}

auto pass_instance = make_instance_maker<ItemDb>();
#endif

/* static */ const ItemDb & ItemDb::instance() {
#   ifdef MACRO_USE_MT_INITIALIZATION
    return *pass_instance();
#   else
    static ItemDb instance_;
    return instance_;
#   endif
}

/* private */ ItemDb::ItemDb() {
    // std::this_thread::sleep_for(std::chrono::seconds(10));
    add_weapons(m_info_map);
    add_units  (m_info_map);
    add_mags   (m_info_map);
    add_tools  (m_info_map);
    add_esranks(m_info_map);
    if (k_is_ephinea) {
        add_ephinea(m_info_map);
    }
    add_frames  (m_info_map, m_def_map);
    add_barriers(m_info_map, m_def_map);
}

// ----------------------- inside <anonymous> namespace -----------------------
// helpers "level 2"

void add_item_only_once(ItemInfoMap & map, uint32_t fullcode, const char * name,
                        Rarity rarity, bool has_kill_counter)
{
    assert(map.find(fullcode) == map.end());
    map[fullcode] = ItemInfo { name, rarity, has_kill_counter };
}

void add_def_only_once
    (DefItemMap & defmap, ItemInfoMap & itemmap,
     uint32_t fullcode, const char * name, const DfpPair & dfpp, const EvpPair & evpp,
     Rarity rarity, bool has_kill_counter)
{
    add_item_only_once(itemmap, fullcode, name, rarity, has_kill_counter);
    assert(defmap.find(fullcode) == defmap.end());
    assert(evpp.first <= evpp.second);
    assert(dfpp.first <= dfpp.second);

    defmap[fullcode] = DefenseItemInfo { dfpp.second, dfpp.first, evpp.second, evpp.first };
}

} // end of <anonymous> namespace
