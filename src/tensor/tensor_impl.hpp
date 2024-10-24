// SPDX-FileCopyrightText: 2024 Baptiste Legouix
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include <ddc/ddc.hpp>

namespace sil {

namespace tensor {

// struct representing an index mu or nu in a tensor Tmunu.
template <class... CDim>
struct TensorNaturalIndex
{
    using type_seq_dimensions = ddc::detail::TypeSeq<CDim...>;

    static constexpr std::size_t rank()
    {
        return 1;
    }

    static constexpr std::size_t size()
    {
        return sizeof...(CDim);
    }

    static constexpr std::size_t mem_size()
    {
        return size();
    }

    static constexpr std::size_t access_size()
    {
        return size();
    }

    template <class ODim>
    static constexpr std::pair<std::vector<double>, std::vector<std::size_t>> mem_id()
    {
        return std::pair<std::vector<double>, std::vector<std::size_t>>(
                std::vector<double> {},
                std::vector<std::size_t> {ddc::type_seq_rank_v<ODim, type_seq_dimensions>});
    }

    template <class ODim>
    static constexpr std::size_t access_id()
    {
        return std::get<1>(mem_id<ODim>())[0];
    }

    static constexpr std::pair<std::vector<double>, std::vector<std::size_t>> access_id_to_mem_id(
            std::size_t access_id)
    {
        return std::pair<
                std::vector<double>,
                std::vector<
                        std::size_t>>(std::vector<double> {}, std::vector<std::size_t> {access_id});
    }

    template <class Tensor, class Elem, class Id>
    static constexpr Tensor::element_type process_access(
            std::function<typename Tensor::element_type(Tensor, Elem)> access,
            Tensor tensor,
            Elem elem)
    {
        return access(tensor, elem);
    }
};

// Helpers to build the access_id() function which computes the ids of subindexes of an index.
namespace detail {
// For Tmunu and index=nu, returns 1
template <class Index, class...>
struct NbDimsBeforeIndex;

template <class Index, class IndexHead, class... IndexTail>
struct NbDimsBeforeIndex<Index, ddc::detail::TypeSeq<IndexHead, IndexTail...>>
{
    static constexpr std::size_t run(std::size_t nb_dims_before_index)
    {
        if constexpr (std::is_same_v<IndexHead, Index>) {
            return nb_dims_before_index;
        } else {
            return NbDimsBeforeIndex<Index, ddc::detail::TypeSeq<IndexTail...>>::run(
                    nb_dims_before_index + IndexHead::rank());
        }
    }
};

// Offset and index sequence
template <std::size_t Offset, class IndexSeq>
struct OffsetIndexSeq;

template <std::size_t Offset, std::size_t... Is>
struct OffsetIndexSeq<Offset, std::integer_sequence<std::size_t, Is...>>
{
    using type = std::integer_sequence<std::size_t, Offset + Is...>;
};

template <std::size_t Offset, class IndexSeq>
using offset_index_seq_t = OffsetIndexSeq<Offset, IndexSeq>::type;

// Returns dimensions from integers (ie. for Tmunu, <1> gives nu)
template <class CDimTypeSeq, class IndexSeq>
struct TypeSeqDimsAtInts;

template <class CDimTypeSeq, std::size_t... Is>
struct TypeSeqDimsAtInts<CDimTypeSeq, std::integer_sequence<std::size_t, Is...>>
{
    using type = ddc::detail::TypeSeq<ddc::type_seq_element_t<Is, CDimTypeSeq>...>;
};

template <class CDimTypeSeq, class IndexSeq>
using type_seq_dims_at_ints_t = TypeSeqDimsAtInts<CDimTypeSeq, IndexSeq>::type;

// Returns Index::access_id but from a type seq (in place of a variadic template CDim...)
template <class Index, class TypeSeqDims>
struct IdFromTypeSeqDims;

template <class Index, class... CDim>
struct IdFromTypeSeqDims<Index, ddc::detail::TypeSeq<CDim...>>
{
    static constexpr std::size_t run()
    {
        return Index::template access_id<CDim...>();
    }
};

// Returns Index::access_id for the subindex Index of the IndexesTypeSeq
template <class Index, class IndexesTypeSeq, class... CDim>
static constexpr std::size_t access_id()
{
    return IdFromTypeSeqDims<
            Index,
            type_seq_dims_at_ints_t<
                    ddc::detail::TypeSeq<CDim...>,
                    offset_index_seq_t<
                            NbDimsBeforeIndex<Index, IndexesTypeSeq>::run(0),
                            std::make_integer_sequence<std::size_t, Index::rank()>>>>::run();
}

} // namespace detail

// TensorAccessor class, allows to build a domain which represents the tensor and access elements.
template <class... Index>
class TensorAccessor
{
public:
    explicit constexpr TensorAccessor();

    static constexpr ddc::DiscreteDomain<Index...> mem_domain();

    static constexpr ddc::DiscreteDomain<Index...> access_domain();

    template <class... CDim>
    static constexpr ddc::DiscreteElement<Index...> element();
};

namespace detail {

template <class Dom>
struct TensorAccessorForDomain;

template <class... Index>
struct TensorAccessorForDomain<ddc::DiscreteDomain<Index...>>
{
    using type = TensorAccessor<Index...>;
};

} // namespace detail

template <class Dom>
using tensor_accessor_for_domain_t = detail::TensorAccessorForDomain<Dom>::type;

template <class... Index>
constexpr TensorAccessor<Index...>::TensorAccessor()
{
}

template <class... Index>
constexpr ddc::DiscreteDomain<Index...> TensorAccessor<Index...>::mem_domain()
{
    return ddc::DiscreteDomain<Index...>(
            ddc::DiscreteElement<Index...>(ddc::DiscreteElement<Index>(0)...),
            ddc::DiscreteVector<Index...>(ddc::DiscreteVector<Index>(Index::mem_size())...));
}

template <class... Index>
constexpr ddc::DiscreteDomain<Index...> TensorAccessor<Index...>::access_domain()
{
    return ddc::DiscreteDomain<Index...>(
            ddc::DiscreteElement<Index...>(ddc::DiscreteElement<Index>(0)...),
            ddc::DiscreteVector<Index...>(ddc::DiscreteVector<Index>(Index::access_size())...));
}

template <class... Index>
template <class... CDim>
constexpr ddc::DiscreteElement<Index...> TensorAccessor<Index...>::element()
{
    return ddc::DiscreteElement<Index...>(ddc::DiscreteElement<Index>(
            detail::access_id<Index, ddc::detail::TypeSeq<Index...>, CDim...>())...);
}

namespace detail {

// Helpers to handle memory access and processing for particular tensor structures (ie. eventual multiplication with -1 for antisymmetry or non-stored zeros)
template <class DDim>
struct IsTensorIndex
{
    using type = std::true_type; // TODO FIX
};

template <class... SubIndex>
struct IsTensorIndex<TensorNaturalIndex<SubIndex...>>
{
    using type = std::true_type;
};

template <class DDim>
static constexpr bool is_tensor_index_v = IsTensorIndex<DDim>::type::value;

template <class DDimInterest, class... DDim>
ddc::DiscreteElement<DDim...> replace_access_id_with_mem_id(ddc::DiscreteElement<DDim...> elem)
{
    return ddc::DiscreteElement<DDim...>(
            (std::is_same_v<DDimInterest, DDim> && detail::is_tensor_index_v<DDim>
                     ? DDim::access_id_to_mem_id(ddc::DiscreteElement<DDim>(elem).uid())
                     : ddc::DiscreteElement<DDim>(elem).uid())...);
}
template <
        class TensorField,
        class Element,
        class IndexHeadsTypeSeq,
        class IndexInterest,
        class... IndexTail>
struct Access;

template <class TensorField, class Element, class... IndexHead, class IndexInterest>
struct Access<TensorField, Element, ddc::detail::TypeSeq<IndexHead...>, IndexInterest>
{
    template <class Elem>
    static TensorField::element_type run(TensorField tensor_field, Elem const& elem)
    {
        if constexpr (detail::is_tensor_index_v<IndexInterest>) {
            return IndexInterest::template process_access<TensorField, Elem, IndexInterest>(
                    [](TensorField tensor_field_, Elem elem_) -> TensorField::element_type {
                        std::pair<std::vector<double>, std::vector<std::size_t>> mem_id
                                = IndexInterest::access_id_to_mem_id(
                                        elem_.template uid<IndexInterest>());

                        double tensor_field_value;
                        if (std::get<0>(mem_id).size() == 0) {
                            tensor_field_value
                                    = tensor_field_
                                              .mem(ddc::DiscreteElement<IndexHead...>(elem_),
                                                   ddc::DiscreteElement<IndexInterest>(
                                                           std::get<1>(mem_id)[0]));
                        } else {
                            tensor_field_value = 0.;
                            for (std::size_t i = 0; i < std::get<0>(mem_id).size(); ++i) {
                                tensor_field_value
                                        += std::get<0>(mem_id)[i]
                                           * tensor_field_
                                                     .mem(ddc::DiscreteElement<IndexHead...>(elem_),
                                                          ddc::DiscreteElement<IndexInterest>(
                                                                  std::get<1>(mem_id)[i]));
                            }
                        }

                        return tensor_field_value;
                    },
                    tensor_field,
                    elem);
        } else {
            return tensor_field(elem);
        }
    }
};

template <
        class TensorField,
        class Element,
        class... IndexHead,
        class IndexInterest,
        class... IndexTail>
struct Access<TensorField, Element, ddc::detail::TypeSeq<IndexHead...>, IndexInterest, IndexTail...>
{
    template <class Elem>
    static TensorField::element_type run(TensorField tensor_field, Elem const& elem)
    {
        if constexpr (detail::is_tensor_index_v<IndexInterest>) {
            return IndexInterest::template process_access<TensorField, Elem, IndexInterest>(
                    [](TensorField tensor_field_, Elem elem_) -> TensorField::element_type {
                        // TODO probably too simple
                        return Access<
                                TensorField,
                                Element,
                                ddc::detail::TypeSeq<IndexHead..., IndexInterest>,
                                IndexTail...>::run(tensor_field_, elem_);
                    },
                    tensor_field,
                    elem);
        } else {
            return Access<
                    TensorField,
                    Element,
                    ddc::detail::TypeSeq<IndexHead..., IndexInterest>,
                    IndexTail...>::run(tensor_field, elem);
        }
    }
};

// Functor for memory element access (if defined)
template <typename InterestDim>
struct LambdaMemElem
{
    static ddc::DiscreteElement<InterestDim> run(ddc::DiscreteElement<InterestDim> elem)
    {
        return elem;
    }
};

template <typename InterestDim>
requires detail::is_tensor_index_v<InterestDim>
struct LambdaMemElem<InterestDim>
{
    static ddc::DiscreteElement<InterestDim> run(ddc::DiscreteElement<InterestDim> elem)
    {
        // TODO static_assert unique mem_id
        return ddc::DiscreteElement<InterestDim>(std::get<1>(InterestDim::access_id_to_mem_id(
                ddc::DiscreteElement<InterestDim>(elem).uid()))[0]);
    }
};

} // namespace detail

template <class ElementType, class SupportType, class LayoutStridedPolicy, class MemorySpace>
class Tensor
{
};

} // namespace tensor

} // namespace sil

namespace ddc {

template <class ElementType, class SupportType, class LayoutStridedPolicy, class MemorySpace>
inline constexpr bool enable_chunk<
        sil::tensor::Tensor<ElementType, SupportType, LayoutStridedPolicy, MemorySpace>> = true;

template <class ElementType, class SupportType, class LayoutStridedPolicy, class MemorySpace>
inline constexpr bool enable_borrowed_chunk<
        sil::tensor::Tensor<ElementType, SupportType, LayoutStridedPolicy, MemorySpace>> = true;

} // namespace ddc

namespace sil {

namespace tensor {

/**
 * Tensor class
 */
template <class ElementType, class... DDim, class LayoutStridedPolicy, class MemorySpace>
class Tensor<ElementType, ddc::DiscreteDomain<DDim...>, LayoutStridedPolicy, MemorySpace>
    : public ddc::
              ChunkSpan<ElementType, ddc::DiscreteDomain<DDim...>, LayoutStridedPolicy, MemorySpace>
{
protected:
    using base_type = ddc::
            ChunkSpan<ElementType, ddc::DiscreteDomain<DDim...>, LayoutStridedPolicy, MemorySpace>;

public:
    using ddc::
            ChunkSpan<ElementType, ddc::DiscreteDomain<DDim...>, LayoutStridedPolicy, MemorySpace>::
                    ChunkSpan;
    using reference = ddc::
            ChunkSpan<ElementType, ddc::DiscreteDomain<DDim...>, LayoutStridedPolicy, MemorySpace>::
                    reference;

    using ddc::
            ChunkSpan<ElementType, ddc::DiscreteDomain<DDim...>, LayoutStridedPolicy, MemorySpace>::
            operator();

    KOKKOS_FUNCTION constexpr explicit Tensor(ddc::ChunkSpan<
                                              ElementType,
                                              ddc::DiscreteDomain<DDim...>,
                                              LayoutStridedPolicy,
                                              MemorySpace> other) noexcept
        : base_type(other)
    {
    }

    static constexpr TensorAccessor<DDim...> accessor()
    {
        return TensorAccessor<DDim...>();
    }

    template <class... DElems>
    KOKKOS_FUNCTION ElementType get(DElems const&... delems) const noexcept
    {
        return detail::Access<
                Tensor<ElementType,
                       ddc::DiscreteDomain<DDim...>,
                       std::experimental::layout_right,
                       MemorySpace>,
                ddc::DiscreteElement<DDim...>,
                ddc::detail::TypeSeq<>,
                DDim...>::run(*this, ddc::DiscreteElement(delems...));
    }

    template <class... DElems>
    KOKKOS_FUNCTION constexpr reference mem(DElems const&... delems) const noexcept
    {
        return ddc::ChunkSpan<
                ElementType,
                ddc::DiscreteDomain<DDim...>,
                LayoutStridedPolicy,
                MemorySpace>::operator()(delems...);
    }

    template <class... DElems>
    KOKKOS_FUNCTION constexpr reference operator()(DElems const&... delems) const noexcept
    {
        // TODO static_assert unique mem_id
        return ddc::ChunkSpan<
                ElementType,
                ddc::DiscreteDomain<DDim...>,
                LayoutStridedPolicy,
                MemorySpace>::
        operator()(ddc::DiscreteElement<DDim...>(
                detail::LambdaMemElem<DDim>::run(ddc::DiscreteElement<DDim>(delems...))...));
    }

    template <class... ODDim>
    KOKKOS_FUNCTION constexpr auto operator[](
            ddc::DiscreteElement<ODDim...> const& slice_spec) const noexcept
    {
        // TODO static_assert unique mem_id
        return Tensor<
                ElementType,
                ddc::detail::convert_type_seq_to_discrete_domain_t<ddc::type_seq_remove_t<
                        ddc::detail::TypeSeq<DDim...>,
                        ddc::detail::TypeSeq<ODDim...>>>,
                LayoutStridedPolicy,
                MemorySpace>(ddc::ChunkSpan<
                             ElementType,
                             ddc::DiscreteDomain<DDim...>,
                             LayoutStridedPolicy,
                             MemorySpace>::
                             operator[](ddc::DiscreteElement<ODDim...>(
                                     detail::LambdaMemElem<ODDim>::run(slice_spec)...)));
    }

    void apply_lambda(std::function<
                      void(Tensor<ElementType,
                                  ddc::DiscreteDomain<DDim...>,
                                  LayoutStridedPolicy,
                                  MemorySpace>,
                           ddc::DiscreteElement<DDim...>)> lambda_func)
    {
        ddc::for_each(this->domain(), [&](ddc::DiscreteElement<DDim...> elem) {
            lambda_func(*this, elem);
        });
    }

    Tensor<ElementType, ddc::DiscreteDomain<DDim...>, LayoutStridedPolicy, MemorySpace>& operator+=(
            const Tensor<
                    ElementType,
                    ddc::DiscreteDomain<DDim...>,
                    LayoutStridedPolicy,
                    MemorySpace>& tensor)
    {
        ddc::for_each(this->domain(), [&](ddc::DiscreteElement<DDim...> elem) {
            (*this)(elem) += tensor(elem);
        });
        return *this;
    }

    Tensor<ElementType, ddc::DiscreteDomain<DDim...>, LayoutStridedPolicy, MemorySpace>& operator*=(
            const ElementType scalar)
    {
        ddc::for_each(this->domain(), [&](ddc::DiscreteElement<DDim...> elem) {
            (*this)(elem) *= scalar;
        });
        return *this;
    }
};

// Sum of tensors
template <
        class... DDim,
        class ElementType,
        class LayoutStridedPolicy,
        class MemorySpace,
        class... TensorType>
Tensor<ElementType,
       ddc::DiscreteDomain<DDim...>,
       std::experimental::layout_right,
       Kokkos::DefaultHostExecutionSpace::memory_space>
tensor_sum(
        Tensor<ElementType,
               ddc::DiscreteDomain<DDim...>,
               std::experimental::layout_right,
               Kokkos::DefaultHostExecutionSpace::memory_space> sum_tensor,
        TensorType... tensor)
{
    ddc::for_each(sum_tensor.domain(), [&](ddc::DiscreteElement<DDim...> elem) {
        sum_tensor(elem) = (tensor(elem) + ...);
    });
    return sum_tensor;
}

namespace detail {

// Domain of a tensor result of product between two tensors
template <class Dom1, class Dom2>
struct NaturalTensorProdDomain;

template <class... DDim1, class... DDim2>
struct NaturalTensorProdDomain<ddc::DiscreteDomain<DDim1...>, ddc::DiscreteDomain<DDim2...>>
{
    using type = ddc::detail::convert_type_seq_to_discrete_domain_t<ddc::type_seq_merge_t<
            ddc::type_seq_remove_t<ddc::detail::TypeSeq<DDim1...>, ddc::detail::TypeSeq<DDim2...>>,
            ddc::type_seq_remove_t<
                    ddc::detail::TypeSeq<DDim2...>,
                    ddc::detail::TypeSeq<DDim1...>>>>;
};

} // namespace detail

template <class Dom1, class Dom2>
using natural_tensor_prod_domain_t = detail::NaturalTensorProdDomain<Dom1, Dom2>::type;

template <class Tensor1, class Tensor2>
natural_tensor_prod_domain_t<
        typename Tensor1::discrete_domain_type,
        typename Tensor2::discrete_domain_type>
natural_tensor_prod_domain(Tensor1 tensor1, Tensor2 tensor2)
{
    return natural_tensor_prod_domain_t<
            typename Tensor1::discrete_domain_type,
            typename Tensor2::discrete_domain_type>(tensor1.domain(), tensor2.domain());
}

namespace detail {

// Product between two tensors naturally indexed.
template <class HeadDDim1TypeSeq, class ContractDDimTypeSeq, class TailDDim2TypeSeq>
struct NaturalTensorProd;

template <class... HeadDDim1, class... ContractDDim, class... TailDDim2>
struct NaturalTensorProd<
        ddc::detail::TypeSeq<HeadDDim1...>,
        ddc::detail::TypeSeq<ContractDDim...>,
        ddc::detail::TypeSeq<TailDDim2...>>
{
    template <class ElementType, class LayoutStridedPolicy, class MemorySpace>
    static Tensor<
            ElementType,
            ddc::DiscreteDomain<HeadDDim1..., TailDDim2...>,
            LayoutStridedPolicy,
            MemorySpace>
    run(Tensor<ElementType,
               ddc::DiscreteDomain<HeadDDim1..., TailDDim2...>,
               std::experimental::layout_right,
               Kokkos::DefaultHostExecutionSpace::memory_space> prod_tensor,
        Tensor<ElementType,
               ddc::DiscreteDomain<HeadDDim1..., ContractDDim...>,
               LayoutStridedPolicy,
               MemorySpace> tensor1,
        Tensor<ElementType,
               ddc::DiscreteDomain<ContractDDim..., TailDDim2...>,
               LayoutStridedPolicy,
               MemorySpace> tensor2)
    {
        ddc::for_each(
                prod_tensor.domain(),
                [&](ddc::DiscreteElement<HeadDDim1..., TailDDim2...> elem) {
                    prod_tensor(elem) = ddc::transform_reduce(
                            tensor1.template domain<ContractDDim...>(),
                            0.,
                            ddc::reducer::sum<ElementType>(),
                            [&](ddc::DiscreteElement<ContractDDim...> contract_elem) {
                                return tensor1(ddc::select<HeadDDim1...>(elem), contract_elem)
                                       * tensor2(ddc::select<TailDDim2...>(elem), contract_elem);
                            });
                });
        return prod_tensor;
    }
};

} // namespace detail

template <
        class... ProdDDim,
        class... DDim1,
        class... DDim2,
        class ElementType,
        class LayoutStridedPolicy,
        class MemorySpace>
Tensor<ElementType,
       ddc::DiscreteDomain<ProdDDim...>,
       std::experimental::layout_right,
       Kokkos::DefaultHostExecutionSpace::memory_space>
natural_tensor_prod(
        Tensor<ElementType,
               ddc::DiscreteDomain<ProdDDim...>,
               std::experimental::layout_right,
               Kokkos::DefaultHostExecutionSpace::memory_space> prod_tensor,
        Tensor<ElementType, ddc::DiscreteDomain<DDim1...>, LayoutStridedPolicy, MemorySpace>
                tensor1,
        Tensor<ElementType, ddc::DiscreteDomain<DDim2...>, LayoutStridedPolicy, MemorySpace>
                tensor2)
{
    static_assert(std::is_same_v<
                  ddc::type_seq_remove_t<
                          ddc::detail::TypeSeq<DDim1...>,
                          ddc::detail::TypeSeq<ProdDDim...>>,
                  ddc::type_seq_remove_t<
                          ddc::detail::TypeSeq<DDim2...>,
                          ddc::detail::TypeSeq<ProdDDim...>>>);
    return detail::NaturalTensorProd<
            ddc::type_seq_remove_t<
                    ddc::detail::TypeSeq<ProdDDim...>,
                    ddc::detail::TypeSeq<DDim2...>>,
            ddc::type_seq_remove_t<
                    ddc::detail::TypeSeq<DDim1...>,
                    ddc::detail::TypeSeq<ProdDDim...>>,
            ddc::type_seq_remove_t<
                    ddc::detail::TypeSeq<ProdDDim...>,
                    ddc::detail::TypeSeq<DDim1...>>>::run(prod_tensor, tensor1, tensor2);
}

namespace detail {

template <class HeadDom, class InterestDom, class TailDom>
struct PrintTensor;

template <class... HeadDDim, class InterestDDim, class HeadOfTailDDim, class... TailOfTailDDim>
struct PrintTensor<
        ddc::DiscreteDomain<HeadDDim...>,
        ddc::DiscreteDomain<InterestDDim>,
        ddc::DiscreteDomain<HeadOfTailDDim, TailOfTailDDim...>>
{
    template <class ElementType, class LayoutStridedPolicy, class MemorySpace>
    static std::string run(
            std::string& str,
            Tensor<ElementType,
                   ddc::DiscreteDomain<
                           HeadDDim...,
                           InterestDDim,
                           HeadOfTailDDim,
                           TailOfTailDDim...>,
                   LayoutStridedPolicy,
                   MemorySpace> const& tensor,
            ddc::DiscreteElement<HeadDDim...> i)
    {
        str += "[";
        for (ddc::DiscreteElement<InterestDDim> elem :
             ddc::DiscreteDomain<InterestDDim>(tensor.domain())) {
            str = PrintTensor<
                    ddc::DiscreteDomain<HeadDDim..., InterestDDim>,
                    ddc::DiscreteDomain<HeadOfTailDDim>,
                    ddc::DiscreteDomain<TailOfTailDDim...>>::
                    run(str, tensor, ddc::DiscreteElement<HeadDDim..., InterestDDim>(i, elem));
        }
        str += "]\n";
        return str;
    }
};

template <class... HeadDDim, class InterestDDim>
struct PrintTensor<
        ddc::DiscreteDomain<HeadDDim...>,
        ddc::DiscreteDomain<InterestDDim>,
        ddc::DiscreteDomain<>>
{
    template <class ElementType, class LayoutStridedPolicy, class MemorySpace>
    static std::string run(
            std::string& str,
            Tensor<ElementType,
                   ddc::DiscreteDomain<HeadDDim..., InterestDDim>,
                   LayoutStridedPolicy,
                   MemorySpace> const& tensor,
            ddc::DiscreteElement<HeadDDim...> i)
    {
        for (ddc::DiscreteElement<InterestDDim> elem :
             ddc::DiscreteDomain<InterestDDim>(tensor.domain())) {
            str = str + " "
                  + std::to_string(
                          tensor(ddc::DiscreteElement<HeadDDim..., InterestDDim>(i, elem)));
        }
        str += "\n";
        return str;
    }
};

} // namespace detail

template <
        class ElementType,
        class HeadDDim,
        class... TailDDim,
        class LayoutStridedPolicy,
        class MemorySpace>
std::ostream& operator<<(
        std::ostream& os,
        Tensor<ElementType,
               ddc::DiscreteDomain<HeadDDim, TailDDim...>,
               LayoutStridedPolicy,
               MemorySpace> const& tensor)
{
    std::string str = "";
    os << detail::PrintTensor<
            ddc::DiscreteDomain<>,
            ddc::DiscreteDomain<HeadDDim>,
            ddc::DiscreteDomain<TailDDim...>>::run(str, tensor, ddc::DiscreteElement<>());
    return os;
}

} // namespace tensor

} // namespace sil
