# Graph

## Overview

This part provides a set of classes and structures to represent and manipulate graphs of theorems and tactics. The base classes provide the initial implementation, while the Lean classes extend these for the specific Lean 4 use-case.

Overall, we construct a directed acyclic graph. This graph has theorems for states and tactics as edges.
Note that a single tactic applied on a single theorem can lead to multiple nodes, so this is a hypergraph.
Multiple tactics are applied on the same nodes, and it is enough if one of these tactics solves the node. Solving can be defined as "exists a tactic such that all children are solved". The end of this recursive definition are tactics that lead to an empty set of children, which implies the proof state of the parent is solved with this tactic.
Once such a tactic is added to the graph, we try and propagate the solved state to parents.

## Usage
The Graph class stores a lot of the main logic. The MCTS implementation adds a critic on top of this.

### Seeding
Set env var `SEED` to a number to seed the random number generator.

## Classes and Structures

### `tactic`

Represents a tactic used to prove a theorem.

- **Attributes:**
    - `std::string unique_string`: A unique identifier for the tactic.
    - `bool is_valid`: Indicates if the tactic is valid.
    - `size_t duration`: Duration of the tactic in milliseconds.

- **Methods:**
    - `static tactic from_json(const std::string &json)`: Creates a `tactic` object from a JSON string.
    - `bool operator==(const tactic &t) const`: Equality operator.

### `hypothesis`

Represents a hypothesis in a theorem.

- **Attributes:**
    - `std::string identifier`: Identifier of the hypothesis.
    - `std::string type`: Type of the hypothesis.

- **Methods:**
    - `static hypothesis from_json(const std::string &json)`: Creates a `hypothesis` object from a JSON string.
    - `bool operator==(const hypothesis &h) const`: Equality operator.

### `theorem`

Represents a theorem with a conclusion and hypotheses.

- **Attributes:**
    - `std::string conclusion`: Conclusion of the theorem.
    - `std::vector<hypothesis> hypotheses`: List of hypotheses.
    - `std::string unique_string`: A unique identifier for the theorem.

- **Methods:**
    - `static theorem from_json(const std::string &json)`: Creates a `theorem` object from a JSON string.
    - `bool operator==(const theorem &t) const`: Equality operator.

### `proof`

Represents a proof consisting of a theorem, an optional tactic, and child proofs.

- **Attributes:**
    - `theorem proof_theorem`: Theorem being proved.
    - `std::optional<tactic> proof_tactic`: Tactic used to prove the theorem.
    - `std::vector<proof> children`: Child proofs.

### `lean_context`

Represents the context for Lean theorems.

- **Attributes:**
    - `std::unordered_set<std::string> namespaces`: Set of namespaces in the context.

- **Methods:**
    - `lean_context() = default`: Default constructor.
    - `lean_context(const lean_context &) = default`: Copy constructor.
    - `lean_context(lean_context &&) = default`: Move constructor.
    - `lean_context(std::unordered_set<std::string> namespaces)`: Constructor with namespaces.

### `lean_theorem`

Represents a Lean theorem, extending the base `theorem` class.

- **Attributes:**
    - `lean_context context`: Context for the theorem.


## Design decisions
I left out use_count_threshold_for_solved for now