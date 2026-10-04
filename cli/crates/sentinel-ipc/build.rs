use std::{fs, path::Path};
fn main() {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../CMakeLists.txt");
    println!("cargo:rerun-if-changed={}", path.display());
    let cmake = fs::read_to_string(path).expect("canonical Sentinel version");
    let version = cmake
        .lines()
        .find_map(|line| {
            line.strip_prefix("set(SENTINEL_APP_VERSION \"")
                .and_then(|rest| rest.split('"').next())
        })
        .expect("SENTINEL_APP_VERSION");
    println!("cargo:rerun-if-env-changed=SENTINEL_APP_VERSION");
    let version = std::env::var("SENTINEL_APP_VERSION").unwrap_or_else(|_| version.to_owned());
    println!("cargo:rustc-env=SENTINEL_APP_VERSION={version}");
}
