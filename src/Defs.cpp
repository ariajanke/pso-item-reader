/****************************************************************************

    File: Defs.cpp
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

#include "Defs.hpp"

#include <sys/uio.h>

#include <ncurses.h>

#include <cassert>

namespace {

template <typename T>
void rev(uint8_t *);

} // end of <anonymous> namespace

Event to_event(int key) {
    switch (key) {
    case KEY_UP   : return Event(SpecialKey::up       );
    case KEY_DOWN : return Event(SpecialKey::down     );
    case KEY_RIGHT: return Event(SpecialKey::right    );
    case KEY_LEFT : return Event(SpecialKey::left     );
    case KEY_PPAGE: return Event(SpecialKey::page_up  );
    case KEY_NPAGE: return Event(SpecialKey::page_down);
    case KEY_END  : return Event(SpecialKey::end      );
    case KEY_HOME : return Event(SpecialKey::home     );
    case '\n': case KEY_ENTER: return Event(SpecialKey::enter);
    case KEY_BACKSPACE: return Event(SpecialKey::backspace);
    case KEY_DC       : return Event(SpecialKey::delete_);
    case 27: return Event(SpecialKey::escape);
    default:
        if (key < ' ' || key > '~') return Event();
        return Event(TextEvent { char(key) });
    }
}

/* vtable anchor */ MemoryRecorder::~MemoryRecorder() {}

void AddressRecorder::record(Address addr, const uint8_t *, std::size_t) {
    m_addresses.push_back(addr);
}

/* static */ MemoryRecorder & MemoryRecorder::null_recorder() {
    class NullRecorder final : public MemoryRecorder {
    public:
        void clear() override {}
        void record(Address, const uint8_t *, std::size_t) override {}
        int results_count() const override { return 0; }
    };
    static NullRecorder inst;
    return inst;
}

void read_memory_to
    (int pid, Address targets_addr, uint8_t * buffer, std::size_t buffer_len)
{
    using Error = std::runtime_error;

    struct iovec local;
    struct iovec remote;
    remote.iov_len  = local.iov_len = buffer_len;
    local .iov_base = buffer;
    remote.iov_base = reinterpret_cast<void *>(targets_addr);
    if (std::size_t(process_vm_readv(pid, &local, 1, &remote, 1, 0)) != buffer_len) {
        // error strings straight out of:
        // https://man7.org/linux/man-pages/man2/process_vm_readv.2.html
        switch (errno) {
        case EINVAL: throw Error("Flags are not 0 or liovcnt or riovcnt is too large.");
        case EFAULT: throw Error(
            "The memory described by local_iov is outside the caller's "
            "accessible address space. OR\n"
            "The memory described by remote_iov is outside the accessible "
            "address space of the process pid.");
        case ENOMEM: throw Error(
            "Could not allocate memory for internal copies of the iovec structures.");
        case EPERM:  throw PermissionError(
            "The caller does not have permission to access the address space "
            "of the process pid.");
        case ESRCH: throw Error("No process with pid (" + std::to_string(pid) + ") exists.");
        };
    }
}


decltype(k_big_endian) get_machine_endianness() {
    // lsb first == little endian
    uint16_t u = 1;
    return *reinterpret_cast<const uint8_t *>(&u) ? k_little_endian : k_big_endian;
}

void process_endian_u16(uint16_t & u, EndiannessEnum endn) {
    if (endn == get_machine_endianness()) return;
    rev<uint16_t>(reinterpret_cast<uint8_t *>(&u));
}

void process_endian_u32(uint32_t & u, EndiannessEnum endn) {
    if (endn == get_machine_endianness()) return;
    rev<uint32_t>(reinterpret_cast<uint8_t *>(&u));
}

void process_endian_u64(uint64_t & u, EndiannessEnum endn) {
    if (endn == get_machine_endianness()) return;
    rev<uint64_t>(reinterpret_cast<uint8_t *>(&u));
}


/* free fn */ bool is_real_pid(int pid) { return pid > 0; }


namespace {

template <typename BigT>
BigT revt(BigT a);

template <typename T>
void rev(uint8_t * ptr) {
    T & obj = *reinterpret_cast<T *>(ptr);
    obj = revt<T>(obj);
}

template <typename T>
struct SmallerOf { using Type = void; };

template <>
struct SmallerOf<uint64_t> { using Type = uint32_t; };
template <>
struct SmallerOf<uint32_t> { using Type = uint16_t; };
template <>
struct SmallerOf<uint16_t> { using Type = uint8_t ; };

template <typename BigT>
BigT revt(BigT a) {
    using SmaT = typename SmallerOf<BigT>::Type;
    SmaT high = a >> sizeof(SmaT)*8;
    SmaT low  = a & ~SmaT(0);
    if constexpr (sizeof(SmaT) > 1) {
        high = revt<SmaT>(high);
        low  = revt<SmaT>(low );
    }
    return (BigT(low) << sizeof(SmaT)*8) | high;
}

} // end of <anonymous> namespace
