#pragma once

#include "boost/variant.hpp"
#include <stdexcept>

#include "codec/all_ones_sequence.hpp"
#include "codec/compact_ranked_bitvector.hpp"
#include "codec/strict_elias_fano.hpp"
#include "global_parameters.hpp"

namespace pisa {

struct strict_sequence {
    enum index_type {
        elias_fano = 0,
        ranked_bitvector = 1,
        all_ones = 2,

        index_types = 3
    };

    static const uint64_t type_bits = 1;  // all_ones is implicit

    static global_parameters strict_params(global_parameters params)
    {
        // we do not need to index the zeros
        params.ef_log_sampling0 = 63;
        params.rb_log_rank1_sampling = 63;
        return params;
    }

    static PISA_FLATTEN_FUNC uint64_t
    bitsize(global_parameters const& params, uint64_t universe, uint64_t n)
    {
        uint64_t best_cost = all_ones_sequence::bitsize(params, universe, n);
        auto sparams = strict_params(params);

        uint64_t ef_cost = strict_elias_fano::bitsize(sparams, universe, n) + type_bits;
        if (ef_cost < best_cost) {
            best_cost = ef_cost;
        }

        uint64_t rb_cost = compact_ranked_bitvector::bitsize(sparams, universe, n) + type_bits;
        if (rb_cost < best_cost) {
            best_cost = rb_cost;
        }

        return best_cost;
    }

    template <typename Iterator>
    static void write(
        bit_vector_builder& bvb,
        Iterator begin,
        uint64_t universe,
        uint64_t n,
        global_parameters const& params)
    {
        auto sparams = strict_params(params);
        uint64_t best_cost = all_ones_sequence::bitsize(params, universe, n);
        int best_type = all_ones;

        if (best_cost) {
            uint64_t ef_cost = strict_elias_fano::bitsize(sparams, universe, n) + type_bits;
            if (ef_cost < best_cost) {
                best_cost = ef_cost;
                best_type = elias_fano;
            }

            uint64_t rb_cost = compact_ranked_bitvector::bitsize(sparams, universe, n) + type_bits;
            if (rb_cost < best_cost) {
                best_cost = rb_cost;
                best_type = ranked_bitvector;
            }

            bvb.append_bits(best_type, type_bits);
        }

        switch (best_type) {
        case elias_fano: strict_elias_fano::write(bvb, begin, universe, n, sparams); break;
        case ranked_bitvector:
            compact_ranked_bitvector::write(bvb, begin, universe, n, sparams);
            break;
        case all_ones: all_ones_sequence::write(bvb, begin, universe, n, sparams); break;
        default: assert(false);
        }
    }

    class enumerator {
      public:
        using value_type = std::pair<uint64_t, uint64_t>;  // (position, value)

        enumerator() = default;

        enumerator(
            bit_vector const& bv,
            uint64_t offset,
            uint64_t universe,
            uint64_t n,
            global_parameters const& params)
        {
            auto sparams = strict_params(params);

            if (all_ones_sequence::bitsize(params, universe, n) == 0) {
                m_type = all_ones;
            } else {
                m_type = index_type(bv.get_word56(offset) & ((uint64_t(1) << type_bits) - 1));
            }

            switch (m_type) {
            case elias_fano:
                m_enumerator =
                    strict_elias_fano::enumerator(bv, offset + type_bits, universe, n, sparams);
                break;
            case ranked_bitvector:
                m_enumerator = compact_ranked_bitvector::enumerator(
                    bv, offset + type_bits, universe, n, sparams);
                break;
            case all_ones:
                m_enumerator =
                    all_ones_sequence::enumerator(bv, offset + type_bits, universe, n, sparams);
                break;
            default: throw std::invalid_argument("Unsupported type");
            }
        }

        value_type move(uint64_t position)
        {
            return boost::apply_visitor(
                [&position](auto&& e) { return e.move(position); }, m_enumerator);
        }

        value_type next()
        {
            return boost::apply_visitor([](auto&& e) { return e.next(); }, m_enumerator);
        }

        uint64_t size() const
        {
            return boost::apply_visitor([](auto&& e) { return e.size(); }, m_enumerator);
        }

        uint64_t prev_value() const
        {
            return boost::apply_visitor([](auto&& e) { return e.prev_value(); }, m_enumerator);
        }

      private:
        index_type m_type;
        boost::variant<strict_elias_fano::enumerator, compact_ranked_bitvector::enumerator, all_ones_sequence::enumerator>
            m_enumerator;
    };
};
}  // namespace pisa
