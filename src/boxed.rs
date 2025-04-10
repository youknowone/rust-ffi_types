/// A type alias for `std::boxed::Box<T>`.
///
/// `Box` doesn't require a wrapper because it is guaranteed to be layout as a pointer.
pub type Box<T> = std::boxed::Box<T>;

/// A type alias for `Option<Box<T>>`.
///
/// `None` value means a null pointer.
#[repr(C)]
pub struct OptionBox<T> {
    pub ptr: *mut T,
}
static_assertions::assert_eq_size!(OptionBox<u8>, *const u8);
static_assertions::assert_eq_size!(OptionBox<u8>, Box<u8>);
static_assertions::assert_eq_size!(OptionBox<u8>, Option<Box<u8>>);

impl<T> std::fmt::Debug for OptionBox<T>
where
    T: 'static + std::fmt::Debug,
{
    #[inline(always)]
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        unsafe { self.ptr.as_ref().fmt(f) }
    }
}

impl<T> OptionBox<T> {
    #[inline(always)]
    pub fn new(boxed: Box<T>) -> Self {
        Self {
            ptr: Box::into_raw(boxed),
        }
    }

    #[inline(always)]
    pub fn from_value(value: T) -> Self {
        Self::new(Box::new(value))
    }

    #[inline(always)]
    pub fn into_box(self) -> Option<Box<T>> {
        if self.ptr.is_null() {
            return None;
        }
        Some(unsafe { Box::from_raw(self.ptr) })
    }

    #[inline(always)]
    pub const fn none() -> Self {
        Self {
            ptr: std::ptr::null_mut(),
        }
    }

    #[inline]
    pub fn as_ref(&self) -> Option<&T> {
        unsafe { self.ptr.as_ref() } // SAFETY: `ptr` is a value of a valid Box
    }
}

impl<T> From<Box<T>> for OptionBox<T> {
    #[inline]
    fn from(boxed: Box<T>) -> Self {
        Self::new(boxed)
    }
}

impl<T> From<Option<Box<T>>> for OptionBox<T> {
    #[inline]
    fn from(boxed: Option<Box<T>>) -> Self {
        if let Some(boxed) = boxed {
            Self::new(boxed)
        } else {
            Self::none()
        }
    }
}

impl<T> Default for OptionBox<T> {
    #[inline(always)]
    fn default() -> Self {
        Self::none()
    }
}

// impl<T> std::convert::AsRef<Option<Box<T>>> for OptionBox<T> {
//     #[inline(always)]
//     fn as_ref(&self) -> &Option<Box<T>> {
//         &self.0
//     }
// }

// impl<T> std::borrow::Borrow<Option<Box<T>>> for OptionBox<T> {
//     #[inline(always)]
//     fn borrow(&self) -> &Option<Box<T>> {
//         self.as_ref()
//     }
// }

// impl<T> std::ops::Deref for OptionBox<T> {
//     type Target = Option<Box<T>>;

//     #[inline(always)]
//     fn deref(&self) -> &Self::Target {
//         self.as_ref()
//     }
// }
