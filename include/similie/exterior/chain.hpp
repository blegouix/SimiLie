// SPDX-FileCopyrightText: 2024 Baptiste Legouix
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <ddc/ddc.hpp>

#include <similie/misc/are_all_same.hpp>
#include <similie/misc/specialization.hpp>

#include <Kokkos_StdAlgorithms.hpp>

#include "simplex.hpp"

namespace sil {

namespace exterior {

/// Chain class
template <class SimplexType, class ExecSpace = Kokkos::DefaultHostExecutionSpace>
class Chain
{
public:
    using memory_space = typename ExecSpace::memory_space;

    using simplex_type = SimplexType;
    using simplices_type = Kokkos::<SimplexType*, memory_space>;
    using discrete_element_type = typename simplex_type::discrete_element_type;
    using discrete_vector_type = typename simplex_type::discrete_vector_type;

private:
    static constexpr bool s_is_local = false;
    static constexpr std::size_t s_k = simplex_type::dimension();
    simplices_type m_simplices;

public:
    KOKKOS_FUNCTION constexpr explicit Chain() noexcept : m_simplices {} {}

    template <class... T>
        requires misc::are_all_same<T...>
    KOKKOS_FUNCTION constexpr explicit Chain(T... simplex) noexcept : m_simplices {simplex...}
    {
        assert(check() == 0 && "there are duplicate simplices in the chain");
    }

    KOKKOS_FUNCTION constexpr explicit Chain(
            Kokkos::<simplex_type*, memory_space>simplices) noexcept
        : m_simplices(simplices)
    {
        assert(check() == 0 && "there are duplicate simplices in the chain");
    }

    static KOKKOS_FUNCTION constexpr bool is_local() noexcept
    {
        return s_is_local;
    }

    static KOKKOS_FUNCTION constexpr std::size_t dimension() noexcept
    {
        return s_k;
    }

    KOKKOS_FUNCTION std::size_t size() noexcept
    {
        return m_simplices.size();
    }

    KOKKOS_FUNCTION std::size_t const size() const noexcept
    {
        return m_simplices.size();
    }

    KOKKOS_FUNCTION int check()
    {
        for (auto i = Kokkos::experimental::begin(m_simplices);
             i < Kokkos::experimental::end(m_simplices) - 1;
             ++i) {
            for (auto j = i + 1; j < Kokkos::experimental::end(m_simplices); ++j) {
                if (*i == *j) {
                    return -1;
                }
            }
        }
        return 0;
    }

    KOKKOS_FUNCTION void optimize()
    {
        auto simplices_host = Kokkos::create_mirror_view_and_copy(Kokkos::HostSpace(), m_simplices);
        std::vector<simplex_type> simplices_vect(
                simplices_host.data(),
                simplices_host.data() + simplices_host.extent(0));
        for (auto i = simplices_vect.begin(); i < simplices_vect.end() - 1; ++i) {
            auto k = i;
            for (auto j = i + 1; k == i && j < simplices_vect.end(); ++j) {
                if (*i == -*j) {
                    k = j;
                }
            }
            if (k != i) {
                simplices_vect.erase(k);
                simplices_vect.erase(i--);
            }
        }
        Kokkos::<simplex_type*, Kokkos::HostSpace>new_simplices_host(
                simplices_vect.data(),
                simplices_vect.data() + simplices_vect.size());
        m_simplices = Kokkos::create_mirror_view_and_copy(memory_space, new_simplices_host);
        assert(check() == 0 && "there are duplicate simplices in the chain");
    }

    KOKKOS_FUNCTION auto begin()
    {
        return Kokkos::experimental::begin(m_simplices);
    }

    KOKKOS_FUNCTION auto begin() const
    {
        return Kokkos::experimental::begin(m_simplices);
    }

    KOKKOS_FUNCTION auto end()
    {
        return Kokkos::experimental::end(m_simplices);
    }

    KOKKOS_FUNCTION auto end() const
    {
        return Kokkos::experimental::end(m_simplices);
    }

    KOKKOS_FUNCTION auto cbegin() const
    {
        return Kokkos::experimental::cbegin(m_simplices);
    }

    KOKKOS_FUNCTION auto cend() const
    {
        return Kokkos::experimental::cend(m_simplices);
    }

    KOKKOS_FUNCTION simplex_type& operator[](std::size_t i) noexcept
    {
        return m_simplices[i];
    }

    KOKKOS_FUNCTION simplex_type const& operator[](std::size_t i) const noexcept
    {
        return m_simplices[i];
    }

    KOKKOS_FUNCTION void push_back(const simplex_type& simplex)
    {
        m_simplices.resize(m_simplices.size() + 1);
        m_simplices[m_simplices.size()] = simplex;
    }

    KOKKOS_FUNCTION void push_back(const Chain<simplex_type>& simplices_to_add)
    {
        std::size_t old_size = m_simplices.size();
        m_simplices.resize(old_size + simplices_to_add.size());
        for (auto i = simplices_to_add.begin(); i < simplices_to_add.end(); ++i) {
            m_simplices[old_size + Kokkos::distance(simplices_to_add.begin(), i)] = *i;
        }
    }

    KOKKOS_FUNCTION Chain<simplex_type> operator-()
    {
        simplices_type simplices = m_simplices;
        for (auto i = Kokkos::experimental::begin(simplices);
             i < Kokkos::experimental::end(simplices) - 1;
             ++i) {
            *i = -*i;
        }
        return Chain<simplex_type>(simplices);
    }

    KOKKOS_FUNCTION Chain<simplex_type> operator+(simplex_type simplex)
    {
        simplices_type simplices = m_simplices;
        simplices.push_back(simplex);
        return Chain<simplex_type>(simplices);
    }

    KOKKOS_FUNCTION Chain<simplex_type> operator+(Chain<simplex_type> simplices_to_add)
    {
        simplices_type simplices = m_simplices;
        simplices.push_back(simplices_to_add);
        return Chain<simplex_type>(simplices);
    }

    template <class T>
    KOKKOS_FUNCTION auto operator-(T t)
    {
        return *this + (-t);
    }

    template <class T>
    KOKKOS_FUNCTION auto operator*(T t)
    {
        if (t == 1) {
            return *this;
        } else if (t == -1) {
            return -*this;
        } else {
            assert(false && "chain must be multiplied  by 1 or -1");
            return *this;
        }
    }

    KOKKOS_FUNCTION bool operator==(Chain<simplex_type> simplices)
    {
        for (auto i = simplices.begin(); i < simplices.end(); ++i) {
            if (*i != m_simplices[Kokkos::distance(simplices.begin(), i)]) {
                return false;
            }
        }
        return true;
    }
};

template <class Head, class... Tail>
Chain(Head, Tail...) -> Chain<Head>;

template <misc::Specialization<Chain> ChainType>
std::ostream& operator<<(std::ostream& out, ChainType const& chain)
{
    out << "[\n";
    for (typename ChainType::simplex_type const& simplex : chain) {
        out << " " << simplex << "\n";
    }
    out << "]";
    return out;
}

} // namespace exterior

} // namespace sil
