open-htps: Fast and Open-Source HyperTree Proof Search
======================================================

**open-htps** is a high-performance CPython extension designed to efficiently implement HyperTree Proof Search.
Built specifically for research teams of varying sizes, it facilitates rapid iterations on tactics, conjecture generation, and environment interaction methods.
Leveraging the speed of C++ and offering an intuitive Python interface, **open-htps** abstracts the complexities of tree search, enabling users to focus on innovative aspects of their research rather than tedious search implementation details.

Key Features
============

- **Efficient C++ Implementation**: High-performance search algorithms ensure that tree searches does not become a bottleneck.
- **User-Friendly Python Interface**: Seamless integration into Python-based workflows.
- **Flexible Environment Support**: Compatible with arbitrary environments, enabling wide-ranging applications.
- **Customizable Tactic Generation**: Easily integrate various methods of tactic generation into your workflow.
- **Unlimited Scalability**: Supports parallel execution of multiple search processes, scaling with your computational resources.

Flexibility and Extensibility
-----------------------------

Extendable to Multiple Environments
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**open-htps** is designed for prototyping in Automated Theorem Proving. The only requirement is that environment-specific data must be convertible into the `EnvExpansion` format. An example implementation for the Lean REPL is provided.

If your use-case cannot currently be expressed as an `EnvExpansion`, please create an issue, and adjustments to the core C++ code will be made promptly.

Custom Tactic Generation
^^^^^^^^^^^^^^^^^^^^^^^^

Integrate your preferred tactic generation tools effortlessly. **open-htps** provides `Theorem` objects that represent current search states, enabling the integration of arbitrary tactic generators.
After tactic generation, simply pass the results back into your environment and return the corresponding `EnvExpansion` to the search algorithm.

Conjecture Generation
^^^^^^^^^^^^^^^^^^^^^

Generate and explore new theorem statements with ease. Each new conjecture can initialise a fresh search object, integrating into your existing tactic generation and environment processing pipeline.

---

With **open-htps**, tree search complexities are minimized, empowering you to advance swiftly through your research's creative and innovative components.

Contents
========
.. toctree::
   :maxdepth: 2

   python/overview
   python/htps
   cpp/htps