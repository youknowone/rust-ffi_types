pub trait AsFfiType {
    type FfiType;

    fn as_ffi_type(&self) -> Self::FfiType;
}

pub trait ToFfiType {
    type FfiType;

    fn to_ffi_type(self) -> Self::FfiType;
}

pub trait IntoFfiType
where
    Self: Into<Self::FfiType>,
{
    type FfiType;

    fn into_ffi_type(self) -> Self::FfiType {
        self.into()
    }
}

pub trait IntoFfiSlice {
    type FfiType;
    type StaticFfiType;

    fn into_ffi_slice(self) -> Self::FfiType;
    unsafe fn into_ffi_slice_unbound(self) -> Self::StaticFfiType;
}
