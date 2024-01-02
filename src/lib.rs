mod boxed;
#[cfg(feature = "cxx")]
mod c;
#[cfg(feature = "cxx")]
pub mod cbindgen;
mod convert;
mod slice;
mod str;

pub use boxed::{Box, OptionBox};
#[cfg(feature = "cxx")]
pub use c::{
    CBox, CBoxedSlice, CBoxedStr, CByteSliceRef, COptionBox, CSliceRef, CStrRef, CharStrRef,
    CXX_HEADER_CONTENT, CXX_HEADER_PATH, CXX_INCLUDE_PATH,
};
pub use convert::{AsFfiType, IntoFfiType, ToFfiType};
pub use slice::{BoxedSlice, ByteSliceRef, MutSliceRef, SliceRef};
pub use str::{BoxedStr, StrRef};

pub type Array<T, const N: usize> = [T; N];

unsafe fn into_static<T: ?Sized>(value: &T) -> &'static T {
    std::mem::transmute(value)
}

unsafe fn into_static_mut<T: ?Sized>(value: &mut T) -> &'static mut T {
    std::mem::transmute(value)
}
