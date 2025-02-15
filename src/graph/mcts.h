//
// Created by simon on 12.02.25.
//

#ifndef HTPS_MCTS_H
#define HTPS_MCTS_H

#include "graph.h"
#include "base.h"
#include "../model/policy.h"
#include "../env/core.h"
#include <memory>
#include <utility>
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
        MCTSSampleEffect(const std::shared_ptr<theorem> goal, const std::shared_ptr<tactic> tac,
                         const std::vector<std::shared_ptr<theorem>> &children) : goal(std::move(goal)),
                                                                                  tac(std::move(tac)),
                                                                                  children(children) {}


        std::shared_ptr<theorem> get_goal() const;

        std::shared_ptr<tactic> get_tactic() const;

        void get_children(std::vector<std::shared_ptr<theorem>> &children) const;

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
                goal(std::move(goal)), q_estimate(q_estimate), solved(solved), bad(bad), critic(critic),
                visit_count(visit_count) {
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
        MCTSSampleTactics(std::shared_ptr<theorem> goal, const std::vector<std::shared_ptr<tactic>> &tactics,
                          const std::vector<double> &target_pi, enum InProof inproof,
                          const std::vector<double> &q_estimates,
                          size_t visit_count) :
                goal(std::move(goal)), tactics(tactics), target_pi(target_pi), inproof(inproof),
                q_estimates(q_estimates),
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
        MinimalProofSolving, // uses Minproof if the root of the Graph is proven, otherwise solving
        NodeMaskCount
    };


    struct mcts_params {
        double exploration;
        PolicyType policy_type;
        size_t num_expansions;
        size_t succ_expansions; //no idea?
        bool early_stopping;
        bool no_critic;
        size_t count_threshold;
        bool tactic_p_threshold;
        bool tactic_sample_q_conditioning;
        bool only_learn_best_tactics;
        double tactic_init_value;
        QValueSolved q_value_solved;
        double policy_temperature;
        Metric metric;
        NodeMask node_mask;
        double effect_subsampling_rate;
        double critic_subsampling_rate;
        // If the root is not proven, we do not explore already solved nodes
        bool early_stopping_solved_if_root_not_proven;
        size_t virtual_loss; // The number of virtual count added for each visit
    };

    // A single simulation of the MCTS algorithm
    class Simulation {
    private:
        TheoremMap<std::shared_ptr<theorem>> theorems;
        TheoremMap<size_t> tactic_ids;
        TheoremMap<std::shared_ptr<tactic>> tactics;
        TheoremMap<size_t> depth;
        TheoremMap<std::vector<std::shared_ptr<theorem>>> children_for_theorem;
        TheoremMap<double> values;
        TheoremMap<bool> solved;
        TheoremMap<TheoremSet> seen; // Seen theorems, we immediately free the memory since this can get large
        std::shared_ptr<theorem> root;
        size_t expansions;

    public:
        std::vector<std::shared_ptr<theorem>> leaves() const;

        void leaves(std::vector<std::shared_ptr<theorem>> &leaves) const;

        size_t leave_count() const;

        /* Two simulations are considered equal if they have the same root and visit the same children in the
         * same order for all theorems, while using the same tactics. */
        bool operator==(const Simulation &other) const;

        explicit Simulation(std::shared_ptr<theorem> &root)
                : theorems(), tactics(), depth(), children_for_theorem(), values(), tactic_ids(),
                  root(root), expansions(0) {
            theorems.insert(root, root);
            depth.insert(root, 0);
        }

        void set_depth(const std::shared_ptr<theorem> &thm, size_t d);

        size_t get_depth(const std::shared_ptr<theorem> &thm) const;

        bool has_depth(const std::shared_ptr<theorem> &thm) const;

        void update_depth(const std::shared_ptr<theorem> &thm, size_t d);

        void set_value(const std::shared_ptr<theorem> &thm, double v);

        double get_value(const std::shared_ptr<theorem> &thm) const;

        bool is_solved(const std::shared_ptr<theorem> &thm) const;

        void set_solved(const std::shared_ptr<theorem> &thm, bool s);

        void set_tactic(const std::shared_ptr<theorem> &thm, const std::shared_ptr<tactic> &tac);

        std::shared_ptr<tactic> get_tactic(const std::shared_ptr<theorem> &thm) const;

        void set_tactic_id(const std::shared_ptr<theorem> &thm, size_t id);

        size_t get_tactic_id(const std::shared_ptr<theorem> &thm) const;

        void set_theorem_set(const std::shared_ptr<theorem> &thm, TheoremSet &set);

        TheoremSet &get_theorem_set(const std::shared_ptr<theorem> &thm);

        void add_theorem(const std::shared_ptr<theorem> &thm, const std::shared_ptr<theorem> &parent, size_t thm_depth);

        bool erase_theorem_set(const std::shared_ptr<theorem> &thm);

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
                                               std::vector<double> &q_values) const;

        void
        get_tactics_sample_regular(Metric metric, NodeMask node_mask, bool only_learn_best_tactics, double p_threshold,
                                   std::vector<std::shared_ptr<tactic>> &valid_tactics,
                                   std::vector<double> &valid_priors, std::vector<double> &valid_targets) const;

    public:
        MCTSNode(std::shared_ptr<theorem> &thm, std::vector<std::shared_ptr<tactic>> &tactics,
                 std::vector<std::vector<std::shared_ptr<theorem>>> &children_for_tactic,
                 const std::shared_ptr<Policy> &policy, std::vector<double> &priors,
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

        bool should_send(size_t count_threshold) const;

        void get_effect_samples(std::vector<MCTSSampleEffect> &samples, double subsampling_rate = 1.0) const;

        std::vector<MCTSSampleEffect> get_effect_samples(double subsampling_rate = 1.0) const;

        std::optional<MCTSSampleCritic> get_critic_sample(double subsampling_rate = 1.0) const;

        std::optional<MCTSSampleTactics>
        get_tactics_sample(Metric metric, NodeMask node_mask, bool only_learn_best_tactics, double p_threshold = 0.0,
                           size_t count_threshold = 0, bool for_q_conditioning = false) const;

        bool kill_tactic(size_t tactic_id) override;

        void compute_policy(std::vector<double> &result, bool force_expansion = false) const;

        std::vector<double> compute_policy(bool force_expansion = false) const;

        void update(size_t tactic_id, double backup_value);

        /* Get the logarithmic value of the node.
         * */
        double get_value() const;

        void add_virtual_count(size_t tactic_id, size_t count);
    };
}

template<>
struct std::hash<htps::MCTSNode> {
    std::size_t operator()(const htps::MCTSNode &n) const;
};

namespace htps {

/**
 * @class FailedTacticException
 * @brief Custom exception class for handling failed tactics.
 *
 * This exception is thrown whenever a tactic would introduce a circle.
 */
    class FailedTacticException : public std::exception {
    public:
        FailedTacticException() noexcept = default;

        const char *what() const noexcept override {
            return "FailedTactic exception occurred, circle present!";
        }
    };


    class MCTS : Graph<MCTSNode, PrioritizedNode> {
    private:
        std::shared_ptr<Policy> policy;
        mcts_params params;
        size_t expansion_count;
        std::vector<Simulation> simulations; // Currently ongoing simulations. Once a simulation is at 0 awaiting expansions, it is removed
        TheoremMap<std::vector<std::shared_ptr<Simulation>>> simulations_for_theorem; // The Simulations that need to be adjusted if we receive an expanded theorem
        std::vector<MCTSSampleEffect> train_samples_effects;
        std::vector<MCTSSampleCritic> train_samples_critic;
        std::vector<MCTSSampleTactics> train_samples_tactics;
        bool propagate_needed; // Whether propagation is required. Is set to true whenever find_to_expand fails

    protected:
        bool is_leaf(const MCTSNode &node) const;

    public:
        void get_train_samples(std::vector<MCTSSampleEffect> &samples_effects,
                               std::vector<MCTSSampleCritic> &samples_critic,
                               std::vector<MCTSSampleTactics> &samples_tactics);

        void get_proof_samples(std::vector<MCTSSampleTactics> &proof_samples_tactics);

        void find_unexplored_and_propagate_expandable();

        bool dead_root() const override;

        Simulation find_leaves_to_expand(std::vector<std::shared_ptr<theorem>> &terminal,
                                         std::vector<std::shared_ptr<theorem>> &to_expand);

        /* Upon receiving an expansion which we add to the graph, we need to update the MCTS statistics.
         * For this, the value is set in each Simulation that still has the theorem in its
         *
         * */
        void receive_expansion(std::shared_ptr<theorem> &thm, double value, bool solved);

        void expand(std::vector<std::shared_ptr<env_expansion>> &expansions);

        void backup();

        void backup_leaves(std::vector<std::shared_ptr<theorem>> &leaves, bool only_value);

        void cleanup(Simulation to_clean);

        void mcts_move();

    };

}

#endif //HTPS_MCTS_H
