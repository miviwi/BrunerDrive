#pragma once

#include <types.h>

#include <type_traits>
#include <algorithm>

namespace brdrive {

// Dynamically sized view into a range
//   of a number's bits
// Heavily inspired by byuu's 'nall' library (nall::BitRange)
template <size_t NumBits>
struct BitRange {
  static_assert(NumBits >= 1 && NumBits <= 64, "Invalid size specified!");

  static constexpr size_t Bits = NumBits;

  // Type of the target number
  using Type = std::conditional_t<
      Bits <= 8,  u8,  std::conditional_t<
      Bits <= 16, u16, std::conditional_t<
      Bits <= 32, u32, std::conditional_t<
      Bits <= 64, u64,
    void>>>>;

  template <typename T>
  BitRange(T *target, int index) :
    target_(*(Type *)target)
  {
    static_assert(sizeof(T) == sizeof(Type), "sanity check failed!");

    // Negative index indicates we're indexing from the RIGHT
    if(index < 0) index = NumBits + index;

    mask_  = 1ull << index;
    shift_ = index;
  }

  template <typename T>
  BitRange(T *target, int lo, int hi) :
    target_(*(Type *)target)
  {
    static_assert(sizeof(T) == sizeof(Type), "sanity check failed!");

    // Negative lo/hi indicates we're indexing from the RIGHT
    if(lo < 0) lo = NumBits + lo;
    if(hi < 0) hi = NumBits + hi;

    // The range could get flipped when converting from
    //   right-based indices to left-based ones
    if(lo > hi) std::swap(lo, hi);

    auto range_width = hi-lo + 1;
    auto base_mask   = (1ull << range_width)-1;

    mask_  = base_mask << lo;
    shift_ = lo;
  }

  BitRange(const BitRange&) = delete;

  auto get() const -> Type { return (target_ & mask_) >> shift_; }
  operator Type() const { return get(); }

  auto get() -> Type { return (target_ & mask_) >> shift_; }
  operator Type() { return get(); }

  auto& operator=(const BitRange& source)
  {
    auto source_bits = (source.target_ & source.mask_) >> source.shift_;

    target_ = bitsOutside() | shiftAndMask(source_bits);

    return *this;
  }

  auto operator++(int)
  {
    auto v = get();
    target_ = bitsOutside() | target_ + shiftAndMask(1);

    return v;
  }
  auto operator--(int)
  {
    auto v = get();
    target_ = bitsOutside() | target_ - shiftAndMask(1);

    return v;
  }
  auto& operator++()
  {
    target_ = bitsOutside() | target_ + shiftAndMask(1);
    return *this;
  }
  auto operator--()
  {
    target_ = bitsOutside() | target_ - shiftAndMask(1);
    return *this;
  }

  template <typename T>
  auto& operator=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(source);
    return *this;
  }
  template <typename T>
  auto& operator+=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() + source);
    return *this;
  }
  template <typename T>
  auto& operator-=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() - source);
    return *this;
  }
  template <typename T>
  auto& operator*=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() * source);
    return *this;
  }
  template <typename T>
  auto& operator/=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() / source);
    return *this;
  }
  template <typename T>
  auto& operator%=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() % source);
    return *this;
  }
  template <typename T>
  auto& operator<<=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() << source);
    return *this;
  }
  template <typename T>
  auto& operator>>=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() >> source);
    return *this;
  }
  template <typename T>
  auto& operator&=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() & source);
    return *this;
  }
  template <typename T>
  auto& operator|=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() | source);
    return *this;
  }
  template <typename T>
  auto& operator^=(const T& source)
  {
    target_ = bitsOutside() | shiftAndMask(get() ^ source);
    return *this;
  }

private:
  auto bitsOutside() const { return target_ & ~mask_; }
  auto shiftAndMask(Type v) const { return (v << shift_) & mask_; }

  Type& target_;
  Type mask_;
  unsigned shift_;
};

}
