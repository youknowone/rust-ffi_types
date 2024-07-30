namespace ffi_types {

#define EMPTY_SLICE_BEGIN(T) reinterpret_cast<T*>(1)

template<typename T>
T* _wrap_null(T* ptr) {
    return ptr ? ptr : reinterpret_cast<T*>(1);
}

// C++ std::ranges compatibility layer until C++17.
#if __cpp_lib_ranges
namespace ranges {
using std::ranges::data;
using std::ranges::size;
}  // namespace ranges
#define SAFE_R R&&
#else
namespace ranges {
template <typename C>
auto data(C& x) noexcept {
    auto begin = x.begin();

    using iterator_type = decltype(begin);
    using pointer_type = decltype(&*begin);

    if constexpr (std::is_same_v<iterator_type, pointer_type>) {
        // same type
        return begin;
    } else if constexpr (sizeof(iterator_type) == sizeof(uintptr_t)) {
        // likely to be a pointer
        return *reinterpret_cast<pointer_type*>(&begin);
    } else {
        if (begin != x.end()) {
            return &*begin;
        } else {
            return static_cast<pointer_type>(nullptr);
        }
    }
}

template <typename C>
size_t size(C& x) noexcept {
    return x.end() - x.begin();
}
}  // namespace ranges
// R&& is not safe without actual ranges
#define SAFE_R const R&
#endif

struct CharStrRef;
struct StrRef;
struct CStrRef;
struct CByteSliceRef;
class BoxedStr;

template <typename T>
struct CMutSliceRef;
template <typename T>
struct CSliceRef;
template <typename T>
struct CBoxedSlice;

/// A minimal range type of slices to be compatible with internal ranges.
template <typename T>
struct _SliceRange {
    T* _data;
    usize _size;

    auto* begin() const noexcept {
        return this->_data;
    }
    auto* end() const noexcept {
        return this->_data + _size;
    }
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
#if __cpp_lib_span
    using iterator = typename std::span<element_type>::iterator;
#else
    using iterator = pointer;
#endif
    using reverse_iterator = std::reverse_iterator<iterator>;

    // observers
    size_type size() const noexcept {
        return static_cast<const I<T>*>(this)->_size;
    }
    size_type size_bytes() const noexcept {
        return this->size() * sizeof(element_type);
    }
    bool empty() const noexcept {
        return this->size() == 0;
    }

    // element access
    reference operator[](size_type idx) const noexcept {
        assert(idx < this->size());
        return this->data()[idx];
    }
    reference front() const noexcept {
        return this->data()[0];
    }
    reference back() const noexcept {
        return this->data()[size() - 1];
    }
    pointer data() const noexcept {
        return static_cast<const I<T>*>(this)->_data;
    }

    // iterator
#if __cpp_lib_span
    iterator begin() const noexcept {
        return span().begin();
    }
#else
    iterator begin() const noexcept {
        return data();
    }
#endif
    iterator end() const noexcept {
        return begin() + size();
    }
    reverse_iterator rbegin() const noexcept {
        return reverse_iterator(end());
    }
    reverse_iterator rend() const noexcept {
        return reverse_iterator(begin());
    }

    _SliceRange<element_type> get() const noexcept {
        return {data(), size()};
    }

#if __cpp_lib_span
    /// Returns the slice as a `std::span`.
    std::span<element_type> span() const noexcept {
        return std::span<T>(data(), size());
    }
#endif
};

/// C++ counterpart of Rust `&mut [T]` and &[T]`.
/// The interface is following `std::span` design.
template <typename T>
struct MutSliceRef : public _SliceInterface<T, MutSliceRef> {
    T* _data;
    usize _size;

    MutSliceRef() noexcept : _data(EMPTY_SLICE_BEGIN(T)), _size(0) {}
    MutSliceRef(const MutSliceRef&) = default;
    MutSliceRef(MutSliceRef&&) = default;
    explicit MutSliceRef(T* head, usize size) noexcept : _data(_wrap_null(head)), _size(size) {}
    template <class R>
    MutSliceRef(SAFE_R range) noexcept : _data(_wrap_null(ranges::data(range))), _size(static_cast<usize>(ranges::size(range))) {}

    MutSliceRef& operator=(const MutSliceRef<T>&) = default;
    MutSliceRef& operator=(MutSliceRef<T>&&) = default;

    CMutSliceRef<T> into() const noexcept;

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
    SliceRef() noexcept : MutSliceRef<const T>(){};
    SliceRef(const SliceRef&) = default;
    SliceRef(SliceRef&&) = default;
    explicit SliceRef(const T* head, usize size) noexcept : MutSliceRef<const T>(head, size) {}
    template <class R>
    SliceRef(SAFE_R range) noexcept : MutSliceRef<const T>(range) {}

    SliceRef& operator=(const SliceRef<T>&) = default;
    SliceRef& operator=(SliceRef<T>&&) = default;

    typename std::conditional<std::is_same_v<T, uint8_t>, CByteSliceRef, CSliceRef<T>>::type into() const noexcept;

    template <typename B, typename U = T>
    static std::enable_if_t<std::is_same_v<U, uint8_t>, SliceRef<uint8_t>> from_buffer(const B& buffer) noexcept {
        return SliceRef<uint8_t>(reinterpret_cast<const uint8_t*>(&buffer), sizeof(B));
    }
};
static_assert(std::is_trivially_copyable<SliceRef<usize>>::value);
static_assert(std::is_standard_layout<SliceRef<usize>>::value);

/// C++ counterpart of `&[u8]` as alias for `SliceRef<uint8_t>`.
/// This alias is useful to pass `&[u8]` in cbindgen without configuration and boilerplate.
using ByteSliceRef = SliceRef<uint8_t>;

template <typename T>
struct CMutSliceRef {
    CMutSliceRef() = _COPY_DELETE;
    CMutSliceRef(const CMutSliceRef&) = default;
    CMutSliceRef& operator=(const CMutSliceRef<T>&) = default;

#if _MSC_VER
    /// Creates a `CMutSliceRef` from a `MutSliceRef`.
    /// This is a workaround for MSVC constructor limitation.
    static CMutSliceRef from(const MutSliceRef<T>& slice) noexcept {
        CMutSliceRef s;
        s._data = slice._data;
        s._size = slice._size;
        return s;
    }
#else
    CMutSliceRef(const MutSliceRef<T>& slice) noexcept {
        this->_data = slice._data;
        this->_size = slice._size;
    }
    /// Creates a `CMutSliceRef` from a `MutSliceRef`.
    /// This is a workaround for MSVC constructor limitation.
    static CMutSliceRef from(const MutSliceRef<T>& slice) noexcept {
        return CMutSliceRef(slice);
    }
#endif

    T* _data;
    usize _size;

    MutSliceRef<T> operator()() const noexcept {
        return MutSliceRef<T>(this->_data, this->_size);
    }
};
static_assert(std::is_trivial<CMutSliceRef<usize>>::value);
static_assert(std::is_standard_layout<CMutSliceRef<usize>>::value);

template <typename T>
struct CSliceRef {
    CSliceRef() = _COPY_DELETE;
    CSliceRef(const CSliceRef&) = default;
    CSliceRef& operator=(const CSliceRef<T>&) = default;

#if _MSC_VER
    /// Creates a `CSliceRef` from a `SliceRef`.
    /// This is a workaround for MSVC constructor limitation.
    static CSliceRef from(const SliceRef<T>& slice) noexcept {
        CSliceRef s;
        s._data = slice._data;
        s._size = slice._size;
        return s;
    }
#else
    CSliceRef(const SliceRef<T>& slice) noexcept {
        this->_data = slice._data;
        this->_size = slice._size;
    }
    /// Creates a `CSliceRef` from a `SliceRef`.
    /// This is a workaround for MSVC constructor limitation.
    static CSliceRef from(const SliceRef<T>& slice) noexcept {
        return CSliceRef(slice);
    }
#endif

    const T* _data;
    usize _size;

    SliceRef<T> operator()() const noexcept {
        return SliceRef<T>(this->_data, this->_size);
    }
};
static_assert(std::is_trivial<CSliceRef<usize>>::value);
static_assert(std::is_standard_layout<CSliceRef<usize>>::value);

// Alias is not a C type in MSVC, so this line doesn't work.
// ```c++
// using CByteSliceRef = CSliceRef<uint8_t>;
// ```
struct CByteSliceRef {
    CByteSliceRef() = _COPY_DELETE;
    CByteSliceRef(const CByteSliceRef&) = default;
    CByteSliceRef& operator=(const CByteSliceRef&) = default;

#if _MSC_VER
    /// Creates a `CByteSliceRef` from a `ByteSliceRef`.
    /// This is a workaround for MSVC constructor limitation.
    static CByteSliceRef from(const ByteSliceRef& slice) noexcept {
        CByteSliceRef s;
        s._data = slice._data;
        s._size = slice._size;
        return s;
    }
#else
    CByteSliceRef(const ByteSliceRef& slice) noexcept {
        this->_data = slice._data;
        this->_size = slice._size;
    }
    /// Creates a `CByteSliceRef` from a `ByteSliceRef`.
    /// This is a workaround for MSVC constructor limitation.
    static CByteSliceRef from(const ByteSliceRef& slice) noexcept {
        return CByteSliceRef(slice);
    }
#endif

    const uint8_t* _data;
    usize _size;

    ByteSliceRef operator()() const noexcept {
        ByteSliceRef slice;
        slice._data = this->_data;
        slice._size = this->_size;
        return slice;
    }
};
static_assert(std::is_trivial<CByteSliceRef>::value);
static_assert(std::is_standard_layout<CByteSliceRef>::value);

/// C++ counterpart of `Box<T>`
/// The ownership API is following `std::unique_ptr` design and the slice API is following `std::span` design.
template <typename T>
class BoxedSlice : public MutSliceRef<T> {
public:
    BoxedSlice() = delete;
    BoxedSlice(const BoxedSlice<T>&) = delete;
    BoxedSlice(BoxedSlice<T>&& s) noexcept : MutSliceRef<T>(s) {
        s._data = EMPTY_SLICE_BEGIN(T);
        s._size = 0;
    }
    BoxedSlice(std::nullptr_t) noexcept : MutSliceRef<T>() {}
    ~BoxedSlice() noexcept {
        if (this->_size > 0) {
            this->_drop();
        }
    }
    BoxedSlice<T>& operator=(BoxedSlice<T>&& b) noexcept {
        this->reset(b.release());
        return *this;
    }

    void _drop() noexcept;

    CBoxedSlice<T> into() noexcept;
    const CBoxedSlice<T>& as_c() const noexcept {
        return *reinterpret_cast<const CBoxedSlice<T>*>(this);
    }
    CBoxedSlice<T>& as_c() noexcept {
        return *reinterpret_cast<CBoxedSlice<T>*>(this);
    }

    void reset(_SliceRange<T> s) noexcept {
        if (this->_size > 0) {
            this->_drop();
        }
        this->_data = s._data;
        this->_size = s._size;
    }

    _SliceRange<T> release() noexcept {
        const auto range = this->get();
        this->_data = EMPTY_SLICE_BEGIN(T);
        this->_size = 0;
        return range;
    }

    /// Returns a mutable slice of the boxed slice.
    auto as_slice() noexcept {
        return MutSliceRef<T>(this->_data, this->_size);
    }

    /// Returns a read-only slice of the boxed slice.
    auto as_slice() const noexcept {
        return SliceRef<T>(this->_data, this->_size);
    }
};
static_assert(sizeof(usize) * 2 == sizeof(BoxedSlice<int>));
static_assert(std::is_standard_layout<BoxedSlice<usize>>::value);

/// C++ wrapper for a boxed slice with C ABI compatible layout.
///
/// @warning This type does *NOT* implement a safe destructor.
///          To avoid leak, see general note about C-prefixed types at the top of module documentation.
template <typename T>
struct [[nodiscard]] CBoxedSlice {
    CBoxedSlice() = _COPY_DELETE;
    CBoxedSlice(const CBoxedSlice&) = _COPY_DELETE;
    CBoxedSlice& operator=(const CBoxedSlice<T>&) = _COPY_DELETE;

#if _MSC_VER
    /// This is a workaround for MSVC constructor limitation.
    static CBoxedSlice from(BoxedSlice<T>&& slice) noexcept {
        auto r = slice.release();
        CBoxedSlice s;
        s._data = r._data;
        s._size = r._size;
        return s;
    }
#else
    CBoxedSlice(CBoxedSlice&&) = default;
    CBoxedSlice& operator=(CBoxedSlice<T>&&) = default;
    /// Constructs a `CBoxedSlice` by moving a `BoxedSlice`.
    CBoxedSlice(BoxedSlice<T>&& slice) noexcept {
        auto r = slice.release();
        this->_data = r._data;
        this->_size = r._size;
    }

    /// This is a workaround for MSVC constructor limitation.
    static CBoxedSlice from(BoxedSlice<T>&& slice) noexcept {
        return CBoxedSlice(std::move(slice));
    }
#endif

    T* _data;
    usize _size;

    /// Conversion operator to `BoxedSlice`.
    ///
    /// @note This conversion *MUST* be called unless the value is going to be passed to Rust side.
    BoxedSlice<T> operator()() noexcept {
        auto slice = BoxedSlice<T>(nullptr);
        auto r = this->release();
        slice._data = r._data;
        slice._size = r._size;
        return slice;
    }

    _SliceRange<T> get() const noexcept {
        return {this->_data, this->_size};
    }

    _SliceRange<T> release() noexcept {
        const auto range = this->get();
        this->_data = EMPTY_SLICE_BEGIN(T);
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
struct CharStrRef {
    CharStrRef() = _COPY_DELETE;
    CharStrRef(const CharStrRef&) = default;
    CharStrRef& operator=(const CharStrRef&) = default;
    // #if !_MSC_VER
    CharStrRef(const char* head, usize size) noexcept : _data(_wrap_null(head)), _size(size) {}
    template <class R>
    CharStrRef(SAFE_R range) noexcept : _data(_wrap_null(ranges::data(range))), _size(ranges::size(range)) {}
    CharStrRef(std::nullptr_t) noexcept : _data(EMPTY_SLICE_BEGIN(const char)), _size(0) {}
    // #endif

    const char* _data;
    usize _size;

    // constants and types
    using element_type = const char;
    using value_type = typename std::remove_cv_t<const char>;
    using size_type = uintptr_t;
    using difference_type = intptr_t;
    using pointer = element_type*;
    using const_pointer = const element_type*;
    using reference = element_type&;
    using const_reference = const element_type&;
    using iterator = typename std::string_view::iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    // observers
    size_type size() const noexcept {
        return this->_size;
    }
    size_type size_bytes() const noexcept {
        return this->size() * sizeof(element_type);
    }
    bool empty() const noexcept {
        return this->size() == 0;
    }

    // element access
    reference operator[](size_type idx) const noexcept {
        assert(idx < this->size());
        return this->data()[idx];
    }
    reference front() const noexcept {
        return this->data()[0];
    }
    reference back() const noexcept {
        return this->data()[size() - 1];
    }
    pointer data() const noexcept {
        return this->_data;
    }

    // iterator
    iterator begin() const noexcept {
        return view().begin();
    }
    iterator end() const noexcept {
        return begin() + size();
    }
    reverse_iterator rbegin() const noexcept {
        return reverse_iterator(end());
    }
    reverse_iterator rend() const noexcept {
        return reverse_iterator(begin());
    }

    _SliceRange<element_type> get() const noexcept {
        return {data(), size()};
    }

#if __cpp_lib_span
    /// Returns the slice as a `std::span`.
    std::span<element_type> span() const noexcept {
        return std::span<element_type>(data(), size());
    }
#endif

    StrRef as_str_unchecked() const noexcept;

#if __cpp_lib_string_view
    /// Constructs a `CharStrRef` from a null-terminated string.
    CharStrRef(const char* s) noexcept : CharStrRef(std::string_view(s)) {}

    /// Returns a `std::string_view` of the string.
    std::string_view view() const noexcept {
        return std::string_view(this->data(), this->size());
    }

    /// Conversion operator to `std::string_view`.
    operator std::string_view() const noexcept {
        return this->view();
    }

    /// Create a `std::string` by copying data.
    /// @return The newly created `std::string`.
    std::string to_string() const noexcept {
        return std::string(this->view());
    }
#endif
};
static_assert(sizeof(SliceRef<char>) == sizeof(CharStrRef));
static_assert(std::is_trivial<CharStrRef>::value);
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
    StrRef(const BoxedStr&) noexcept;
    StrRef(std::nullptr_t) noexcept : CharStrRef(EMPTY_SLICE_BEGIN(const char), 0) {}

    StrRef& operator=(const StrRef&) = default;
};
static_assert(std::is_trivially_copyable<StrRef>::value);
static_assert(std::is_standard_layout<StrRef>::value);

/// C++ wrapper for a str with C ABI compatible layout.
struct CStrRef {
    CStrRef() = _COPY_DELETE;
    CStrRef(const CStrRef&) = default;
    CStrRef& operator=(const CStrRef&) = default;

#if !_MSC_VER
    CStrRef(const StrRef& slice) noexcept {
        this->_data = slice.data();
        this->_size = slice.size();
    }
#endif

    const char* _data;
    usize _size;

    StrRef operator()() const noexcept {
        auto s = StrRef(nullptr);
        s._data = this->_data;
        s._size = this->_size;
        return s;
    }
};
static_assert(std::is_trivial<CStrRef>::value);
static_assert(std::is_standard_layout<CStrRef>::value);

/// C++ counterpart of Rust `Box<str>`.
class BoxedStr : public StrRef {
public:
    BoxedStr() = delete;
    BoxedStr(BoxedStr&) = delete;
    BoxedStr(BoxedStr&& s) noexcept : StrRef(s) {
        s._data = EMPTY_SLICE_BEGIN(const char);
        s._size = 0;
    }
    BoxedStr(std::nullptr_t) noexcept : StrRef(nullptr) {}

    ~BoxedStr() noexcept {
        if (this->_size > 0) {
            this->_drop();
        }
    }

    BoxedStr& operator=(BoxedStr&& b) noexcept {
        this->reset(b.release());
        return *this;
    }

    void _drop() noexcept;

    void reset(_SliceRange<const char> s) noexcept {
        if (this->_size > 0) {
            this->_drop();
        }
        this->_data = s._data;
        this->_size = s._size;
    }

    _SliceRange<const char> release() noexcept {
        const auto range = this->get();
        this->_data = EMPTY_SLICE_BEGIN(const char);
        this->_size = 0;
        return range;
    }

    StrRef as_str() const noexcept {
        return this->as_str_unchecked();
    }
};
static_assert(sizeof(StrRef) == sizeof(BoxedStr));
static_assert(std::is_standard_layout<BoxedStr>::value);

/// C++ wrapper for Rust `Box<str>` with C ABI compatible layout.
///
/// @warning This type does *NOT* implement a safe destructor.
///          To avoid leak, see general note about C-prefixed types at the top of module documentation.
struct [[nodiscard]] CBoxedStr {
    CBoxedStr() = _COPY_DELETE;
    CBoxedStr(const CBoxedStr&) = _COPY_DELETE;
    CBoxedStr& operator=(const CBoxedStr&) = _COPY_DELETE;

    const char* _data;
    usize _size;

#if _MSC_VER
    /// This is a workaround for MSVC constructor limitation.
    static CBoxedStr from(BoxedStr&& str) noexcept {
        auto r = str.release();
        CBoxedStr s;
        s._data = r._data;
        s._size = r._size;
        return s;
    }
#else
    CBoxedStr(CBoxedStr&&) = default;
    CBoxedStr& operator=(CBoxedStr&&) = default;
    CBoxedStr(BoxedStr&& str) noexcept {
        auto r = str.release();
        this->_data = r._data;
        this->_size = r._size;
    }

    /// This is a workaround for MSVC constructor limitation.
    static CBoxedStr from(BoxedStr&& slice) noexcept {
        return CBoxedStr(std::move(slice));
    }
#endif

    BoxedStr operator()() noexcept {
        auto slice = BoxedStr(nullptr);
        auto r = this->release();
        slice._data = r._data;
        slice._size = r._size;
        return slice;
    }

    _SliceRange<const char> release() noexcept {
        const auto range = _SliceRange<const char>{_data, _size};
        this->_data = EMPTY_SLICE_BEGIN(const char);
        this->_size = 0;
        return range;
    }
};
static_assert(std::is_trivial<CBoxedStr>::value);
static_assert(std::is_standard_layout<CBoxedStr>::value);

template <typename T>
inline CMutSliceRef<T> MutSliceRef<T>::into() const noexcept {
    return CMutSliceRef<T>::from(*this);
}

template <typename T>
inline typename std::conditional<std::is_same_v<T, uint8_t>, CByteSliceRef, CSliceRef<T>>::type SliceRef<T>::into()
        const noexcept {
    if constexpr (std::is_same_v<T, uint8_t>) {
        return CByteSliceRef::from(*this);
    } else {
        return CSliceRef<T>::from(*this);
    }
}

template <typename T>
inline CBoxedSlice<T> BoxedSlice<T>::into() noexcept {
    return CBoxedSlice<T>::from(std::move(*this));
}

inline StrRef::StrRef(const BoxedStr& owned) noexcept : CharStrRef(owned._data, owned._size) {}

inline StrRef CharStrRef::as_str_unchecked() const noexcept {
    return *reinterpret_cast<const StrRef*>(this);
}
template <>
template <>
inline CharStrRef MutSliceRef<const uint8_t>::as_char_str<const uint8_t>() const noexcept {
    return CharStrRef(reinterpret_cast<const char*>(this->_data), this->_size);
}
template <>
template <>
inline StrRef MutSliceRef<const uint8_t>::as_str_unchecked<const uint8_t>() const noexcept {
    return as_char_str().as_str_unchecked();
}

#undef SAFE_R
#undef EMPTY_SLICE_BEGIN

}  // namespace ffi_types
