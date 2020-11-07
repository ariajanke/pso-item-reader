/****************************************************************************

    File: ProcessWatcher.cpp
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

#include "ProcessWatcher.hpp"
#include "ItemReaderStates.hpp"
#include "../MemoryReader.hpp"

#include "../Defs.hpp"

#include <cstdio>
#include <cassert>

namespace {

auto popen_to_uptr(const char * command, const char * mode) {
    struct ClosePFile
        { void operator () (FILE * ptr) const { (void)pclose(ptr); } };
    return std::unique_ptr<FILE, ClosePFile>(popen(command, mode));
}

} // end of <anonymous> namespace

void PsobbProcessWatcher::handle_event(const Event & event) {
    if (auto * sp = event.as_pointer<SpecialKey>()) {
        if (*sp == SpecialKey::escape) {
            throw QuitAppException();
        }
    }
}

void PsobbProcessWatcher::render_to(TargetGrid & target) const {
    if (m_has_permission) {
        static const std::string k_short_head = "Searching for PSOBB process!";
        static const std::string k_long_head  = k_short_head + " (press escape to quit)";
        const std::string * sel_header = &k_long_head;
        if (target.width() > int(sel_header->size())) sel_header = &k_short_head;
        auto mid_line = target.height() / 2;
        render_string_centered(target, *sel_header, mid_line,
                               TargetGrid::k_highlight_colors);
    } else {
        auto mid_line = (target.height() - int(m_error_lines.size())) / 2;
        mid_line = std::max(mid_line, 0);
        for (const auto & line : m_error_lines) {
            if (mid_line == target.height()) break;
            int x = 0;
            for (char c : line) {
                assert(x < target.width());
                target.set_cell(x++, mid_line, c, TargetGrid::k_normal_colors);
            }
            ++mid_line;
        }
    }
}

void PsobbProcessWatcher::handle_tick(double) {
    static constexpr const int k_read_size = 1024;
    auto pfile = popen_to_uptr("pgrep psobb", "r");
    // oh well, try again later
    if (!pfile) return;

    std::string contents;
    std::array<char, k_read_size> buf {};
    while (fgets(buf.data(), k_read_size, pfile.get())) {
        contents += buf.data();
    }

    try {
        int pid = k_no_pid;
        if (string_to_number_multibase(contents, pid)) {
            switch_state<BankViewState>().setup(
                MemoryReader::make_process_reader(pid));
        }
    } catch (PermissionError &) {
        m_has_permission = false;
        update_bad_permission_message();
        [[maybe_unused]] auto * stateptr = &switch_state<PsobbProcessWatcher>();
        assert(this == stateptr);
    }
}

void PsobbProcessWatcher::handle_resize(const GridSize & gsize) {
    m_max_height = gsize.height();
    m_max_width  = gsize.width ();
    update_bad_permission_message();
}

void PsobbProcessWatcher::update_bad_permission_message() {
    if (m_has_permission) return;
    static const std::vector<std::string> k_perm_fail = {
        "\"ptrace\" permissions is needed by this application. To grant this "
        "permission please run the following command (you will need root "
        "permissions):",
        "\"setcap 'CAP_SYS_PTRACE+ep' /path/to/binary/apir\"",
        "(of course replacing the file path with the path to this binary)",
        "Press escape to close this application."
    };
    render_wrapped_lines_to(k_perm_fail, m_max_width, m_max_height, m_error_lines);
}
