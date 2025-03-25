from setuptools import setup, Extension

setup(
    name="htps",
    author="Ash Vardanian",
    license="Apache-2.0",
    include_dirs=[],
    setup_requires=[],
    ext_modules=[
        Extension(
        "htps",
        ["python/htps.cpp", "src/graph/lean.cpp", "src/graph/htps.cpp", "src/tokenization/tokenization_utils.cpp", "src/tokenization/tokenizer.cpp", "src/graph/base.cpp", "src/graph/graph.cpp", "src/env/core.cpp"],
        include_dirs=["src", "external/Tokenizer", "external/Tokenizer/include", "external/Tokenizer/build", "external/glob/single_include"],
        library_dirs=["external/Tokenizer/build"],
        runtime_library_dirs=["external/Tokenizer/build"],
        libraries=["OpenNMTTokenizer"],
        extra_compile_args=["-std=c++20", "-O3", "-pedantic", "-DPYTHON_BINDINGS"], # Compiler specific
        extra_link_args=[],
        define_macros=[],
    ),
],
)
