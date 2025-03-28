import pytest

from htps import *


def _compare_hypotheses(ha, hb):
    assert len(ha) == len(hb)
    for orig, new in zip(ha, hb):
        assert new.identifier == orig.identifier
        assert new.value == orig.value

def _compare_tactics(ta, tb):
    assert len(ta) == len(tb)
    for orig, new in zip(ta, tb):
        assert new.unique_string == orig.unique_string
        assert new.is_valid == orig.is_valid
        assert new.duration == orig.duration

def _compare_theorem(a, b):
    assert a.conclusion == b.conclusion and a.unique_string == b.unique_string and \
           a.context.namespaces == b.context.namespaces
    _compare_hypotheses(a.hypotheses, b.hypotheses)
    _compare_tactics(a.past_tactics, b.past_tactics)


def test_params():
    params =SearchParams(0.3, PolicyType.RPO, 10, 3, True, True, True, True, 0.99, 10, True, True, True, 0.0, QValueSolved.One, 0.7, Metric.Time, NodeMask.NoMask, 1.0, 1.0, True, 1)
    assert params.policy_type == PolicyType.RPO
    assert params.q_value_solved == QValueSolved.One
    assert params.metric == Metric.Time
    assert params.node_mask == NodeMask.NoMask


def test_tactic():
    tactic = Tactic("", True, 5)
    tactic = Tactic("a", True, 5)
    tactic = Tactic("∧", True, 5)
    assert tactic.unique_string == "∧"


def test_hypothesis():
    hypothesis = Hypothesis("", "")
    hypothesis = Hypothesis(" ", "")
    hypothesis = Hypothesis("", " ")
    hypothesis = Hypothesis(" ", " ")
    hypothesis = Hypothesis("∧", " ")
    assert hypothesis.identifier == "∧"
    hypothesis = Hypothesis(" ", "∧")
    assert hypothesis.value == "∧"


def test_context():
    context = Context([])
    del context
    context = Context(["", " "])
    del context
    context = Context(["∧", "", " "])
    assert any(ns == "∧" for ns in context.namespaces)
    del context


def test_theorem_context():
    context = Context(["∧", "", " "])
    theorem = Theorem("", "", [], context, [])
    assert theorem.unique_string == ""
    assert theorem.conclusion == ""
    assert theorem.context.namespaces == context.namespaces
    context2 = Context(["a∧a"])
    theorem.context = context2
    assert theorem.context.namespaces == context2.namespaces
    context3 = Context([])
    theorem.context = context3
    assert theorem.context.namespaces == context3.namespaces


def test_theorem_hypothesis():
    context = Context(["∧", "", " "])
    hypotheses = [Hypothesis("∧", " "), Hypothesis("", " "), Hypothesis("", ""), Hypothesis(" ", "")]
    theorem = Theorem("andb∧", "∧ac", hypotheses, context, [])
    assert theorem.unique_string == "∧ac"
    assert theorem.conclusion == "andb∧"

    hyp_list = theorem.hypotheses
    assert isinstance(hyp_list, list)
    _compare_hypotheses(hypotheses, hyp_list)

    hypotheses = []
    theorem.hypotheses = hypotheses
    hyp_list = theorem.hypotheses
    assert isinstance(hyp_list, list)
    _compare_hypotheses(hypotheses, hyp_list)

    hypotheses = [Hypothesis("∧", " "), Hypothesis("", " "), Hypothesis("", ""), Hypothesis(" ", "")]
    theorem.hypotheses = hypotheses
    hyp_list = theorem.hypotheses
    assert isinstance(hyp_list, list)
    _compare_hypotheses(hypotheses, hyp_list)

    with pytest.raises(TypeError):
        hypotheses_fail = [Hypothesis("∧", " "), Hypothesis("", " "), Hypothesis("", ""), 3]
        theorem.hypotheses = hypotheses_fail

    # Test old is still valid
    hyp_list = theorem.hypotheses
    assert isinstance(hyp_list, list)
    _compare_hypotheses(hypotheses, hyp_list)


def test_theorem_tactic():
    context = Context(["∧", "", " "])
    hypotheses = [Hypothesis("∧", " "), Hypothesis("", " "), Hypothesis("", ""), Hypothesis(" ", "")]
    tactics = [Tactic("", True, 5), Tactic("a", True, 5), Tactic("∧", True, 5)]
    theorem = Theorem("andb∧", "∧ac", hypotheses, context, tactics)
    assert theorem.unique_string == "∧ac"
    assert theorem.conclusion == "andb∧"

    tac_list = theorem.past_tactics
    assert isinstance(tac_list, list)
    _compare_tactics(tactics, tac_list)

    tactics = []
    theorem.past_tactics = tactics
    tac_list = theorem.past_tactics
    assert isinstance(tac_list, list)
    _compare_tactics(tactics, tac_list)

    tactics = [Tactic("", True, 5), Tactic("a", True, 5), Tactic("∧", True, 5)]
    theorem.past_tactics = tactics
    tac_list = theorem.past_tactics
    assert isinstance(tac_list, list)
    _compare_tactics(tactics, tac_list)

    with pytest.raises(TypeError):
        tactics2 = [Tactic("", True, 5), Tactic("a", True, 5), 5]
        theorem.past_tactics = tactics2
    # Check still old
    tac_list = theorem.past_tactics
    assert isinstance(tac_list, list)
    _compare_tactics(tactics, tac_list)


def test_env_effects():
    context = Context(["∧", "", " "])
    hypotheses = [Hypothesis("∧", " "), Hypothesis("", " "), Hypothesis("", ""), Hypothesis(" ", "")]
    tactics = [Tactic("", True, 5), Tactic("a", True, 5), Tactic("∧", True, 5)]
    goal = Theorem("andb∧", "∧ac", hypotheses, context, tactics)
    tactic = Tactic("asdfasfd", True, 10)
    effect = EnvEffect(goal, tactic, [])
    _compare_theorem(effect.goal, goal)
    goal = Theorem("ab∧", "∧c", hypotheses, context, tactics)
    effect.goal = goal
    _compare_theorem(effect.goal, goal)
    tactic = Tactic("asdfanmlaksdf", True, 30)
    effect.tactic = tactic
    assert tactic.unique_string == effect.tactic.unique_string
    assert tactic.is_valid == effect.tactic.is_valid
    assert tactic.duration == effect.tactic.duration
    assert effect.children == []
    effect.children = [goal]
    assert len(effect.children) == 1
    _compare_theorem(effect.children[0], goal)

def test_env_expansion():
    context = Context(["∧", "", " "])
    hypotheses = [Hypothesis("H1", "A"),Hypothesis("H2", "B"),Hypothesis("H3", "C"),Hypothesis("H4", "D")]
    tactic_a = Tactic("tac_a", True, 5)
    tactic_b = Tactic("tac_b", True, 10)
    tactic_c = Tactic("tac_c", True, 15)
    tactics = [tactic_a, tactic_b, tactic_c]

    goal = Theorem("goal_conclusion", "goal_unique", hypotheses, context, tactics)

    effect1 = EnvEffect(goal, tactic_b, [])
    effect2 = EnvEffect(goal, tactic_c, [goal])
    effects = [effect1, effect2]
    expander_duration = 100
    generation_duration = 200
    env_durations = [10, 20]
    log_critic = 1.234
    expansion_tactics = [tactic_a, tactic_b]
    children_for_tactic = [[goal], []]
    priors = [0.5, 0.7]
    expansion = EnvExpansion(
        thm=goal,
        expander_duration=expander_duration,
        generation_duration=generation_duration,
        env_durations=env_durations,
        effects=effects,
        log_critic=log_critic,
        tactics=expansion_tactics,
        children_for_tactic=children_for_tactic,
        priors=priors
    )
    _compare_theorem(expansion.thm, goal)
    assert expansion.expander_duration == expander_duration, "expander_duration mismatch"
    assert expansion.generation_duration == generation_duration, "generation_duration mismatch"
    assert expansion.env_durations == env_durations, "env_durations mismatch"

    _compare_theorem(expansion.effects[0].goal, goal)
    assert expansion.effects[0].tactic.unique_string == tactic_b.unique_string, "first effect tactic unique_string mismatch"
    assert expansion.effects[0].children == [], "first effect children not empty"

    _compare_theorem(expansion.effects[1].goal, goal)
    assert expansion.effects[1].tactic.unique_string == tactic_c.unique_string, "second effect tactic unique_string mismatch"
    assert len(expansion.effects[1].children) == 1, "second effect should have one child"
    _compare_theorem(expansion.effects[1].children[0], goal)

    assert expansion.log_critic == log_critic, "log_critic mismatch"

    for i, tac in enumerate(expansion.tactics):
        assert tac.unique_string == expansion_tactics[i].unique_string, f"tactic[{i}] unique_string mismatch"
        assert tac.is_valid == expansion_tactics[i].is_valid, f"tactic[{i}] is_valid mismatch"
        assert tac.duration == expansion_tactics[i].duration, f"tactic[{i}] duration mismatch"

    assert len(expansion.children_for_tactic) == len(children_for_tactic), "children_for_tactic length mismatch"
    _compare_theorem(expansion.children_for_tactic[0][0], goal)
    assert expansion.children_for_tactic[1] == [], "second children_for_tactic entry should be empty"
    assert expansion.priors == priors, "priors mismatch"
    assert expansion.error is None, "error field should be None"
    assert expansion.is_error == False, "is_error should be False"



def test_htps_basic():
    context = Context([])
    hypotheses = []
    tactics = []
    theorem = Theorem("andb∧", "∧ac", hypotheses, context, tactics)
    params = SearchParams(0.3, PolicyType.RPO, 10, 3, True, True, True, True, 0.99, 10, True, True, True, 0.0, QValueSolved.One, 0.7, Metric.Time, NodeMask.NoMask, 1.0, 1.0, True, 1)
    search = HTPS(theorem, params)


def test_htps_theorems():
    context = Context([])
    hypotheses = []
    tactics = []
    theorem = Theorem("andb∧", "∧ac", hypotheses, context, tactics)
    params = SearchParams(0.3, PolicyType.RPO, 10, 3, True, True, True, True, 0.99, 10, True, True, True, 0.0, QValueSolved.One, 0.7, Metric.Time, NodeMask.NoMask, 1.0, 1.0, True, 1)
    search = HTPS(theorem, params)
    # Will find the root
    theorems = search.theorems_to_expand()
    assert len(theorems) == 1
    _compare_theorem(theorems[0], theorem)


def test_htps_expansion():
    context = Context([])
    hypotheses = []
    tactics = []
    theorem = Theorem("andb∧", "A", hypotheses, context, tactics)
    theorem2 = Theorem("andb∧d", "B", hypotheses, context, tactics)
    params = SearchParams(0.3, PolicyType.RPO, 10, 3, True, True, True, True, 0.99, 10, True, True, True, 0.0, QValueSolved.One, 0.7, Metric.Time, NodeMask.NoMask, 1.0, 1.0, True, 1)
    search = HTPS(theorem, params)
    print(theorem)
    # Will find the root
    theorems = search.theorems_to_expand()
    env_durations = [1, 1]
    log_critic = 1.234
    tactic_a = Tactic("dummy_tactic", True, 1)
    tactic_b = Tactic("dummy_tactic", True, 1)
    expansion_tactics = [tactic_a, tactic_b]
    children_for_tactic = [[theorem2], [theorem2]]
    priors = [0.5, 0.7]
    effect1 = EnvEffect(theorem, tactic_b, [theorem2])
    effect2 = EnvEffect(theorem, tactic_a, [theorem2])
    effects = [effect1, effect2]
    expansion = EnvExpansion(theorems[0], 100, 200, env_durations, effects, log_critic, expansion_tactics, children_for_tactic, priors)
    search.expand_and_backup([expansion])

    theorems = search.theorems_to_expand()
    assert len(theorems) == 1
    _compare_theorem(theorems[0], theorem2)

    env_durations = [10, 20]
    log_critic = 1.234
    tactic_a = Tactic("tac_a", True, 5)
    tactic_b = Tactic("tac_b", True, 10)
    expansion_tactics = [tactic_a]
    children_for_tactic = [[]]
    priors = [0.5, 0.7]
    effect2 = EnvEffect(theorem, tactic_a, [])
    effects = [effect2]
    expansion = EnvExpansion(theorems[0], 100, 200, env_durations, effects, log_critic, expansion_tactics, children_for_tactic, priors)
    search.expand_and_backup([expansion])
    assert search.proven()
