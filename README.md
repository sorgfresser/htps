# Open HTPS

**Open HTPS** is an open-source implementation of **HyperTreeProofSearch (HTPS)**, an efficient proof search algorithm introduced by Lample et al. (2020). This repository addresses a crucial gap in the Automated Theorem Proving community: the need for a fast, fully open-source, and easy-to-use proof search tool.

Unlike the non-functional original implementation, Open HTPS provides a complete and reliable version implemented efficiently in C++, combined with intuitive Python bindings for easy integration into your research workflows.

## Why Open HTPS?
- **Efficiency:** Fast C++ implementation ensures high-performance proof searches.
- **Usability:** Python bindings simplify integration, allowing researchers to easily incorporate it into existing pipelines.
- **Research-Focused:** Abstracts away complex tree-search mechanics, enabling researchers to concentrate on tactic generation, critics, environments, and conjecture generation.
- **Open Source:** Fully available for customization and extension by the research community.

## Key Features
- ✅ **Fast and Efficient Proof Search**
- ✅ **Proof Assistant Agnostic**: Works seamlessly with various proof assistants.
- ✅ **User-Friendly Python Interface**
- ✅ **Training Data Generation**: Provides training samples for tactic generators, critics, and dynamics models.

## Quick Start

### Installation

Open HTPS is available on PyPI. Before installation, ensure you have a C++ compiler and Python development headers (`Python-dev`) installed:

```bash
pip3 install htps
```

### Example Usage

The overall workflow is detailed below. Since this implementation does not rely on any environments, the code below is missing the environment-specific expansion. 

```python
from htps import HTPS

# Initialize proof search with a theorem
search = HTPS(thm)

# Retrieve theorems ready for expansion
theorems = search.theorems_to_expand()

# -----Expand here-----

# Generate and apply tactics to expand theorems
search.expand_and_backup(expansions)

# Retrieve search result
result = search.get_result()

# Access generated training samples
tactic_samples = result.tactic_samples
critic_samples = result.critic_samples
effect_samples = result.effect_samples
```
The full information can be found in the [documentation](https://open-htps.readthedocs.io/en/latest/).


## Contributing

Contributions are welcome! Feel free to open issues or submit pull requests. I will make sure to address any issues as soon as possible.

# Rationale

This project is based on one belief: more search is beneficial, but the exact search algorithm does not significantly impact the research outcome.

Currently, every research team has to implement their own proof search algorithm, which is a significant waste of time and resources.
This repository is meant to address that, ideally leading to faster prototyping on tactic generation, critics, dynamic models and conjecture generation.
Less time wasted on debugging tree search implementations (which is quite painful, as I am now very much aware), more time spent on the meaningful research.
