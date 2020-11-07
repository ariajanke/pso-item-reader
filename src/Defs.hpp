/****************************************************************************

    File: Defs.hpp
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
#include <cstring>

#include <string>

#include <common/StringUtil.hpp>
#include <common/MultiType.hpp>

// ---------------------------------- events ----------------------------------

struct TextEvent { char code; };

enum class SpecialKey {
    up, down, left, right, escape, backspace, enter, delete_,
    home, end, page_up, page_down,
    no_special_key
};

using Event = MultiType<TextEvent, SpecialKey>;

Event to_event(int key);

// ----------------------------------------------------------------------------

using Address = uintptr_t;
constexpr const int     k_no_pid       = -1;
constexpr const Address k_no_address   =  0;

// ------------------------------ low level stuff -----------------------------

enum EndiannessEnum { k_big_endian, k_little_endian };

void process_endian_u16(uint16_t &, EndiannessEnum);
void process_endian_u32(uint32_t &, EndiannessEnum);
void process_endian_u64(uint64_t &, EndiannessEnum);

EndiannessEnum get_machine_endianness();

bool is_real_pid(int);

class PermissionError final : public std::exception {
public:
    PermissionError(const char * what_): m_what(what_) {}
    const char * what() const noexcept override { return m_what; }
private:
    const char * m_what;
};

void read_memory_to(int pid, Address targets_addr, uint8_t * buffer, std::size_t buffer_len);

// ------------------------------ string parsing ------------------------------

inline bool is_whitespace    (char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
inline bool is_not_whitespace(char c) { return !is_whitespace(c); }
inline bool is_newline       (char c) { return c == '\n'; }
inline bool is_pipe          (char c) { return c == '|'; }
inline bool is_alphanumeric  (char c) { return (c >= 'a' && c <= 'z') ||
                                               (c >= 'A' && c <= 'Z') ||
                                               (c >= '0' && c <= '9'); }
#if 0
template <typename T>
bool string_to_number_mr(const char *, T &);

template <typename T>
bool string_to_number_mr(const char *, const char * end, T &);

template <typename T>
bool string_to_number_mr(const std::string &, T &);

void test_string_to_number();
#endif
// ----------------------------------------------------------------------------

class MemoryRecorder {
public:
    using AddressPair = std::pair<Address, Address>;
    virtual ~MemoryRecorder();
    virtual void clear() = 0;
    virtual void record(Address, const uint8_t *, std::size_t) = 0;
    virtual int results_count() const = 0;

    static MemoryRecorder & null_recorder();
};

class AddressRecorder final : public MemoryRecorder {
public:
    AddressRecorder(std::vector<Address> & addrs): m_addresses(addrs) {}

    void clear() override {}
    void record(Address addr, const uint8_t *, std::size_t) override;
    const std::vector<Address> & addresses() const { return m_addresses; }
    int results_count() const override { return int(m_addresses.size()); }
private:
    std::vector<Address> & m_addresses;
};
#if 0
// ----------------------------------------------------------------------------

template <typename T>
bool string_to_number_mr(const char * beg, T & obj)
    { return string_to_number_mr(beg, beg + strlen(beg), obj); }

template <typename T>
bool string_to_number_mr(const char * beg, const char * end, T & obj) {
    trim<is_whitespace>(beg, end);
    if (beg == end) return false;

    std::array<char, sizeof(T)*8> tempbuf;
    bool is_neg = false;
    if (*beg == '-') {
        if constexpr (std::is_signed_v<T>) {
            ++beg;
            is_neg = true;
            tempbuf[0] = '-';
        } else {
            return false;
        }
    }

    T base = 10;
    if (*beg == '0') {
        if ((beg + 1) == end) {
            obj = 0;
            return true;
        } else {
            ++beg;
            if (*beg == 'x' || *beg == 'X') {
                base = 16;
                ++beg;
            } else {
                base = 8;
            }
        }
    }

    if (is_neg) {
        std::copy(beg, end, tempbuf.data() + 1);
        auto len = (end - beg) + 1;
        beg = end = tempbuf.data();
        end += len;
    }
    return string_to_number(beg, end, obj, base);
}

template <typename T>
bool string_to_number_mr(const std::string & str, T & out)
    { return string_to_number_mr(str.c_str(), str.c_str() + str.length(), out); }
#endif
