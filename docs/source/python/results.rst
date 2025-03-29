Otaining Results
================

Once a search has been run, the results can be accessed through the `Results` object.

Results
-------
A result consists of various training samples useful for the different models in expert iteration.
Specifically, the following samples are available:

- `tactic_samples`: to train a tactic generation model
- `critic_samples`: to train a critic model
- `effect_samples`: to train a dynamics model to predict the environment
- `proof_samples_tactics`: only the tactic samples that are used in the minimal proof

Minimal here refers to the minimal proof that is found by the search algorithm, with minimality defined by the metric specified in the search parameters.
The node mask is applied to the samples to determine which nodes are included in the training set, see the search parameters for more details.

The `Results` object contains a few more fields

- `metric`: The metric for minimality used in the search
- `goal`: The initial root goal
- `proof`: The found minimal proof as a `Proof` object for the root goal (see below)

Proof objects
-------------
A `Proof` object refers to a single node in the proof tree. It consists of three fields:

- `theorem`: The goal of the node
- `tactic`: The tactic used in the proof for the goal
- `children`: A list of `Proof` objects that are the children of this node, forming the proof tree

Note that many proofs can be found in a single HTPS, but the returned proof is only the minimal one. If you want all proofs, use the tactic samples in the `Results` object.


Usage
-----
For a given search, the result can be accessed via `get_result` on the search object.

.. code-block:: python

    from htps import Result

    result = search.get_result()
    result.goal # Theorem
    result.metric # Metric
    result.tactic_samples # List[SampleTactic]
    result.critic_samples # List[SampleCritic]
    result.effect_samples # List[SampleEffect]
    result.proof_samples_tactics # List[SampleTactic]
    result.proof # Proof

Result samples
--------------
The samples are dataclasses, containing various fields useful for training.
If any of the dataclasses is lacking attributes you need, please open an issue on the GitHub repository and I will be happy to add them.

Tactic samples
^^^^^^^^^^^^^^
The `SampleTactic` dataclass contains the goal, the tactics generated, the probability of each tactic given by the tree policy, the action value estimated by the search, and the visit count of the node.
The `SampleTactic` dataclass also contains an indication for whether the given sample was part of a proof or not, in the `inproof` enum.

.. code-block:: python

    from htps import SampleTactic

    sample = result.tactic_samples[0]
    sample.goal
    sample.tactics
    sample.target_pi
    sample.q_estimates
    sample.visit_count
    sample.inproof # NotInProof, InProof, InMinimalProof

Critic samples
^^^^^^^^^^^^^^
The `SampleCritic` dataclass comprises the goal, the target action value estimate by the search, the initial critic value, the visit count of the node, and a boolean whether the node was solved.

.. code-block:: python

    from htps import SampleCritic

    sample = result.critic_samples[0]
    sample.goal
    sample.q_estimate
    sample.critic # Initial critic value
    sample.visit_count
    sample.solved

Effect samples
^^^^^^^^^^^^^^
The `SampleEffect` dataclass is made up of the goal, the applied tactic, and the children of the node.

.. code-block:: python

    from htps import SampleEffect

    sample = result.effect_samples[0]
    sample.goal
    sample.tactic
    sample.children
