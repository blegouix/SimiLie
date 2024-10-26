// SPDX-FileCopyrightText: 2024 Baptiste Legouix
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <ddc/ddc.hpp>

#include "tensor_impl.hpp"

namespace sil {

namespace tensor {

// struct representing an identity tensor (no storage).
template <std::size_t q, class... TensorIndex>
struct LorentzianSignTensorIndex
{
    static constexpr bool is_natural_tensor_index = false;

    using subindexes_domain_t = ddc::DiscreteDomain<TensorIndex...>;

    static constexpr subindexes_domain_t subindexes_domain()
    {
        return ddc::DiscreteDomain<TensorIndex...>(
                ddc::DiscreteElement<TensorIndex...>(ddc::DiscreteElement<TensorIndex>(0)...),
                ddc::DiscreteVector<TensorIndex...>(
                        ddc::DiscreteVector<TensorIndex>(TensorIndex::size())...));
    }

    static constexpr std::size_t rank()
    {
        return (TensorIndex::rank() + ...);
    }

    static constexpr std::size_t size()
    {
        return (TensorIndex::size() * ...);
    }

    static constexpr std::size_t mem_size()
    {
        return 0;
    }

    static constexpr std::size_t access_size()
    {
        return 3;
    }

private:
    template <class Head, class... Tail>
    inline static constexpr bool are_all_same = (std::is_same_v<Head, Tail> && ...);

public:
    static constexpr std::pair<std::vector<double>, std::vector<std::size_t>> mem_id(
            std::array<std::size_t, sizeof...(TensorIndex)> const ids)
    {
        assert(false);
        return std::pair<
                std::vector<double>,
                std::vector<std::size_t>>(std::vector<double> {}, std::vector<std::size_t> {});
    }

    static constexpr std::size_t access_id(
            std::array<std::size_t, sizeof...(TensorIndex)> const ids)
    {
        if (!std::all_of(ids.begin(), ids.end(), [&](const std::size_t id) {
                return id == *ids.begin();
            })) {
            return 0;
        } else {
            if (ids[0] < q) {
                return 1;
            } else {
                return 2;
            }
        }
    }

    static constexpr std::pair<std::vector<double>, std::vector<std::size_t>> access_id_to_mem_id(
            std::size_t access_id)
    {
        assert(false && "There is no mem_id");
        return std::pair<
                std::vector<double>,
                std::vector<std::size_t>>(std::vector<double> {}, std::vector<std::size_t> {});
    }

    template <class Tensor, class Elem, class Id>
    static constexpr Tensor::element_type process_access(
            std::function<typename Tensor::element_type(Tensor, Elem)> access,
            Tensor tensor,
            Elem elem)
    {
        if (elem.template uid<Id>() == 0) {
            return 0.;
        } else if (elem.template uid<Id>() == 1) {
            return -1.;
        } else {
            return 1.;
        }
    }
};

namespace detail {
template <std::size_t q, class... SubIndex>
struct IsTensorIndex<LorentzianSignTensorIndex<q, SubIndex...>>
{
    using type = std::true_type;
};

} // namespace detail

} // namespace tensor

} // namespace sil