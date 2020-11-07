/****************************************************************************

    File: MemoryReader.cpp
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

#include "MemoryReader.hpp"

#include <fstream>
#include <random>

#include <cassert>

namespace {

using Error            = std::runtime_error            ;
using MemoryReaderSPtr = MemoryReader::MemoryReaderSPtr;

class ProcessMemoryReader final : public MemoryReader {
public:
    ProcessMemoryReader(int pid): m_pid(pid) {}

    void read(Address addr, uint8_t * buf, std::size_t bytes_in_buf) const override
        { read_memory_to(m_pid, addr, buf, bytes_in_buf); }

    void describe_source(std::ostream & out) const override {
        out << "Process id " << std::dec << m_pid << ".";
    }
private:
    int m_pid;
};

class BuiltinMemoryReader final : public MemoryReader {
public:
    BuiltinMemoryReader(int offset): m_offset(offset) {}
    void read(Address, uint8_t *, std::size_t) const override;

    static std::size_t get_builtin_size() { return get_builtin_data().size(); }

    void describe_source(std::ostream & out) const override {
        out << "Builtin/testing data.";
    }

private:
    static const std::vector<uint8_t> & get_builtin_data();
    int m_offset;
};

} // end of <anonymous> namespace

/* vtable anchor */ void MemoryReader::describe_source(std::ostream &) const {
    throw Error("MemoryReader::describe_source: source not meant to be described.");
}

 int8_t  MemoryReader::read_i8 (Address addr) const { return read_datum< int8_t >(addr); }
uint8_t  MemoryReader::read_u8 (Address addr) const { return read_datum<uint8_t >(addr); }
 int16_t MemoryReader::read_i16(Address addr) const { return read_datum< int16_t>(addr); }
uint16_t MemoryReader::read_u16(Address addr) const { return read_datum<uint16_t>(addr); }
 int32_t MemoryReader::read_i32(Address addr) const { return read_datum< int32_t>(addr); }
uint32_t MemoryReader::read_u32(Address addr) const { return read_datum<uint32_t>(addr); }
 int64_t MemoryReader::read_i64(Address addr) const { return read_datum< int64_t>(addr); }
uint64_t MemoryReader::read_u64(Address addr) const { return read_datum<uint64_t>(addr); }

float MemoryReader::read_f32(Address addr) const {
    static_assert(sizeof(float) == 4, "");
    return read_datum<float>(addr);
}

double MemoryReader::read_f64(Address addr) const {
    static_assert(sizeof(double) == 8, "");
    return read_datum<double>(addr);
}

/* static */ MemoryReaderSPtr MemoryReader::make_process_reader(int pid) {
    return std::make_shared<ProcessMemoryReader>(pid);
}

/* static */ MemoryReaderSPtr MemoryReader::make_builtin_reader(int offset) {
    return std::make_shared<BuiltinMemoryReader>(offset);
}

/* static */ int MemoryReader::get_builtin_size()
    { return int(BuiltinMemoryReader::get_builtin_size()); }

/* vtable anchor */ void SimpleBlockReader::read
    (Address, uint8_t * buf, std::size_t bsize) const
{
    if (bsize > m_size) {
        throw Error("SimpleBlockReader::read: cannot fill request.");
    }
    std::copy(m_block, m_block + bsize, buf);
}

namespace {

template <typename T, typename U>
void append_data(std::vector<uint8_t> &, std::initializer_list<U>);

void BuiltinMemoryReader::read(Address addr, uint8_t * buf, std::size_t bytes_in_buf) const {
    bool is_under = int(addr) < m_offset;
    addr -= m_offset;
    if (is_under || addr + bytes_in_buf > get_builtin_data().size()) {
        throw Error("BuiltinMemoryReader::read: Address is not in range of the builtin data.");
    }

    auto start = get_builtin_data().begin() + addr;
    std::copy(start, start + bytes_in_buf, buf);
}

/* static */ const std::vector<uint8_t> &
    BuiltinMemoryReader::get_builtin_data()
{
    static std::vector<uint8_t> rv;
    if (!rv.empty()) return rv;
    append_data<uint8_t >(rv, { 0x01, 0x02, 0x03, 0x04 });
    append_data<uint16_t>(rv, { 0x0102, 0x0304, 0x0506, 0x0708 });
    append_data<uint32_t>(rv, { 0x12345678u, 0x1A2A3A4Au, 0xFFAABBFFu,
                                0x00FF44FFu });
    append_data<uint64_t>(rv, { 0xFFFFFFFFFFFFFFFFull });
    append_data<double  >(rv, { 0.0, -100.0, 456.99, 1000e100 });

    append_data<uint32_t>(rv, {
        0x11111111u, 0x11111111u, 0x11111111u, 0x11111111u,
        0x22222222u, 0x22222222u, 0x22222222u, 0x22222222u,
        0x33333333u, 0x33333333u, 0x33333333u, 0x33333333u,
        0x44444444u, 0x44444444u, 0x44444444u, 0x44444444u,
        0x55555555u, 0x55555555u, 0x55555555u, 0x55555555u,
        0x66666666u, 0x66666666u, 0x66666666u, 0x66666666u,
        0x77777777u, 0x77777777u, 0x77777777u, 0x77777777u,
        0x88888888u, 0x88888888u, 0x88888888u, 0x88888888u,
    });

    std::default_random_engine rng { std::random_device()() };
    auto dist = std::uniform_int_distribution<uint8_t>();
    for (int i = 0; i != 4096; ++i) {
        rv.push_back(dist(rng));
    }
    return rv;
}

template <typename T, typename U>
void append_data
    (std::vector<uint8_t> & data, std::initializer_list<U> list)
{
    for (auto i : list) {
        T obj = T(i);
        assert(obj == i);
        while (data.size() % sizeof(T) != 0) {
            // pad until aligned
            data.push_back(0);
        }
        auto old_size = data.size();
        data.resize(data.size() + sizeof(T));
        *reinterpret_cast<T *>(&data[old_size]) = obj;
    }
}

} // end of <anonymous> namespace
