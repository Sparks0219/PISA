#pragma once

#include <fstream>
#include <iterator>
#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <gsl/span>

namespace pisa {

namespace detail {

    using size_type = std::size_t;

    template <typename Payload_View = std::string_view>
    struct Payload_Vector_Iterator {
        using size_type = pisa::detail::size_type;
        using value_type = Payload_View;
        using difference_type = std::make_signed_t<size_type>;

        typename gsl::span<size_type const>::iterator offset_iter;
        typename gsl::span<std::byte const>::iterator payload_iter;

        constexpr auto operator++() -> Payload_Vector_Iterator &
        {
            ++offset_iter;
            std::advance(payload_iter, *offset_iter - *std::prev(offset_iter));
            return *this;
        }

        [[nodiscard]]
        constexpr auto operator++(int) -> Payload_Vector_Iterator
        {
            Payload_Vector_Iterator next_iter{offset_iter, payload_iter};
            ++(*this);
            return next_iter;
        }

        constexpr auto operator--() -> Payload_Vector_Iterator & { return *this -= 1; }

        [[nodiscard]]
        constexpr auto operator--(int) -> Payload_Vector_Iterator
        {
            Payload_Vector_Iterator next_iter{offset_iter, payload_iter};
            --(*this);
            return next_iter;
        }

        [[nodiscard]]
        constexpr auto operator+(size_type n) const -> Payload_Vector_Iterator
        {
            return {std::next(offset_iter, n),
                    std::next(payload_iter, *std::next(offset_iter, n) - *offset_iter)};
        }

        [[nodiscard]]
        constexpr auto operator+=(size_type n) -> Payload_Vector_Iterator &
        {
            std::advance(payload_iter, *std::next(offset_iter, n) - *offset_iter);
            std::advance(offset_iter, n);
            return *this;
        }

        [[nodiscard]]
        constexpr auto operator-(size_type n) const -> Payload_Vector_Iterator
        {
            return {std::prev(offset_iter, n),
                    std::prev(payload_iter, *offset_iter - *std::prev(offset_iter, n))};
        }

        [[nodiscard]]
        constexpr auto operator-=(size_type n) -> Payload_Vector_Iterator &
        {
            return *this = *this - n;
        }

        [[nodiscard]]
        constexpr auto operator-(Payload_Vector_Iterator const& other) -> difference_type
        {
            return offset_iter - other.offset_iter;
        }

        [[nodiscard]]
        constexpr auto operator*() -> value_type
        {
            return value_type(reinterpret_cast<char const *>(&*payload_iter),
                              *std::next(offset_iter) - *offset_iter);
        }

        [[nodiscard]] constexpr auto operator==(Payload_Vector_Iterator const &other) const -> bool
        {
            return offset_iter == other.offset_iter;
        }

        [[nodiscard]] constexpr auto operator!=(Payload_Vector_Iterator const &other) const -> bool
        {
            return offset_iter != other.offset_iter;
        }
    };

    template <typename... Ts>
    struct all_pod;

    template <typename Head, typename... Tail>
    struct all_pod<Head, Tail...> {
        static const bool value = std::is_pod_v<Head> && all_pod<Tail...>::value;
    };

    template <typename T>
    struct all_pod<T> {
        static const bool value = std::is_pod_v<T>;
    };

    template <typename... Ts>
    struct sizeofs;

    template <typename Head, typename... Tail>
    struct sizeofs<Head, Tail...> {
        static const size_t value = sizeof(Head) + sizeofs<Tail...>::value;
    };

    template <typename T>
    struct sizeofs<T> {
        static const size_t value = sizeof(T);
    };

    template <typename T, typename... Ts>
    [[nodiscard]] static constexpr auto unpack(std::byte const *ptr) -> std::tuple<T, Ts...>
    {
        if constexpr (sizeof...(Ts) == 0u) {
            return std::tuple<T>(*reinterpret_cast<const T *>(ptr));
        } else {
            return std::tuple_cat(std::tuple<T>(*reinterpret_cast<const T *>(ptr)),
                                  unpack<Ts...>(ptr + sizeof(T)));
        }
    }

} // namespace detail

struct Payload_Vector_Buffer {
    using size_type = detail::size_type;

    std::vector<size_type> const offsets;
    std::vector<std::byte> const payloads;

    [[nodiscard]] static auto from_file(std::string const &filename) -> Payload_Vector_Buffer;
    void to_file(std::string const &filename) const;
    void to_stream(std::ostream &is) const;

    template <typename InputIterator, typename PayloadEncodingFn>
    [[nodiscard]] static auto make(InputIterator first,
                                   InputIterator last,
                                   PayloadEncodingFn encoding_fn) -> Payload_Vector_Buffer
    {
        std::vector<size_type> offsets;
        offsets.push_back(0u);
        std::vector<std::byte> payloads;
        for (; first != last; ++first) {
            encoding_fn(*first, std::back_inserter(payloads));
            offsets.push_back(payloads.size());
        }
        return Payload_Vector_Buffer{std::move(offsets), std::move(payloads)};
    }
};

template <typename InputIterator, typename PayloadEncodingFn>
auto encode_payload_vector(InputIterator first, InputIterator last, PayloadEncodingFn encoding_fn)
{
    return Payload_Vector_Buffer::make(first, last, encoding_fn);
}

template <typename Payload, typename PayloadEncodingFn>
auto encode_payload_vector(gsl::span<Payload const> values, PayloadEncodingFn encoding_fn)
{
    return encode_payload_vector(values.begin(), values.end(), encoding_fn);
}

template <typename InputIterator>
auto encode_payload_vector(InputIterator first, InputIterator last)
{
    return Payload_Vector_Buffer::make(first, last, [](auto str, auto out_iter) {
        std::transform(
            str.begin(), str.end(), out_iter, [](auto ch) { return static_cast<std::byte>(ch); });
    });
}

[[nodiscard]] auto encode_payload_vector(gsl::span<std::string const> values)
    -> Payload_Vector_Buffer;

template <typename... T>
constexpr auto unpack_head(gsl::span<std::byte const> mem) -> std::tuple<T..., gsl::span<std::byte const>>
{
    static_assert(detail::all_pod<T...>::value);
    auto offset = detail::sizeofs<T...>::value;
    if (offset > mem.size()) {
        throw std::runtime_error(fmt::format(
            "Cannot unpack span of size {} into structure of size {}", mem.size(), offset));
    }
    auto tail = mem.subspan(offset);
    auto head = detail::unpack<T...>(mem.data());
    return std::tuple_cat(head, std::tuple<gsl::span<std::byte const>>(tail));
}

[[nodiscard]] auto split(gsl::span<std::byte const> mem, std::size_t offset)
    -> std::tuple<gsl::span<std::byte const>, gsl::span<std::byte const>>;

template <typename T>
[[nodiscard]] auto cast_span(gsl::span<std::byte const> mem) -> gsl::span<T const>
{
    auto type_size = sizeof(T);
    if (mem.size() % type_size != 0) {
        throw std::runtime_error(
            fmt::format("Failed to cast byte-span to span of T of size {}", type_size));
    }
    return gsl::make_span(reinterpret_cast<T const *>(mem.data()), mem.size() / type_size);
}

template <typename Payload_View = std::string_view>
class Payload_Vector {
   public:
    using size_type = detail::size_type;
    using difference_type = std::make_signed_t<size_type>;
    using payload_type = Payload_View;
    using iterator = detail::Payload_Vector_Iterator<Payload_View>;

    Payload_Vector(Payload_Vector_Buffer const &container)
        : offsets_(container.offsets), payloads_(container.payloads)
    {}

    Payload_Vector(gsl::span<size_type const> offsets, gsl::span<std::byte const> payloads)
        : offsets_(std::move(offsets)),
          payloads_(std::move(payloads))
    {}

    template <typename ContiguousContainer>
    [[nodiscard]]
    constexpr static auto from(ContiguousContainer&& mem) -> Payload_Vector
    {
        return from(gsl::make_span(reinterpret_cast<std::byte const *>(mem.data()), mem.size()));
    }

    [[nodiscard]]
    static auto from(gsl::span<std::byte const> mem) -> Payload_Vector
    {
        size_type length;
        gsl::span<std::byte const> tail;
        try {
            std::tie(length, tail) = unpack_head<size_type>(mem);
        } catch (std::runtime_error const &err) {
            throw std::runtime_error(std::string("Failed to parse payload vector length: ") +
                                     err.what());
        }

        gsl::span<std::byte const> offsets, payloads;
        try {
            std::tie(offsets, payloads) = split(tail, (length + 1u) * sizeof(size_type));
        } catch (std::runtime_error const& err) {
            throw std::runtime_error(std::string("Failed to parse payload vector offset table: ") +
                                     err.what());
        }
        return Payload_Vector<Payload_View>(cast_span<size_type>(offsets), payloads);
    }

    [[nodiscard]]
    constexpr auto operator[](size_type idx) const -> payload_type
    {
        if (idx >= offsets_.size()) {
            throw std::out_of_range(fmt::format(
                "Index {} too large for payload vector of size {}", idx, offsets_.size()));
        }
        if (offsets_[idx] >= payloads_.size()) {
            throw std::runtime_error(
                fmt::format("Offset {} too large for payload array of {} bytes",
                            offsets_[idx],
                            payloads_.size()));
        }
        return *(begin() + idx);
    }

    [[nodiscard]]
    constexpr auto begin() const -> iterator
    {
        return {offsets_.begin(), payloads_.begin()};
    }
    [[nodiscard]]
    constexpr auto end() const -> iterator
    {
        return {std::prev(offsets_.end()), payloads_.end()};
    }
    [[nodiscard]] constexpr auto cbegin() const -> iterator { return begin(); }
    [[nodiscard]] constexpr auto cend() const -> iterator { return end(); }
    [[nodiscard]]
    constexpr auto size() const -> size_type
    {
        return offsets_.size() - size_type{1};
    }

   private:
    gsl::span<size_type const> offsets_;
    gsl::span<std::byte const> payloads_;
};

} // namespace pisa

namespace std {

template <typename Payload_View>
struct iterator_traits<pisa::detail::Payload_Vector_Iterator<Payload_View>> {
    using size_type = pisa::detail::size_type;
    using difference_type = std::make_signed_t<size_type>;
    using value_type = Payload_View;
    using pointer = Payload_View;
    using reference = Payload_View;
    using iterator_category = std::random_access_iterator_tag;
};

} // namespace std