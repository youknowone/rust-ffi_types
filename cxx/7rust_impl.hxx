#pragma once


//! This header is intended to be included in rust_types.hh file.
    

namespace ffi_types {

extern "C" {

void _rust_ffi_boxed_str_drop(ffi_types::CBoxedStr _string);

void _rust_ffi_boxed_bytes_drop(ffi_types::CBoxedSlice<uint8_t> _slice);

}  // extern "C"

}  // namespace ffi_types
