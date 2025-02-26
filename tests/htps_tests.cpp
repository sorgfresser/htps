//
// Created by simon on 25.02.25.
//

// C++
#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <stdexcept>
#include "../src/graph/htps.h"

using namespace htps;

class DummyTheorem : public theorem {
public:
    DummyTheorem(const std::string &conclusion,
                 const std::vector<hypothesis> &hyps = {})
            : theorem(conclusion, hyps) {}

    std::vector<std::string> tokenize() const override {
        return {"dummy_token"};
    }

    void tokenize(std::vector<std::string> &tokens) const override {
        tokens.emplace_back("dummy_token");
    }
};

class DummyTactic : public tactic {
public:
    DummyTactic(const std::string &str, bool valid = true, size_t dur = 1) {
        unique_string = str;
        is_valid = valid;
        duration = dur;
    }

    std::vector<std::string> tokenize() const override {
        return {"tactic_token"};
    }

    void tokenize(std::vector<std::string> &tokens) const override {
        tokens.emplace_back("tactic_token");
    }
};

// Dummy policy that simply returns the priors as the computed policy.
class DummyPolicy : public Policy {
public:
    void get_policy(const std::vector<double> &q_values,
                    const std::vector<double> &priors,
                    const std::vector<size_t> &counts,
                    std::vector<double> &result) const override {
        result = priors;
    }

    DummyPolicy() : Policy(PolicyType::AlphaZero, 0.2) {}
};

class DummyParams : public htps_params {
public:
    DummyParams() : htps_params(0.2, PolicyType::AlphaZero, 500000000, 10) {}
};
// For the tests we need to set up a minimal HTPS instance.
// We assume that HTPS has a constructor taking a shared_ptr<theorem> as root
// and that its parameters can be tweaked or are set to reasonable defaults.

class HTPSTest : public ::testing::Test {
protected:
    std::shared_ptr<struct theorem> root;
    std::unique_ptr<HTPS> htps_instance;
    std::shared_ptr<DummyTactic> dummyTac;
    std::shared_ptr<DummyTactic> dummyTac2;
    std::shared_ptr<Policy> dummyPolicy;

    void SetUp() override {
        // Create a dummy root theorem.
        root = std::make_shared<DummyTheorem>("A");
        // Create a dummy tactic.
        dummyTac = std::make_shared<DummyTactic>("dummy_tactic");
        // Create a second dummy tactic.
        dummyTac2 = std::make_shared<DummyTactic>("dummy_tactic2");
        // Create a dummy policy.
        dummyPolicy = std::make_shared<DummyPolicy>();

        DummyParams dummyParams;
        dummyParams.node_mask = MinimalProof;
        dummyParams.critic_subsampling_rate = 1.0;
        dummyParams.effect_subsampling_rate = 1.0;
        dummyParams.depth_penalty = 0.99;

        // Construct HTPS using the root.
        htps_instance = std::make_unique<htps::HTPS>(root, dummyParams, dummyPolicy);
    }
};

TEST_F(HTPSTest, ConstructorTest) {
    // Verify that the instance is not proven.
    EXPECT_FALSE(htps_instance->is_proven());
}


// Test for expansion: creates a dummy env_expansion representing a solved node.
TEST_F(HTPSTest, ExpansionSolvedTest) {
    EXPECT_FALSE(htps_instance->is_proven());
    htps_instance->theorems_to_expand();

    // Create a dummy expansion effect for the root theorem.
    std::vector<std::shared_ptr<htps::env_effect>> effects;
    auto effect = std::make_shared<htps::env_effect>();
    effect->goal = root;
    effect->tac = dummyTac;
    // When the children vector is empty the node is considered solved.
    effect->children = {};
    effects.push_back(effect);

    // Create a dummy env_expansion representing a solved expansion.
    std::vector<std::shared_ptr<htps::tactic>> tactics = {dummyTac};
    std::vector<std::vector<std::shared_ptr<htps::theorem>>> childrenForTactic = {{}};
    std::vector<double> priors = {1.0};
    std::vector<size_t> envDurations = {1};

    htps::env_expansion expansion(root, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors);
    std::vector<std::shared_ptr<htps::env_expansion>> expansions = {std::make_shared<htps::env_expansion>(expansion)};
    // Call expand and backup. This should mark the node as solved.
    htps_instance->expand_and_backup(expansions);

    // Verify that the internal graph now contains a solved node.
    EXPECT_TRUE(htps_instance->is_proven());
}

// Test for getting theorems to expand.
TEST_F(HTPSTest, TheoremsToExpandTest) {
    // Prepare the instance with a solved root to force an expansion.
    // For this test, call batch_to_expand and verify that the returned list is not empty.
    std::vector<std::shared_ptr<htps::theorem>> toExpand = htps_instance->theorems_to_expand();
    EXPECT_FALSE(toExpand.empty());
    // In our minimal instance the root should be among the theorems to expand.
    bool found = false;
    for (auto &thm: toExpand) {
        if (thm->unique_string == root->unique_string) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

// Test for get_result: simulate a solved proof and retrieve the result.
TEST_F(HTPSTest, GetResultImmediatelySolvedTest) {
    htps_instance->theorems_to_expand();

    // First, simulate an expansion that solves the root.
    std::vector<std::shared_ptr<htps::env_effect>> effects;
    auto effect = std::make_shared<htps::env_effect>();
    effect->goal = root;
    effect->tac = dummyTac;
    effect->children = {}; // Empty children => solved node.
    effects.push_back(effect);

    std::vector<std::shared_ptr<htps::tactic>> tactics = {dummyTac};
    std::vector<std::vector<std::shared_ptr<htps::theorem>>> childrenForTactic = {{}};
    std::vector<double> priors = {1.0};
    std::vector<size_t> envDurations = {1};

    htps::env_expansion expansion(root, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors);

    // Expand and backup to update the internal state.
    std::vector<std::shared_ptr<htps::env_expansion>> expansions = {std::make_shared<htps::env_expansion>(expansion)};
    htps_instance->expand_and_backup(expansions);

    // Retrieve the result.
    htps::HTPSResult result = htps_instance->get_result();

    // Verify that the proof in the result has the root theorem.
    EXPECT_EQ(result.get_proof().proof_theorem->unique_string, root->unique_string);
    // Other result components like samples can be checked for non-emptiness.
    auto [samples_critic, samples_tactic, samples_effect, metric, proof_samples_tactics] = result.get_samples();
    EXPECT_FALSE(samples_critic.empty());
    EXPECT_FALSE(samples_tactic.empty());
    EXPECT_FALSE(samples_effect.empty());
}


// Test for expansion: creates multiple expansions
TEST_F(HTPSTest, ExpansionMultiTest) {
    EXPECT_FALSE(htps_instance->is_proven());
    htps_instance->theorems_to_expand();

    // Create children for the root theorem.
    std::vector<std::shared_ptr<htps::theorem>> children;
    for (int i = 0; i < 3; i++) {
        auto child = std::make_shared<DummyTheorem>("B" + std::to_string(i));
        children.push_back(child);
    }

    // Create a dummy expansion effect for the root theorem.
    std::vector<std::shared_ptr<htps::env_effect>> effects;
    auto effect = std::make_shared<htps::env_effect>();
    effect->goal = root;
    effect->tac = dummyTac;
    // Multiple children => unsolved node.
    effect->children = children;
    effects.push_back(effect);

    // Create a dummy env_expansion representing an unsolved expansion.
    std::vector<std::shared_ptr<htps::tactic>> tactics = {dummyTac};
    std::vector<std::vector<std::shared_ptr<htps::theorem>>> childrenForTactic = {children};
    std::vector<double> priors = {1.0};
    std::vector<size_t> envDurations = {1};

    htps::env_expansion expansion(root, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors);
    std::vector<std::shared_ptr<htps::env_expansion>> expansions = {std::make_shared<htps::env_expansion>(expansion)};

    // Call receive_expansion. This should not mark the node as solved.
    htps_instance->expand_and_backup(expansions);

    // Verify that the internal graph now contains an unsolved node.
    EXPECT_FALSE(htps_instance->is_proven());
    auto to_expand = htps_instance->theorems_to_expand();
    EXPECT_FALSE(to_expand.empty());
    // This should be exactly the children
    EXPECT_TRUE(to_expand.size() == children.size());
    EXPECT_TRUE(std::mismatch(to_expand.begin(), to_expand.end(), children.begin()).first == to_expand.end());
    bool root_found = std::find(to_expand.begin(), to_expand.end(), root) != to_expand.end();
    EXPECT_FALSE(root_found);
    EXPECT_TRUE(std::find(to_expand.begin(), to_expand.end(), children[0]) != to_expand.end());


    // Create a dummy expansion effect for the first child that solves it.
    std::vector<std::shared_ptr<htps::env_effect>> effects_child;
    auto effect_child = std::make_shared<htps::env_effect>();
    effect_child->goal = children[0];
    effect_child->tac = dummyTac;
    // Empty children => solved node.
    effect_child->children = {};
    effects_child.push_back(effect_child);

    // Create a dummy expansion for the two remaining children that remain unsolved.
    auto child3 = std::make_shared<DummyTheorem>("B3");
    auto child4 = std::make_shared<DummyTheorem>("B4");

    auto effect_child1 = std::make_shared<htps::env_effect>();
    effect_child1->goal = children[1];
    effect_child1->tac = dummyTac;
    effect_child1->children = {child3};

    auto effect_child2 = std::make_shared<htps::env_effect>();
    effect_child2->goal = children[2];
    effect_child2->tac = dummyTac;
    effect_child2->children = {child4};

    // Create a dummy env_expansion representing a solved expansion.
    std::vector<std::shared_ptr<htps::tactic>> tactics_child = {dummyTac};
    std::vector<std::vector<std::shared_ptr<htps::theorem>>> childrenForTactic_child = {{}};
    std::vector<double> priors_child = {1.0};
    std::vector<size_t> envDurations_child = {1};

    htps::env_expansion expansion_child(children[0], 1, 1, envDurations_child, effects_child, 0.0, tactics_child,
                                        childrenForTactic_child, priors_child);
    childrenForTactic_child = {{child3}};
    effects_child = {effect_child1};
    htps::env_expansion expansion_child2(children[1], 1, 1, envDurations_child, effects_child, 0.0, tactics_child,
                                         childrenForTactic_child, priors_child);
    childrenForTactic_child = {{child4}};
    effects_child = {effect_child2};
    htps::env_expansion expansion_child3(children[2], 1, 1, envDurations_child, effects_child, 0.0, tactics_child,
                                         childrenForTactic_child, priors_child);

    std::vector<std::shared_ptr<htps::env_expansion>> expansions_child = {
            std::make_shared<htps::env_expansion>(expansion_child),
            std::make_shared<htps::env_expansion>(expansion_child2),
            std::make_shared<htps::env_expansion>(expansion_child3)};

    // Call expand and backup. This should mark the node as solved.
    htps_instance->expand_and_backup(expansions_child);

    // Verify that the internal graph now contains a solved node and two unsolved children.
    EXPECT_FALSE(htps_instance->is_proven());
    to_expand = htps_instance->theorems_to_expand();
    EXPECT_FALSE(to_expand.empty());
    auto expected_children = {child3, child4};
    // This should be exactly the children except the solved one
    EXPECT_TRUE(to_expand.size() == 2);
    EXPECT_TRUE(std::mismatch(to_expand.begin(), to_expand.end(), expected_children.begin()).first == to_expand.end());
    root_found = std::find(to_expand.begin(), to_expand.end(), root) != to_expand.end();
    EXPECT_FALSE(root_found);
    // First child should not be in the list
    EXPECT_TRUE(std::find(to_expand.begin(), to_expand.end(), children[0]) == to_expand.end());
    // Second child should not be in the list
    EXPECT_TRUE(std::find(to_expand.begin(), to_expand.end(), children[1]) == to_expand.end());
    // Third child should not be in the list
    EXPECT_TRUE(std::find(to_expand.begin(), to_expand.end(), children[2]) == to_expand.end());
    // Child 3 should be in the list
    EXPECT_TRUE(std::find(to_expand.begin(), to_expand.end(), child3) != to_expand.end());
    // Child 4 should be in the list
    EXPECT_TRUE(std::find(to_expand.begin(), to_expand.end(), child4) != to_expand.end());

    // Create a dummy expansion that solves the children 3 and 4.
    std::vector<std::shared_ptr<htps::env_effect>> effects_grandchildren;
    auto effect_child3 = std::make_shared<htps::env_effect>();
    effect_child3->goal = child3;
    effect_child3->tac = dummyTac2;
    // Empty children => solved node.
    effect_child3->children = {};
    auto effect_child4 = std::make_shared<htps::env_effect>();
    effect_child4->goal = child4;
    effect_child4->tac = dummyTac2;
    // Empty children => solved node.
    effect_child4->children = {};

    // Create a dummy env_expansion representing a solved expansion.
    std::vector<std::shared_ptr<htps::tactic>> tactics_grandchildren = {dummyTac2};
    std::vector<std::vector<std::shared_ptr<htps::theorem>>> childrenForTactic_grandchildren = {{}};
    std::vector<double> priors_grandchildren = {1.0};
    std::vector<size_t> envDurations_grandchildren = {1};
    effects_grandchildren = {effect_child3};
    auto child3_ptr = static_cast<std::shared_ptr<htps::theorem>>(child3);
    htps::env_expansion expansion_grandchild(child3_ptr, 1, 1, envDurations_grandchildren, effects_grandchildren, 0.0,
                                             tactics_grandchildren, childrenForTactic_grandchildren,
                                             priors_grandchildren);
    std::vector<std::shared_ptr<htps::env_expansion>> expansions_grandchildren = {
            std::make_shared<htps::env_expansion>(expansion_grandchild)};
    effects_grandchildren = {effect_child4};
    auto child4_ptr = static_cast<std::shared_ptr<htps::theorem>>(child4);
    htps::env_expansion expansion_grandchild2(child4_ptr, 1, 1, envDurations_grandchildren, effects_grandchildren, 0.0,
                                              tactics_grandchildren, childrenForTactic_grandchildren,
                                              priors_grandchildren);
    expansions_grandchildren.push_back(std::make_shared<htps::env_expansion>(expansion_grandchild2));

    // Call expand and backup. This should mark the node as solved.
    htps_instance->expand_and_backup(expansions_grandchildren);

    // Verify that the internal graph now contains a solved node and two unsolved children.
    to_expand = htps_instance->theorems_to_expand();
    EXPECT_TRUE(htps_instance->is_proven());
    EXPECT_TRUE(to_expand.empty());
}