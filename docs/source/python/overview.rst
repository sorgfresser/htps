Python Overview
===============

The HyperTreeProofSearch algorithm treats a proof search as a hypergraph, where a single goal is treated as a node.
A tactic is a hyperedge from one child to a list of children.
The original algorithm was developed by Lample et al. in "HyperTree Proof Search for Neural Theorem Proving" (2022).

To start a new search, an initial theorem is required.
A ``Theorem`` consists of the conclusion (i.e. the goal), the list of hypotheses, and the currently open namespaces, wrapped in a ``Context`` object.
Furthermore, a ``Theorem`` has a list of past tactics --- initially, this list is empty.

To construct a new context, use the ``Context`` constructor.

.. code-block:: python

   # Open the namespaces Nat, Filter, and Topology
   from htps import Context
   context = Context(["Nat", "Filter", "Topology"])

The hypotheses of a theorem are ``Hypothesis`` objects, made up of a unique identifier and the type (which we call value, to not collide with Python's type keyword).
Most objects in **open-htps** require some kind of uniqueness, and multiple objects with the same unique identifier will be merged internally.
For most use-cases, the unique_string of a hypothesis will simply be the type of the hypothesis --- but maybe, your use-case requires a different definition.

.. code-block:: python

   from htps import Hypothesis
   hypothesis = Hypothesis("p", "p")


As theorems should be unique, and uniqueness might be defined differently depending on the environment and task, theorems explicitly require a unique string.
In most cases, this will be either the conclusion or the concatenation of hypotheses and conclusion --- but your use-case might require a different definition.

To construct a new theorem, use the ``Theorem`` constructor.

.. code-block:: python

   from htps import Theorem
   theorem = Theorem(conclusion="p ∧ q", unique_string="p ∧ q",
        hypotheses=[Hypothesis("p", "p"), Hypothesis("q", "q")],
        context=context, past_tactics=[])

Now that we have a theorem in place, we can start the search.
The search algorithm is implemented in the ``HTPS`` class, which is the main entry point for the algorithm.
In addition to the theorem, the search requires a set of parameters, which will be explained later.

.. code-block:: python

   from htps import HTPS
   search = HTPS(theorem, params)
   while not search.is_done():
       """Get theorems, generate tactics, and pass them to the environment.
          Report the results back to the search algorithm."""


To get new theorems for which tactics should be generated, call the ``theorems_to_expand`` method.
This method returns a list of ``Theorem`` objects, each one goal, for which tactics shoould be generated (called expand).

.. code-block:: python

   theorems = search.theorems_to_expand()

Once tactics are generated, these tactics need to be passed into the environment. The result will be some kind of triple consisting of (1) the old theorem object (2) the tactic and (3) the new goals.

.. code-block:: python

   old_theorem, tactic, new_goals

Given these three, an ``EnvExpansion`` object can be constructed. To allow for multiple tactics to be generated at the same time, the ``EnvExpansion`` object can store the results of multiple tactics for the same theorem.
In addition, an ``EnvExpansion`` object can store additional information, such as the duration of the tactic generation, the duration of the environment, and the overall expander duration.
Furthermore, if a critic model is used to assess the quality of each goal, the log value of the old goal can be stored and will subsequently be used internally to decide which goal to expand next.

.. code-block:: python

   from htps import Tactic, EnvEffect, EnvExpansion

   tactic_1 = Tactic("apply h", is_valid=True, duration=60)
   tactic_2 = Tactic("simp", is_valid=True, duration=40)

   effects = [EnvEffect(old_theorem, tactic_1, new_goals_1), EnvEffect(old_theorem, tactic_2, new_goals_2)]

   expansion = EnvExpansion(thm=old_theorem, expander_duration=100, generation_duration=50, env_durations=[20, 20],
    effects=effects, log_critic=-0.4, tactics=[tactic_1, tactic_2], children_for_tactic=[new_goals_1, new_goals_2], priors=[0.5, 0.5])

Okay, this was a lot in one go - let's break it down a bit.
The ``EnvExpansion`` object is the main object that is passed between the environment and the algorithm.
It contains all the information that the algorithm needs to decide which goal to expand next.

The ``EnvEffect`` object is a simple container for the old goal, the tactic, and the new goals.
A single expansion can contain multiple effects, as multiple tactics can be generated for a single goal.
Each tactic should be applied in the environment, and the results should be stored in the ``EnvEffect`` object.
A tactic, initially a string, is wrapped in a ``Tactic`` object, which contains the string, a boolean indicating whether the tactic is valid (i.e. whether it lead to an error in the environment), and the duration of the tactic in the environment.
This, together with the overall duration information, is then stored in the ``EnvExpansion`` object.
Furthermore, the ``EnvExpansion`` object contains the log critic value of the old goal (if a critic model is used), and the prior probabilities of the tactics generated which will be used internally to decide which goal to expand next.

That's it! You now know how to interact with the **open-htps** library.
Next up, consider learning about the parameters of the search algorithm, or take a look at the LeanREPL example to see how the algorithm can be used in practice.
