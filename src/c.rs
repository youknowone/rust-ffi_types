use crate::slice::SliceInner;

pub const CXX_INCLUDE_PATH: &str = concat!(env!("CARGO_MANIFEST_DIR"), "/include");
pub const CXX_HEADER_PATH: &str = concat!(env!("CARGO_MANIFEST_DIR"), "/include/rust_types.hxx");
pub const CXX_HEADER_CONTENT: &str = include_str!("../include/rust_types.hxx");

#[allow(non_camel_case_types)]
#[cfg(feature = "libc")]
type c_char = libc::c_char;
#[allow(non_camel_case_types)]
#[cfg(not(feature = "libc"))]
type c_char = u8;

pub type COptionBox<T> = crate::OptionBox<T>;
pub type CBox<T> = COptionBox<T>;

pub type CSliceRef<T> = crate::SliceRef<T>;
pub type CBoxedSlice<T> = crate::BoxedSlice<T>;
pub type CByteSliceRef = crate::ByteSliceRef;

pub type CStrRef = crate::StrRef;

/// not related to [`std::ffi::CStr`] or [`std::ffi::CString`]
pub type CharStrRef = crate::SliceRef<c_char>;

impl From<crate::StrRef> for CharStrRef {
    #[inline(always)]
    fn from(s: crate::StrRef) -> Self {
        Self(SliceInner {
            ptr: s.0.ptr as *mut c_char,
            len: s.0.len,
        })
    }
}

impl CharStrRef {
    #[cfg(feature = "libc")]
    #[inline(always)]
    pub fn as_bytes(&self) -> &[u8] {
        let len = self.len();
        let ptr = self.as_ptr();
        unsafe { std::slice::from_raw_parts(ptr as *const _, len) }
    }
    #[cfg(not(feature = "libc"))]
    #[inline(always)]
    fn as_bytes(&self) -> &[u8] {
        self.as_ref()
    }

    #[inline(always)]
    pub fn to_str(&self) -> Result<&str, std::str::Utf8Error> {
        std::str::from_utf8(self.as_bytes())
    }

    #[inline(always)]
    pub fn unwrap_str(&self) -> &str {
        self.to_str().unwrap()
    }

    #[inline(always)]
    pub fn expect_str(&self, msg: &str) -> &str {
        self.to_str().expect(msg)
    }

    /// # Safety
    /// `self` must be a valid utf-8 string.
    #[inline(always)]
    pub unsafe fn into_rust_unchecked(self) -> crate::StrRef {
        let inner = self.0;
        crate::StrRef(SliceInner {
            ptr: inner.ptr as *mut _,
            len: inner.len,
        })
    }

    #[inline(always)]
    pub fn into_rust(self) -> Result<crate::StrRef, std::str::Utf8Error> {
        std::str::from_utf8(self.as_bytes())?;
        Ok(unsafe { self.into_rust_unchecked() })
    }
}

pub type CBoxedStr = crate::BoxedStr;

pub mod ffi {
    use super::*;

    #[unsafe(export_name = "_rust_ffi_boxed_str_drop")]
    pub unsafe extern "C" fn boxed_str_drop(_string: CBoxedStr) {}

    #[unsafe(export_name = "_rust_ffi_boxed_bytes_drop")]
    pub unsafe extern "C" fn boxed_bytes_drop(_slice: CBoxedSlice<u8>) {}
}

#[test]
fn test_empty_str() {
    // ensure dropping empty str is no-op
    let empty = CBoxedStr::new("".into());
    drop(empty);
}

#[test]
fn test_empty_slice() {
    // ensure dropping empty slice is no-op
    let empty = CBoxedSlice::<u8>::empty();
    drop(empty);
}

#[test]
fn test_empty_char_str() {
    // ensure dropping empty char str is no-op
    let empty = CharStrRef::new(&[]);
    let bytes = empty.as_bytes();
    assert_eq!(bytes, b"");

    let empty = CharStrRef {
        0: SliceInner {
            ptr: 1 as _,
            len: 0,
        },
    };
    let bytes = empty.as_bytes();
    assert_eq!(bytes, b"");
}
