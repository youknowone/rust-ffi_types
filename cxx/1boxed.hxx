namespace ffi_types {

template <typename T>
struct OptionBox;

/// C++ wrapper for Rust `Box<T>` with C ABI compatible layout.
///
/// @warning This type does *NOT* implement a safe destructor.
///          To avoid leak, see general note about C-prefixed types at the top of module documentation.
template <typename T>
struct [[nodiscard]] CBox {
    CBox() = delete;
    CBox(const CBox&) = delete;
    CBox(CBox&&) = default;

    CBox(OptionBox<T>&& box) noexcept;

    T* _ptr;

    /// Conversion operator to check if the box is not null.
    explicit operator bool() const noexcept { return _ptr != nullptr; }

    /// Equality operator with nullptr.
    bool operator==(std::nullptr_t) noexcept { return _ptr == nullptr; }

    /// Conversion operator to `OptionBox<T>`.
    ///
    /// Though the creation of `CBox` is always expected to be non-null by Rust Side,
    /// `CBox` itself still can be a null due to move.
    ///
    /// @note This conversion *MUST* be called unless the value is going to be passed to Rust side.
    OptionBox<T> operator()() noexcept;

    T* release() noexcept {
        auto* ptr = this->_ptr;
        this->_ptr = nullptr;
        return ptr;
    }
};
static_assert(sizeof(CBox<int>) == sizeof(std::unique_ptr<int>));
static_assert(std::is_trivial<CBox<int>>::value);
static_assert(std::is_standard_layout<CBox<int>>::value);

/// C++ wrapper for Rust `Option<Box<T>>` as alias of `CBox<T>`.
///
/// This is a cbindgen helper to pass `Option<Box<T>>` to C++ side.
///
/// @see CBox
template <typename T>
using COptionBox = CBox<T>;

/// C++ counterpart for Rust `Option<Box<T>>` managed by C++ ownership model.
//
/// The API design is similar to `std::unique_ptr`, though construction and destruction
/// must be executed in Rust side.
///
/// @warning The underlying memory must be allocated from Rust side.
template <typename T>
struct OptionBox {
    using element_type = T;
    using pointer = T*;

    T* _ptr;

    // constructors
    OptionBox() = delete;
    OptionBox(OptionBox&) = delete;
    OptionBox(OptionBox&& b) noexcept : _ptr(b.release()) {}
    OptionBox(CBox<T>&& box) noexcept : _ptr(box.release()) {}
    explicit OptionBox(std::nullptr_t) noexcept : _ptr(nullptr) {}

    // destructor and helper
    ~OptionBox() noexcept {
        if (this->get()) this->_drop();
    }
    void _drop() noexcept;

    // assignment
    OptionBox& operator=(OptionBox&& b) noexcept {
        this->_ptr = b.release();
        return *this;
    }

    // observers
    typename std::add_lvalue_reference<element_type>::type operator*() const { return *get(); }
    pointer operator->() const { return get(); }
    pointer get() const { return this->_ptr; }
    explicit operator bool() const noexcept { return get() != nullptr; }

    // operators
    bool operator==(std::nullptr_t) const { return this->get() == nullptr; }
    bool operator!=(std::nullptr_t) const { return this->get() != nullptr; }

    // modifiers
    T* release() noexcept {
        auto* ptr = this->_ptr;
        this->_ptr = 0;
        return ptr;
    }
    void reset(pointer p) noexcept {
        if (this->get()) this->_drop();
        this->_ptr = p;
    }
};
static_assert(sizeof(OptionBox<int>) == sizeof(int*));
static_assert(std::is_standard_layout<CBox<int>>::value);

/// C++ counterpart for Rust `Box<T>` as alias of `OptionBox<T>`.
/// Since `Box<T>` can be moved or released, a value of this type is still not guaranteed to be non-null.
///
/// @warning The underlying memory must be allocated from Rust side.
template <typename T>
struct Box : public OptionBox<T> {};

template <typename T>
CBox<T>::CBox(OptionBox<T>&& box) noexcept : _ptr(box.release()) {}
template <typename T>
OptionBox<T> CBox<T>::operator()() noexcept {
    return OptionBox<T>(std::move(*this));
}

template struct CBox<void>;

template <>
inline void OptionBox<void>::_drop() noexcept {
    assert("void box doesn't support drop" && false);
}

}  // namespace ffi_types
