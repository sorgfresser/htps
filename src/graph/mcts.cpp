//
// Created by simon on 12.02.25.
//

#include "mcts.h"
#include "../model/policy.h"
#include <memory>
#include <vector>
#include <numeric>
#include <iostream>
#include <optional>

using namespace htps;

std::mt19937 htps::setup_gen() {
    std::random_device rd;
    size_t seed = std::getenv("SEED") ? std::stoi(std::getenv("SEED")) : rd();
    std::cout << "Seed: " << seed << std::endl;
    return std::mt19937(seed);
}

std::shared_ptr<theorem> MCTSSampleEffect::get_goal() const {
    return goal;
}

std::shared_ptr<tactic> MCTSSampleEffect::get_tactic() const {
    return tac;
}

std::vector<std::shared_ptr<theorem>> MCTSSampleEffect::get_children() const {
    return children;
}

void MCTSSampleEffect::get_children(std::vector<std::shared_ptr<theorem>> &children) const {
    children = this->children;
}

size_t MCTSSampleEffect::size_goal() const {
    if (!goal) {
        throw std::runtime_error("Goal is null, cannot get size");
    }
    return (goal->tokenize()).size();
}

size_t MCTSSampleCritic::size_goal() const {
    if (!goal) {
        throw std::runtime_error("Goal is null, cannot get size");
    }
    return (goal->tokenize()).size();
}

size_t MCTSSampleTactics::size_goal() const {
    if (!goal) {
        throw std::runtime_error("Goal is null, cannot get size");
    }
    return (goal->tokenize()).size();
}

size_t MCTSSampleTactics::avg_size_tactic() const {
    size_t sum = 0;
    for (const auto &tactic: tactics) {
        sum += tactic->tokenize().size();
    }

    return sum / tactics.size();
}

std::vector<std::shared_ptr<theorem>> Simulation::leaves() const {
    std::vector<std::shared_ptr<theorem>> result;
    leaves(result);
    return result;
}

void Simulation::leaves(std::vector<std::shared_ptr<theorem>> &leaves_vector) const {
    for (const auto &[thm_str, children]: children_for_theorem)
        if (children.empty())
            leaves_vector.push_back(theorems.at(thm_str));
}

bool Simulation::operator==(const Simulation &other) const {
    if (root != other.root)
        return false;
    if (children_for_theorem.size() != other.children_for_theorem.size())
        return false;
    for (const auto &[thm_str, children]: children_for_theorem) {
        // Tactics must match
        if (!other.tactics.contains(thm_str))
            return false;
        if (*tactics.at(thm_str) != *other.tactics.at(thm_str))
            return false;
        // And children must match
        auto other_it = other.children_for_theorem.find(thm_str);
        if (other_it == other.children_for_theorem.end())
            return false;
        auto other_children = other_it->second;
        if (children.size() != other_children.size())
            return false;
        // No need to sort etc., as the order must match
        for (size_t i = 0; i < children.size(); i++) {
            if (children[i] != other_children[i])
                return false;
        }
    }
    // Now the other way around. Since we now that they match on each theorem of the first simulation,
    // we only need to check that the other simulation has no extra theorems
    return std::all_of(other.children_for_theorem.begin(), other.children_for_theorem.end(), [this](const auto &pair) {
        return children_for_theorem.contains(pair.first);
    });
}

void Simulation::set_depth(const std::shared_ptr<theorem> &thm, size_t d) {
    depth.insert_or_assign(thm, d);
}

size_t Simulation::get_depth(const std::shared_ptr<theorem> &thm) const {
    return depth.at(thm);
}

bool Simulation::has_depth(const std::shared_ptr<theorem> &thm) const {
    return depth.contains(thm);
}

void Simulation::update_depth(const std::shared_ptr<theorem> &thm, size_t d) {
    auto it = depth.find(thm);
    size_t min_depth;
    if (it != depth.end()) {
        min_depth = std::min(it->second, d);
    } else {
        min_depth = d;
    }
    depth.insert_or_assign(thm, min_depth);
}

void Simulation::set_value(const std::shared_ptr<theorem> &thm, double v) {
    values.insert_or_assign(thm, v);
}

double Simulation::get_value(const std::shared_ptr<theorem> &thm) const {
    return values.at(thm);
}

void Simulation::set_solved(const std::shared_ptr<theorem> &thm, bool s) {
    solved.insert_or_assign(thm, s);
}

bool Simulation::is_solved(const std::shared_ptr<theorem> &thm) const {
    return solved.at(thm);
}

void Simulation::set_tactic(const std::shared_ptr<theorem> &thm, const std::shared_ptr<tactic> &tac) {
    tactics.insert_or_assign(thm, tac);
}

std::shared_ptr<tactic> Simulation::get_tactic(const std::shared_ptr<theorem> &thm) const {
    return tactics.at(thm);
}

void Simulation::set_tactic_id(const std::shared_ptr<theorem> &thm, size_t id) {
    tactic_ids.insert_or_assign(thm, id);
}

size_t Simulation::get_tactic_id(const std::shared_ptr<theorem> &thm) const {
    return tactic_ids.at(thm);
}

void Simulation::set_theorem_set(const std::shared_ptr<theorem> &thm, TheoremSet &set) {
    seen.insert_or_assign(thm, set);
}

TheoremSet &Simulation::get_theorem_set(const std::shared_ptr<theorem> &thm) {
    return seen.at(thm);
}

void
Simulation::add_theorem(const std::shared_ptr<theorem> &thm, const std::shared_ptr<theorem> &parent, size_t thm_depth) {
    assert(theorems.contains(parent));
    theorems.insert_or_assign(thm, thm);
    depth.insert_or_assign(thm, thm_depth);
    auto seen_set = seen.at(parent);
    seen_set.insert(thm);
    seen.insert_or_assign(thm, seen_set);
    children_for_theorem.at(parent).push_back(thm);
}

size_t Simulation::leave_count() const {
    size_t count = 0;
    for (const auto &[thm_str, children]: children_for_theorem)
        if (children.empty())
            count++;
    return count;
}

void MCTSNode::reset_mcts_stats() {
    // implies we will simply set logW to the first value we receive
    reset_mask = std::vector<bool>(tactics.size(), true);
    counts = std::vector<size_t>(tactics.size(), 0);
    virtual_counts = std::vector<size_t>(tactics.size(), 0);
}

bool MCTSNode::should_send(size_t count_threshold) const {
    if (solved) {
        return true;
    }
    size_t count_sum = std::accumulate(counts.begin(), counts.end(), 0);
    return count_sum >= count_threshold;
}

void MCTSNode::get_effect_samples(std::vector<MCTSSampleEffect> &samples, double subsampling_rate) const {
    for (const auto &[tac, children]: effects) {
        if (dis(gen) > subsampling_rate)
            continue;
        samples.emplace_back(thm, tac, children);
    }
}

std::vector<MCTSSampleEffect> MCTSNode::get_effect_samples(double subsampling_rate) const {
    std::vector<MCTSSampleEffect> samples;
    get_effect_samples(samples, subsampling_rate);
    return samples;
}

std::optional<MCTSSampleCritic> MCTSNode::get_critic_sample(double subsampling_rate) const {
    if (dis(gen) > subsampling_rate) {
        return std::nullopt;
    }
    // Compute visit count of the node by summing up all action counts
    size_t visit_sum = std::accumulate(counts.begin(), counts.end(), static_cast<size_t>(0));
    return MCTSSampleCritic(thm, std::exp(get_value()), solved, false, log_critic_value, visit_sum);
}


void MCTSNode::get_tactics_sample_q_conditioning(size_t count_threshold,
                                                 std::vector<std::shared_ptr<tactic>> &valid_tactics,
                                                 std::vector<double> &valid_priors,
                                                 std::vector<double> &valid_targets,
                                                 std::vector<double> &q_values) const {
    std::vector<size_t> selected_tactics_ids;
    for (size_t i = 0; i < tactics.size(); i++) {
        if (solving_tactics.find(i) != solving_tactics.end()) {
            selected_tactics_ids.push_back(i);
        } else if (!tactics[i]->is_valid) {
            selected_tactics_ids.push_back(i);
        } else if (counts[i] >= count_threshold) {
            selected_tactics_ids.push_back(i);
        }
    }
    if (selected_tactics_ids.empty()) {
        return;
    }
    valid_tactics.reserve(selected_tactics_ids.size());
    valid_priors.reserve(selected_tactics_ids.size());
    valid_targets.reserve(selected_tactics_ids.size());
    q_values.reserve(selected_tactics_ids.size());
    for (const auto &id: selected_tactics_ids) {
        valid_tactics.push_back(tactics[id]);
        valid_priors.push_back(priors[id]);
        valid_targets.push_back(-1.0); // Not used in this case
        // If the tactic solves the node, we assign a 1, if it is invalid, we assign a 0
        // Otherwise, use the average action value
        if (solving_tactics.find(id) != solving_tactics.end()) {
            q_values.push_back(1.0);
        } else if (!tactics[id]->is_valid) {
            q_values.push_back(0.0);
        } else {
            if (counts[id] == 0) {
                q_values.push_back(tactic_init_value);
            } else {
                q_values.push_back(std::exp(log_w[id]) / counts[id]);
            }
        }
    }
    assert (q_values.size() == valid_tactics.size());
    assert (q_values.size() == selected_tactics_ids.size());
    assert (q_values.size() == valid_priors.size());
    assert (q_values.size() == valid_targets.size());
}

void MCTSNode::get_tactics_sample_regular(Metric metric, NodeMask node_mask,
                                          bool only_learn_best_tactics, double p_threshold,
                                          std::vector<std::shared_ptr<tactic>> &valid_tactics,
                                          std::vector<double> &valid_priors,
                                          std::vector<double> &valid_targets) const {
    if (all_tactics_killed())
        return;
    std::vector<double> targets;
    compute_policy(targets);
    std::vector<size_t> selected_tactic_ids;
    if (n_solving_tactics() <= 0) {
        for (size_t i = 0; i < tactics.size(); i++) {
            if (tactics[i]->is_valid && targets[i] > p_threshold)
                selected_tactic_ids.push_back(i);
        }
        if (selected_tactic_ids.empty()) {
            return;
        }
    } else {
        if (only_learn_best_tactics || (node_mask == MinimalProof)) {
            selected_tactic_ids = minimum_tactics.get_tactics(metric);
        } else
            selected_tactic_ids = std::vector<size_t>(solving_tactics.begin(), solving_tactics.end());
        assert(!selected_tactic_ids.empty());
    }
    valid_tactics.reserve(selected_tactic_ids.size());
    valid_priors.reserve(selected_tactic_ids.size());
    valid_targets.reserve(selected_tactic_ids.size());

    for (const auto &id: selected_tactic_ids) {
        valid_tactics.push_back(tactics[id]);
        valid_priors.push_back(priors[id]);
    }

    if (n_solving_tactics() <= 0) {
        for (const auto &id: selected_tactic_ids) {
            valid_targets.push_back(targets[id]);
        }
    }
        // If the node is solved, take the uniform distribution over all solving tactics
    else {
        std::fill(valid_targets.begin(), valid_targets.end(), 1.0 / tactics.size());
    }
    assert(selected_tactic_ids.size() == valid_targets.size());
    assert(selected_tactic_ids.size() == valid_priors.size());
    assert(selected_tactic_ids.size() == valid_tactics.size());
}

std::optional<MCTSSampleTactics> MCTSNode::get_tactics_sample(Metric metric, NodeMask node_mask,
                                                              bool only_learn_best_tactics, double p_threshold,
                                                              size_t count_threshold, bool for_q_conditioning) const {
    if (!should_send(count_threshold)) {
        return std::nullopt;
    }

    switch (node_mask) {
        case Solving:
            if (n_solving_tactics() <= 0)
                return std::nullopt;
            break;
        case Proof:
            if (!is_in_proof())
                return std::nullopt;
            break;
        case MinimalProof:
            if (!is_in_minimum_proof(metric))
                return std::nullopt;
            break;
        default:
            throw std::runtime_error("Invalid node mask");
            break;
    }

    std::vector<std::shared_ptr<tactic>> valid_tactics;
    std::vector<double> valid_priors;
    std::vector<double> valid_targets;
    std::vector<double> q_values;

    if (for_q_conditioning)
        get_tactics_sample_q_conditioning(count_threshold, valid_tactics, valid_priors, valid_targets, q_values);
    else
        get_tactics_sample_regular(metric, node_mask, only_learn_best_tactics, p_threshold,
                                   valid_tactics, valid_priors, valid_targets);
    // If the above fails, return an empty optional
    if (valid_tactics.empty())
        return std::nullopt;

    enum InProof inproof;
    if (in_minimum_proof.get(metric)) {
        inproof = InProof::InMinimalProof;
    } else if (is_in_proof()) {
        inproof = InProof::InProof;
    } else {
        inproof = InProof::NotInProof;
    }
    // Compute visit count of the node by summing up all action counts
    size_t visit_sum = std::accumulate(counts.begin(), counts.end(), static_cast<size_t>(0));
    return MCTSSampleTactics(thm, valid_tactics, valid_targets, inproof, q_values, visit_sum);
}

bool MCTSNode::kill_tactic(size_t tactic_id) {
    bool killed = Node::kill_tactic(tactic_id);
    if (all_tactics_killed()) {
        assert(log_critic_value > MIN_FLOAT);
        old_critic_value = log_critic_value;
        log_critic_value = MIN_FLOAT;
    }
    return killed;
}

void MCTSNode::compute_policy(std::vector<double> &result, bool force_expansion) const {
    std::vector<size_t> full_counts;
    full_counts.reserve(tactics.size());
    result.reserve(tactics.size());
    for (size_t i = 0; i < tactics.size(); i++) {
        full_counts.push_back(counts[i] + virtual_counts[i]);
    }
    std::vector<double> q_values(tactics.size(), tactic_init_value);
    for (size_t i = 0; i < tactics.size(); i++) {
        if (full_counts[i] > 0) {
            assert(!reset_mask[i]);
            q_values[i] = std::exp(log_w[i]) / static_cast<double>(full_counts[i]);
        }
    }
    for (const auto &tac: solving_range()) {
        switch (q_value_solved) {
            case OneOverCounts:
                if (full_counts[tac] > 0)
                    q_values[tac] = 1.0 / static_cast<double>(full_counts[tac]);
                break;
            case CountOverCounts:
                if (full_counts[tac] > 0)
                    q_values[tac] = static_cast<double>(counts[tac]) / static_cast<double>(full_counts[tac]);
                break;
            case One:
                q_values[tac] = 1.0;
                break;
            case OneOverVirtualCounts:
                q_values[tac] = 1.0 / static_cast<double>(1 + virtual_counts[tac]);
            case OneOverCountsNoFPU:
                q_values[tac] = 1.0 / static_cast<double>(std::max(static_cast<size_t>(1), full_counts[tac]));
            case CountOverCountsNoFPU:
                q_values[tac] = static_cast<double>(std::max(static_cast<size_t>(1), counts[tac])) /
                                static_cast<double>(std::max(static_cast<size_t>(1), full_counts[tac]));
            default:
                throw std::runtime_error("Invalid q value solved parameter");
        }
    }
    // Check that at least one valid tactic is expandable before we apply this
    bool expandable_only = false;
    if (force_expansion) {
        for (size_t i = 0; i < tactics.size(); i++) {
            if (tactic_expandable[i] && tactics[i]->is_valid) {
                expandable_only = true;
                break;
            }
        }
    }

    for (std::size_t i = 0; i < tactics.size(); i++) {
        if (!tactics[i]->is_valid || (expandable_only && !tactic_expandable[i])) {
            q_values[i] = MIN_FLOAT;
            full_counts[i] = 0;
        }
    }
    double max_q = *std::max_element(q_values.begin(), q_values.end());
    assert(max_q > MIN_FLOAT);
    policy->get_policy(q_values, priors, full_counts, result);
    for (const auto &tac: killed_tactics) {
        assert(result[tac] <= 1e-9); // Check that killed tactics have zero probability
    }
}

std::vector<double> MCTSNode::compute_policy(bool force_expansion) const {
    std::vector<double> result;
    compute_policy(result, force_expansion);
    return result;
}

void MCTSNode::update(size_t tactic_id, double backup_value) {
    counts[tactic_id]++;
    // Compute logsumexp of these two values. We simplify the computation by assuming logw is the larger of the two
    // We can make that simplification because the equations hold for arbitrary constants c, therefore also for the smaller value
    // Also, note that by shifting by logw, it is exp(logw - logw) = 1
    if (reset_mask[tactic_id]) {
        log_w[tactic_id] = backup_value;
        reset_mask[tactic_id] = false;
    } else {
        log_w[tactic_id] += std::log(1 + std::exp(backup_value - log_w[tactic_id]));
    }
}

double MCTSNode::get_value() const {
    if (solved)
        return 0.0;
    if (is_terminal())
        return MIN_FLOAT;
    size_t visit_sum = std::accumulate(counts.begin(), counts.end(), 0);
    if (visit_sum == 0) {
        assert (log_critic_value <= 0);
        return std::min(0.0, log_critic_value);
    }

    std::vector<double> policy_values;
    compute_policy(policy_values);
    size_t max_id = std::distance(policy_values.begin(),
                                  std::max_element(policy_values.begin(), policy_values.end()));
    if (counts[max_id] == 0) {
        assert (log_critic_value <= 0.0);
        return std::min(0.0, log_critic_value);
    }
    double result = log_w[max_id] - std::log(counts[max_id]);
    assert(result <= 0.0);
    return std::min(0.0, result);
}

void MCTSNode::add_virtual_count(size_t tactic_id, size_t count) {
    virtual_counts[tactic_id] += count;
}

std::size_t std::hash<MCTSNode>::operator()(const MCTSNode &n) const {
    return std::hash<theorem>()(*n.get_theorem());
}

bool MCTS::is_leaf(const htps::MCTSNode &node) const {
    return node.is_solved() && !(is_proven()) && (params.early_stopping_solved_if_root_not_proven);
}

void MCTS::find_unexplored_and_propagate_expandable() {
    if (!propagate_needed)
        return;
    bool ignore_solved = params.early_stopping || (!is_proven() && params.early_stopping_solved_if_root_not_proven);
    Graph::find_unexplored_and_propagate_expandable(ignore_solved);
}

bool MCTS::dead_root() const {
    return (propagate_needed && unexplored_theorems.empty()) || (nodes.contains(root) && nodes.at(root).is_bad());
}

void
MCTS::get_train_samples(std::vector<MCTSSampleEffect> &samples_effects, std::vector<MCTSSampleCritic> &samples_critic,
                        std::vector<MCTSSampleTactics> &samples_tactics) {
    std::vector<MCTSSampleCritic> critic_solved;
    std::vector<MCTSSampleCritic> critic_unsolved;
    // Will be less than the number of nodes, but we don't know how many
    critic_solved.reserve(nodes.size());
    critic_unsolved.reserve(nodes.size());
    samples_critic.reserve(nodes.size());
    samples_tactics.reserve(nodes.size());
    samples_effects.reserve(nodes.size());
    std::vector<MCTSSampleEffect> node_samples;
    NodeMask node_mask = params.node_mask;
    if (node_mask == MinimalProofSolving) {
        if (is_proven())
            node_mask = MinimalProof;
        else
            node_mask = Solving;
    }

    for (const auto &[thm, node]: nodes) {
        node.get_effect_samples(node_samples, params.effect_subsampling_rate);
        samples_effects.insert(samples_effects.end(), node_samples.begin(), node_samples.end());
        node_samples.clear();
        auto critic_sample = node.get_critic_sample(params.critic_subsampling_rate);
        if (critic_sample) {
            if (node.is_solved()) {
                critic_solved.push_back(critic_sample.value());
            } else {
                critic_unsolved.push_back(critic_sample.value());
            }
        }
        auto tactic_sample = node.get_tactics_sample(params.metric, node_mask, params.only_learn_best_tactics,
                                                     params.tactic_p_threshold, params.count_threshold,
                                                     params.tactic_sample_q_conditioning);
        if (tactic_sample) {
            samples_tactics.push_back(tactic_sample.value());
        }
        samples_critic = std::move(critic_solved);
        samples_critic.insert(samples_critic.end(), critic_unsolved.begin(), critic_unsolved.end());
    }
    samples_critic.shrink_to_fit();
    samples_tactics.shrink_to_fit();
    samples_effects.shrink_to_fit();
}

void MCTS::get_proof_samples(std::vector<MCTSSampleTactics> &proof_samples_tactics) {
    if (!is_proven())
        return;
    // Upper bound on the number of samples
    proof_samples_tactics.reserve(nodes.size());
    for (const auto &[thm, node]: nodes) {
        auto tactic_sample = node.get_tactics_sample(params.metric, MinimalProof, params.only_learn_best_tactics,
                                                     params.tactic_p_threshold, params.count_threshold,
                                                     params.tactic_sample_q_conditioning);
        if (tactic_sample) {
            proof_samples_tactics.push_back(tactic_sample.value());
        }
    }
    proof_samples_tactics.shrink_to_fit();
}

Simulation MCTS::find_leaves_to_expand(std::vector<std::shared_ptr<theorem>> &terminal,
                                       std::vector<std::shared_ptr<theorem>> &to_expand) {
    Simulation sim = Simulation(root);
    std::deque<std::shared_ptr<theorem>> to_process;
    std::vector<double> node_policy;
    to_process.push_back(root);

    while (!to_process.empty()) {
        node_policy.clear();
        std::shared_ptr<theorem> current = to_process.front();
        to_process.pop_front();
        if (!nodes.contains(current)) {
            to_expand.push_back(current);
            continue;
        }
        auto mcts_node = nodes.at(current);
        bool is_leaf_node = is_leaf(mcts_node);
        if (mcts_node.is_terminal() || is_leaf_node) {
            assert(mcts_node.is_solved() || is_leaf_node);
            sim.set_value(current, mcts_node.get_value());
            sim.set_solved(current, true);
            terminal.push_back(current);
            continue;
        }
        if (params.early_stopping && mcts_node.is_solved()) {
            sim.set_value(current, 0.0);
            sim.set_solved(current, true);
            terminal.push_back(current);
            continue;
        }
        // Select subsequent tactic
        mcts_node.compute_policy(node_policy, true);
        size_t tactic_id;
        if (params.policy_temperature == 0) {
            tactic_id = std::distance(node_policy.begin(), std::max_element(node_policy.begin(), node_policy.end()));
        } else {
            // Normal softmax, but with the logarithmic policy. I.e. exp(log(p)/temperature), which cancels out
            // So instead, we compute p^(1/temperature) directly
            double p_sum = 0;
            for (auto &p: node_policy) {
                p = std::pow(p, 1.0 / params.policy_temperature);
                p_sum += p;
            }
            for (auto &p: node_policy) {
                p = p / p_sum;
            }
            std::discrete_distribution<size_t> dist(node_policy.begin(), node_policy.end());
            tactic_id = dist(gen);
        }
        assert(!mcts_node.killed(tactic_id));
        auto tactic_ptr = mcts_node.get_tactic(tactic_id);
        sim.set_tactic(current, tactic_ptr);
        sim.set_tactic_id(current, tactic_id);
        auto children = mcts_node.get_children_for_tactic(tactic_id);
        TheoremSet &seen = sim.get_theorem_set(current);
        // If any child has been seen, we have a circle, i.e. kill the tactic
        if (std::any_of(children.begin(), children.end(), [seen](const auto &thm) { return seen.contains(thm); })) {
            kill_tactic(mcts_node, tactic_id);
            cleanup(sim);
            find_unexplored_and_propagate_expandable();
            throw FailedTacticException();
        }
        mcts_node.add_virtual_count(tactic_id, params.virtual_loss);
        for (const auto &child: children) {
            sim.add_theorem(child, current, sim.get_depth(current) + 1);
            to_process.push_front(child);
        }
        // Free up memory
        sim.erase_theorem_set(current);
    }

    std::vector<std::shared_ptr<theorem>> all_leaves = sim.leaves();
    all_leaves.insert(all_leaves.end(), terminal.begin(), terminal.end());
    assert(!all_leaves.empty());
    assert(std::all_of(to_expand.begin(), to_expand.end(),
                       [this](const auto &thm) { return !this->nodes.contains(thm); }));
    assert(sim.leave_count() == all_leaves.size());
    for (const auto &thm: all_leaves) {
        sim.erase_theorem_set(thm);
    }
    return sim;
}

