//
// Created by simon on 12.02.25.
//

#ifndef HTPS_HTPS_H
#define HTPS_HTPS_H
#ifdef PYTHON_BINDINGS
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#endif
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

    size_t get_seed();
    std::mt19937 setup_gen();

    extern std::mt19937 gen;
    extern std::uniform_real_distribution<double> dis;
    extern size_t seed;

    class HTPSSampleEffect {
    private:
        TheoremPointer goal;
        std::shared_ptr<tactic> tac;
        std::vector<TheoremPointer> children;
    public:
        HTPSSampleEffect(const TheoremPointer &goal, const std::shared_ptr<tactic> &tac,
                         const std::vector<TheoremPointer> &children) : goal(goal),
                                                                                  tac(tac),
                                                                                  children(children) {}

        HTPSSampleEffect() = default;

        TheoremPointer get_goal() const;

        std::shared_ptr<tactic> get_tactic() const;

        void set_children(std::vector<TheoremPointer> &children) const;

        std::vector<TheoremPointer> get_children() const;
    };

    class HTPSSampleCritic {
    private:
        TheoremPointer goal;
        double q_estimate{};
        bool solved{};
        bool bad{};
        double critic{};
        size_t visit_count{};

    public:
        HTPSSampleCritic(const TheoremPointer& goal, double q_estimate, bool solved, bool bad, double critic,
                         size_t visit_count) :
                goal(goal), q_estimate(q_estimate), solved(solved), bad(bad), critic(critic),
                visit_count(visit_count) {
            assert(q_estimate >= 0);
            assert(q_estimate <= 1 + 1e-4);
        }

        HTPSSampleCritic() = default;

        TheoremPointer get_goal() const {
            return goal;
        }

        double get_q_estimate() const {
            return q_estimate;
        }

        bool is_solved() const {
            return solved;
        }

        bool is_bad() const {
            return bad;
        }

        double get_critic() const {
            return critic;
        }

        size_t get_visit_count() const {
            return visit_count;
        }
    };

    enum InProof {
        NotInProof,
        IsInProof,
        InMinimalProof,
        InProofCount
    };

    class HTPSSampleTactics {
    private:
        TheoremPointer goal;
        std::vector<std::shared_ptr<tactic>> tactics;
        std::vector<double> target_pi;
        enum InProof inproof;
        std::vector<double> q_estimates; // q-estimate per tactic
        size_t visit_count;

    public:
        HTPSSampleTactics(TheoremPointer goal, const std::vector<std::shared_ptr<tactic>> &tactics,
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

        HTPSSampleTactics() = default;

        TheoremPointer get_goal() const {
            return goal;
        }

        std::vector<std::shared_ptr<tactic>> get_tactics() const {
            return tactics;
        }

        std::vector<double> get_target_pi() const {
            return target_pi;
        }

        enum InProof get_inproof() const {
            return inproof;
        }

        std::vector<double> get_q_estimates() const {
            return q_estimates;
        }

        size_t get_visit_count() const {
            return visit_count;
        }
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


    struct htps_params {
#ifdef PYTHON_BINDINGS
        PyObject_HEAD
#endif
        double exploration;
        PolicyType policy_type;
        size_t num_expansions; // maximal number of expansions until the HTPS is considered done
        size_t succ_expansions; // number of expansions in a batch
        bool early_stopping;
        bool no_critic;
        bool backup_once; // Backup
        // Simulations only once, keeping track of a hash of the Simulation
        bool backup_one_for_solved; // Backup value of 1.0 for solved nodes. If false, will use critic value nonetheless
        double depth_penalty; // Penalty for depth, i.e. the discount factor
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

        operator nlohmann::json() const;

        static htps_params from_json(const nlohmann::json &j);
    };

    // A single simulation of the HTPS algorithm
    class Simulation {
    private:
        TheoremIncrementalMap<TheoremPointer> theorems;
        TheoremIncrementalMap<size_t> tactic_ids;
        TheoremIncrementalMap<std::shared_ptr<tactic>> tactics;
        TheoremIncrementalMap<size_t> depth;
        TheoremIncrementalMap<std::vector<TheoremPointer>> children_for_theorem;
        TheoremIncrementalMap<TheoremPointer> parent_for_theorem;
        TheoremIncrementalMap<double> values;
        TheoremIncrementalMap<bool> solved;
        TheoremIncrementalMap<bool> virtual_count_added;
        TheoremIncrementalMap<TheoremSet> seen; // Seen theorems, we immediately free the memory since this can get large
        TheoremPointer root;
        size_t expansions; // Number of expansions currently awaiting. If it reaches 0, values should be backed up

        friend struct std::hash<Simulation>;
    public:
        std::vector<std::pair<TheoremPointer, size_t>> leaves() const;

        void leaves(std::vector<std::pair<TheoremPointer, size_t>> &leaves) const;

        size_t leave_count() const;

        /* Two simulations are considered equal if they have the same root and visit the same children in the
         * same order for all theorems, while using the same tactics. */
        bool operator==(const Simulation &other) const;

        explicit Simulation(TheoremPointer &root)
                : theorems(), tactic_ids(), tactics(), depth(), children_for_theorem(), parent_for_theorem(),
                  values(), solved(), virtual_count_added(), seen(), root(root), expansions(0) {
            theorems.insert(root, root, 0);
            depth.insert(root, 0, 0);
            TheoremSet thm_set;
            thm_set.insert(root);
            seen.insert(root, thm_set, 0);
            parent_for_theorem.insert(root, TheoremPointer(), 0);
            children_for_theorem.insert(root, {}, 0);
            virtual_count_added.insert(root, false, 0);
        }

        Simulation() = default;

        size_t get_hash(const TheoremPointer &thm, const size_t &previous) const;

        size_t get_depth(const TheoremPointer &thm, size_t previous) const;

        bool has_depth(const TheoremPointer &thm, const size_t &previous) const;

        void update_depth(const TheoremPointer &thm, size_t d, const size_t &previous);

        void set_value(const TheoremPointer &thm, double v, const size_t &previous);

        double get_value(const size_t &hash_) const;

        double get_value(const TheoremPointer &thm, const size_t &previous) const;

        bool is_solved(const TheoremPointer &thm, const size_t &previous) const;

        void set_solved(const TheoremPointer &thm, bool s, const size_t &previous);

        void set_tactic(const TheoremPointer &thm, const std::shared_ptr<tactic> &tac, const size_t &previous);

        std::shared_ptr<tactic> get_tactic(const TheoremPointer &thm, const size_t &previous) const;

        void set_tactic_id(const TheoremPointer &thm, size_t id, const size_t &previous);

        size_t get_tactic_id(const size_t &hash_) const;

        size_t get_tactic_id(const TheoremPointer &thm, const size_t &previous) const;

        void set_theorem_set(const TheoremPointer &thm, TheoremSet &set, const size_t &previous);

        void set_theorem_set(const TheoremPointer &thm, TheoremSet set, const size_t &previous);

        TheoremSet &get_theorem_set(const TheoremPointer &thm, const size_t &previous);

        void add_theorem(const TheoremPointer &thm, const TheoremPointer &parent, const size_t &parent_hash, const size_t thm_depth);

        bool erase_theorem_set(const TheoremPointer &thm, const size_t &previous);

        void receive_expansion(const TheoremPointer &thm, double value, bool is_solved, const size_t &previous);

        auto begin() const {
            return theorems.begin();
        }

        auto end() const {
            return theorems.end();
        }

        bool get_virtual_count_added(const size_t &hash_) const;

        bool get_virtual_count_added(const TheoremPointer &thm, const size_t &previous) const;

        void set_virtual_count_added(const TheoremPointer &thm, bool value, const size_t &previous);

        bool should_backup() const;

        TheoremPointer parent(const size_t &hash_) const;

        TheoremPointer parent(const TheoremPointer &thm, const size_t &previous) const;

        std::pair<TheoremPointer, size_t> parent_hash(const size_t &hash_) const;

        std::pair<TheoremPointer, size_t> parent_hash(const TheoremPointer &thm, const size_t &previous) const;

        size_t previous_hash(const size_t &hash_) const;

        std::vector<TheoremPointer> get_children(const size_t &hash_) const;

        std::vector<TheoremPointer> get_children(const TheoremPointer &thm, const size_t &previous) const;

        std::vector<double> child_values(const size_t &hash_) const;

        std::vector<double> child_values(const TheoremPointer &thm, const size_t &previous) const;

        void reset_expansions();

        void increment_expansions();

        void decrement_expansions();

        size_t num_tactics();

        explicit operator nlohmann::json() const;

        static Simulation from_json(const nlohmann::json &j);

        void deduplicate(const TheoremPointer &ptr);
    };
}

/* A Simulation is considered equal if it faces all the same theorems in the same order, while using the same tactics.
 * Therefore, we hash the theorems and their tactics to get a unique hash for the simulation.
 * */
template<>
struct std::hash<htps::Simulation> {
    std::size_t operator()(const htps::Simulation &sim) const {
        std::vector<uint32_t> hashes;
        hashes.reserve(2 * sim.theorems.size());
        for (const auto &[unique_str, thm]: sim) {
            hashes.push_back(thm.second);
            if (sim.tactics.contains(unique_str))
                hashes.push_back(std::hash<htps::tactic>{}(*sim.tactics.at(unique_str).first));
        }
        return hash_vector(hashes);
    }

    static std::size_t hash_vector(std::vector<uint32_t> const &vec) {
        std::size_t seed = vec.size();
        for (auto x: vec) {
            x = ((x >> 16) ^ x) * 0x45d9f3b;
            x = ((x >> 16) ^ x) * 0x45d9f3b;
            x = (x >> 16) ^ x;
            seed ^= x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};


namespace htps {
    class HTPSNode : public Node {
    private:
        double old_critic_value{};
        double log_critic_value{};
        std::vector<double> priors;
        QValueSolved q_value_solved;
        std::vector<std::shared_ptr<env_effect>> effects;
        std::shared_ptr<Policy> policy;
        double exploration{};
        double tactic_init_value = 0.0;
        std::vector<double> log_w; // Total action value
        std::vector<size_t> counts; // Total action count
        std::vector<size_t> virtual_counts;
        std::vector<bool> reset_mask; // Indicates whether logW should be reset, i.e. new values override old ones
        bool error = false;

        void get_tactics_sample_q_conditioning(size_t count_threshold,
                                               std::vector<std::shared_ptr<tactic>> &valid_tactics,
                                               std::vector<double> &valid_targets,
                                               std::vector<double> &q_values) const;

        void
        get_tactics_sample_regular(Metric metric, NodeMask node_mask, bool only_learn_best_tactics, double p_threshold,
                                   std::vector<std::shared_ptr<tactic>> &valid_tactics,
                                   std::vector<double> &valid_targets) const;

        bool _validate() const;

    public:
        HTPSNode(const TheoremPointer &thm, const std::vector<std::shared_ptr<tactic>> &tactics,
                 const std::vector<std::vector<TheoremPointer>> &children_for_tactic,
                 const std::shared_ptr<Policy> &policy, const std::vector<double> &priors,
                 const double exploration, const double log_critic_value, const QValueSolved q_value_solved,
                 const double tactic_init_value,
                 const std::vector<std::shared_ptr<env_effect>> &effects, const bool error = false) :
                Node(thm, tactics, children_for_tactic), old_critic_value(0.0), log_critic_value(log_critic_value),
                priors(priors), q_value_solved(q_value_solved), effects(effects), policy(policy), exploration(exploration),
                tactic_init_value(tactic_init_value), log_w(tactics.size()), counts(), virtual_counts(),
                reset_mask(tactics.size()), error(error) {
            assert(_validate());
            reset_HTPS_stats();
        }

        HTPSNode(const HTPSNode &node)
                : Node(node),
                  old_critic_value(node.old_critic_value),
                  log_critic_value(node.log_critic_value),
                  priors(node.priors),
                  q_value_solved(node.q_value_solved),
                  effects(node.effects),
                  policy(node.policy),
                  exploration(node.exploration),
                  tactic_init_value(node.tactic_init_value),
                  log_w(node.log_w),
                  reset_mask(node.reset_mask),
                  error(node.error) {
            assert(_validate());
            reset_HTPS_stats();
        }

        HTPSNode() = default;

        /* Reset the HTPS statistics, resetting counts and logW values.
         * */
        void reset_HTPS_stats();

        bool should_send(size_t count_threshold) const;

        void get_effect_samples(std::vector<HTPSSampleEffect> &samples, double subsampling_rate = 1.0) const;

        std::vector<HTPSSampleEffect> get_effect_samples(double subsampling_rate = 1.0) const;

        std::optional<HTPSSampleCritic> get_critic_sample(double subsampling_rate = 1.0) const;

        std::optional<HTPSSampleTactics>
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

        void subtract_virtual_count(size_t tactic_id, size_t count);

        bool has_virtual_count(size_t tactic_id) const;

        bool has_virtual_count() const;

        static HTPSNode from_json(const nlohmann::json &j);

        explicit operator nlohmann::json() const;
    };
}

template<>
struct std::hash<htps::HTPSNode> {
    std::size_t operator()(const htps::HTPSNode &n) const;
};

namespace htps {
    class HTPSResult {
    private:
        std::vector<HTPSSampleCritic> samples_critic;
        std::vector<HTPSSampleTactics> samples_tactic;
        std::vector<HTPSSampleEffect> samples_effect;
        Metric metric;
        std::vector<HTPSSampleTactics> proof_samples_tactics;
        TheoremPointer goal;
        std::optional<struct proof> p;
    public:
        HTPSResult(std::vector<HTPSSampleCritic> &samples_critic, std::vector<HTPSSampleTactics> &samples_tactic,
                   std::vector<HTPSSampleEffect> &samples_effect, Metric metric,
                   std::vector<HTPSSampleTactics> &proof_samples_tactics, TheoremPointer &goal,
                   std::optional<struct proof> &p) :
                samples_critic(samples_critic), samples_tactic(samples_tactic), samples_effect(samples_effect),
                metric(metric), proof_samples_tactics(proof_samples_tactics), goal(goal),
                p(p) {}

        HTPSResult() = default;

        std::optional<struct proof> get_proof() const;

        TheoremPointer get_goal() const;

        Metric get_metric() const;

        std::tuple<std::vector<HTPSSampleCritic>, std::vector<HTPSSampleTactics>, std::vector<HTPSSampleEffect>, Metric, std::vector<HTPSSampleTactics>>
        get_samples() const;

        std::vector<HTPSSampleTactics> get_proof_samples() const;

        std::vector<HTPSSampleCritic> get_critic_samples() const;

        std::vector<HTPSSampleTactics> get_tactic_samples() const;

        std::vector<HTPSSampleEffect> get_effect_samples() const;


    };

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


    class HTPS : public Graph<HTPSNode, PrioritizedNode> {
    private:
        std::shared_ptr<Policy> policy;
        htps_params params;
        size_t expansion_count;
        std::vector<std::shared_ptr<Simulation>> simulations; // Currently ongoing simulations. Once a simulation is at 0 awaiting expansions, it is removed
        TheoremMap<std::vector<std::pair<std::shared_ptr<Simulation>, size_t>>> simulations_for_theorem; // The Simulations that need to be adjusted if we receive an expanded theorem, together with the hash for the leaf that is needed to backup correctly
        std::vector<HTPSSampleEffect> train_samples_effects;
        std::vector<HTPSSampleCritic> train_samples_critic;
        std::vector<HTPSSampleTactics> train_samples_tactics;
        std::unordered_set<size_t> backedup_hashes;
        TheoremSet currently_expanding; // Theorems that are currently being expanded
        bool propagate_needed; // Whether propagation is required. Is set to true whenever find_to_expand fails
        bool done;

        void _single_to_expand(std::vector<TheoremPointer> &theorems, Simulation &sim, std::vector<std::pair<TheoremPointer, std::size_t>> &leaves_to_expand);

    protected:
        bool is_leaf(const std::shared_ptr<HTPSNode> &node) const;

        /* Upon receiving an expansion which we add to the graph, we need to update the HTPS statistics.
         * For this, the value is set in each Simulation that still has the theorem in its
         *
         * */
        void receive_expansion(TheoremPointer &thm, double value, bool solved);

        void expand(std::vector<std::shared_ptr<env_expansion>> &expansions);

        void backup();

        void backup_leaves(std::shared_ptr<Simulation> &sim, bool only_value);

        std::vector<TheoremPointer> batch_to_expand();

        void batch_to_expand(std::vector<TheoremPointer> &theorems);

        void cleanup(Simulation &to_clean);

    public:
        HTPS(TheoremPointer &root, const htps_params &params, std::shared_ptr<Policy> &policy) :
                Graph<HTPSNode, PrioritizedNode>(root), policy(policy), params(params), expansion_count(0),
                train_samples_effects(), train_samples_critic(), train_samples_tactics(), backedup_hashes(),
                currently_expanding(), propagate_needed(true), done(false) {};

        HTPS(TheoremPointer &root, const htps_params &params) :
            Graph<HTPSNode, PrioritizedNode>(root), params(params), expansion_count(0),
                train_samples_effects(), train_samples_critic(), train_samples_tactics(), backedup_hashes(),
                currently_expanding(), propagate_needed(true), done(false) {
            policy = std::make_shared<Policy>(params.policy_type, params.exploration);
        }

        HTPS() : Graph<HTPSNode, PrioritizedNode>(), params(), expansion_count(0),
                train_samples_effects(), train_samples_critic(), train_samples_tactics(), backedup_hashes(),
                currently_expanding(), propagate_needed(true), done(false) {};

        void set_root(TheoremPointer &thm);

        void set_params(const htps_params &new_params);



        void get_train_samples(std::vector<HTPSSampleEffect> &samples_effects,
                               std::vector<HTPSSampleCritic> &samples_critic,
                               std::vector<HTPSSampleTactics> &samples_tactics) const;

        void get_proof_samples(std::vector<HTPSSampleTactics> &proof_samples_tactics) const;

        void find_unexplored_and_propagate_expandable();

        bool dead_root() const override;

        bool is_done() const;

        Simulation find_leaves_to_expand(std::vector<TheoremPointer> &terminal, std::vector<std::pair<TheoremPointer, size_t>> &to_expand);

        void expand_and_backup(std::vector<std::shared_ptr<env_expansion>> &expansions);

        std::vector<TheoremPointer> theorems_to_expand();

        void theorems_to_expand(std::vector<TheoremPointer> &theorems);

        HTPSResult get_result();

        static HTPS from_json(const nlohmann::json &j);

        explicit operator nlohmann::json() const;

        htps::htps_params get_params() const;
    };

}

#endif //HTPS_HTPS_H
