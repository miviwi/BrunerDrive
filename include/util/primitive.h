#pragma once

#include <util/natural.h>
#include <util/integer.h>

namespace brdrive {

// Specializations with default width arguments
template <size_t NumBits = 64> struct Natural;
template <size_t NumBits = 64> struct Integer;

template <size_t NumBits>
auto Natural<NumBits>::slice(int bit_index) const  { return Natural<>{ bit(bit_index) }; }
template <size_t NumBits>
auto Natural<NumBits>::slice(int lo, int hi) const { return Natural<>{ bit(lo, hi) }; }

template <size_t NumBits>
auto Integer<NumBits>::slice(int bit_index) const  { return Natural<>{ bit(bit_index) }; }
template <size_t NumBits>
auto Integer<NumBits>::slice(int lo, int hi) const { return Natural<>{ bit(lo, hi) }; }

template <size_t NumBits>
auto Natural<NumBits>::toInteger() const -> Integer<NumBits>
{
  return Integer<NumBits>(*this);
}
template <size_t NumBits>
auto Integer<NumBits>::toNatural() const -> Natural<NumBits>
{
  return Integer<NumBits>(*this);
}

}
