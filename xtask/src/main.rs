use std::path::Path;

fn crate_dir() -> &'static Path {
    Path::new(env!("CARGO_MANIFEST_DIR")).parent().unwrap()
}

fn generate_impl() -> anyhow::Result<()> {
    let crate_dir = crate_dir();

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
    for name in &[
        "CBoxedStr",
        "CBoxedSlice",
        "CBoxedByteSlice",
        "CBox",
        "COptionBox",
        "SliceRef",
    ] {
        config.export.exclude.push(name.to_string());
        config
            .export
            .rename
            .insert(name.to_string(), format!("ffi_types::{}", name));
    }

    let builder = cbindgen::Builder::new()
        .with_config(config)
        .with_crate(crate_dir);
    builder
        .generate()?
        .write_to_file(crate_dir.join("cxx/7rust_impl.hxx"));

    Ok(())
}

fn concat_header() -> anyhow::Result<()> {
    let crate_dir = crate_dir();
    let cxx_dir = crate_dir.join("cxx");

    let mut paths = Vec::new();
    for entry in std::fs::read_dir(&cxx_dir)? {
        let entry = entry?;
        let path = entry.path();
        if path.is_file() && path.extension().unwrap_or_default() == "hxx" {
            paths.push(path);
        }
    }
    paths.sort_unstable();

    let mut out_content = String::new();
    for path in &paths {
        let content = std::fs::read_to_string(path)?;
        out_content.push_str(&content);
    }

    let out_path = crate_dir.join("include/rust_types.hxx");
    std::fs::write(out_path, out_content)?;

    Ok(())
}

fn verify_header() -> anyhow::Result<()> {
    let crate_dir = crate_dir();
    let test_file = crate_dir.join("cxx/test.cxx");
    let out_dir = crate_dir.join("target/xtask-tmp");
    std::fs::create_dir_all(&out_dir)?;
    let out_file = out_dir.join("cxx_header_test.o");

    let cxx = std::env::var("CXX").unwrap_or_else(|_| "c++".to_string());
    let status = std::process::Command::new(&cxx)
        .arg("-std=c++17")
        .arg("-c")
        .arg(&test_file)
        .arg("-o")
        .arg(&out_file)
        .status()?;

    anyhow::ensure!(status.success(), "C++ header verification failed");
    Ok(())
}

fn main() -> anyhow::Result<()> {
    let args: Vec<String> = std::env::args().skip(1).collect();
    let cmd = args.first().map(|s| s.as_str());

    match cmd {
        Some("header") => {
            generate_impl()?;
            concat_header()?;
            verify_header()?;
            println!("Header generated and verified successfully.");
        }
        _ => {
            eprintln!("Usage: cargo xtask <command>");
            eprintln!();
            eprintln!("Commands:");
            eprintln!("  header    Generate and verify C++ headers");
            std::process::exit(1);
        }
    }

    Ok(())
}
