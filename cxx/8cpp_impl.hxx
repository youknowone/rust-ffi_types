namespace ffi_types {

inline void BoxedStr::_drop() noexcept {
    ffi_types::_rust_ffi_boxed_str_drop(CBoxedStr::from(std::move(*this)));
}

template <>
inline void BoxedSlice<uint8_t>::_drop() noexcept {
    ffi_types::_rust_ffi_boxed_bytes_drop(CBoxedSlice<uint8_t>::from(std::move(*this)));
}

}  // namespace ffi_types