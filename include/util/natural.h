#pragma once

#include <types.h>
#include <util/bit.h>

#include <type_traits>
#include <algorithm>

namespace brdrive {

// Forward declaration
template <size_t NumBits>
struct Integer;

// Unsigned arbitrary-width integer
//
// Heavily inspired by byuu's 'nall' library (nall::Integer)
template <size_t NumBits>
struct Natural {
  static_assert(NumBits >= 1 && NumBits <= 64, "Invalid size specified!");

  static constexpr size_t Bits = NumBits;

  // Underlying native type used for storage
  using StorageType = std::conditional_t<
      Bits <= 8,  u8,  std::conditional_t<
      Bits <= 16, u16, std::conditional_t<
      Bits <= 32, u32, std::conditional_t<
      Bits <= 64, u64,
    void>>>>;

  enum : StorageType {
    Mask = ~0ull >> (64 - NumBits),
    Sign = 1ull << (NumBits-1ull),
  };

  Natural() : data_(0) { }
  template <size_t OtherBits> Natural(Natural<OtherBits> other) : data_(cast(other.data_)) { }
  template <typename T> Natural(const T& other) : data_(cast(other)) { }

  auto get() const -> StorageType { return data_; }
  operator StorageType() const { return get(); }

  auto get() -> StorageType { return data_; }
  operator StorageType() { return get(); }

  auto operator++(int)
  {
    auto v = *this;
    data_ = cast(data_ + 1);

    return v;
  }
  auto operator--(int)
  {
    auto v = *this;
    data_ = cast(data_ - 1);

    return v;
  }

  auto& operator++() { data_ = cast(data_ + 1); return *this; }
  auto& operator--() { data_ = cast(data_ - 1); return *this; }

  template <typename T>
  auto& operator=(const T& v)
  {
    data_ = cast(v);
    return *this;
  }
  template <typename T>
  auto& operator+=(const T& v)
  {
    data_ = cast(data_ + v);
    return *this;
  }
  template <typename T>
  auto& operator-=(const T& v)
  {
    data_ = cast(data_ - v);
    return *this;
  }
  template <typename T>
  auto& operator*=(const T& v)
  {
    data_ = cast(data_ * v);
    return *this;
  }
  template <typename T>
  auto& operator/=(const T& v)
  {
    data_ = cast(data_ / v);
    return *this;
  }
  template <typename T>
  auto& operator%=(const T& v)
  {
    data_ = cast(data_ % v);
    return *this;
  }
  template <typename T>
  auto& operator<<=(const T& v)
  {
    data_ = cast(data_ << v);
    return *this;
  }
  template <typename T>
  auto& operator>>=(const T& v)
  {
    data_ = cast(data_ >> v);
    return *this;
  }
  template <typename T>
  auto& operator&=(const T& v)
  {
    data_ = cast(data_ & v);
    return *this;
  }
  template <typename T>
  auto& operator|=(const T& v)
  {
    data_ = cast(data_ | v);
    return *this;
  }
  template <typename T>
  auto& operator^=(const T& v)
  {
    data_ = cast(data_ ^ v);
    return *this;
  }

  auto bit(int bit_index) -> BitRange<NumBits> { return { &data_, bit_index }; }
  auto bit(int bit_index) const -> const BitRange<NumBits> { return { &data_, bit_index }; }

  auto bit(int lo, int hi) -> BitRange<NumBits> { return { &data_, lo, hi }; }
  auto bit(int lo, int hi) const -> const BitRange<NumBits> { return { &data_, lo, hi }; }

  auto operator()(int bit_index) -> BitRange<NumBits> { return bit(bit_index); }
  auto operator()(int bit_index) const -> const BitRange<NumBits> { return bit(bit_index); }

  auto operator()(int lo, int hi) -> BitRange<NumBits> { return bit(lo, hi); }
  auto operator()(int lo, int hi) const -> const BitRange<NumBits> { return bit(lo, hi); }

  auto byte(int index) -> BitRange<NumBits> { return { &data_, index*8 + 0, index*8 + 7 }; }
  auto byte(int index) const -> const BitRange<NumBits> { return { &data_, index*8 + 0, index*8 + 7 }; }

  auto slice(int bit_index) const;
  auto slice(int lo, int hi) const;

  auto clamp(unsigned bits) const -> StorageType
  {
    const i64 b = (1ull << bits) - 1;
    const i64 m = b - 1;

    return std::min(m, std::max(-b, data_));
  }

  auto clip(unsigned bits) const -> StorageType
  {
    const i64 b = (1ull << bits) - 1;
    const i64 m = b*2 - 1;

    return (data_ & m ^ b) - b;
  }

  auto toInteger() const -> Integer<NumBits>;

private:
  // Down/upcast a value to a width of 'Bits'
  //   WITHOUT performing ign extension
  static auto cast(StorageType val) -> StorageType
  {
    return val & Mask;
  }

  StorageType data_;

};

}
