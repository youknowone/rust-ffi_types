#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#if __cpp_lib_ranges
#include <ranges>
#endif
#if __cpp_lib_string_view
#include <string_view>
#endif
#if __cpp_lib_span
#include <span>
#endif
#include <type_traits>

//! @file rust_types.hh
//! @brief This file contains matching C++ types for `ffi_types` crate.
//!
//! The types can be used for either bindgen, cbindgen or hand-crafted C/C++ code.
//! The types without C-prefixes are ownership-aware types compatible with Rust types.
//! The types with C-prefixes are C ABI compatible layout of its paired owned types.
//! C-prefixes types are very fragile! Please read the following warning to use it safe.
//!
//! @warning All types prefixed with `C` in this file are C ABI compatible layout of owned types without `C` prefix.
//!          To avoid memory leak, you must convert it to owned type or pass it to Rust side.
//!          You *MUST* run one of the following to avoid memory leak:
//!
//!          1. Calling operator() to convert to owned type.
//!             1.1 When used in bindgen generated functions' arguments.
//!             ```c++
//!             void foo(CBoxedSlice<uint32_t> ints) {
//!                 auto owned = ints();  // owned is managed as BoxedSlice<uint32_t>
//!             }
//!             ```
//!
//!             1.2 When used in cbindgen generated functions' return value.
//!             ```rust
//!             fn foo() -> CBoxedSlice<u32> { ... }
//!             ```
//!             ```c++
//!             auto owned = foo()();  // owned is managed as BoxedSlice<uint32_t>
//!             ```
//!
//!             1.3 Dropping in C++ side.
//!             The owned value in either 1.1 or 1.2 must be returned to Rust side or dropped.
//!             See 2.1 to return value.
//!             Implement template specialization for `_drop()` for the related type to drop it in C++ side.
//!             (e.g. BoxedSlice<uint32_t>::_drop() for BoxedSlice<uint32_t>)
//!             The _drop function must call `std::mem::drop()` from rust side.
//!
//!          2. Passing as an argument of Rust function.
//!             2.1 When used in cbindgen generated functions' arguments.
//!             ```rust
//!             fn foo() -> CBoxedSlice<u32> { ... }   // See 1.2 for this case
//!             fn bar(ints: CBoxedSlice<u32>) { ... }
//!             ```
//!             ```c++
//!             auto owned = foo()();  // owned is managed as BoxedSlice<uint32_t>
//!             bar(owned);  // BoxedSlice can be passed to CBoxedSlice
//!             ```
//!
//!          3. Returning from C++ function. Note that this case will be very unlikely to happen because Box must be
//!          created from Rust side.
//!
//! @note allocate or deallocate a boxed type in C++ side is not allowed.

namespace ffi_types {

/// Alias for Rust `std::usize`.
using usize = uintptr_t;

/// Alias for Rust `[T; N]`. Interpreted as std::array in C++ side.
template <typename T, usize N>
using Array = std::array<T, N>;

}  // namespace ffi_types

#if _MSC_VER
#define _COPY_DELETE default
#else
#define _COPY_DELETE delete
#endif
