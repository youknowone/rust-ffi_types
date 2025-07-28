const CXX_TYPE_NAMES: &[&str] = &[
    // array
    "Array",
    // simple box
    "Box",
    "OptionBox",
    // slices
    "SliceRef",
    "MutSliceRef",
    "BoxedSlice",
    "ByteSliceRef",
    // strings
    "StrRef",
    "BoxedStr",
];
const CXX_WRAPPER_NAMES: &[&str] = &[
    // simple box
    "CBox",
    "COptionBox",
    // slices
    "CSliceRef",
    "CByteSliceRef",
    "CBoxedSlice",
    // strings
    "CStrRef",
    "CBoxedStr",
    "CharStrRef",
];

#[must_use]
pub fn with_cxx_ffi_types(builder: cbindgen::Builder) -> cbindgen::Builder {
    with_cxx_ffi_types_with_namespace(builder, "ffi_types")
}

#[must_use]
pub fn with_cxx_ffi_types_with_namespace(
    mut builder: cbindgen::Builder,
    namespace: &str,
) -> cbindgen::Builder {
    // builder = builder.with_sys_include("rust_types.h");
    for name in CXX_TYPE_NAMES {
        builder = builder.exclude_item(name);
        builder = builder.rename_item(name, &format!("{namespace}::{name}").as_str());
    }
    for name in CXX_WRAPPER_NAMES {
        builder = builder.exclude_item(name);
        builder = builder.rename_item(name, &format!("{namespace}::{name}").as_str());
    }

    builder
}
