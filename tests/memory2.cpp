/*
    tests/memory2.cpp -- tests for more complex memory operations

    Enoki is a C++ template library that enables transparent vectorization
    of numerical kernels using SIMD instruction sets available on current
    processor architectures.

    Copyright (c) 2018 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a BSD-style
    license that can be found in the LICENSE file.
*/

#include "test.h"
#include <enoki/matrix.h>

ENOKI_TEST_ALL(test01_extract) {
    auto idx = index_sequence<T>();
    for (size_t i = 0; i < Size; ++i)
        assert(extract(idx, eq(idx, Value(i))) == Value(i));
}

ENOKI_TEST_ALL(test02_compress) {
    alignas(alignof(T)) Value tmp[T::ActualSize];
    auto value = index_sequence<T>();
    Value *tmp2 = tmp;
    compress(tmp2, value, value >= Value(2));
    for (int i = 0; i < int(Size) - 2; ++i)
        assert(tmp[i] == Value(2 + i));
    assert(int(tmp2 - tmp) == std::max(0, int(Size) - 2));
}

ENOKI_TEST_ALL(test03_transform) {
    Value tmp[T::ActualSize] = { 0 };
    auto index = index_sequence<uint_array_t<T>>();
    auto index2 = uint_array_t<T>(0u);

    transform<T>(tmp, index, [](auto& value) { value += Value(1); });
    transform<T>(tmp, index, mask_t<T>(false), [](auto& value) { value += Value(1); });

    transform<T>(tmp, index2, [](auto& value) { value += Value(1); });
    transform<T>(tmp, index2, mask_t<T>(false), [](auto& value) { value += Value(1); });

    assert(tmp[0] == Size + 1);
    for (size_t i = 1; i < Size; ++i) {
        assert(tmp[i] == 1);
    }
}

#if defined(_MSC_VER)
  template <typename T, std::enable_if_t<T::Size == 8, int> = 0>
#else
  template <typename T, std::enable_if_t<T::Size != 31, int> = 0>
#endif
void test04_nested_gather_impl() {
    using Value = value_t<T>;
    using UInt32P = Packet<uint32_t, T::Size>;
    using Vector3 = Array<Value, 3>;
    using Matrix3 = Matrix<Value, 3>;
    using Matrix3P = Matrix<T, 3>;
    using Array33 = Array<Vector3, 3>;

    Matrix3 x[32];
    for (size_t i = 0; i<32; ++i)
        for (size_t k = 0; k<Matrix3::Size; ++k)
            for (size_t j = 0; j<Matrix3::Size; ++j)
                x[i /* slice */ ][k /* col */ ][j /* row */ ] = Value(i*100+k*10+j);

    for (size_t i = 0; i<32; ++i) {
        /* Direct load */
        Matrix3 q = gather<Matrix3>(x, i);
        Matrix3 q2 = Array33(Matrix3(
            0, 10, 20,
            1, 11, 21,
            2, 12, 22
        )) + Value(100*i);
        assert(q == q2);
    }

    /* Variant 2 */
    for (size_t i = 0; i < 32; ++i)
        assert(gather<Matrix3>(x, i) == x[i]);

    /* Nested gather + slice */
    auto q = gather<Matrix3P>(x, index_sequence<UInt32P>());
    for (size_t i = 0; i < T::Size; ++i)
        assert(slice(q, i) == x[i]);

    /* Masked nested gather */
    q = gather<Matrix3P>(x, index_sequence<UInt32P>(), index_sequence<UInt32P>() < 2u);
    for (size_t i = 0; i < T::Size; ++i) {
        if (i < 2u)
            assert(slice(q, i) == x[i]);
        else
            assert(slice(q, i) == zero<Matrix3>());
    }

    /* Nested scatter */
    q = gather<Matrix3P>(x, index_sequence<UInt32P>());
    scatter(x, q + fill<Matrix3>(Value(1000)), index_sequence<UInt32P>());
    scatter(x, q + fill<Matrix3>(Value(2000)), index_sequence<UInt32P>(), index_sequence<UInt32P>() < 2u);
    for (size_t i = 0; i < T::Size; ++i) {
        if (i < 2u)
            assert(slice(q, i) + fill<Matrix3>(Value(2000)) == x[i]);
        else
            assert(slice(q, i) + fill<Matrix3>(Value(1000)) == x[i]);
    }

    /* Nested prefetch -- doesn't do anything here, let's just make sure it compiles */
    prefetch<Matrix3P>(x, index_sequence<UInt32P>());
    prefetch<Matrix3P>(x, index_sequence<UInt32P>(), index_sequence<UInt32P>() < 2u);

    /* Nested gather + slice for dynamic arrays */
    using UInt32X   = DynamicArray<UInt32P>;
    using TX        = DynamicArray<T>;
    using Matrix3X  = Matrix<TX, 3>;
    auto q2 = gather<Matrix3X>(x, index_sequence<UInt32X>(2));
    q2 = q2 + fill<Matrix3>(Value(1000));
    scatter(x, q2, index_sequence<UInt32X>(2));

    for (size_t i = 0; i < T::Size; ++i) {
        if (i < 2u)
            assert(slice(q, i) + fill<Matrix3>(Value(3000)) == x[i]);
        else
            assert(slice(q, i) + fill<Matrix3>(Value(1000)) == x[i]);
    }
}

#if defined(_MSC_VER)
  template <typename T, std::enable_if_t<T::Size != 8, int> = 0>
#else
  template <typename T, std::enable_if_t<T::Size == 31, int> = 0>
#endif
void test04_nested_gather_impl() { }

ENOKI_TEST_ALL(test04_nested_gather) {
    test04_nested_gather_impl<T>();
}

ENOKI_TEST_ALL(test05_range) {
    alignas(alignof(T)) Value mem[Size*10];
    for (size_t i = 0; i < Size*10; ++i)
        mem[i] = 1;
    using Index = uint_array_t<T>;
    T sum = zero<T>();
    for (auto pair : range<Index>(Size+1, (10*Size)/3))
        sum += gather<T>(mem, pair.first, pair.second);
    assert((10*Size/3) - (Size + 1) == hsum(sum));
}
