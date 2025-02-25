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
                    std::vector<double> &result) const {
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
    std::shared_ptr<Policy> dummyPolicy;

    void SetUp() override {
        // Create a dummy root theorem.
        root = std::make_shared<DummyTheorem>("A");
        // Create a dummy tactic.
        dummyTac = std::make_shared<DummyTactic>("dummy_tactic");
        // Create a dummy policy.
        dummyPolicy = std::make_shared<DummyPolicy>();

        DummyParams dummyParams;
        dummyParams.node_mask = MinimalProof;
        dummyParams.critic_subsampling_rate = 1.0;
        dummyParams.effect_subsampling_rate = 1.0;

        // Construct HTPS using the root.
        htps_instance = std::make_unique<htps::HTPS>(root, dummyParams, dummyPolicy);
    }
};

TEST_F(HTPSTest, ConstructorTest) {
    // Verify that the instance is not proven.
    EXPECT_FALSE(htps_instance->is_proven());
}


// Test for expansion: creates a dummy env_expansion representing a solved node.
TEST_F(HTPSTest, ExpansionTest) {
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
    // Call receive_expansion. This should mark the node as solved.
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
TEST_F(HTPSTest, GetResultTest) {
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
