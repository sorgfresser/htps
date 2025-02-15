//
// Created by simon on 12.02.25.
//

#include "mcts.h"
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


void MCTSNode::reset_mcts_stats() {
    // implies we will simply set logW to the first value we receive
    reset_mask = std::vector<bool>(tactics.size(), true);
    counts = std::vector<size_t>(tactics.size(), 0);
    virtual_counts = std::vector<size_t>(tactics.size(), 0);
}

bool MCTSNode::should_send(size_t count_threshold) {
    if (solved) {
        return true;
    }
    size_t count_sum = std::accumulate(counts.begin(), counts.end(), 0);
    return count_sum >= count_threshold;
}

void MCTSNode::get_effect_samples(std::vector<MCTSSampleEffect> &samples, double subsampling_rate) {
    for (const auto &[tac, children]: effects) {
        if (dis(gen) > subsampling_rate)
            continue;
        samples.emplace_back(thm, tac, children);
    }
}

std::vector<MCTSSampleEffect> MCTSNode::get_effect_samples(double subsampling_rate) {
    std::vector<MCTSSampleEffect> samples;
    get_effect_samples(samples, subsampling_rate);
    return samples;
}

std::optional<MCTSSampleCritic> MCTSNode::get_critic_sample(double subsampling_rate) {
    if (dis(gen) > subsampling_rate) {
        return std::nullopt;
    }
    // Not entirely sure whether visit_sum is correct here
    size_t visit_sum = std::accumulate(counts.begin(), counts.end(), 0);
    return MCTSSampleCritic(thm, std::exp(get_value()), solved, false, log_critic_value, visit_sum);
}


void MCTSNode::get_tactics_sample_q_conditioning(size_t count_threshold,
                                                 std::vector<std::shared_ptr<tactic>> &valid_tactics,
                                                 std::vector<double> &valid_priors,
                                                 std::vector<double> &valid_targets,
                                                 std::vector<double> &q_values) {
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
                                          std::vector<double> &valid_targets) {
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
                                                              size_t count_threshold, bool for_q_conditioning) {
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
    // Not entirely sure whether visit_sum is correct here
    size_t visit_sum = std::accumulate(counts.begin(), counts.end(), 0);
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

void MCTSNode::compute_policy(std::vector<double> &result, bool force_expansion) {
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
            q_values[i] = std::exp(log_w[i]) / full_counts[i];
        }
    }
    for (const auto &tac: solving_range()) {
        switch (q_value_solved) {
            case OneOverCounts:
                if (full_counts[tac] > 0)
                    q_values[tac] = 1.0 / full_counts[tac];
                break;
            case CountOverCounts:
                if (full_counts[tac] > 0)
                    q_values[tac] = counts[tac] / full_counts[tac];
                break;
            case One:
                q_values[tac] = 1.0;
                break;
            case OneOverVirtualCounts:
                q_values[tac] = 1.0 / (1 + virtual_counts[tac]);
            case OneOverCountsNoFPU:
                q_values[tac] = 1.0 / std::max(static_cast<size_t>(1), full_counts[tac]);
            case CountOverCountsNoFPU:
                q_values[tac] = std::max(static_cast<size_t>(1), counts[tac]) /
                                std::max(static_cast<size_t>(1), full_counts[tac]);
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

std::vector<double> MCTSNode::compute_policy(bool force_expansion) {
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

double MCTSNode::get_value() {
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