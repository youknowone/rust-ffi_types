/// A type alias for `std::boxed::Box<T>`.
///
/// `Box` doesn't require a wrapper because it is guaranteed to be layout as a pointer.
pub type Box<T> = alloc::boxed::Box<T>;

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

impl<T> core::fmt::Debug for OptionBox<T>
where
    T: 'static + core::fmt::Debug,
{
    #[inline(always)]
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        unsafe { self.ptr.as_ref().fmt(f) }
    }
}

impl<T> Clone for OptionBox<T>
where
    T: 'static + Clone,
{
    #[inline(always)]
    fn clone(&self) -> Self {
        if self.ptr.is_null() {
            return Self::none();
        }
        let borrowed_box = unsafe { Box::from_raw(self.ptr) };
        let cloned = borrowed_box.clone();
        core::mem::forget(borrowed_box);
        cloned.into()
    }
}

impl<T> Drop for OptionBox<T> {
    #[inline(always)]
    fn drop(&mut self) {
        if !self.ptr.is_null() {
            drop(unsafe { Box::from_raw(self.ptr) });
        }
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
            ptr: core::ptr::null_mut(),
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

// impl<T> core::convert::AsRef<Option<Box<T>>> for OptionBox<T> {
//     #[inline(always)]
//     fn as_ref(&self) -> &Option<Box<T>> {
//         &self.0
//     }
// }

// impl<T> core::borrow::Borrow<Option<Box<T>>> for OptionBox<T> {
//     #[inline(always)]
//     fn borrow(&self) -> &Option<Box<T>> {
//         self.as_ref()
//     }
// }

// impl<T> core::ops::Deref for OptionBox<T> {
//     type Target = Option<Box<T>>;

//     #[inline(always)]
//     fn deref(&self) -> &Self::Target {
//         self.as_ref()
//     }
// }
