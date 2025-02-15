//
// Created by simon on 12.02.25.
//

#ifndef HTPS_MCTS_H
#define HTPS_MCTS_H

#include "graph.h"
#include "base.h"
#include "../model/policy.h"
#include <memory>
#include <vector>
#include <numeric>
#include <cassert>
#include <algorithm>
#include <random>

namespace htps {

    constexpr double SOLVED_LOG_VALUE = 0.0; // e(0) = 1

    std::mt19937 setup_gen();

    std::mt19937 gen = setup_gen();
    std::uniform_real_distribution<double> dis = std::uniform_real_distribution<double>(0, 1);


    class MCTSSampleEffect {
    private:
        std::shared_ptr<theorem> goal;
        std::shared_ptr<tactic> tac;
        std::vector<std::shared_ptr<theorem>> children;
    public:
        MCTSSampleEffect(std::shared_ptr<theorem> goal, std::shared_ptr<tactic> tac,
                         std::vector<std::shared_ptr<theorem>> children) : goal(goal), tac(tac), children(children) {}


        std::shared_ptr<theorem> get_goal() const;

        std::shared_ptr<tactic> get_tactic() const;

        std::vector<std::shared_ptr<theorem>> get_children() const;

        size_t size_goal() const;
    };

    class MCTSSampleCritic {
    private:
        std::shared_ptr<theorem> goal;
        double q_estimate;
        bool solved;
        bool bad;
        double critic;
        size_t visit_count;

    public:
        MCTSSampleCritic(std::shared_ptr<theorem> goal, double q_estimate, bool solved, bool bad, double critic,
                         size_t visit_count) :
                goal(goal), q_estimate(q_estimate), solved(solved), bad(bad), critic(critic), visit_count(visit_count) {
            assert(q_estimate >= 0);
            assert(q_estimate <= 1 + 1e-4);
        }

        size_t size_goal() const;
    };

    enum InProof {
        NotInProof,
        InProof,
        InMinimalProof,
        InProofCount
    };

    class MCTSSampleTactics {
    private:
        std::shared_ptr<theorem> goal;
        std::vector<std::shared_ptr<tactic>> tactics;
        std::vector<double> target_pi;
        enum InProof inproof;
        std::vector<double> q_estimates; // q-estimate per tactic
        size_t visit_count;

    public:
        MCTSSampleTactics(std::shared_ptr<theorem> goal, std::vector<std::shared_ptr<tactic>> tactics,
                          std::vector<double> target_pi, enum InProof inproof, std::vector<double> q_estimates,
                          size_t visit_count) :
                goal(goal), tactics(tactics), target_pi(target_pi), inproof(inproof), q_estimates(q_estimates),
                visit_count(visit_count) {
            assert(target_pi.size() == tactics.size());
            assert(q_estimates.size() == tactics.size() || q_estimates.empty());
            assert(inproof != InProofCount);
            assert(!tactics.empty());
        }


        size_t size_goal() const;

        /* Average size of tokenized tactics. Note that this will be an integer, i.e. we drop the remainder.
         * */
        size_t avg_size_tactic() const;
    };


    enum QValueSolved {
        OneOverCounts,
        CountOverCounts,
        One,
        OneOverVirtualCounts,
        OneOverCountsNoFPU,
        CountOverCountsNoFPU, // No First Play Urgency
        QValueSolvedCount
    };

    enum NodeMask {
        None,
        Solving,
        Proof,
        MinimalProof,
        NodeMaskCount
    };

    class MCTSNode : public Node {
    private:
        double old_critic_value;
        double log_critic_value;
        std::vector<double> priors;
        QValueSolved q_value_solved;
        std::vector<std::pair<std::shared_ptr<tactic>, std::vector<std::shared_ptr<theorem>>>> effects;
        std::shared_ptr<Policy> policy;
        double exploration;
        double tactic_init_value = 0.0;
        std::vector<double> log_w; // Total action value
        std::vector<size_t> counts; // Total action count
        std::vector<size_t> virtual_counts;
        std::vector<bool> reset_mask; // Indicates whether logW should be reset, i.e. new values override old ones


        void get_tactics_sample_q_conditioning(size_t count_threshold,
                                               std::vector<std::shared_ptr<tactic>> &valid_tactics,
                                               std::vector<double> &valid_priors, std::vector<double> &valid_targets,
                                               std::vector<double> &q_values);

        void
        get_tactics_sample_regular(Metric metric, NodeMask node_mask, bool only_learn_best_tactics, double p_threshold,
                                   std::vector<std::shared_ptr<tactic>> &valid_tactics,
                                   std::vector<double> &valid_priors, std::vector<double> &valid_targets);

    public:
        MCTSNode(std::shared_ptr<theorem> &thm, std::vector<std::shared_ptr<tactic>> &tactics,
                 std::vector<std::vector<std::shared_ptr<theorem>>> &children_for_tactic,
                 std::shared_ptr<Policy> policy, std::vector<double> &priors,
                 double exploration, double log_critic_value, QValueSolved q_value_solved, double tactic_init_value,
                 std::vector<std::pair<std::shared_ptr<tactic>, std::vector<std::shared_ptr<theorem>>>> &effects,
                 size_t seed = 42) :
                Node(thm, tactics, children_for_tactic), old_critic_value(0.0), log_critic_value(log_critic_value),
                priors(priors), q_value_solved(q_value_solved), policy(policy), exploration(exploration),
                tactic_init_value(tactic_init_value), effects(effects), reset_mask(tactics.size()),
                log_w(tactics.size()) {
            assert(policy);
            assert(q_value_solved != QValueSolvedCount);
            assert(!tactics.empty());
            assert(children_for_tactic.size() == tactics.size());
            assert(children_for_tactic.size() == priors.size());
            assert(log_critic_value <= 0);
            // Assert is probability
            double sum = std::accumulate(priors.begin(), priors.end(), 0.0);
            assert(sum > 0.99 && sum < 1.01);
            // Assert we have at least one valid tactic
            bool is_valid = std::any_of(tactics.begin(), tactics.end(), [](const auto &t) { return t->is_valid; });
            assert(is_valid);
            reset_mcts_stats();
        }

        /* Reset the MCTS statistics, resetting counts and logW values.
         * */
        void reset_mcts_stats();

        bool should_send(size_t count_threshold);

        void get_effect_samples(std::vector<MCTSSampleEffect> &samples, double subsampling_rate = 1.0);

        std::vector<MCTSSampleEffect> get_effect_samples(double subsampling_rate = 1.0);

        std::optional<MCTSSampleCritic> get_critic_sample(double subsampling_rate = 1.0);

        std::optional<MCTSSampleTactics>
        get_tactics_sample(Metric metric, NodeMask node_mask, bool only_learn_best_tactics, double p_threshold = 0.0,
                           size_t count_threshold = 0, bool for_q_conditioning = false);

        bool kill_tactic(size_t tactic_id);

        void compute_policy(std::vector<double> &result, bool force_expansion = false);

        std::vector<double> compute_policy(bool force_expansion = false);

        void update(size_t tactic_id, double backup_value);

        /* Get the logarithmic value of the node.
         * */
        double get_value();
    };
}

template<>
struct std::hash<htps::MCTSNode> {
    std::size_t operator()(const htps::MCTSNode &n) const;
};

#endif //HTPS_MCTS_H
