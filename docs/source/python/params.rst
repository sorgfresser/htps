HTPS Parameters
===============

This writeup provides detailed descriptions of parameters available for configuring the HyperTree Proof Search (HTPS) algorithm within the **open-htps** library.

Parameters
----------

- **exploration** (*double*):
  Exploration parameter in UCB (Upper Confidence Bound), often referred to as `c`.

- **policy_type** (*enum*):
  Policy selection, either `"AlphaZero"` or `"RPO"`.

- **num_expansions** (*int*):
  Total number of expansions to perform before terminating the search.

- **succ_expansions** (*int*):
  Number of expansions performed per `theorems_to_expand` call. Determines how many theorems are returned for expansion at each step.

- **early_stopping** (*bool*):
  Whether already solved nodes should be excluded from further expansions. Not to be confused with `early_stopping_solved_if_root_not_proven`.

- **no_critic** (*bool*):
  Indicates if a critic model should be used or omitted during the search.

- **backup_once** (*bool*):
  If set to `true`, each subtree is backed up only once, useful when a subtree is selected multiple times due to high `succ_expansions`.

- **backup_one_for_solved** (*bool*):
  Determines whether solved nodes should always back up with a value of 1.0. If `false`, uses the critic value instead.

- **depth_penalty** (*double*):
  A discount factor applied as a penalty based on node depth.

- **count_threshold** (*int*):
  Minimum number of visits a node must have to qualify as a training example.

- **tactic_p_threshold** (*double*):
  Probability threshold from the tree policy for a tactic to be considered a training example.

- **tactic_sample_q_conditioning** (*bool*):
  Enables returning tactic samples for Q-conditioning (for details, see the original HTPS paper).

- **only_learn_best_tactics** (*bool*):
  If `true`, returns only tactic samples involved in minimal proofs.

- **tactic_init_value** (*double*):
  Initial tactic value assigned before a goal has been evaluated by the critic or expanded.

- **q_value_solved** (*enum*):
  Determines the Q-value assignment for solved node-tactic pairs.

- **policy_temperature** (*double*):
  Temperature parameter applied to the tree policy distribution.

- **metric** (*enum*):
  Metric used in minimal proof search. Options include:

  - `"depth"`
  - `"length"`
  - `"time"`

- **node_mask** (*enum*):
  Specifies nodes returned as training examples:

  - `"NoMask"`: all samples are returned
  - `"Solving"`: only samples from solved nodes are returned (does not require the root to be proven)
  - `"Proof"`: only samples from proof nodes are returned (requires the root to be proven)
  - `"MinimalProof"`: only samples from nodes on the minimal proof are returned
  - `"MinimalProofSolving"`: same as `"MinimalProof"` if the root is proven, otherwise same as `"Solving"`

- **effect_subsampling_rate** (*double*):
  Subsampling rate applied to effect samples.

- **critic_subsampling_rate** (*double*):
  Subsampling rate applied to critic samples.

- **early_stopping_solved_if_root_not_proven** (*bool*):
  Stops expansion of solved nodes if the root has not yet been proven, prioritizing proving the root first.

- **virtual_loss** (*int*):
  The number of virtual counts added for each node visit to encourage exploration.



