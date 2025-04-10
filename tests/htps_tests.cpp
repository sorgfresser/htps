//
// Created by simon on 25.02.25.
//

// C++
#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <stdexcept>
#include <fstream>
#include "../src/graph/htps.h"

using namespace htps;

class DummyTheorem : public theorem {
public:
    DummyTheorem(const std::string &conclusion,
                 const std::vector<hypothesis> &hyps = {})
            : theorem(conclusion, hyps) {}
};

class DummyTactic : public tactic {
public:
    DummyTactic(const std::string &str, bool valid = true, size_t dur = 1) {
        unique_string = str;
        is_valid = valid;
        duration = dur;
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
    TheoremPointer root;
    std::unique_ptr<HTPS> htps_instance;
    std::shared_ptr<DummyTactic> dummyTac;
    std::shared_ptr<DummyTactic> dummyTac2;
    std::shared_ptr<DummyTactic> dummyTac3;
    std::shared_ptr<Policy> dummyPolicy;
    DummyParams dummyParams;

    void SetUp() override {
        // Create a dummy root theorem.
        root = std::make_shared<DummyTheorem>("A");
        // Create dummy tactics
        dummyTac = std::make_shared<DummyTactic>("dummy_tactic");
        dummyTac2 = std::make_shared<DummyTactic>("dummy_tactic2");
        dummyTac3 = std::make_shared<DummyTactic>("dummy_tactic3");
        // Create a dummy policy.
        dummyPolicy = std::make_shared<DummyPolicy>();

        dummyParams.node_mask = MinimalProof;
        dummyParams.critic_subsampling_rate = 1.0;
        dummyParams.effect_subsampling_rate = 1.0;
        dummyParams.depth_penalty = 0.99;

        // Construct HTPS using the root.
        htps_instance = std::make_unique<htps::HTPS>(root, dummyParams, dummyPolicy);
        htps::seed = 42;
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
    std::vector<std::vector<htps::TheoremPointer>> childrenForTactic = {{}};
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
    std::vector<htps::TheoremPointer> toExpand = htps_instance->theorems_to_expand();
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
    std::vector<std::vector<htps::TheoremPointer>> childrenForTactic = {{}};
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
    std::vector<htps::TheoremPointer> children;
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
    std::vector<std::vector<htps::TheoremPointer>> childrenForTactic = {children};
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
    std::vector<std::vector<htps::TheoremPointer>> childrenForTactic_child = {{}};
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
    std::vector<std::vector<htps::TheoremPointer>> childrenForTactic_grandchildren = {{}};
    std::vector<double> priors_grandchildren = {1.0};
    std::vector<size_t> envDurations_grandchildren = {1};
    effects_grandchildren = {effect_child3};
    auto child3_ptr = static_cast<htps::TheoremPointer>(child3);
    htps::env_expansion expansion_grandchild(child3_ptr, 1, 1, envDurations_grandchildren, effects_grandchildren, 0.0,
                                             tactics_grandchildren, childrenForTactic_grandchildren,
                                             priors_grandchildren);
    std::vector<std::shared_ptr<htps::env_expansion>> expansions_grandchildren = {
            std::make_shared<htps::env_expansion>(expansion_grandchild)};
    effects_grandchildren = {effect_child4};
    auto child4_ptr = static_cast<htps::TheoremPointer>(child4);
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

    // Get results
    HTPSResult result = htps_instance->get_result();
    EXPECT_TRUE(result.get_goal() == root);

    // Check proof
    struct proof p = result.get_proof();
    EXPECT_TRUE(p.proof_theorem == root);
    EXPECT_TRUE(p.proof_tactic ==dummyTac);
    auto p_children = p.children;
    EXPECT_TRUE(p_children.size() == 3);
    const auto& p_child = p_children[0];
    EXPECT_TRUE(p_child.proof_theorem == children[0]);
    EXPECT_TRUE(p_child.proof_tactic == dummyTac);
    EXPECT_TRUE(p_child.children.empty());
    const auto &p_child2 = p_children[1];
    EXPECT_TRUE(p_child2.proof_theorem == children[1]);
    EXPECT_TRUE(p_child2.proof_tactic == dummyTac);
    EXPECT_TRUE(p_child2.children.size() == 1);
    const auto &p_child3 = p_children[2];
    EXPECT_TRUE(p_child3.proof_theorem == children[2]);
    EXPECT_TRUE(p_child3.proof_tactic == dummyTac);
    EXPECT_TRUE(p_child3.children.size() == 1);
    const auto &p_child4 = p_child2.children[0];
    EXPECT_TRUE(p_child4.proof_theorem == child3);
    EXPECT_TRUE(p_child4.proof_tactic == dummyTac2);
    EXPECT_TRUE(p_child4.children.empty());
    const auto &p_child5 = p_child3.children[0];
    EXPECT_TRUE(p_child5.proof_theorem == child4);
    EXPECT_TRUE(p_child5.proof_tactic == dummyTac2);
    EXPECT_TRUE(p_child5.children.empty());

    // Check samples
    auto [samples_critic, samples_tactic, samples_effect, metric, proof_samples_tactics] = result.get_samples();
    EXPECT_TRUE(metric == dummyParams.metric);
    EXPECT_FALSE(samples_critic.empty());
    EXPECT_TRUE(samples_critic.size() == 6);
    EXPECT_TRUE(samples_tactic.size() == 6);
    EXPECT_TRUE(samples_effect.size() == 6);
    EXPECT_TRUE(proof_samples_tactics.size() == 6);
}

TEST_F(HTPSTest, TestBackupOnce) {
    // Modify params
    auto params = dummyParams;
    dummyParams.backup_once = true;
    htps_instance->set_params(dummyParams);

    EXPECT_FALSE(htps_instance->is_proven());

    htps_instance->theorems_to_expand();
    // Create children for the root theorem.
    std::vector<htps::TheoremPointer> children;
    for (int i = 0; i < 2; i++) {
        auto child = std::make_shared<DummyTheorem>("B");
        children.push_back(child);
    }

    // Create a dummy expansion effect for the root theorem.
    std::vector<std::shared_ptr<htps::env_effect>> effects;
    auto effect = std::make_shared<htps::env_effect>();
    effect->goal = root;
    effect->tac = dummyTac;
    effect->children = {children[0]};
    effects.push_back(effect);
    effect = std::make_shared<htps::env_effect>();
    effect->goal = root;
    effect->tac = dummyTac;
    effect->children = {children[1]};
    effects.push_back(effect);

    std::vector<std::shared_ptr<htps::tactic>> tactics = {dummyTac, dummyTac};
    std::vector<std::vector<htps::TheoremPointer>> childrenForTactic = {{children[0]}, {children[1]}};
    std::vector<double> priors = {0.5, 0.5};
    std::vector<size_t> envDurations = {1, 1};

    htps::env_expansion expansion(root, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors);
    std::vector<std::shared_ptr<htps::env_expansion>> expansions = {std::make_shared<htps::env_expansion>(expansion)};

    htps_instance->expand_and_backup(expansions);

    auto theorems = htps_instance->theorems_to_expand();
    EXPECT_TRUE(theorems[0] == children[0]);

    effects.clear();
    effect->goal = children[0];
    effect->tac = dummyTac2;
    effect->children = {};
    effects.push_back(effect);
    tactics = {dummyTac2};
    childrenForTactic = {{}};
    priors = {1.0};
    envDurations = {1};

    expansion = {children[0], 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors};
    expansions = {std::make_shared<htps::env_expansion>(expansion)};
    htps_instance->expand_and_backup(expansions);
    EXPECT_TRUE(htps_instance->is_proven());
    htps_instance->get_result();
}


TEST_F(HTPSTest, TestVirtualLoss) {
    auto params = dummyParams;
    dummyParams.virtual_loss = 1;

    htps_instance->set_params(dummyParams);

    EXPECT_FALSE(htps_instance->is_proven());

    htps_instance->theorems_to_expand();
    // Create children for the root theorem.
    std::vector<htps::TheoremPointer> children;
    for (int i = 0; i < 2; i++) {
        auto child = std::make_shared<DummyTheorem>("B");
        children.push_back(child);
    }

    // Create a dummy expansion effect for the root theorem.
    std::vector<std::shared_ptr<htps::env_effect>> effects;
    auto effect = std::make_shared<htps::env_effect>();
    effect->goal = root;
    effect->tac = dummyTac;
    effect->children = {children[0]};
    effects.push_back(effect);
    effect = std::make_shared<htps::env_effect>();
    effect->goal = root;
    effect->tac = dummyTac;
    effect->children = {children[1]};
    effects.push_back(effect);

    std::vector<std::shared_ptr<htps::tactic>> tactics = {dummyTac, dummyTac};
    std::vector<std::vector<htps::TheoremPointer>> childrenForTactic = {{children[0]}, {children[1]}};
    std::vector<double> priors = {0.5, 0.5};
    std::vector<size_t> envDurations = {1, 1};

    htps::env_expansion expansion(root, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors);
    std::vector<std::shared_ptr<htps::env_expansion>> expansions = {std::make_shared<htps::env_expansion>(expansion)};

    htps_instance->expand_and_backup(expansions);

    auto theorems = htps_instance->theorems_to_expand();
    EXPECT_TRUE(theorems[0] == children[0]);

    effects.clear();
    effect->goal = children[0];
    effect->tac = dummyTac2;
    effect->children = {};
    effects.push_back(effect);
    tactics = {dummyTac2};
    childrenForTactic = {{}};
    priors = {1.0};
    envDurations = {1};

    expansion = {children[0], 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors};
    expansions = {std::make_shared<htps::env_expansion>(expansion)};
    htps_instance->expand_and_backup(expansions);
    EXPECT_TRUE(htps_instance->is_proven());
    auto res = htps_instance->get_result();
    auto [samples_critic, samples_tactic, samples_effect, metric, proof_samples_tactics] = res.get_samples();
    for (auto &sample: samples_tactic) {
        EXPECT_TRUE(sample.get_inproof() == htps::InMinimalProof);
    }
}


TEST_F(HTPSTest, TestCountThreshold) {
    auto params = dummyParams;
    dummyParams.count_threshold = 10;

    htps_instance->set_params(dummyParams);

    EXPECT_FALSE(htps_instance->is_proven());

    htps_instance->theorems_to_expand();
    // Create children for the root theorem.
    std::vector<htps::TheoremPointer> children;
    for (int i = 0; i < 2; i++) {
        auto child = std::make_shared<DummyTheorem>("B");
        children.push_back(child);
    }

    // Create a dummy expansion effect for the root theorem.
    std::vector<std::shared_ptr<htps::env_effect>> effects;
    auto effect = std::make_shared<htps::env_effect>();
    effect->goal = root;
    effect->tac = dummyTac;
    effect->children = {children[0]};
    effects.push_back(effect);
    effect = std::make_shared<htps::env_effect>();
    effect->goal = root;
    effect->tac = dummyTac;
    effect->children = {children[1]};
    effects.push_back(effect);

    std::vector<std::shared_ptr<htps::tactic>> tactics = {dummyTac, dummyTac};
    std::vector<std::vector<htps::TheoremPointer>> childrenForTactic = {{children[0]}, {children[1]}};
    std::vector<double> priors = {0.5, 0.5};
    std::vector<size_t> envDurations = {1, 1};

    htps::env_expansion expansion(root, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors);
    std::vector<std::shared_ptr<htps::env_expansion>> expansions = {std::make_shared<htps::env_expansion>(expansion)};

    htps_instance->expand_and_backup(expansions);

    auto theorems = htps_instance->theorems_to_expand();
    EXPECT_TRUE(theorems[0] == children[0]);

    effects.clear();
    effect->goal = children[0];
    effect->tac = dummyTac2;
    effect->children = {};
    effects.push_back(effect);
    tactics = {dummyTac2};
    childrenForTactic = {{}};
    priors = {1.0};
    envDurations = {1};

    expansion = {children[0], 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors};
    expansions = {std::make_shared<htps::env_expansion>(expansion)};
    htps_instance->expand_and_backup(expansions);
    EXPECT_TRUE(htps_instance->is_proven());
    auto res = htps_instance->get_result();
    auto [samples_critic, samples_tactic, samples_effect, metric, proof_samples_tactics] = res.get_samples();
    for (auto &sample: samples_tactic) {
        EXPECT_TRUE(sample.get_inproof() == htps::InMinimalProof);
    }
}


nlohmann::json load_json_from_file(const std::string &filename) {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    nlohmann::json j = nlohmann::json::parse(file);
    return j;
}

void dump_json_to_file(const nlohmann::json &j, const std::string &filename) {
    std::ofstream file(filename);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    // Dump JSON object to the file with an indent of 4 spaces.
    file << j.dump(4);

    if (!file.good()) {
        throw std::runtime_error("Error writing to file: " + filename);
    }
}


TEST_F(HTPSTest, TestInfiniteLoop) {
    auto params = dummyParams;
    htps_instance->set_params(dummyParams);
    EXPECT_FALSE(htps_instance->is_proven());

    htps_instance->theorems_to_expand();

    TheoremPointer child = static_pointer_cast<htps::theorem>(std::make_shared<DummyTheorem>("B"));
    // Create a dummy expansion effect for the root theorem.
    std::vector<std::shared_ptr<htps::env_effect>> effects;
    auto effect = std::make_shared<htps::env_effect>();
    effect->goal = root;
    effect->tac = dummyTac;
    effect->children = {root};
    effects.push_back(effect);
    effect = std::make_shared<htps::env_effect>();
    effect->goal = root;
    effect->tac = dummyTac2;
    effect->children = {child};
    effects.push_back(effect);

    std::vector<std::shared_ptr<htps::tactic>> tactics = {dummyTac, dummyTac2};
    std::vector<std::vector<htps::TheoremPointer>> childrenForTactic = {{root}, {child}};
    std::vector<double> priors = {0.5, 0.5};
    std::vector<size_t> envDurations = {1, 1};

    htps::env_expansion expansion(root, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors);
    std::vector<std::shared_ptr<htps::env_expansion>> expansions = {std::make_shared<htps::env_expansion>(expansion)};

    htps_instance->expand_and_backup(expansions);

    auto theorems = htps_instance->theorems_to_expand();
    EXPECT_TRUE(theorems[0] == child);

    effects.clear();
    effect->goal = child;
    effect->tac = dummyTac3;
    effect->children = {root};
    effects.push_back(effect);
    tactics = {dummyTac3};
    childrenForTactic = {{root}};
    priors = {1.0};
    envDurations = {1};

    expansion = {child, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors};
    expansions = {std::make_shared<htps::env_expansion>(expansion)};
    htps_instance->expand_and_backup(expansions);
    htps_instance->theorems_to_expand();

    EXPECT_TRUE(htps_instance->is_done());
    EXPECT_FALSE(htps_instance->is_proven());
    auto res = htps_instance->get_result();
    auto [samples_critic, samples_tactic, samples_effect, metric, proof_samples_tactics] = res.get_samples();
    // Proof should be empty
    EXPECT_TRUE(!res.get_proof().proof_theorem);
    EXPECT_TRUE(!res.get_proof().proof_tactic);
    EXPECT_TRUE(res.get_proof().children.empty());
    // Still, we might have samples
    EXPECT_TRUE(samples_tactic.empty());
    bool is_bad = std::all_of(samples_critic.begin(), samples_critic.end(), [](const auto &sample) {return sample.is_bad();});
    EXPECT_TRUE(is_bad && !samples_critic.empty());
    EXPECT_TRUE(samples_effect.size() == 3);
    // No proof samples though, as there is no proof
    EXPECT_TRUE(proof_samples_tactics.empty());

    auto json = nlohmann::json(*htps_instance);
    dump_json_to_file(json, "samples/test2.json");

    auto json2 = nlohmann::json(expansion);
    dump_json_to_file(json2, "samples/test3.json");
}

/* Within a single proof tree, two sibling branches might face the same theorem at some point.
 * This case broke on the initial setup, and a regression test was added to prevent this.
 * Here, tactic 0 is selected from the root node, leading to two children B1 and B2. These two sibling branches
 * both result in B3. The HTPS has to be able to handle this case.
 */
TEST_F(HTPSTest, TestSiblingEquality) {
    auto params = dummyParams;
    htps_instance->set_params(dummyParams);
    EXPECT_FALSE(htps_instance->is_proven());

    htps_instance->theorems_to_expand();

    TheoremPointer child1 = static_pointer_cast<htps::theorem>(std::make_shared<DummyTheorem>("B1"));
    TheoremPointer child2 = static_pointer_cast<htps::theorem>(std::make_shared<DummyTheorem>("B2"));

    std::vector<std::shared_ptr<htps::env_effect>> effects;
    auto effect = std::make_shared<htps::env_effect>();
    effect->goal = root;
    effect->tac = dummyTac;
    effect->children = {child1, child2};
    effects.push_back(effect);

    std::vector<std::shared_ptr<htps::tactic>> tactics = {dummyTac};
    std::vector<std::vector<htps::TheoremPointer>> childrenForTactic = {{child1, child2}};
    std::vector<double> priors = {1.0};
    std::vector<size_t> envDurations = {1};

    htps::env_expansion expansion(root, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors);
    std::vector<std::shared_ptr<htps::env_expansion>> expansions = {std::make_shared<htps::env_expansion>(expansion)};

    htps_instance->expand_and_backup(expansions);

    auto theorems = htps_instance->theorems_to_expand();
    EXPECT_TRUE(theorems[0] == child1 && theorems[1] == child2);

    TheoremPointer child3 = static_pointer_cast<htps::theorem>(std::make_shared<DummyTheorem>("B3"));

    effects.clear();
    effect->goal = child1;
    effect->tac = dummyTac2;
    effect->children = {child3};
    effects.push_back(effect);
    tactics = {dummyTac2};
    childrenForTactic = {{child3}};
    priors = {1.0};
    envDurations = {1};

    expansion = {child1, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors};
    expansions = {std::make_shared<htps::env_expansion>(expansion)};

    effects.clear();
    effect = std::make_shared<htps::env_effect>();
    effect->goal = child2;
    effect->tac = dummyTac3;
    effect->children = {child3};
    effects.push_back(effect);
    tactics = {dummyTac3};
    childrenForTactic = {{child3}};
    priors = {1.0};
    envDurations = {1};
    expansion = {child2, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors};
    expansions.push_back(std::make_shared<htps::env_expansion>(expansion));

    htps_instance->expand_and_backup(expansions);
    htps_instance->theorems_to_expand();
}


/* It suffices if a single goal of each tactic is killed for a node to become unsolvable.
 * This test asserts that indeed, if only one goal of a tactic is killed, we set the tactic as killed
 */
TEST_F(HTPSTest, TestLoopMultipleChildren) {
    auto params = dummyParams;
    htps_instance->set_params(dummyParams);
    EXPECT_FALSE(htps_instance->is_proven());

    htps_instance->theorems_to_expand();

    TheoremPointer child1 = static_pointer_cast<htps::theorem>(std::make_shared<DummyTheorem>("B1"));
    TheoremPointer child2 = static_pointer_cast<htps::theorem>(std::make_shared<DummyTheorem>("B2"));

    std::vector<std::shared_ptr<htps::env_effect>> effects;
    auto effect = std::make_shared<htps::env_effect>();
    effect->goal = root;
    effect->tac = dummyTac;
    effect->children = {child1, child2};
    effects.push_back(effect);

    std::vector<std::shared_ptr<htps::tactic>> tactics = {dummyTac};
    std::vector<std::vector<htps::TheoremPointer>> childrenForTactic = {{child1, child2}};
    std::vector<double> priors = {1.0};
    std::vector<size_t> envDurations = {1};

    htps::env_expansion expansion(root, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors);
    std::vector<std::shared_ptr<htps::env_expansion>> expansions = {std::make_shared<htps::env_expansion>(expansion)};

    htps_instance->expand_and_backup(expansions);

    auto theorems = htps_instance->theorems_to_expand();
    EXPECT_TRUE(theorems[0] == child1 && theorems[1] == child2);

    TheoremPointer child3 = static_pointer_cast<htps::theorem>(std::make_shared<DummyTheorem>("B3"));
    TheoremPointer child4 = static_pointer_cast<htps::theorem>(std::make_shared<DummyTheorem>("B4"));

    effects.clear();
    effect->goal = child1;
    effect->tac = dummyTac2;
    effect->children = {child3};
    effects.push_back(effect);
    tactics = {dummyTac2};
    childrenForTactic = {{child3}};
    priors = {1.0};
    envDurations = {1};

    expansion = {child1, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors};
    expansions = {std::make_shared<htps::env_expansion>(expansion)};

    effects.clear();
    effect = std::make_shared<htps::env_effect>();
    effect->goal = child2;
    effect->tac = dummyTac3;
    effect->children = {child4};
    effects.push_back(effect);
    tactics = {dummyTac3};
    childrenForTactic = {{child4}};
    priors = {1.0};
    envDurations = {1};
    expansion = {child2, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors};
    expansions.push_back(std::make_shared<htps::env_expansion>(expansion));

    htps_instance->expand_and_backup(expansions);
    htps_instance->theorems_to_expand();

    TheoremPointer child5 = static_pointer_cast<htps::theorem>(std::make_shared<DummyTheorem>("B5"));

    expansions.clear();
    effects.clear();
    effect->goal = child3;
    effect->tac = dummyTac2;
    effect->children = {child1};
    effects.push_back(effect);
    tactics = {dummyTac2};
    childrenForTactic = {{child1}};
    priors = {1.0};
    envDurations = {1};
    // Killed B3 by circle to B1, which will also kill B1
    expansion = {child3, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors};
    expansions.push_back(std::make_shared<htps::env_expansion>(expansion));

    effects.clear();
    effect = std::make_shared<htps::env_effect>();
    effect->goal = child4;
    effect->tac = dummyTac3;
    effect->children = {child5};
    effects.push_back(effect);
    tactics = {dummyTac3};
    childrenForTactic = {{child5}};
    priors = {1.0};
    envDurations = {1};
    expansion = {child4, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors};
    expansions.push_back(std::make_shared<htps::env_expansion>(expansion));

    htps_instance->expand_and_backup(expansions);
    htps_instance->theorems_to_expand();

    // Assertions
    EXPECT_TRUE(htps_instance->is_done());
    EXPECT_FALSE(htps_instance->is_proven());
    EXPECT_TRUE(htps_instance->dead_root());

    HTPSResult result = htps_instance->get_result();
    auto [samples_critic, samples_tactic, samples_effect, metric, proof_samples_tactics] = result.get_samples();
    // Proof should be empty
    EXPECT_TRUE(!result.get_proof().proof_theorem);
    EXPECT_TRUE(!result.get_proof().proof_tactic);
    EXPECT_TRUE(result.get_proof().children.empty());
    // Still, we have samples
    EXPECT_TRUE(samples_tactic.empty());
    size_t bad_counts = std::count_if(samples_critic.begin(), samples_critic.end(), [](const auto &sample) {return sample.is_bad();});
    EXPECT_TRUE(bad_counts == 3 && samples_critic.size() == 5);
    EXPECT_TRUE(samples_effect.size() == 5);
    // No proof samples though, as there is no proof
    EXPECT_TRUE(proof_samples_tactics.empty());
}

/* If all tactics of a node are killed, all the tactics leading to this node are also killed.
 * But if another way to get to that node is found after the node has been killed, the new way has to be killed too.
 * The case is tested by creating nodes B1, B2 from root, and B1 being killed and afterwards B2 reaching B1.
 * This regression test ensures this works.
 */
TEST_F(HTPSTest, TestAddToKilled) {
    auto params = dummyParams;
    htps_instance->set_params(dummyParams);
    EXPECT_FALSE(htps_instance->is_proven());

    htps_instance->theorems_to_expand();

    TheoremPointer child1 = static_pointer_cast<htps::theorem>(std::make_shared<DummyTheorem>("B1"));
    TheoremPointer child2 = static_pointer_cast<htps::theorem>(std::make_shared<DummyTheorem>("B2"));

    std::vector<std::shared_ptr<htps::env_effect>> effects;
    auto effect = std::make_shared<htps::env_effect>();
    effect->goal = root;
    effect->tac = dummyTac;
    effect->children = {child1};
    effects.push_back(effect);
    effect = std::make_shared<htps::env_effect>();
    effect->goal = root;
    effect->tac = dummyTac2;
    effect->children = {child2};
    effects.push_back(effect);

    std::vector<std::shared_ptr<htps::tactic>> tactics = {dummyTac, dummyTac2};
    std::vector<std::vector<htps::TheoremPointer>> childrenForTactic = {{child1}, {child2}};
    std::vector<double> priors = {0.5, 0.5};
    std::vector<size_t> envDurations = {1, 1};

    htps::env_expansion expansion(root, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors);
    std::vector<std::shared_ptr<htps::env_expansion>> expansions = {std::make_shared<htps::env_expansion>(expansion)};

    htps_instance->expand_and_backup(expansions);
    expansions.clear();

    auto theorems = htps_instance->theorems_to_expand();
    EXPECT_TRUE(theorems[0] == child1 && theorems.size() == 1);

    // Now kill B1 by circling to root
    effects.clear();
    effect = std::make_shared<htps::env_effect>();
    effect->goal = child1;
    effect->tac = dummyTac;
    effect->children = {root};
    effects.push_back(effect);
    tactics = {dummyTac};
    childrenForTactic = {{root}};
    priors = {1.0};
    envDurations = {1};
    expansion = {child1, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors};
    expansions.push_back(std::make_shared<htps::env_expansion>(expansion));
    htps_instance->expand_and_backup(expansions);
    expansions.clear();

    // Now B1 is dead, but B2 remains
    theorems = htps_instance->theorems_to_expand();
    EXPECT_TRUE(theorems[0] == child2 && theorems.size() == 1);
    EXPECT_FALSE(htps_instance->is_proven());
    EXPECT_FALSE(htps_instance->is_done());
    EXPECT_FALSE(htps_instance->dead_root());

    // Now B2 reaches B1
    effects.clear();
    effect = std::make_shared<htps::env_effect>();
    effect->goal = child2;
    effect->tac = dummyTac2;
    effect->children = {child1};
    effects.push_back(effect);
    tactics = {dummyTac2};
    childrenForTactic = {{child1}};
    priors = {1.0};
    envDurations = {1};
    expansion = {child2, 1, 1, envDurations, effects, 0.0, tactics, childrenForTactic, priors};
    expansions.push_back(std::make_shared<htps::env_expansion>(expansion));
    htps_instance->expand_and_backup(expansions);

    htps_instance->theorems_to_expand();

    // Now all should be dead
    EXPECT_TRUE(htps_instance->is_done());
    EXPECT_FALSE(htps_instance->is_proven());
    EXPECT_TRUE(htps_instance->dead_root());

    HTPSResult result = htps_instance->get_result();
    auto [samples_critic, samples_tactic, samples_effect, metric, proof_samples_tactics] = result.get_samples();
    // Proof should be empty
    EXPECT_TRUE(!result.get_proof().proof_theorem);
    EXPECT_TRUE(!result.get_proof().proof_tactic);
    EXPECT_TRUE(result.get_proof().children.empty());
    // Still, we have samples
    EXPECT_TRUE(samples_tactic.empty());
    size_t bad_counts = std::count_if(samples_critic.begin(), samples_critic.end(), [](const auto &sample) {return sample.is_bad();});
    EXPECT_TRUE(bad_counts == 3 && samples_critic.size() == 3);
    EXPECT_TRUE(samples_effect.size() == 4);
    // No proof samples though, as there is no proof
    EXPECT_TRUE(proof_samples_tactics.empty());
}


TEST_F(HTPSTest, TestJsonLoading) {
    auto j = load_json_from_file("samples/test.json");

    HTPS search = htps::HTPS::from_json(j);
    EXPECT_FALSE(search.is_done());
    // Because we are awaiting expansions
    EXPECT_THROW(search.theorems_to_expand(), std::runtime_error);
}



TEST_F(HTPSTest, TestJsonExpectations) {
    auto params = dummyParams;
    htps_instance->set_params(dummyParams);

    auto j = load_json_from_file("samples/search.json");

    HTPS search = htps::HTPS::from_json(j);
    EXPECT_FALSE(search.is_done());

    for (size_t index = 1; index < 3; index++) {
        j = load_json_from_file("samples/expansions_" + std::to_string(index) + ".json");
        std::vector<std::shared_ptr<htps::env_expansion>> expansions;
        for (auto &expansion: j) {
            expansions.push_back(std::make_shared<htps::env_expansion>(htps::env_expansion::from_json(expansion)));
        }
        auto theorems = search.theorems_to_expand();
        search.expand_and_backup(expansions);
    }
}