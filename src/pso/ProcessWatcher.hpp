/****************************************************************************

    File: ProcessWatcher.hpp
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

#include "../AppStateDefs.hpp"

class PsobbProcessWatcher final : public AppState {
public:
    void handle_event(const Event &) override;
    void render_to(TargetGrid &) const override;
    void handle_tick(double) override;
    void handle_resize(const GridSize &) override;
    UpdateStyle update_style() const noexcept override {
        return m_has_permission ? k_continuous_updates : k_until_next_event;
    }
private:
    void update_bad_permission_message();
    bool m_has_permission = true;
    int m_max_width = 0, m_max_height = 0;

    std::vector<std::string> m_error_lines;
};
