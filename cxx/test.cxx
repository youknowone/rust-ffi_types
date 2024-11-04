#include "0header.hxx"
#include "1boxed.hxx"
#include "2slice.hxx"
#include "7rust_impl.hxx"
#include "8cxx_impl.hxx"
#include "9footer.hxx"

void ffi_types::_rust_ffi_boxed_str_drop(ffi_types::CBoxedStr) {}

template struct ffi_types::CBox<char>;
template <>
void ffi_types::OptionBox<char>::_drop() noexcept {}

template struct ffi_types::CMutSliceRef<char>;
template struct ffi_types::CSliceRef<char>;
template struct ffi_types::CBoxedSlice<char>;
template <>
void ffi_types::BoxedSlice<char>::_drop() noexcept {}

extern "C" {
ffi_types::CBox<char> signature_c_box(ffi_types::CBox<char> c) {
    return c;
}
ffi_types::CMutSliceRef<char> signature_c_mut_slice_ref(ffi_types::CMutSliceRef<char> c) {
    return c;
}
ffi_types::CSliceRef<char> signature_c_slice_ref(ffi_types::CSliceRef<char> c) {
    return c;
}
ffi_types::CBoxedSlice<char> signature_c_boxed_slice(ffi_types::CBoxedSlice<char> c) {
    return c;
}
ffi_types::CByteSliceRef signature_byte_slice_ref(ffi_types::CByteSliceRef c) {
    return c;
}
ffi_types::CStrRef signature_c_str_ref(ffi_types::CStrRef c) {
    return c;
}
ffi_types::CBoxedStr signature_c_boxed_str(ffi_types::CBoxedStr c) {
    return c;
}
ffi_types::CharStrRef signature_char_str_ref(ffi_types::CharStrRef c) {
    return c;
}
}

void test_box() {
    char* x = nullptr;
    ffi_types::Box<char> b = ffi_types::Box<char>(x + 50);
    assert(b.release() == x + 50);
}

template <typename C>
void test_iterator_begin() {
    C c(nullptr, 0);
    c._data += 1;

    auto begin = c.begin();
    auto data = ffi_types::ranges::data(c);
    assert(data == c.data());
}

void test_char_str() {
    auto str1 = ffi_types::CharStrRef("hello");
    assert(str1.view() == "hello");

    auto str2 = ffi_types::CharStrRef("hello", 3);
    assert(str2.view() == "hel");

    auto str3 = ffi_types::CharStrRef(str1);
    assert(str3.view() == "hello");

    auto str4 = ffi_types::CharStrRef(str1.view());
    assert(str4.view() == "hello");

    auto string = str4.to_string();
    auto str5 = ffi_types::CharStrRef(string);
    assert(str5.view() == "hello");

    auto array = std::array<char, 5>{'h', 'e', 'l', 'l', 'o'};
    auto str6 = ffi_types::CharStrRef(array);
    assert(str6.view() == "hello");
}

void test_null_str() {
    auto str_raw = ffi_types::CharStrRef((char*)1, 0);
    {
        auto str = ffi_types::CharStrRef(nullptr);
        assert(str.view() == str_raw.view());
        assert(str.size() == 0);
        assert(str.view() == "");
    }
    {
        auto str = ffi_types::CharStrRef(std::array<char, 0>{});
        assert(str.view() == str_raw.view());
        assert(str.size() == 0);
        assert(str.view() == "");
    }
    {
        auto str = ffi_types::StrRef(nullptr);
        assert(str.view() == str_raw.view());
        assert(str.size() == 0);
        assert(str.view() == "");
    }
    {
        auto bstr = reinterpret_cast<ffi_types::BoxedStr*>(&str_raw);
        auto& str = *bstr;
        assert(str.view() == str_raw.view());
        assert(str.size() == 0);
        assert(str.view() == "");
    }
}

void test_move_boxed_slice() {
    const auto* buffer = "hello";
    auto fake_boxed = ffi_types::BoxedSlice<char>(nullptr);
    auto* forced_slice = reinterpret_cast<ffi_types::MutSliceRef<char>*>(&fake_boxed);
    forced_slice->_data = const_cast<char*>(buffer);
    forced_slice->_size = 5;
    assert(fake_boxed[0] == 'h');
    assert(fake_boxed.size() == 5);

    auto moved_boxed = std::move(fake_boxed);
    assert(fake_boxed.size() == 0);
    assert(moved_boxed[0] == 'h');
    assert(moved_boxed.size() == 5);
}

void test_move_boxed_str() {
    const auto* buffer = "hello";
    auto fake_boxed = ffi_types::BoxedStr(nullptr);
    auto* forced_slice = reinterpret_cast<ffi_types::MutSliceRef<char>*>(&fake_boxed);
    forced_slice->_data = const_cast<char*>(buffer);
    forced_slice->_size = 5;
    assert(fake_boxed[0] == 'h');
    assert(fake_boxed.size() == 5);

    auto moved_boxed = std::move(fake_boxed);
    assert(fake_boxed.data() == reinterpret_cast<const char*>(1));
    assert(fake_boxed.size() == 0);
    assert(moved_boxed[0] == 'h');
    assert(moved_boxed.size() == 5);
}

int main() {
    test_box();
    test_char_str();
    test_null_str();
    test_move_boxed_slice();
    test_move_boxed_str();
    test_iterator_begin<ffi_types::CharStrRef>();
    test_iterator_begin<ffi_types::SliceRef<char>>();
    test_iterator_begin<ffi_types::SliceRef<const char>>();
    return 0;
}