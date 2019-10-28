#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <variant>

#include <gsl/gsl_assert>
#include <gsl/span>

#include "v1/bit_cast.hpp"
#include "v1/types.hpp"

namespace pisa::v1 {

template <typename Int>
auto write_little_endian(Int number, gsl::span<std::byte> bytes)
{
    static_assert(std::is_integral_v<Int>);
    Expects(bytes.size() == sizeof(Int));

    Int mask{0xFF};
    for (unsigned int byte_num = 0; byte_num < sizeof(Int); byte_num += 1) {
        auto byte_value = static_cast<std::byte>((number & mask) >> (8U * byte_num));
        mask <<= 8U;
    }
}

struct FormatVersion {
    std::uint8_t major = 0;
    std::uint8_t minor = 0;
    std::uint8_t patch = 0;

    constexpr static auto parse(gsl::span<std::byte const> bytes) -> FormatVersion
    {
        Expects(bytes.size() == 3);
        return FormatVersion{
            bit_cast<std::uint8_t>(bytes.first(1)),
            bit_cast<std::uint8_t>(bytes.subspan(1, 1)),
            bit_cast<std::uint8_t>(bytes.subspan(2, 1)),
        };
    };

    constexpr auto write(gsl::span<std::byte> bytes) -> void
    {
        bytes[0] = std::byte{major};
        bytes[1] = std::byte{minor};
        bytes[2] = std::byte{patch};
    };

    constexpr static auto current() -> FormatVersion { return FormatVersion{0, 1, 0}; };
};

enum class Primitive { Int = 0, Float = 1 };

struct Array {
    Primitive type;
};

struct Tuple {
    Primitive type;
    std::uint8_t size;
};

using ValueType = std::variant<Primitive, Array, Tuple>;

template <typename T>
constexpr static auto value_type() -> ValueType
{
    if constexpr (std::is_integral_v<T>) {
        return Primitive::Int;
    } else if constexpr (std::is_floating_point_v<T>) {
        return Primitive::Float;
    } else {
        // TODO(michal): array and tuple
        throw std::runtime_error("");
    }
}

constexpr auto parse_type(std::byte const byte) -> ValueType
{
    auto element_type = [byte]() {
        switch (std::to_integer<std::uint8_t>((byte & std::byte{0b00000100}) >> 2)) {
        case 0U:
            return Primitive::Int;
        case 1U:
            return Primitive::Float;
        }
        Unreachable();
    };
    switch (std::to_integer<std::uint8_t>(byte & std::byte{0b00000011})) {
    case 0U:
        return Primitive::Int;
    case 1U:
        return Primitive::Float;
    case 2U:
        return Array{element_type()};
    case 3U:
        return Tuple{element_type(),
                     std::to_integer<std::uint8_t>((byte & std::byte{0b11111000}) >> 3)};
    }
    Unreachable();
};

constexpr auto to_byte(ValueType type) -> std::byte
{
    std::byte byte{};
    std::visit(overloaded{[&byte](Primitive primitive) {
                              switch (primitive) {
                              case Primitive::Int:
                                  byte = std::byte{0b00000000};
                                  break;
                              case Primitive::Float:
                                  byte = std::byte{0b00000001};
                                  break;
                              }
                          },
                          [&byte](Array arr) {
                              switch (arr.type) {
                              case Primitive::Int:
                                  byte = std::byte{0b00000010};
                                  break;
                              case Primitive::Float:
                                  byte = std::byte{0b00000110};
                                  break;
                              }
                          },
                          [&byte](Tuple tup) {
                              switch (tup.type) {
                              case Primitive::Int:
                                  byte = std::byte{0b00000011};
                                  break;
                              case Primitive::Float:
                                  byte = std::byte{0b00000111};
                                  break;
                              }
                              byte |= (std::byte{tup.size} << 3);
                          }},
               type);
    return byte;
};

using Encoding = std::uint32_t;

struct PostingFormatHeader {
    FormatVersion version;
    ValueType type;
    Encoding encoding;

    constexpr static auto parse(gsl::span<std::byte const> bytes) -> PostingFormatHeader
    {
        Expects(bytes.size() == 8);
        auto version = FormatVersion::parse(bytes.first(3));
        auto type = parse_type(bytes[3]);
        auto encoding = bit_cast<std::uint32_t>(bytes.subspan(4));
        return {version, type, encoding};
    };

    void write(gsl::span<std::byte> bytes)
    {
        Expects(bytes.size() == 8);
        FormatVersion::current().write(bytes.first(3));
        bytes[3] = to_byte(type);
        write_little_endian(encoding, bytes.subspan(4));
    };
};

} // namespace pisa::v1
