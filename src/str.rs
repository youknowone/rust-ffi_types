use crate::slice::SliceInner;

/// Rust wrapper for &str.
///
/// This type *MUST* guarantee the internal value is a valid UTF-8 string.
/// Useful mostly to expose Rust string out, because a valid Rust string is a valid C++ string and mostly for C too.
///
/// To get strings from outside, use [`CharStrRef`].
#[repr(C)]
pub struct StrRef(pub(crate) SliceInner<u8>);
static_assertions::assert_eq_size!(StrRef, &str);

/// Rust wrapper for Box<str>.
///
/// Since boxed types are only created from Rust side, the value is expected to be valid under safe operations.
#[repr(C)]
pub struct BoxedStr(SliceInner<u8>);
static_assertions::assert_eq_size!(BoxedStr, std::boxed::Box<str>);

impl Clone for StrRef {
    #[inline(always)]
    fn clone(&self) -> Self {
        *self
    }
}

impl Copy for StrRef {}

impl StrRef {
    /// Create a new wrapper for a `&'static str`.
    /// See [`StrRef::new_unbound`] to remove lifetime bound.
    #[inline(always)]
    pub const fn new(value: &'static str) -> Self {
        Self(SliceInner::from_str(value))
    }

    /// Create a new wrapper for a `&str`.
    /// `unbound` means bounded lifetime will be removed to be static.
    ///
    /// # Safety
    /// The returned object must not outlive the given slice.
    #[inline(always)]
    pub unsafe fn new_unbound(value: &'_ str) -> Self {
        Self::new(crate::into_static(value))
    }

    #[inline(always)]
    pub const fn as_str(&self) -> &'static str {
        self.into_str()
    }

    /// Inverse of [`StrRef::new`].
    #[inline(always)]
    pub const fn into_str(self) -> &'static str {
        let union = StrUnion { inner: self.0 };
        unsafe { union.str }
    }
}

impl From<&'static str> for StrRef {
    #[inline(always)]
    fn from(s: &'static str) -> Self {
        Self::new(s)
    }
}

impl From<StrRef> for &'static str {
    #[inline(always)]
    fn from(s: StrRef) -> Self {
        s.into_str()
    }
}

impl std::convert::AsRef<str> for StrRef {
    #[inline(always)]
    fn as_ref(&self) -> &'static str {
        self.into_str()
    }
}

impl std::borrow::Borrow<str> for StrRef {
    #[inline(always)]
    fn borrow(&self) -> &str {
        self.as_ref()
    }
}

impl std::ops::Deref for StrRef {
    type Target = str;

    #[inline(always)]
    fn deref(&self) -> &Self::Target {
        self.as_ref()
    }
}

impl BoxedStr {
    /// Create a new wrapper for a `Box<str>`.
    #[inline(always)]
    pub fn new(value: Box<str>) -> Self {
        let inner = SliceInner::from_str(&value);
        let raw = Box::into_raw(value);
        assert_eq!(inner.ptr, raw as *mut _);
        Self(inner)
    }

    /// Inverse of [`BoxedStr::new`].
    #[inline(always)]
    pub fn into_boxed_str(self) -> Box<str> {
        let union = self.0.str_union();
        std::mem::ManuallyDrop::into_inner(unsafe { union.boxed })
    }
}

impl From<std::boxed::Box<str>> for BoxedStr {
    #[inline(always)]
    fn from(value: std::boxed::Box<str>) -> Self {
        Self::new(value)
    }
}

impl From<BoxedStr> for std::boxed::Box<str> {
    #[inline(always)]
    fn from(value: BoxedStr) -> Self {
        value.into_boxed_str()
    }
}

impl std::convert::AsRef<str> for BoxedStr {
    #[inline(always)]
    fn as_ref(&self) -> &str {
        let union = self.0.str_union();
        unsafe { union.str }
    }
}

impl std::convert::AsRef<Box<str>> for BoxedStr {
    #[inline(always)]
    fn as_ref(&self) -> &Box<str> {
        unsafe { &*(&self.0 as *const SliceInner<u8> as *const Box<str>) }
    }
}

impl std::borrow::Borrow<str> for BoxedStr {
    #[inline(always)]
    fn borrow(&self) -> &str {
        self.as_ref()
    }
}

impl std::ops::Deref for BoxedStr {
    type Target = str;

    #[inline(always)]
    fn deref(&self) -> &Self::Target {
        self.as_ref()
    }
}

pub(crate) union StrUnion {
    inner: SliceInner<u8>,
    str: &'static str,
    boxed: std::mem::ManuallyDrop<std::boxed::Box<str>>,
}
static_assertions::assert_eq_size!(SliceInner<u8>, StrUnion);

impl SliceInner<u8> {
    #[inline(always)]
    pub const fn from_str(value: &str) -> Self {
        let bytes = value.as_bytes();
        Self::from_slice(bytes)
    }

    #[inline(always)]
    pub const fn str_union(self) -> StrUnion {
        StrUnion { inner: self }
    }
}
