fn main() {
    let dst = cmake::Config::new("..")
        .define("CMAKE_C_COMPILER", "clang")
        .define("CMAKE_CXX_COMPILER", "clang++")
        .define("SPLINTER_LIB_ONLY", "On")
        .build();

    println!("cargo:rustc-link-search=native={}/lib", dst.display());
    println!("cargo:rustc-link-lib=static=Splinter");
}
