namespace ffi_types {

// C++ std::ranges compatibility layer until C++17.
#if __cpp_lib_ranges
using std::ranges;
#define SAFE_R R&&
#else
namespace ranges {
template <typename C>
auto data(C& x) noexcept {
    return &*x.begin();
}

template <typename C>
usize size(C& x) noexcept {
    return x.end() - x.begin();
}
}  // namespace ranges
// R&& is not safe without actual ranges
#define SAFE_R const R&
#endif

class BoxedStr;
struct CBoxedStr;
struct CharStrRef;
struct StrRef;
template <typename T>
struct CBoxedSlice;

/// A minimal range type of slices.
template <typename T>
struct _SliceRange {
    T* _data;
    usize _size;

    auto* begin() const noexcept { return this->_data; }
    auto* end() const noexcept { return this->_data + _size; }
};

/// Common C++ STL-like interface for &[T] and &str.
/// @see std::span
template <typename T, template <typename _> typename I>
struct _SliceInterface {
    // constants and types
    using element_type = T;
    using value_type = typename std::remove_cv_t<T>;
    using size_type = uintptr_t;
    using difference_type = intptr_t;
    using pointer = element_type*;
    using const_pointer = const element_type*;
    using reference = element_type&;
    using const_reference = const element_type&;
    using iterator = pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;

    // observers
    size_type size() const noexcept { return static_cast<const I<T>*>(this)->_size; }
    size_type size_bytes() const noexcept { return this->size() * sizeof(T); }
    bool empty() const noexcept { return this->size() == 0; }

    // element access
    reference operator[](size_type idx) const noexcept {
        assert(idx < this->size());
        return this->data()[idx];
    }
    reference front() const noexcept { return this->data()[0]; }
    reference back() const noexcept { return this->data()[size() - 1]; }
    pointer data() const noexcept { return static_cast<const I<T>*>(this)->_data; }

    // iterator
    iterator begin() const noexcept { return data(); }
    iterator end() const noexcept { return data() + size(); }
    reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
    reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }

    _SliceRange<T> get() const noexcept { return {data(), size()}; }

#if __cpp_lib_span
    /// Returns the slice as a `std::span`.
    std::span<T> span() const noexcept { return std::span<T>(this->data, this->len); }
#endif
};

/// Internal type to represent Rust slice-reference-like object memory layout.
template <typename T>
struct _SliceInner {
    T* _data;
    usize _size;

    /// Creates a `_SliceInner` from a range.
    template <class R>
    static _SliceInner<T> from_range(SAFE_R range) noexcept {
        return {&*range.begin(), static_cast<usize>(range.end() - range.begin())};
    }
};
static_assert(sizeof(_SliceInner<int>) == sizeof(uintptr_t) * 2);
static_assert(std::is_trivial<_SliceInner<usize>>::value);
static_assert(std::is_standard_layout<_SliceInner<usize>>::value);

/// C++ counterpart of Rust `&mut [T]` and &[T]`.
/// The interface is following `std::span` design.
template <typename T>
struct MutSliceRef : _SliceInner<T>, public _SliceInterface<T, MutSliceRef> {
    MutSliceRef() noexcept : _SliceInner<T>{nullptr, 0} {}
    MutSliceRef(const MutSliceRef&) = default;
    MutSliceRef(MutSliceRef&&) = default;
    explicit MutSliceRef(T* head, usize size) noexcept : _SliceInner<T>{head, size} {}
    template <class R>
    MutSliceRef(SAFE_R range) : _SliceInner<T>(_SliceInner<T>::from_range(range)) {}

    /// Convert &[u8] to C++ string wrapper.
    /// This is &[u8] specialization as bytes
    template <typename U = T>
    std::enable_if_t<std::is_same_v<U, const uint8_t>, CharStrRef> as_char_str() const noexcept;

    /// Convert &[u8] to Rust string wrapper without UTF-8 checking.
    /// This is &[u8] specialization as bytes
    ///
    /// @warning Safety: Only when the data is a valid UTF-8 string.
    template <typename U = T>
    std::enable_if_t<std::is_same_v<U, const uint8_t>, StrRef> as_str_unchecked() const noexcept;
};
static_assert(std::is_trivially_copyable<MutSliceRef<usize>>::value);
static_assert(std::is_standard_layout<MutSliceRef<usize>>::value);

/// SliceRef<T> is a read-only slice `&[T]` in Rust and a conceptional alias to `MutSliceRef<const T>` in C++.
/// This is not a hard alias because:
/// - template alias doesn't support generic parameter deduction.
/// - bindgen resolve template alias to the original type, which we don't supposed to do.
template <typename T>
struct SliceRef : public MutSliceRef<const T> {
    SliceRef() = default;
    SliceRef(const SliceRef&) = default;
    SliceRef(SliceRef&&) = default;
    explicit SliceRef(const T* head, usize size) noexcept : MutSliceRef<const T>(head, size) {}
    template <class R>
    SliceRef(SAFE_R range) : MutSliceRef<const T>(range) {}
};
static_assert(std::is_trivially_copyable<SliceRef<usize>>::value);
static_assert(std::is_standard_layout<SliceRef<usize>>::value);

/// C++ counterpart of `&[u8]` as alias for `SliceRef<uint8_t>`.
/// This alias is useful to pass `&[u8]` in cbindgen without configuration and boilerplate.
using ByteSliceRef = SliceRef<uint8_t>;

/// C++ counterpart of `Box<T>`
/// The ownership API is following `std::unique_ptr` design and the slice API is following `std::span` design.
template <typename T>
class BoxedSlice : public MutSliceRef<T> {
public:
    BoxedSlice() = delete;
    BoxedSlice(const BoxedSlice<T>&) = delete;
    BoxedSlice(BoxedSlice<T>&&) = default;
    BoxedSlice(CBoxedSlice<T>&&) noexcept;
    BoxedSlice(std::nullptr_t) noexcept : MutSliceRef<T>() {}
    ~BoxedSlice() noexcept {
        if (this->_size > 0) this->_drop();
    }

    void _drop() noexcept;

    void reset(_SliceRange<T> s) noexcept {
        if (this->_size > 0) this->_drop();
        this->_data = s._data;
        this->_size = s._size;
    }

    _SliceRange<T> release() noexcept {
        const auto range = this->get();
        this->_data = nullptr;
        this->_size = 0;
        return range;
    }

    BoxedSlice<T>& operator=(BoxedSlice<T>&& b) noexcept {
        this->reset(b.release());
        return *this;
    }

    /// Returns a mutable slice of the boxed slice.
    auto as_slice() noexcept { return MutSliceRef<T>(this->_data, this->_size); }

    /// Returns a read-only slice of the boxed slice.
    auto as_slice() const noexcept { return SliceRef<T>(this->_data, this->_size); }
};
static_assert(sizeof(_SliceInner<int>) == sizeof(BoxedSlice<int>));
static_assert(std::is_standard_layout<BoxedSlice<usize>>::value);

/// C++ wrapper for a boxed slice with C ABI compatible layout.
///
/// @warning This type does *NOT* implement a safe destructor.
///          To avoid leak, see general note about C-prefixed types at the top of module documentation.
template <typename T>
struct [[nodiscard]] CBoxedSlice : public _SliceInner<T> {
    CBoxedSlice() = delete;
    CBoxedSlice(const CBoxedSlice&) = delete;
    CBoxedSlice(CBoxedSlice&&) = default;

    /// Constructs a `CBoxedSlice` by moving a `BoxedSlice`.
    CBoxedSlice(BoxedSlice<T>&& box) noexcept : _SliceInner<T>(_SliceInner<T>::from_range(box.release())) {}

    /// Conversion operator to `BoxedSlice`.
    ///
    /// @note This conversion *MUST* be called unless the value is going to be passed to Rust side.
    BoxedSlice<T> operator()() noexcept { return BoxedSlice(std::move(*this)); }

    _SliceRange<T> get() const noexcept { return {this->_data, this->_size}; }

    _SliceRange<T> release() noexcept {
        const auto range = this->get();
        this->_data = nullptr;
        this->_size = 0;
        return range;
    }
};
static_assert(std::is_trivial<CBoxedSlice<int>>::value);
static_assert(std::is_standard_layout<CBoxedSlice<int>>::value);

/// C++ unsafe counterpart of Rust `&str`.
///
/// Because `StrRef` in C++ side doesn't have any UTF-8 validation checking,
/// `CharStrRef` is an alternative of `StrRef` creation in C++ side.
/// The value can be converted to either `&[u8]` or `&str` with UTF-8 validation in Rust side.
struct CharStrRef : public MutSliceRef<const char> {
    CharStrRef() : MutSliceRef<const char>() {}
    CharStrRef(const CharStrRef&) = default;
    CharStrRef(CharStrRef&&) = default;
    CharStrRef(const char* s, usize size) noexcept : MutSliceRef<const char>(s, size) {}
    template <class R>
    CharStrRef(SAFE_R range) : MutSliceRef(range) {}

    StrRef as_str_unchecked() const noexcept;

#if __cpp_lib_string_view
    /// Constructs a `CharStrRef` from a null-terminated string.
    CharStrRef(const char* s) noexcept : CharStrRef(std::string_view(s)) {}

    /// Returns a `std::string_view` of the string.
    std::string_view view() const noexcept { return std::string_view(this->data(), this->size()); }

    /// Conversion operator to `std::string_view`.
    operator std::string_view() const noexcept { return this->view(); }

    /// Create a `std::string` by copying data.
    /// @return The newly created `std::string`.
    std::string to_string() const noexcept { return std::string(this->view()); }
#endif
};
static_assert(sizeof(SliceRef<char>) == sizeof(CharStrRef));
static_assert(std::is_trivially_copyable<CharStrRef>::value);
static_assert(std::is_standard_layout<CharStrRef>::value);

/// C++ counterpart of Rust `&str`.
///
/// Because `StrRef` in C++ side doesn't have any UTF-8 validation checking,
/// this type is highly recommended to be created from Rust side.
///
/// To create `StrRef` in C++ side, use `ByteSliceRef::as_str_unchecked()` or `CharStrRef::as_str_unchecked()`
/// at your own risk.
struct StrRef : public CharStrRef {
    StrRef() = delete;
    StrRef(const StrRef&) = default;
    StrRef(StrRef&&) = default;
    StrRef(BoxedStr&) noexcept;
    StrRef(std::nullptr_t) noexcept : CharStrRef() {}
};
static_assert(std::is_trivially_copyable<StrRef>::value);
static_assert(std::is_standard_layout<StrRef>::value);

/// C++ counterpart of Rust `Box<str>`.
///
/// @warning This type does *NOT* implement a safe destructor.
///          To avoid leak, see general note about C-prefixed types at the top of module documentation.
class BoxedStr : public StrRef {
public:
    BoxedStr() = delete;
    BoxedStr(BoxedStr&) = delete;
    BoxedStr(BoxedStr&&) = default;
    BoxedStr(std::nullptr_t) noexcept : StrRef(nullptr) {}

    BoxedStr(CBoxedStr&&) noexcept;
    ~BoxedStr() noexcept {
        if (this->_size > 0) this->_drop();
    }

    void _drop() noexcept;

    void reset(_SliceRange<const char> s) noexcept {
        if (this->_size > 0) this->_drop();
        this->_data = s._data;
        this->_size = s._size;
    }

    _SliceRange<const char> release() noexcept {
        const auto range = this->get();
        this->_data = nullptr;
        this->_size = 0;
        return range;
    }

    BoxedStr& operator=(BoxedStr&& b) noexcept {
        this->reset(b.release());
        return *this;
    }

    StrRef as_str() const noexcept { return this->as_str_unchecked(); }
};
static_assert(sizeof(StrRef) == sizeof(BoxedStr));
static_assert(std::is_standard_layout<BoxedStr>::value);

/// C++ wrapper for Rust `Box<str>` with C ABI compatible layout.
struct [[nodiscard]] CBoxedStr : public _SliceInner<const char> {
    CBoxedStr() = delete;
    CBoxedStr(const CBoxedStr&) = delete;
    CBoxedStr(CBoxedStr&&) = default;

    CBoxedStr(BoxedStr&& box) : _SliceInner<const char>(_SliceInner::from_range(box.release())) {}
    BoxedStr operator()() noexcept { return BoxedStr(std::move(*this)); }

    _SliceRange<const char> release() noexcept {
        const auto range = _SliceRange<const char>{_data, _size};
        this->_data = nullptr;
        this->_size = 0;
        return range;
    }
};
static_assert(std::is_trivial<CBoxedStr>::value);
static_assert(std::is_standard_layout<CBoxedStr>::value);

template <typename T>
inline BoxedSlice<T>::BoxedSlice(CBoxedSlice<T>&& box) noexcept : MutSliceRef<T>(box.release()) {}
inline BoxedStr::BoxedStr(CBoxedStr&& box) noexcept : StrRef(CharStrRef(box.release()).as_str_unchecked()) {}
inline StrRef::StrRef(BoxedStr& owned) noexcept : CharStrRef(owned._data, owned._size) {}

inline StrRef CharStrRef::as_str_unchecked() const noexcept {
    return *reinterpret_cast<const StrRef*>(this);
}
template <>
template <>
inline CharStrRef MutSliceRef<const uint8_t>::as_char_str() const noexcept {
    return CharStrRef(reinterpret_cast<const char*>(this->_data), this->_size);
}
template <>
template <>
inline StrRef MutSliceRef<const uint8_t>::as_str_unchecked() const noexcept {
    return as_char_str().as_str_unchecked();
}

#undef SAFE_R

}  // namespace ffi_types
