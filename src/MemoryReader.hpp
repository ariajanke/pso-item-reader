/****************************************************************************

    File: MemoryReader.hpp
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

#include "Defs.hpp"
#include <memory>

class MemoryReader {
public:
    using MemoryReaderSPtr = std::shared_ptr<MemoryReader>;

    static constexpr const char * const k_builtin_string = "builtin";

    virtual ~MemoryReader() {}
    virtual void read(Address, uint8_t * buf, std::size_t bytes_in_buf) const = 0;
    virtual void describe_source(std::ostream &) const;

    template <typename T>
    T read_datum(Address addr) const;

    // named reader functions
    int8_t read_i8(Address) const;
    uint8_t read_u8(Address) const;
    int16_t read_i16(Address) const;
    int32_t read_i32(Address) const;
    int64_t read_i64(Address) const;
    uint16_t read_u16(Address) const;
    uint32_t read_u32(Address) const;
    uint64_t read_u64(Address) const;

    float read_f32(Address) const;
    double read_f64(Address) const;

    static MemoryReaderSPtr make_process_reader(int pid);
    static MemoryReaderSPtr make_builtin_reader(int offset);
    static int get_builtin_size();
};

class SimpleBlockReader final : public MemoryReader {
public:
    SimpleBlockReader(const uint8_t * p, std::size_t size):
        m_block(p), m_size(size) {}
    void read(Address, uint8_t * buf, std::size_t bsize) const override;
private:
    const uint8_t * m_block;
    std::size_t m_size;
};

// ----------------------------------------------------------------------------

template <typename T>
T MemoryReader::read_datum(Address addr) const {
    static_assert(std::is_arithmetic_v<T>, "Can only read PoD/arithmetic types.");
    T obj;
    read(addr, reinterpret_cast<uint8_t *>(&obj), sizeof(T));
    return obj;
}
