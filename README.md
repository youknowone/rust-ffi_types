# ffi_types

This repository provides C-ABI compatible basic Rust types including box, slice, str and boxed slice/str.
By carefully following instructions, the types are bindgen/cbindgen compatible and owned values are safe under C++.

The project consists of 2 parts.
A rust crate of `#[repr(C)]` wrappers of rust types and a header file for C++ (or a few more later).

The conversions are intended to be zero-cost.

TODO: link to docs and examples

## CBindgen

Use `ffi_types::cbindgen::with_cxx_ffi_types()` to add proper configuration to `cbindgen::Builder`.

## Bindgen

Block the provided header to `blocklist_file`.
Replace all `root::ffi_types::` to `ffi_types::` in generated file.

## Not the best choice for FFI

If you start a new project, please check [cxx](https://cxx.rs/) fits in your case.

Since the goal of `ffi_types` is exposing Rust type to C++ side, you need access to C++ code base.

`ffi_types` only provides types. Nothing is automatic. You still need to manage bindgen/cbindgen.

MSVC is very picky. `ffi_types` limits convenience initializers for C-prefixed types for MSVC. It will be less convenient and more unsafe.
