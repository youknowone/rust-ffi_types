#pragma once


//! This header is intended to be included in rust_types.hh file.
    

namespace ffi_types {

extern "C" {

void _rust_ffi_boxed_str_drop(ffi_types::CBoxedStr _string);

void _rust_ffi_boxed_bytes_drop(ffi_types::CBoxedByteSlice _slice);

/// Clone a BoxedStr by allocating a new copy.
ffi_types::CBoxedStr _rust_ffi_boxed_str_clone(const ffi_types::CBoxedStr *string);

/// Clone a BoxedSlice<u8> by allocating a new copy.
ffi_types::CBoxedByteSlice _rust_ffi_boxed_bytes_clone(const ffi_types::CBoxedByteSlice *slice);

}  // extern "C"

}  // namespace ffi_types
