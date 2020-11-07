/****************************************************************************

    File: NCursesGrid.hpp
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

#include "AppStateDefs.hpp"

#include <common/Grid.hpp>

// who knows, maybe useful on a "pixel" adapter...
class CachedChangeGrid : public TargetGrid {
public:
    void set_cell(int x, int y, char, int cpair) final;

    bool update_size();
    void fill_unpressed_space();
    void do_prerender();

protected:
    bool has_changed(int x, int y) const;
    std::pair<char, int> get_color_char_pair(int x, int y) const;

private:
    Grid<char> m_character_grid;
    Grid<int > m_color_grid;
    Grid<bool> m_changed, m_pressed;
    int m_num_changed = 0;
};

class NCursesGrid final : public CachedChangeGrid {
public:
    NCursesGrid() {}
    NCursesGrid(const NCursesGrid &) = delete;
    NCursesGrid & operator = (const NCursesGrid &) = delete;

    NCursesGrid(NCursesGrid &&) = delete;
    NCursesGrid & operator = (NCursesGrid &&) = delete;

    virtual ~NCursesGrid();
    void setup();

    int width() const override;
    int height() const override;

    void render() const;
};
