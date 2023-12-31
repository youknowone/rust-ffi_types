/// Rust wrapper for &[T].
#[repr(transparent)]
pub struct SliceRef<T: 'static>(pub(crate) SliceInner<T>);
static_assertions::assert_eq_size!(SliceRef<u8>, &[u8]);

/// Rust wrapper for &[u8].
/// Though this alias is not very useful in Rust side, it gives a simple aliased buffer type for cbindgen.
pub type ByteSliceRef = SliceRef<u8>;

/// Rust wrapper for &mut [u8].
#[repr(transparent)]
pub struct MutSliceRef<T: 'static>(SliceInner<T>);
static_assertions::assert_eq_size!(MutSliceRef<u8>, &[u8]);

/// Rust wrapper for Box<[T]>.
///
/// Since boxed types are only created from Rust side, the value is expected to be valid under safe operations.
#[repr(transparent)]
pub struct BoxedSlice<T: 'static>(pub(crate) SliceInner<T>);
static_assertions::assert_eq_size!(BoxedSlice<u8>, Box<[u8]>);

impl<T> Clone for SliceRef<T> {
    #[inline(always)]
    fn clone(&self) -> Self {
        *self
    }
}

impl<T> Copy for SliceRef<T> {}

impl<T> SliceRef<T> {
    /// Create a new wrapper for a static slice `&'static [T]`.
    /// See [`SliceRef::new_unbound`] to remove lifetime bound.
    #[inline(always)]
    pub const fn new(slice: &'static [T]) -> Self {
        Self(SliceInner::from_slice(slice))
    }

    /// Create a new wrapper for a slice `&[T]`.
    /// `unbound` means bounded lifetime will be removed to be static.
    ///
    /// # Safety
    /// The returned object must not outlive the given slice.
    #[inline(always)]
    pub unsafe fn new_unbound(slice: &'_ [T]) -> Self {
        Self::new(crate::into_static(slice))
    }

    /// Inverse of [`SliceRef::new`].
    #[inline(always)]
    pub fn into_slice(self) -> &'static [T] {
        let union = SliceUnion { inner: self.0 };
        unsafe { union.slice }
    }
}

impl<T> From<&'static [T]> for SliceRef<T> {
    #[inline(always)]
    fn from(slice: &'static [T]) -> Self {
        Self::new(slice)
    }
}

impl<T> From<SliceRef<T>> for &'static [T] {
    #[inline(always)]
    fn from(slice: SliceRef<T>) -> Self {
        slice.into_slice()
    }
}

impl<T> std::convert::AsRef<[T]> for SliceRef<T> {
    #[inline(always)]
    fn as_ref(&self) -> &[T] {
        self.into_slice()
    }
}

impl<T> std::borrow::Borrow<[T]> for SliceRef<T> {
    #[inline(always)]
    fn borrow(&self) -> &[T] {
        self.as_ref()
    }
}

impl<T> std::ops::Deref for SliceRef<T> {
    type Target = [T];

    #[inline(always)]
    fn deref(&self) -> &Self::Target {
        self.as_ref()
    }
}

impl<T> MutSliceRef<T> {
    /// Create a new wrapper for a static slice `&'static mut [T]`.
    /// See [`MutSliceRef::new_unbound`] to remove lifetime bound.
    #[inline(always)]
    pub fn new(slice: &'static mut [T]) -> Self {
        Self(SliceInner::from_slice(slice))
    }

    /// Create a new wrapper for a slice `&mut [T]`.
    /// `unbound` means bounded lifetime will be removed to be static.
    ///
    /// # Safety
    /// The returned object must not outlive the given slice.
    #[inline(always)]
    pub unsafe fn new_unbound(slice: &'_ mut [T]) -> Self {
        Self::new(crate::into_static_mut(slice))
    }

    /// Inverse of [`MutSliceRef::new`].
    #[inline(always)]
    pub fn into_mut_slice(self) -> &'static mut [T] {
        let union = SliceUnion { inner: self.0 };
        unsafe { union.mut_slice }
    }
}

impl<T> From<&'static mut [T]> for MutSliceRef<T> {
    #[inline(always)]
    fn from(slice: &'static mut [T]) -> Self {
        Self::new(slice)
    }
}

impl<T> From<MutSliceRef<T>> for &'static mut [T] {
    #[inline(always)]
    fn from(slice: MutSliceRef<T>) -> Self {
        slice.into_mut_slice()
    }
}

impl<T> std::convert::AsRef<[T]> for MutSliceRef<T> {
    #[inline(always)]
    fn as_ref(&self) -> &[T] {
        let union = self.0.union();
        unsafe { union.mut_slice }
    }
}

impl<T> std::convert::AsMut<[T]> for MutSliceRef<T> {
    #[inline(always)]
    fn as_mut(&mut self) -> &mut [T] {
        let union = self.0.union();
        unsafe { union.mut_slice }
    }
}

impl<T> std::borrow::Borrow<[T]> for MutSliceRef<T> {
    #[inline(always)]
    fn borrow(&self) -> &[T] {
        self.as_ref()
    }
}

impl<T> std::borrow::BorrowMut<[T]> for MutSliceRef<T> {
    #[inline(always)]
    fn borrow_mut(&mut self) -> &mut [T] {
        self.as_mut()
    }
}

impl<T> std::ops::Deref for MutSliceRef<T> {
    type Target = [T];

    #[inline(always)]
    fn deref(&self) -> &Self::Target {
        self.as_ref()
    }
}

impl<T> std::ops::DerefMut for MutSliceRef<T> {
    #[inline(always)]
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.as_mut()
    }
}

impl<T> Drop for BoxedSlice<T> {
    #[inline(always)]
    fn drop(&mut self) {
        let union: SliceUnion<'_, _> = self.0.union();
        let boxed: Box<[T]> = std::mem::ManuallyDrop::into_inner(unsafe { union.boxed });
        drop(boxed);
    }
}

impl<T> BoxedSlice<T> {
    /// Create a new wrapper for a boxed slice `Box<[T]>`.
    #[inline(always)]
    pub fn new(boxed: std::boxed::Box<[T]>) -> Self {
        let inner = SliceInner::from_slice(boxed.as_ref());
        let raw = Box::into_raw(boxed);
        assert_eq!(inner.ptr, raw as *mut _);
        Self(inner)
    }

    #[inline(always)]
    pub const fn empty() -> Self {
        Self(SliceInner::empty())
    }

    /// Inverse of [`BoxedSlice::new`].
    #[inline(always)]
    pub fn into_boxed_slice(self) -> std::boxed::Box<[T]> {
        let union = self.0.union();
        std::mem::ManuallyDrop::into_inner(unsafe { union.boxed })
    }
}

impl<T> From<std::boxed::Box<[T]>> for BoxedSlice<T> {
    #[inline]
    fn from(value: std::boxed::Box<[T]>) -> Self {
        Self::new(value)
    }
}

impl<T> From<BoxedSlice<T>> for std::boxed::Box<[T]> {
    #[inline(always)]
    fn from(value: BoxedSlice<T>) -> Self {
        value.into_boxed_slice()
    }
}

impl<T> std::convert::AsRef<[T]> for BoxedSlice<T> {
    #[inline(always)]
    fn as_ref(&self) -> &[T] {
        let union = self.0.union();
        unsafe { union.slice }
    }
}

impl<T> std::convert::AsMut<[T]> for BoxedSlice<T> {
    #[inline(always)]
    fn as_mut(&mut self) -> &mut [T] {
        let union = self.0.union();
        unsafe { union.mut_slice }
    }
}

impl<T> std::convert::AsRef<Box<[T]>> for BoxedSlice<T> {
    #[inline(always)]
    fn as_ref(&self) -> &Box<[T]> {
        unsafe { &*(&self.0 as *const SliceInner<T> as *const Box<[T]>) }
    }
}

impl<T> std::convert::AsMut<Box<[T]>> for BoxedSlice<T> {
    #[inline(always)]
    fn as_mut(&mut self) -> &mut Box<[T]> {
        unsafe { &mut *(&mut self.0 as *mut SliceInner<T> as *mut Box<[T]>) }
    }
}

impl<T> std::borrow::Borrow<[T]> for BoxedSlice<T> {
    #[inline(always)]
    fn borrow(&self) -> &[T] {
        self.as_ref()
    }
}

impl<T> std::borrow::BorrowMut<[T]> for BoxedSlice<T> {
    #[inline(always)]
    fn borrow_mut(&mut self) -> &mut [T] {
        self.as_mut()
    }
}

impl<T> std::ops::Deref for BoxedSlice<T> {
    type Target = [T];

    #[inline(always)]
    fn deref(&self) -> &Self::Target {
        self.as_ref()
    }
}

impl<T> std::ops::DerefMut for BoxedSlice<T> {
    #[inline(always)]
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.as_mut()
    }
}

#[repr(C)]
pub(crate) struct SliceInner<T> {
    pub(crate) ptr: *mut T,
    pub(crate) len: usize,
}
static_assertions::assert_eq_size!(SliceInner<u8>, &[u8]);

impl<T> Clone for SliceInner<T> {
    #[inline(always)]
    fn clone(&self) -> Self {
        *self
    }
}

impl<T> Copy for SliceInner<T> {}

impl<T> SliceInner<T> {
    #[inline(always)]
    pub(crate) const fn empty() -> Self {
        Self {
            ptr: std::ptr::null_mut(),
            len: 0,
        }
    }

    #[inline(always)]
    pub(crate) const fn from_slice(slice: &[T]) -> Self {
        Self {
            ptr: slice.as_ptr() as *mut _,
            len: slice.len(),
        }
    }
    #[inline(always)]
    const fn union(self) -> SliceUnion<'static, T> {
        SliceUnion { inner: self }
    }
}

union SliceUnion<'a, T> {
    inner: SliceInner<T>,
    slice: &'a [T],
    mut_slice: &'a mut [T],
    boxed: std::mem::ManuallyDrop<std::boxed::Box<[T]>>,
}
static_assertions::assert_eq_size!(SliceInner<u8>, SliceUnion<u8>);
