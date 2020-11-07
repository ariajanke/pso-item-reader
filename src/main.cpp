/****************************************************************************

    File: main.cpp
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

#include <thread>
#include <chrono>

#include <cassert>

#include <ncurses.h>

#include "NCursesGrid.hpp"

#include "pso/ProcessWatcher.hpp"

namespace {

using TimePoint   = decltype(std::chrono::steady_clock::now());
using AppStatePtr = std::shared_ptr<AppState>;
using AppStateMap = AppState::AppStateMap;

double get_elapsed_time(TimePoint &);

void on_new_state(AppStatePtr, TargetGrid &);
void do_render   (AppStatePtr, NCursesGrid &);

} // end of <anonymous> namespace

int main() {
    // run tests before even starting
    NCursesGrid ncgrid;
    AppStateMap statemap;
    AppStatePtr state_ptr
        = AppState::make_state_with_map<PsobbProcessWatcher>(statemap);
    auto lasttime = std::chrono::steady_clock::now();
    ncgrid.setup();
    on_new_state(state_ptr, ncgrid);
    do_render   (state_ptr, ncgrid);

    while (true) {
        try {
            state_ptr->handle_tick(get_elapsed_time(lasttime));
            if (state_ptr->update_style() == AppState::k_continuous_updates) {
                for (int ch = getch(); ch != ERR; ch = getch()) {
                    state_ptr->handle_event(to_event(ch));
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(40));
            } else if (state_ptr->update_style() == AppState::k_until_next_event) {
                state_ptr->handle_event(to_event(getch()));
            }
            if (ncgrid.update_size()) state_ptr->handle_resize(ncgrid);

            if (auto ptr = state_ptr->get_new_state()) {
                state_ptr.swap(ptr);
                on_new_state(state_ptr, ncgrid);
            }
            do_render(state_ptr, ncgrid);
        } catch (QuitAppException &) {
            return 0;
        }
    }
}

namespace {

double get_elapsed_time(TimePoint & then) {
    using namespace std::chrono;
    auto now = steady_clock::now();
    auto rv_ns = duration_cast<microseconds>(now - then).count();
    then = now;
    return double(rv_ns) / 1000000.0;
}

void on_new_state(AppStatePtr state_ptr, TargetGrid & target) {
    state_ptr->handle_resize(target);
    if (state_ptr->update_style() == AppState::k_continuous_updates) {
        nodelay(stdscr, TRUE);
    } else {
        nodelay(stdscr, FALSE);
    }
}

void do_render(AppStatePtr state_ptr, NCursesGrid & target) {
    target.do_prerender();
    state_ptr->render_to(target);
    target.fill_unpressed_space();

    target.render();
}

} // end of <anonymous> namespace
