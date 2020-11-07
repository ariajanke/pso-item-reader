/****************************************************************************

    File: AppStateDefs.hpp
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

#include <memory>

#include "Defs.hpp"

class GridSize {
public:
    virtual ~GridSize();

    virtual int width() const = 0;
    virtual int height() const = 0;
};

class TargetGrid : public GridSize {
public:
    enum {
        k_normal_colors = 1,
        k_highlight_colors,

        k_dark_yellow_colors,

        k_red_text    ,
        k_green_text  ,
        k_yellow_text ,
        k_blue_text   ,
        k_magenta_text,
        k_cyan_text
    };

    virtual void set_cell(int x, int y, char, int cpair) = 0;
};

class AppState;

class AppStateChanger {
public:
    using AppStateMap = std::unordered_map<std::size_t, std::shared_ptr<AppState>>;
    AppStateChanger() {}
    AppStateChanger(std::shared_ptr<AppState> &, AppStateMap &);

    template <typename T>
    T & change_state();
private:
    std::shared_ptr<AppState> * m_target_pointer = nullptr;
    AppStateMap * m_state_map = nullptr;
};

class AppState {
public:
    using AppStateMap = AppStateChanger::AppStateMap;
    enum UpdateStyle {
        k_continuous_updates, // makes copy-pasting very hard
        k_until_next_event    // easy copy-pasting, however blocked until next event
    };
    virtual ~AppState();

    virtual void handle_event(const Event &) = 0;
    virtual void handle_tick(double) {}
    virtual void handle_resize(const GridSize &) {}
    virtual void render_to(TargetGrid &) const = 0;
    virtual UpdateStyle update_style() const noexcept
        { return k_until_next_event; }

    std::shared_ptr<AppState> get_new_state() { return std::move(m_new_state); }

    template <typename T>
    static std::shared_ptr<T> make_state_with_map(AppStateMap & statemap);

protected:
    template <typename T>
    T & switch_state();

    AppStateChanger make_state_changer();

private:
    void assign_state_map(AppStateMap & statemap) { m_state_map = &statemap; }

    template <typename T>
    static std::size_t sm_id() {
        static char c;
        return reinterpret_cast<std::size_t>(&c);
    }

    std::shared_ptr<AppState> m_new_state;
    AppStateMap * m_state_map = nullptr;
};

class QuitAppException final : public std::exception {
public:
    const char * what() const noexcept override { return ""; }
};

void render_string_centered(TargetGrid &, const std::string &, int line, int color);

void render_wrapped_lines_to
    (const std::vector<std::string> &, int max_width, int max_height,
     std::vector<std::string> & display_lines);

// ----------------------------------------------------------------------------

template <typename T>
T & AppStateChanger::change_state() {
    if (!m_state_map) {
        throw std::runtime_error(
            "AppStateChanger::change_state: cannot change state without a map "
            "for the state to live in.");
    }
    if (!m_target_pointer) {
        throw std::runtime_error(
            "AppStateChanger::change_state: cannot change state with the "
            "current state's \"new state\" pointer.");
    }
    auto ptr = AppState::make_state_with_map<T>(*m_state_map);
    *m_target_pointer = ptr;
    return *ptr;
}

template <typename T>
/* static */ std::shared_ptr<T> AppState::make_state_with_map(AppStateMap & statemap) {
    static_assert(std::is_base_of_v<AppState, T>, "Type T must be derived from AppState.");
    auto rv = std::make_shared<T>();
    rv->assign_state_map(statemap);
    statemap[sm_id<T>()] = rv;
    return rv;
}

template <typename T>
/* protected */ T & AppState::switch_state() {
    if (!m_state_map) {
        throw std::runtime_error("switch_state: no state map assigned");
    }
    std::shared_ptr<T> rv;
    auto itr = m_state_map->find(sm_id<T>());
    if (itr == m_state_map->end()) {
        rv = std::make_shared<T>();
        rv->assign_state_map(*m_state_map);
        (*m_state_map)[sm_id<T>()] = rv;
    } else if (( rv = std::dynamic_pointer_cast<T>(itr->second) )) {
        // do nothing
    } else {
        throw std::runtime_error("switch_state: cannot down cast to most derived, this is evidence of a bug.");
    }
    m_new_state = rv;
    return *rv;
}
