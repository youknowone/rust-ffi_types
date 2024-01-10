// TODO: The generation step can be placed out of the crate with a separated crate.

#[allow(dead_code)]
fn crate_dir() -> anyhow::Result<String> {
    let crate_dir = std::env::var("CARGO_MANIFEST_DIR")?;
    Ok(crate_dir)
}

#[cfg(feature = "__build_header")]
fn generate_impl() -> anyhow::Result<()> {
    let mut config = cbindgen::Config::default();
    config.language = cbindgen::Language::Cxx;
    config.namespace = Some("ffi_types".to_owned());
    config.cpp_compat = true;
    config.pragma_once = true;
    config.no_includes = true;
    config.after_includes = Some(
        r#"
//! This header is intended to be included in rust_types.hh file.
    "#
        .to_owned(),
    );
    for name in &["CBoxedStr", "CBoxedSlice", "CBox", "COptionBox", "SliceRef"] {
        config.export.exclude.push(name.to_string());
        config
            .export
            .rename
            .insert(name.to_string(), format!("ffi_types::{}", name));
    }

    let builder = cbindgen::Builder::new()
        .with_config(config)
        .with_crate(crate_dir()?);
    builder.generate()?.write_to_file("cxx/7rust_impl.hxx");

    Ok(())
}

#[cfg(feature = "__build_header")]
fn concat_header() -> anyhow::Result<()> {
    let mut cxx_dir = crate_dir()?;
    cxx_dir.push_str("/cxx");

    let mut paths = Vec::new();
    for entry in std::fs::read_dir(cxx_dir)? {
        let entry = entry?;
        let path = entry.path();
        if path.is_file() && path.extension().unwrap_or_default() == "hxx" {
            paths.push(path);
        }
    }
    paths.sort_unstable();

    let mut out_content = String::new();
    paths.iter().for_each(|path| {
        println!("cargo:rerun-if-changed={:?}", path);
        let content = std::fs::read_to_string(path).unwrap();
        out_content.push_str(&content);
    });

    let mut out_path = crate_dir()?;
    out_path.push_str("/include");
    out_path.push_str("/rust_types.hxx");
    std::fs::write(out_path, out_content)?;

    Ok(())
}

#[cfg(feature = "__build_header")]
fn make_header() -> anyhow::Result<()> {
    generate_impl()?;
    concat_header()?;

    cc::Build::new()
        .std("c++17")
        .file("cxx/test.cxx")
        .cpp(true)
        .compile("cxx_header_test");

    Ok(())
}

fn main() -> anyhow::Result<()> {
    #[cfg(feature = "__build_header")]
    make_header()?;

    Ok(())
}
