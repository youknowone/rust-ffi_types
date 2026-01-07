namespace ffi_types {

inline void BoxedStr::_drop() noexcept {
    ffi_types::_rust_ffi_boxed_str_drop(CBoxedStr::from(std::move(*this)));
}

template <>
inline void BoxedSlice<uint8_t>::_drop() noexcept {
    ffi_types::_rust_ffi_boxed_bytes_drop(CBoxedByteSlice::from(std::move(*this)));
}

inline BoxedStr BoxedStr::clone() const noexcept {
    return ffi_types::_rust_ffi_boxed_str_clone(&this->as_c())();
}

template <>
template <>
inline BoxedSlice<uint8_t> BoxedSlice<uint8_t>::clone() const noexcept {
    return ffi_types::_rust_ffi_boxed_bytes_clone(&this->as_c())();
}

}  // namespace ffi_types