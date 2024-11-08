// SPDX-FileCopyrightText: 2024 Baptiste Legouix
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <ddc/ddc.hpp>

#include "character.hpp"

namespace sil {

namespace tensor {

// Abstract indexes used to characterize a generic metric
struct MetricIndex1
{
};

struct MetricIndex2
{
};

// Relabelize metric
template <class Dom, class Index1, class Index2>
using relabelize_metric_in_domain_t = relabelize_indexes_in_domain_t<
        Dom,
        ddc::detail::TypeSeq<MetricIndex1, MetricIndex2>,
        ddc::detail::TypeSeq<Index1, Index2>>;

template <class Index1, class Index2, class Dom>
relabelize_metric_in_domain_t<Dom, Index1, Index2> relabelize_metric_in_domain(Dom metric_dom)
{
    return relabelize_indexes_in_domain<
            ddc::detail::TypeSeq<MetricIndex1, MetricIndex2>,
            ddc::detail::TypeSeq<Index1, Index2>>(metric_dom);
}

// Compute domain for a tensor product of metrics (ie. g_mu_muprime*g_nu_nuprime*...)
namespace detail {

template <class MetricIndex, class Indexes1, class Indexes2>
struct MetricProdDomainType;

template <class MetricIndex, class... Index1, class... Index2>
struct MetricProdDomainType<
        MetricIndex,
        ddc::detail::TypeSeq<Index1...>,
        ddc::detail::TypeSeq<Index2...>>
{
    using type = ddc::cartesian_prod_t<relabelize_metric_in_domain_t<
            ddc::DiscreteDomain<MetricIndex>,
            Index1,
            ddc::type_seq_element_t<
                    ddc::type_seq_rank_v<Index1, ddc::detail::TypeSeq<Index1...>>,
                    ddc::detail::TypeSeq<Index2...>>>...>;
};

} // namespace detail

template <class MetricIndex, class Indexes1, class Indexes2>
using metric_prod_domain_t =
        typename detail::MetricProdDomainType<MetricIndex, Indexes1, Indexes2>::type;

namespace detail {

// Compute tensor product of metrics (ie. g_mu_muprime*g_nu_nuprime*...)
template <class MetricIndex, class Indexes1, class Indexes2>
struct MetricProdType;

template <class MetricIndex, class... Index1, class... Index2>
struct MetricProdType<MetricIndex, ddc::detail::TypeSeq<Index1...>, ddc::detail::TypeSeq<Index2...>>
{
    using type = sil::tensor::Tensor<
            double,
            metric_prod_domain_t<
                    MetricIndex,
                    ddc::detail::TypeSeq<Index1...>,
                    ddc::detail::TypeSeq<Index2...>>,
            std::experimental::layout_right,
            Kokkos::DefaultHostExecutionSpace::memory_space>;
};

} // namespace detail

template <class MetricIndex, class Indexes1, class Indexes2>
using metric_prod_t = typename detail::MetricProdType<MetricIndex, Indexes1, Indexes2>::type;

namespace detail {

template <class Indexes1, class Indexes2>
struct MetricProd;

template <>
struct MetricProd<ddc::detail::TypeSeq<>, ddc::detail::TypeSeq<>>
{
    template <class MetricType, class MetricProdType, class MetricProdType_>
    static MetricProdType run(
            MetricProdType metric_prod,
            MetricType metric,
            MetricProdType_ metric_prod_)
    {
        Kokkos::deep_copy(
                metric_prod.allocation_kokkos_view(),
                metric_prod_
                        .allocation_kokkos_view()); // We rely on Kokkos::deep_copy in place of ddc::parallel_deepcopy to avoid type verification of the type dimensions
        return metric_prod;
    }
};

template <class HeadIndex1, class... TailIndex1, class HeadIndex2, class... TailIndex2>
struct MetricProd<
        ddc::detail::TypeSeq<HeadIndex1, TailIndex1...>,
        ddc::detail::TypeSeq<HeadIndex2, TailIndex2...>>
{
    template <class MetricType, class MetricProdType, class MetricProdType_>
    static MetricProdType run(
            MetricProdType metric_prod,
            MetricType metric,
            MetricProdType_ metric_prod_)
    {
        ddc::cartesian_prod_t<
                typename MetricProdType_::discrete_domain_type,
                relabelize_metric_in_domain_t<
                        typename MetricType::discrete_domain_type,
                        HeadIndex1,
                        HeadIndex2>>
                new_metric_prod_dom_(
                        metric_prod_.domain(),
                        relabelize_metric_in_domain<HeadIndex1, HeadIndex2>(metric.domain()));
        ddc::Chunk new_metric_prod_alloc_(new_metric_prod_dom_, ddc::HostAllocator<double>());
        sil::tensor::Tensor<
                double,
                ddc::cartesian_prod_t<
                        typename MetricProdType_::discrete_domain_type,
                        relabelize_metric_in_domain_t<
                                typename MetricType::discrete_domain_type,
                                HeadIndex1,
                                HeadIndex2>>,
                std::experimental::layout_right,
                Kokkos::DefaultHostExecutionSpace::memory_space>
                new_metric_prod_(new_metric_prod_alloc_);

        // TODO tensorial prod ? atm supports only Identity or Lorentzian metrics (new_metric_prod is empty)

        return MetricProd<
                ddc::detail::TypeSeq<TailIndex1...>,
                ddc::detail::TypeSeq<TailIndex2...>>::run(metric_prod, metric, new_metric_prod_);
    }
};
} // namespace detail

template <class MetricIndex, class Indexes1, class Indexes2, class MetricType>
metric_prod_t<MetricIndex, Indexes1, Indexes2> fill_metric_prod(
        metric_prod_t<MetricIndex, Indexes1, Indexes2> metric_prod,
        MetricType metric)
{
    ddc::DiscreteDomain<> dom_;
    ddc::Chunk metric_prod_alloc_(dom_, ddc::HostAllocator<double>());
    sil::tensor::Tensor<
            double,
            ddc::DiscreteDomain<>,
            std::experimental::layout_right,
            Kokkos::DefaultHostExecutionSpace::memory_space>
            metric_prod_(metric_prod_alloc_);

    return detail::MetricProd<Indexes1, Indexes2>::run(metric_prod, metric, metric_prod_);
}

namespace detail {

// Type of index used by projectors or symmetrizers
template <class Index>
struct prime : Index
{
};

template <class Indexes>
struct Primes;

template <class... Index>
struct Primes<ddc::detail::TypeSeq<TensorContravariantNaturalIndex<Index>...>>
{
    using type = ddc::detail::TypeSeq<TensorContravariantNaturalIndex<prime<Index>>...>;
};

template <class... Index>
struct Primes<ddc::detail::TypeSeq<TensorCovariantNaturalIndex<Index>...>>
{
    using type = ddc::detail::TypeSeq<TensorCovariantNaturalIndex<prime<Index>>...>;
};

} // namespace detail

// Apply metrics inplace (like g_mu_muprime*T^muprime^nu)
template <class MetricIndex, class Indexes1, class Indexes2, class MetricType, class TensorType>
relabelize_indexes_of_t<TensorType, Indexes2, Indexes1>
inplace_apply_metrics( // TODO avoid metricS by using concepts
        TensorType tensor,
        MetricType metric)
{
    sil::tensor::tensor_accessor_for_domain_t<metric_prod_domain_t<MetricIndex, Indexes1, Indexes2>>
            metric_prod_accessor;
    ddc::Chunk metric_prod_alloc(metric_prod_accessor.mem_domain(), ddc::HostAllocator<double>());
    sil::tensor::Tensor<
            double,
            metric_prod_domain_t<MetricIndex, Indexes1, Indexes2>,
            std::experimental::layout_right,
            Kokkos::DefaultHostExecutionSpace::memory_space>
            metric_prod(metric_prod_alloc);

    fill_metric_prod<MetricIndex, Indexes1, Indexes2>(metric_prod, metric);

    ddc::Chunk result_alloc(
            relabelize_indexes_in_domain<Indexes2, Indexes1>(tensor.domain()),
            ddc::HostAllocator<double>());
    relabelize_indexes_of_t<TensorType, Indexes2, Indexes1> result(result_alloc);
    tensor_prod3(
            result,
            relabelize_indexes_of<Indexes2, typename detail::Primes<Indexes1>::type>(metric_prod),
            relabelize_indexes_of<Indexes2, typename detail::Primes<Indexes2>::type>(tensor));
    Kokkos::deep_copy(
            tensor.allocation_kokkos_view(),
            result.allocation_kokkos_view()); // We rely on Kokkos::deep_copy in place of ddc::parallel_deepcopy to avoid type verification of the type dimensions

    return relabelize_indexes_of<Indexes2, Indexes1>(tensor);
}

} // namespace tensor

} // namespace sil