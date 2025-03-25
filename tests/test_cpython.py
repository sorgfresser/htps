import pytest

from htps import *


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
    for orig, new in zip(hypotheses, hyp_list, strict=True):
        assert new.identifier == orig.identifier
        assert new.value == orig.value

    hypotheses = []
    theorem.hypotheses = hypotheses
    hyp_list = theorem.hypotheses
    assert isinstance(hyp_list, list)
    for orig, new in zip(hypotheses, hyp_list, strict=True):
        assert new.identifier == orig.identifier
        assert new.value == orig.value

    hypotheses = [Hypothesis("∧", " "), Hypothesis("", " "), Hypothesis("", ""), Hypothesis(" ", "")]
    theorem.hypotheses = hypotheses
    hyp_list = theorem.hypotheses
    assert isinstance(hyp_list, list)
    for orig, new in zip(hypotheses, hyp_list, strict=True):
        assert new.identifier == orig.identifier
        assert new.value == orig.value

    with pytest.raises(TypeError):
        hypotheses_fail = [Hypothesis("∧", " "), Hypothesis("", " "), Hypothesis("", ""), 3]
        theorem.hypotheses = hypotheses_fail

    # Test old is still valid
    hyp_list = theorem.hypotheses
    assert isinstance(hyp_list, list)
    for orig, new in zip(hypotheses, hyp_list, strict=True):
        assert new.identifier == orig.identifier
        assert new.value == orig.value


def test_theorem_tactic():
    context = Context(["∧", "", " "])
    hypotheses = [Hypothesis("∧", " "), Hypothesis("", " "), Hypothesis("", ""), Hypothesis(" ", "")]
    tactics = [Tactic("", True, 5), Tactic("a", True, 5), Tactic("∧", True, 5)]
    theorem = Theorem("andb∧", "∧ac", hypotheses, context, tactics)
    assert theorem.unique_string == "∧ac"
    assert theorem.conclusion == "andb∧"

    tac_list = theorem.past_tactics
    assert isinstance(tac_list, list)
    for orig, new in zip(tactics, tac_list, strict=True):
        assert new.unique_string == orig.unique_string
        assert new.is_valid == orig.is_valid
        assert new.duration == orig.duration

    tactics = []
    theorem.past_tactics = tactics
    tac_list = theorem.past_tactics
    assert isinstance(tac_list, list)
    for orig, new in zip(tactics, tac_list, strict=True):
        assert new.unique_string == orig.unique_string
        assert new.is_valid == orig.is_valid
        assert new.duration == orig.duration

    tactics = [Tactic("", True, 5), Tactic("a", True, 5), Tactic("∧", True, 5)]
    theorem.past_tactics = tactics
    tac_list = theorem.past_tactics
    assert isinstance(tac_list, list)
    for orig, new in zip(tactics, tac_list, strict=True):
        assert new.unique_string == orig.unique_string
        assert new.is_valid == orig.is_valid
        assert new.duration == orig.duration

    with pytest.raises(TypeError):
        tactics2 = [Tactic("", True, 5), Tactic("a", True, 5), 5]
        theorem.past_tactics = tactics2
    # Check still old
    tac_list = theorem.past_tactics
    assert isinstance(tac_list, list)
    for orig, new in zip(tactics, tac_list, strict=True):
        assert new.unique_string == orig.unique_string
        assert new.is_valid == orig.is_valid
        assert new.duration == orig.duration
