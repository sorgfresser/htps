from setuptools import setup, Extension
import os

module = Extension(
    "htps",
    sources=[
        "python/htps.cpp", "src/graph/lean.cpp", "src/graph/htps.cpp",
        "src/graph/base.cpp", "src/graph/graph.cpp", "src/env/core.cpp",
        "src/model/policy.cpp"
    ],
    include_dirs=["src", "python", "external/glob/single_include"],
    extra_compile_args=["-std=c++20", "-O3", "-pedantic", "-DPYTHON_BINDINGS"],
    extra_link_args=[],
    define_macros=[],
)

directory = os.path.abspath(os.path.dirname(__file__))
with open(os.path.join(directory, "README.md"), encoding="utf-8") as f:
    long_description = f.read()


setup(
    name="htps",
    include_dirs=[],
    setup_requires=[],
    license="Apache-2.0",
    license_files=("LICENSE",),
    author="Simon Sorg",
    long_description=long_description,
    long_description_content_type="text/markdown",
    classifiers=[
        "Programming Language :: Python :: 3",
    ],
    ext_modules=[module],
    description="Open-source implementation of HyperTree Proof Search",
    version="0.0.4",
)
