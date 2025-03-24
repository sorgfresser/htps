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
        ["python/htps.cpp"],
        include_dirs=["src"],
        extra_compile_args=["-std=c++20", "-O3", "-pedantic"], # Compiler specific
        extra_link_args=[],
        define_macros=[],
    ),
],
)
