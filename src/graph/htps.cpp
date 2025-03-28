//
// Created by simon on 12.02.25.
//

#include "htps.h"
#include <memory>
#include <vector>
#include <numeric>
#include <iostream>
#include <optional>

using namespace htps;

std::mt19937 htps::setup_gen() {
    std::random_device rd;
    size_t seed = std::getenv("SEED") ? std::stoi(std::getenv("SEED")) : rd();
#ifdef VERBOSE_PRINTS
    printf("Seed: %i", seed);
#endif
    return std::mt19937(seed);
}

std::mt19937 htps::gen = setup_gen();
std::uniform_real_distribution<double> htps::dis = std::uniform_real_distribution<double>(0, 1);

std::shared_ptr<theorem> HTPSSampleEffect::get_goal() const {
    return goal;
}

std::shared_ptr<tactic> HTPSSampleEffect::get_tactic() const {
    return tac;
}

std::vector<std::shared_ptr<theorem>> HTPSSampleEffect::get_children() const {
    return children;
}

void HTPSSampleEffect::get_children(std::vector<std::shared_ptr<theorem>> &children_vec) const {
    children_vec = children;
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

void Simulation::set_theorem_set(const std::shared_ptr<theorem> &thm, TheoremSet set) {
    seen.insert_or_assign(thm, set);
}

TheoremSet &Simulation::get_theorem_set(const std::shared_ptr<theorem> &thm) {
    if (!seen.contains(thm))
        set_theorem_set(thm, TheoremSet());
    return seen.at(thm);
}

void
Simulation::add_theorem(const std::shared_ptr<theorem> &thm, const std::shared_ptr<theorem> &parent, size_t thm_depth) {
    assert(theorems.contains(parent));
    assert(!theorems.contains(thm));
    assert(!depth.contains(thm));
    assert(!seen.contains(thm));
    assert(!children_for_theorem.contains(thm));
    assert(!parent_for_theorem.contains(thm));
    assert(!virtual_count_added.contains(thm));
    theorems.insert_or_assign(thm, thm);
    depth.insert_or_assign(thm, thm_depth);
    auto seen_set = seen.at(parent);
    seen_set.insert(thm);
    seen.insert_or_assign(thm, seen_set);
    children_for_theorem.at(parent).push_back(thm);
    parent_for_theorem.insert_or_assign(thm, parent);
    children_for_theorem.insert_or_assign(thm, std::vector<std::shared_ptr<theorem>>());
    virtual_count_added.insert_or_assign(thm, false);
}

size_t Simulation::leave_count() const {
    size_t count = 0;
    for (const auto &[thm_str, children]: children_for_theorem)
        if (children.empty())
            count++;
    return count;
}

bool Simulation::erase_theorem_set(const std::shared_ptr<theorem> &thm) {
    return seen.erase(thm);
}

void Simulation::receive_expansion(const std::shared_ptr<theorem> &thm, double value, bool is_solved) {
    assert(expansions > 0);
    set_value(thm, value);
    set_solved(thm, is_solved);
    expansions -= 1;
}

bool Simulation::get_virtual_count_added(const std::shared_ptr<theorem> &thm) const {
    if (!virtual_count_added.contains(thm))
        throw std::runtime_error("Virtual count not found");
    return virtual_count_added.at(thm);
}

void Simulation::set_virtual_count_added(const std::shared_ptr<theorem> &thm, bool value) {
    virtual_count_added.insert_or_assign(thm, value);
}

bool Simulation::should_backup() const {
    return expansions == 0;
}

std::shared_ptr<theorem> Simulation::parent(const std::shared_ptr<theorem> &thm) const {
    if (!parent_for_theorem.contains(thm)) {
        throw std::runtime_error("Parent not found");
    }
    return parent_for_theorem.at(thm);
}

std::vector<std::shared_ptr<theorem>> Simulation::get_children(const std::shared_ptr<theorem> &thm) const {
    return children_for_theorem.at(thm);
}

std::vector<double> Simulation::child_values(const std::shared_ptr<theorem> &thm) const {
    std::vector<double> result;
    for (const auto &child: children_for_theorem.at(thm)) {
        result.push_back(values.at(child));
    }
    return result;
}

void Simulation::reset_expansions() {
    expansions = 0;
}

void Simulation::increment_expansions() {
    expansions += 1;
}

size_t Simulation::num_tactics() {
    return tactics.size();
}

void HTPSNode::reset_HTPS_stats() {
    // implies we will simply set logW to the first value we receive
    reset_mask = std::vector<bool>(tactics.size(), true);
    counts = std::vector<size_t>(tactics.size(), 0);
    virtual_counts = std::vector<size_t>(tactics.size(), 0);
}

bool HTPSNode::should_send(size_t count_threshold) const {
    if (solved) {
        return true;
    }
    size_t count_sum = std::accumulate(counts.begin(), counts.end(), static_cast<size_t>(0));
    return count_sum >= count_threshold;
}

void HTPSNode::get_effect_samples(std::vector<HTPSSampleEffect> &samples, double subsampling_rate) const {
    for (const auto &effect: effects) {
        if (dis(gen) > subsampling_rate)
            continue;
        samples.emplace_back(thm, effect->tac, effect->children);
    }
}

std::vector<HTPSSampleEffect> HTPSNode::get_effect_samples(double subsampling_rate) const {
    std::vector<HTPSSampleEffect> samples;
    get_effect_samples(samples, subsampling_rate);
    return samples;
}

std::optional<HTPSSampleCritic> HTPSNode::get_critic_sample(double subsampling_rate) const {
    if (dis(gen) > subsampling_rate) {
        return std::nullopt;
    }
    // Compute visit count of the node by summing up all action counts
    size_t visit_sum = std::accumulate(counts.begin(), counts.end(), static_cast<size_t>(0));
    return HTPSSampleCritic(thm, std::exp(get_value()), solved, false, log_critic_value, visit_sum);
}


void HTPSNode::get_tactics_sample_q_conditioning(size_t count_threshold,
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
                q_values.push_back(std::exp(log_w[id]) / static_cast<double>(counts[id]));
            }
        }
    }
    assert (q_values.size() == valid_tactics.size());
    assert (q_values.size() == selected_tactics_ids.size());
    assert (q_values.size() == valid_priors.size());
    assert (q_values.size() == valid_targets.size());
}

void HTPSNode::get_tactics_sample_regular(Metric metric, NodeMask node_mask,
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
        valid_targets.resize(selected_tactic_ids.size());
        std::fill(valid_targets.begin(), valid_targets.begin(), 1.0 / static_cast<double>(tactics.size()));
    }
    assert(selected_tactic_ids.size() == valid_targets.size());
    assert(selected_tactic_ids.size() == valid_priors.size());
    assert(selected_tactic_ids.size() == valid_tactics.size());
}

std::optional<HTPSSampleTactics> HTPSNode::get_tactics_sample(Metric metric, NodeMask node_mask,
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
        inproof = InProof::IsInProof;
    } else {
        inproof = InProof::NotInProof;
    }
    // Compute visit count of the node by summing up all action counts
    size_t visit_sum = std::accumulate(counts.begin(), counts.end(), static_cast<size_t>(0));
    return HTPSSampleTactics(thm, valid_tactics, valid_targets, inproof, q_values, visit_sum);
}

bool HTPSNode::kill_tactic(size_t tactic_id) {
    bool killed = Node::kill_tactic(tactic_id);
    if (all_tactics_killed()) {
        assert(log_critic_value > MIN_FLOAT);
        old_critic_value = log_critic_value;
        log_critic_value = MIN_FLOAT;
    }
    return killed;
}

void HTPSNode::compute_policy(std::vector<double> &result, bool force_expansion) const {
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

std::vector<double> HTPSNode::compute_policy(bool force_expansion) const {
    std::vector<double> result;
    compute_policy(result, force_expansion);
    return result;
}

void HTPSNode::update(size_t tactic_id, double backup_value) {
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

double HTPSNode::get_value() const {
    if (solved)
        return 0.0;
    if (is_terminal())
        return MIN_FLOAT;
    size_t visit_sum = std::accumulate(counts.begin(), counts.end(), 0ul);
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

void HTPSNode::add_virtual_count(size_t tactic_id, size_t count) {
    virtual_counts[tactic_id] += count;
}

bool HTPSNode::_validate() const {
    if (!policy)
        return false;
    if (q_value_solved == QValueSolvedCount)
        return false;
    if (tactics.empty())
        return false;
    if (children_for_tactic.size() != tactics.size())
        return false;
    if (children_for_tactic.size() != priors.size())
        return false;
    if (log_critic_value > 0.0)
        return false;
    // Assert is probability
    double sum = std::accumulate(priors.begin(), priors.end(), 0.0);
    if (sum < 0.99 || sum > 1.01)
        return false;
    // Assert we have at least one valid tactic
    bool is_valid = std::any_of(tactics.begin(), tactics.end(), [](const auto &t) { return t->is_valid; });
    return is_valid;
}

bool HTPSNode::has_virtual_count(size_t tactic_id) const {
    return virtual_counts[tactic_id] > 0;
}

void HTPSNode::subtract_virtual_count(size_t tactic_id, size_t count) {
    assert(virtual_counts[tactic_id] >= count);
    virtual_counts[tactic_id] -= count;
}

bool HTPSNode::has_virtual_count() const {
    return std::any_of(virtual_counts.begin(), virtual_counts.end(), [](size_t count) { return count > 0; });
}

std::size_t std::hash<HTPSNode>::operator()(const HTPSNode &n) const {
    return std::hash<theorem>()(*n.get_theorem());
}

bool HTPS::is_leaf(const std::shared_ptr<htps::HTPSNode> &node) const {
    return node->is_solved() && !(is_proven()) && (params.early_stopping_solved_if_root_not_proven);
}

void HTPS::find_unexplored_and_propagate_expandable() {
    if (!propagate_needed)
        return;
    bool ignore_solved = params.early_stopping || (!is_proven() && params.early_stopping_solved_if_root_not_proven);
    Graph::find_unexplored_and_propagate_expandable(ignore_solved);
}

bool HTPS::dead_root() const {
    return (propagate_needed && unexplored_theorems.empty()) || (nodes.contains(root) && nodes.at(root)->is_bad());
}

void
HTPS::get_train_samples(std::vector<HTPSSampleEffect> &samples_effects, std::vector<HTPSSampleCritic> &samples_critic,
                        std::vector<HTPSSampleTactics> &samples_tactics) const {
    std::vector<HTPSSampleCritic> critic_solved;
    std::vector<HTPSSampleCritic> critic_unsolved;
    // Will be less than the number of nodes, but we don't know how many
    critic_solved.reserve(nodes.size());
    critic_unsolved.reserve(nodes.size());
    samples_critic.reserve(nodes.size());
    samples_tactics.reserve(nodes.size());
    samples_effects.reserve(nodes.size());
    std::vector<HTPSSampleEffect> node_samples;
    NodeMask node_mask = params.node_mask;
    if (node_mask == MinimalProofSolving) {
        if (is_proven())
            node_mask = MinimalProof;
        else
            node_mask = Solving;
    }

    for (const auto &[thm, node]: nodes) {
        node->get_effect_samples(node_samples, params.effect_subsampling_rate);
        samples_effects.insert(samples_effects.end(), node_samples.begin(), node_samples.end());
        node_samples.clear();
        auto critic_sample = node->get_critic_sample(params.critic_subsampling_rate);
        if (critic_sample) {
            if (node->is_solved()) {
                critic_solved.push_back(critic_sample.value());
            } else {
                critic_unsolved.push_back(critic_sample.value());
            }
        }
        auto tactic_sample = node->get_tactics_sample(params.metric, node_mask, params.only_learn_best_tactics,
                                                     params.tactic_p_threshold, params.count_threshold,
                                                     params.tactic_sample_q_conditioning);
        if (tactic_sample) {
            samples_tactics.push_back(tactic_sample.value());
        }
    }
    samples_critic = std::move(critic_solved);
    samples_critic.insert(samples_critic.end(), critic_unsolved.begin(), critic_unsolved.end());
    samples_critic.shrink_to_fit();
    samples_tactics.shrink_to_fit();
    samples_effects.shrink_to_fit();
}

void HTPS::get_proof_samples(std::vector<HTPSSampleTactics> &proof_samples_tactics) const {
    if (!is_proven())
        return;
    // Upper bound on the number of samples
    proof_samples_tactics.reserve(nodes.size());
    for (const auto &[thm, node]: nodes) {
        auto tactic_sample = node->get_tactics_sample(params.metric, MinimalProof, params.only_learn_best_tactics,
                                                     params.tactic_p_threshold, params.count_threshold,
                                                     params.tactic_sample_q_conditioning);
        if (tactic_sample) {
            proof_samples_tactics.push_back(tactic_sample.value());
        }
    }
    proof_samples_tactics.shrink_to_fit();
}

Simulation HTPS::find_leaves_to_expand(std::vector<std::shared_ptr<theorem>> &terminal,
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
            // TODO: in theory there is some depth stuff here?
            to_expand.push_back(current);
#ifdef VERBOSE_PRINTS
            printf("Adding to to_expand...\n");
#endif
            continue;
        }
        auto HTPS_node = nodes.at(current);
        bool is_leaf_node = is_leaf(HTPS_node);
        if (HTPS_node->is_terminal() || is_leaf_node) {
            assert(HTPS_node->is_solved() || is_leaf_node);
            sim.set_value(current, HTPS_node->get_value());
            sim.set_solved(current, true);
            terminal.push_back(current);
            continue;
        }
        if (params.early_stopping && HTPS_node->is_solved()) {
            sim.set_value(current, 0.0);
            sim.set_solved(current, true);
            terminal.push_back(current);
            continue;
        }
        // Select subsequent tactic
        HTPS_node->compute_policy(node_policy, true);
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
        assert(!HTPS_node->killed(tactic_id));
        auto tactic_ptr = HTPS_node->get_tactic(tactic_id);
#ifdef VERBOSE_PRINTS
        printf("Setting tactic %i\n", tactic_id);
#endif
        sim.set_tactic(current, tactic_ptr);
        sim.set_tactic_id(current, tactic_id);
        auto children = HTPS_node->get_children_for_tactic(tactic_id);
        TheoremSet &seen = sim.get_theorem_set(current);
        // If any child has been seen, we have a circle, i.e. kill the tactic
        if (std::any_of(children.begin(), children.end(), [seen](const auto &thm) { return seen.contains(thm); })) {
            kill_tactic(HTPS_node, tactic_id);
            cleanup(sim);
            find_unexplored_and_propagate_expandable();
            throw FailedTacticException();
        }
        HTPS_node->add_virtual_count(tactic_id, params.virtual_loss);
        sim.set_virtual_count_added(current, true);
        for (const auto &child: children) {
            sim.add_theorem(child, current, sim.get_depth(current) + 1);
            to_process.push_front(child);
        }
        // Free up memory
        sim.erase_theorem_set(current);
    }

    std::vector<std::shared_ptr<theorem>> all_leaves = terminal;
    all_leaves.insert(all_leaves.end(), to_expand.begin(), to_expand.end());
    assert(!all_leaves.empty());
    assert(std::all_of(to_expand.begin(), to_expand.end(),
                       [this](const auto &thm) { return !this->nodes.contains(thm); }));
    assert(sim.leave_count() == all_leaves.size());
    for (const auto &thm: all_leaves) {
#ifdef VERBOSE_PRINT
        printf("Erasing theorem set\n");
#endif
        sim.erase_theorem_set(thm);
    }
    return sim;
}

void HTPS::receive_expansion(std::shared_ptr<theorem> &thm, double value, bool solved) {
    // has to be log value
    assert(value <= 0);
    if (!simulations_for_theorem.contains(thm))
        throw std::runtime_error("No simulation for theorem");
    for (const auto &simulation: simulations_for_theorem.at(thm)) {
        simulation->receive_expansion(thm, value, solved);
    }
    simulations_for_theorem.erase(thm);
    currently_expanding.erase(thm);
}

void HTPS::expand(std::vector<std::shared_ptr<env_expansion>> &expansions) {
    std::vector<HTPSNode> nodes;

    for (const auto &expansion: expansions) {
        if (expansion->is_error()) {
#ifdef VERBOSE_PRINTS
            printf("Is error");
#endif
            HTPSNode current = HTPSNode(
                    expansion->thm, {}, {}, policy, {}, params.exploration, MIN_FLOAT,
                    params.q_value_solved, params.tactic_init_value, expansion->effects);
            receive_expansion(expansion->thm, MIN_FLOAT, false);
            nodes.push_back(current);
            continue;
        }
        assert(expansion->log_critic > MIN_FLOAT);
        assert(!expansion->tactics.empty());
        assert(!expansion->children_for_tactic.empty());
        assert(!expansion->priors.empty());
        // If one tactic solves, all tactics of the expansion should solve
        if (expansion->children_for_tactic.at(0).empty()) {
#ifdef VERBOSE_PRINTS
            printf("Solved!");
#endif
            assert(std::all_of(expansion->children_for_tactic.begin(), expansion->children_for_tactic.end(),
                               [](const auto &children) {
                                   return children.empty();
                               }));
            assert(std::all_of(expansion->tactics.begin(), expansion->tactics.end(),
                               [](const std::shared_ptr<tactic> &tactic) {
                                   return tactic->is_valid;
                               }));
            // Solved gets value 1, i.e. log value 0
            HTPSNode current = HTPSNode(
                    expansion->thm, expansion->tactics, expansion->children_for_tactic, policy, expansion->priors,
                    params.exploration, 0.0, params.q_value_solved, params.tactic_init_value, expansion->effects);
            receive_expansion(expansion->thm, 0.0, true);
            nodes.push_back(current);
            continue;
        }
        HTPSNode current = HTPSNode(
                expansion->thm, expansion->tactics, expansion->children_for_tactic, policy, expansion->priors,
                params.exploration, expansion->log_critic, params.q_value_solved, params.tactic_init_value,
                expansion->effects);
        receive_expansion(expansion->thm, expansion->log_critic, true);
        nodes.push_back(current);
    }
    add_nodes(nodes);
    expansion_count += nodes.size();
}

void HTPS::cleanup(Simulation &to_clean) {
    std::shared_ptr<HTPSNode> current;
    for (const auto &[unique_str, thm]: to_clean) {
        if (!nodes.contains(thm))
            continue;
        current = nodes.at(thm);
        if (to_clean.get_virtual_count_added(thm)) {
            current->subtract_virtual_count(to_clean.get_tactic_id(thm), params.virtual_loss);
        }
    }
}

void HTPS::backup() {
    bool only_value;
    long i = 0;
    while (i < simulations.size()) {
        auto simulation = simulations[i];
        if (!simulation->should_backup()) {
            i += 1;
            continue;
        }
        only_value = false;
        if (params.backup_once) {
            auto hashed = std::hash<htps::Simulation>{}(*simulation);
            if (backedup_hashes.contains(hashed))
                only_value = true;
            else
                backedup_hashes.insert(hashed);
        }
        backup_leaves(simulation, only_value);
        simulations.erase(simulations.begin() + i);
    }
}

void HTPS::backup_leaves(std::shared_ptr<Simulation> &sim, bool only_value) {
    auto leaves = sim->leaves();
    bool updated_root = false;
    std::queue<std::shared_ptr<theorem>> to_process;

    TheoremMap<size_t> children_propagated;
    for (const auto &leaf: leaves) {
        std::shared_ptr<HTPSNode> current = nodes.at(leaf);
        if (sim->get_virtual_count_added(leaf)) {
            current->subtract_virtual_count(sim->get_tactic_id(leaf), params.virtual_loss);
        }
        assert(sim->get_value(leaf) <= 0); // log
        auto parent = sim->parent(leaf);
        if (!parent) {
            assert(leaf == root);
            updated_root = true;
            continue;
        }
        if (!children_propagated.contains(parent)) {
            children_propagated.insert_or_assign(parent, 1);
        } else {
            children_propagated.at(parent) += 1;
            assert(children_propagated.at(parent) <= sim->get_children(parent).size());
        }
        if (children_propagated.at(parent) == sim->get_children(parent).size()) {
            to_process.push(parent);
        }
    }
    while (!to_process.empty()) {
        auto cur = to_process.front();
        to_process.pop();
        std::vector<double> child_values = sim->child_values(cur);
        assert(std::all_of(child_values.begin(), child_values.end(), [](const auto &v) { return v <= 0; }));
        double sum_log = std::accumulate(child_values.begin(), child_values.end(), 0.0);
        std::shared_ptr<HTPSNode> current_node = nodes.at(cur);
        if (current_node->is_solved() && params.backup_one_for_solved) {
            sum_log = 0.0;
        }
        if (params.depth_penalty < 1) {
            sum_log += std::log(params.depth_penalty);
        }
        assert(sum_log <= 0);
        sim->set_value(cur, sum_log);
        if (sim->get_virtual_count_added(cur)) {
            current_node->subtract_virtual_count(sim->get_tactic_id(cur), params.virtual_loss);
        }
        if (!only_value) {
            current_node->update(sim->get_tactic_id(cur), sum_log);
        }
        if (!sim->parent(cur)) {
            updated_root = true;
            continue;
        }
        children_propagated.at(sim->parent(cur)) += 1;
        if (children_propagated.at(sim->parent(cur)) == sim->get_children(sim->parent(cur)).size()) {
            to_process.push(sim->parent(cur));
        }
    }
    assert(updated_root);
}

void HTPS::batch_to_expand(std::vector<std::shared_ptr<theorem>> &theorems) {
    propagate_needed = false;
    TheoremMap<std::shared_ptr<theorem>> result;
    theorems.clear();
    std::vector<std::shared_ptr<theorem>> single_to_expand;

    for (size_t i = 0; i < params.succ_expansions; i++) {
        single_to_expand.clear();
        std::vector<std::shared_ptr<theorem>> terminal;
        std::vector<std::shared_ptr<theorem>> to_expand;
        Simulation sim = find_leaves_to_expand(terminal, to_expand);
        if (to_expand.empty()) {
#ifdef VERBOSE_PRINTS
            printf("To expand is empty!");
#endif
            break;
        }
        _single_to_expand(single_to_expand, sim, to_expand);
        for (const auto &thm: single_to_expand) {
            result.insert(thm, thm);
        }
    }
    if (result.empty()) {
        done = true;
        return;
    }
    // TODO: maybe we need the n_expansions here to decide whether we are done
    for (const auto &[unique_str, thm]: result) {
        theorems.push_back(thm);
    }
}

void HTPS::_single_to_expand(std::vector<std::shared_ptr<theorem>> &theorems, Simulation &sim,
                             std::vector<std::shared_ptr<theorem>> &leaves_to_expand) {
    theorems.clear();
    TheoremSet seen;
    std::shared_ptr<Simulation> sim_ptr = std::make_shared<Simulation>(sim);
    sim_ptr->reset_expansions();
#ifdef VERBOSE_PRINTS
    printf("Adding simulation!");
#endif
    simulations.push_back(sim_ptr);
    for (const auto &leaf: leaves_to_expand) {
        if (!seen.contains(leaf)) {
            seen.insert(leaf);
            if (simulations_for_theorem.contains(leaf))
                simulations_for_theorem.at(leaf).push_back(sim_ptr);
            else
                simulations_for_theorem.insert(leaf, std::vector<std::shared_ptr<Simulation>>{sim_ptr});
            sim_ptr->increment_expansions();
        }
        if (!currently_expanding.contains(leaf)) {
            currently_expanding.insert(leaf);
            theorems.push_back(leaf);
        }
    }
}

std::vector<std::shared_ptr<theorem>> HTPS::batch_to_expand() {
    std::vector<std::shared_ptr<theorem>> theorems;
    batch_to_expand(theorems);
    return theorems;
}

void HTPS::expand_and_backup(std::vector<std::shared_ptr<env_expansion>> &expansions) {
    expand(expansions);
    backup();

    assert(std::none_of(nodes.begin(), nodes.end(), [](const auto &node) {
        return node.second->has_virtual_count();
    }));
    if (is_proven() && !initial_minimum_proof_size.has_value()) {
        build_in_proof();
        get_node_proof_sizes_and_depths();
        for (size_t i = 0; i < METRIC_COUNT; i++) {
            initial_minimum_proof_size.set(static_cast<Metric>(i),
                                           nodes.at(root)->minimum_length(static_cast<Metric>(i)));
            assert(initial_minimum_proof_size.has_value(static_cast<Metric>(i)));
            assert(nodes.at(root)->is_in_minimum_proof(static_cast<Metric>(i)));
        }
        reset_minimum_proof_stats();
    }
    if (is_proven()) {
        done = done || params.early_stopping;
    }
    assert(nodes.size() == expansion_count); // not sure whether this is correct
    done = done || (expansion_count >= params.num_expansions);
//    if (!done)
//        HTPS_move();
}

void HTPS::theorems_to_expand(std::vector<std::shared_ptr<theorem>> &theorems) {
    return batch_to_expand(theorems);
}

std::vector<std::shared_ptr<theorem>> HTPS::theorems_to_expand() {
    std::vector<std::shared_ptr<theorem>> theorems;
    theorems_to_expand(theorems);
    return theorems;
}

HTPSResult HTPS::get_result() {
    check_solved_consistency();
    for (const auto &[thm, node]: nodes) {
        assert(!node->has_virtual_count());
    }
    build_in_proof();
    get_node_proof_sizes_and_depths();
    struct proof p;
    if (is_proven()) {
        for (size_t i = 0; i < METRIC_COUNT; i++) {
            assert(minimum_proof_size.has_value(static_cast<Metric>(i)));
        }
        p = minimal_proof(params.metric, root);
    }
    std::vector<HTPSSampleEffect> samples_effects;
    std::vector<HTPSSampleCritic> samples_critic;
    std::vector<HTPSSampleTactics> samples_tactics;
    get_train_samples(samples_effects, samples_critic, samples_tactics);
    std::vector<HTPSSampleTactics> proof_samples_tactics;
    get_proof_samples(proof_samples_tactics);
    return {samples_critic, samples_tactics, samples_effects, params.metric, proof_samples_tactics, root, p};
}

void HTPS::set_root(std::shared_ptr<theorem> &thm) {
    if (!nodes.empty()) {
        throw std::runtime_error("HTPS has already started, can't set root!");
    }
    root = thm;
    ancestors.add_ancestor(root, nullptr, 0);
    permanent_ancestors.add_ancestor(root, nullptr, 0);
    unexplored_theorems.insert(*root);
}

void HTPS::set_params(const htps_params &new_params) {
    params = new_params;
    policy = std::make_shared<Policy>(params.policy_type, params.exploration);
}


proof HTPSResult::get_proof() const {
    return p;
}

std::shared_ptr<theorem> HTPSResult::get_goal() const {
    return goal;
}

std::tuple<std::vector<HTPSSampleCritic>, std::vector<HTPSSampleTactics>, std::vector<HTPSSampleEffect>, Metric, std::vector<HTPSSampleTactics>>
HTPSResult::get_samples() const {
    return {samples_critic, samples_tactic, samples_effect, metric, proof_samples_tactics};
}

