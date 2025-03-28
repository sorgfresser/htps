from setuptools import setup, Extension

module = Extension(
    "htps",
    sources=[
        "python/htps.cpp", "src/graph/lean.cpp", "src/graph/htps.cpp",
        "src/graph/base.cpp", "src/graph/graph.cpp", "src/env/core.cpp",
        "src/model/policy.cpp"
    ],
    include_dirs=["src", "external/glob/single_include"],
    extra_compile_args=["-std=c++20", "-O3", "-pedantic", "-DPYTHON_BINDINGS"],
    extra_link_args=[],
    define_macros=[],
)

setup(
    name="htps",
    author="Ash Vardanian",
    license="Apache-2.0",
    include_dirs=[],
    setup_requires=[],
    ext_modules=[module],
)
