// SPDX-FileCopyrightText: 2024 Baptiste Legouix
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <ddc/ddc.hpp>

#include "are_all_same.hpp"
#include "chain.hpp"
#include "specialization.hpp"

namespace sil {

namespace form {

/// Cochain class
template <class SimplexType, class ElementType = double>
class Cosimplex
{
public:
    using simplex_type = SimplexType;
    using element_type = ElementType;
    using elem_type = SimplexType::elem_type;
    using vect_type = SimplexType::vect_type;

private:
    SimplexType m_simplex;
    ElementType m_value;

public:
    KOKKOS_FUNCTION constexpr explicit Cosimplex(SimplexType simplex, ElementType value) noexcept
        : m_simplex(simplex)
        , m_value(value)
    {
    }

    static KOKKOS_FUNCTION constexpr std::size_t dimension() noexcept
    {
        return SimplexType::dimension();
    }

    KOKKOS_FUNCTION SimplexType simplex() const noexcept
    {
        return m_simplex;
    }

    KOKKOS_FUNCTION element_type operator()()
    {
        return m_value;
    }
};

template <misc::Specialization<Cosimplex> CosimplexType>
std::ostream& operator<<(std::ostream& out, CosimplexType const& cosimplex)
{
    out << " " << cosimplex.simplex() << ": " << cosimplex() << "\n";
    return out;
}

/*
template <
        misc::NotSpecialization<Chain> SimplexType,
        class ElementType = double,
        class=void>
using Form = Cosimplex<SimplexType, ElementType>;
*/

} // namespace form

} // namespace sil
