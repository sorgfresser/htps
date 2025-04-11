//
// Created by simon on 12.02.25.
//

#include "htps.h"
#include "graph.h"
#include <memory>
#include <vector>
#include <numeric>
#include <iostream>
#include <optional>


using namespace htps;

size_t htps::get_seed() {
    std::random_device rd;
    size_t s = std::getenv("SEED") ? std::stoi(std::getenv("SEED")) : rd();
#ifdef VERBOSE_PRINTS
    printf("Seed: %i", s);
#endif
    return s;
}

std::mt19937 htps::setup_gen() {
    std::random_device rd;
#ifdef VERBOSE_PRINTS
    printf("Seed: %i", htps::seed);
#endif
    return std::mt19937(htps::seed);
}

size_t htps::seed = get_seed();
std::mt19937 htps::gen = setup_gen();
std::uniform_real_distribution<double> htps::dis = std::uniform_real_distribution<double>(0, 1);

TheoremPointer HTPSSampleEffect::get_goal() const {
    return goal;
}

std::shared_ptr<tactic> HTPSSampleEffect::get_tactic() const {
    return tac;
}

std::vector<TheoremPointer> HTPSSampleEffect::get_children() const {
    return children;
}

void HTPSSampleEffect::set_children(std::vector<TheoremPointer> &children_vec) const {
    children_vec = children;
}


std::vector<std::pair<TheoremPointer, size_t>> Simulation::leaves() const {
    std::vector<std::pair<TheoremPointer, size_t>> result;
    leaves(result);
    return result;
}

void Simulation::leaves(std::vector<std::pair<TheoremPointer, size_t>> &leaves_vector) const {
    for (const auto &[thm_hash, children]: children_for_theorem)
        if (children.first.empty())
            leaves_vector.push_back(theorems.at(thm_hash));
}

bool Simulation::operator==(const Simulation &other) const {
    if (*root != *other.root)
        return false;
    if (children_for_theorem.size() != other.children_for_theorem.size())
        return false;
    for (const auto &[thm_str, children]: children_for_theorem) {
        // And children must match
        auto other_it = other.children_for_theorem.find(thm_str);
        if (other_it == other.children_for_theorem.end())
            return false;
        auto other_children = other_it->second;
        if (children.first.size() != other_children.first.size())
            return false;
        // No need to sort etc., as the order must match
        for (size_t i = 0; i < children.first.size(); i++) {
            if (children.first[i] != other_children.first[i])
                return false;
        }
    }
    // Tactics must match
    for (const auto &[thm_str, tactic]: tactics) {
        if (!other.tactics.contains(thm_str))
            return false;
        if (*tactics.at(thm_str).first != *other.tactics.at(thm_str).first)
            return false;
    }
    // Now the other way around. Since we now that they match on each theorem of the first simulation,
    // we only need to check that the other simulation has no extra theorems
    bool has_children = std::all_of(other.children_for_theorem.begin(), other.children_for_theorem.end(), [this](const auto &pair) {
        return children_for_theorem.contains(pair.first);
    });
    bool has_tactics = std::all_of(other.tactics.begin(), other.tactics.end(), [this](const auto &pair) {
        return tactics.contains(pair.first);
    });
    return has_children && has_tactics;
}


size_t Simulation::get_depth(const TheoremPointer &thm, const size_t previous) const {
    return depth.at(thm, previous).first;
}

bool Simulation::has_depth(const TheoremPointer &thm, const size_t &previous) const {
    return depth.contains(thm);
}

void Simulation::update_depth(const TheoremPointer &thm, size_t d, const size_t &previous) {
    auto it = depth.find(thm);
    size_t min_depth;
    if (it != depth.end()) {
        min_depth = std::min(it->second.first, d);
    } else {
        min_depth = d;
    }
    depth.insert_or_assign(thm, min_depth, previous);
}

void Simulation::set_value(const TheoremPointer &thm, double v, const size_t &previous) {
    values.insert_or_assign(thm, v, previous);
}

double Simulation::get_value(const size_t &hash_) const {
    if (!values.contains(hash_))
        throw std::runtime_error("Value not found");
    return values.at(hash_).first;
}

double Simulation::get_value(const TheoremPointer &thm, const size_t &previous) const {
    return get_value(get_hash(thm, previous));
}

bool Simulation::is_solved(const TheoremPointer &thm, const size_t &previous) const {
    return solved.at(thm, previous).first;
}

void Simulation::set_solved(const TheoremPointer &thm, bool s, const size_t &previous) {
    solved.insert_or_assign(thm, s, previous);
}

void Simulation::set_tactic(const TheoremPointer &thm, const std::shared_ptr<tactic> &tac, const size_t &previous) {
    tactics.insert_or_assign(thm, tac, previous);
}

std::shared_ptr<tactic> Simulation::get_tactic(const TheoremPointer &thm, const size_t &previous) const {
    return tactics.at(thm, previous).first;
}

void Simulation::set_tactic_id(const TheoremPointer &thm, size_t id, const size_t &previous) {
    tactic_ids.insert_or_assign(thm, id, previous);
}

size_t Simulation::get_tactic_id(const size_t &hash_) const {
    if (!tactic_ids.contains(hash_))
        throw std::runtime_error("Tactic ID not found");
    return tactic_ids.at(hash_).first;
}

size_t Simulation::get_tactic_id(const TheoremPointer &thm, const size_t &previous) const {
    return get_tactic_id(get_hash(thm, previous));
}

void Simulation::set_theorem_set(const TheoremPointer &thm, TheoremSet &set, const size_t &previous) {
    seen.insert_or_assign(thm, set, previous);
}

void Simulation::set_theorem_set(const TheoremPointer &thm, TheoremSet set, const size_t &previous) {
    seen.insert_or_assign(thm, set, previous);
}

TheoremSet &Simulation::get_theorem_set(const TheoremPointer &thm, const size_t &previous) {
    if (!seen.contains(thm, previous))
        set_theorem_set(thm, TheoremSet(), previous);
    return seen.at(thm, previous).first;
}

void
Simulation::add_theorem(const TheoremPointer &thm, const TheoremPointer &parent, const size_t &parent_hash, const size_t thm_depth) {
    assert(theorems.contains(parent_hash));
    assert(!theorems.contains(thm, parent_hash));
    assert(!depth.contains(thm, parent_hash));
    assert(!seen.contains(thm, parent_hash));
    assert(!children_for_theorem.contains(thm, parent_hash));
    assert(!parent_for_theorem.contains(thm, parent_hash));
    assert(!virtual_count_added.contains(thm, parent_hash));
    theorems.insert_or_assign(thm, thm, parent_hash);
    depth.insert_or_assign(thm, thm_depth, parent_hash);
    auto seen_set = seen.at(parent_hash).first;
    seen_set.insert(thm);
    seen.insert_or_assign(thm, seen_set, parent_hash);
    children_for_theorem.at(parent_hash).first.push_back(thm);
    parent_for_theorem.insert_or_assign(thm, parent, parent_hash);
    children_for_theorem.insert_or_assign(thm, std::vector<TheoremPointer>(), parent_hash);
    virtual_count_added.insert_or_assign(thm, false, parent_hash);
}

size_t Simulation::leave_count() const {
    size_t count = 0;
    for (const auto &[thm_str, children]: children_for_theorem)
        if (children.first.empty())
            count++;
    return count;
}

bool Simulation::erase_theorem_set(const TheoremPointer &thm, const size_t &previous) {
    return seen.erase(thm, previous);
}

void Simulation::receive_expansion(const TheoremPointer &thm, double value, bool is_solved, const size_t &previous) {
    assert(expansions > 0);
    set_value(thm, value, previous);
    set_solved(thm, is_solved, previous);
}

bool Simulation::get_virtual_count_added(const size_t &hash_) const {
    if (!virtual_count_added.contains(hash_))
        throw std::runtime_error("Virtual count not found");
    return virtual_count_added.at(hash_).first;
}

bool Simulation::get_virtual_count_added(const TheoremPointer &thm, const size_t &previous) const {
    return get_virtual_count_added(get_hash(thm, previous));
}

void Simulation::set_virtual_count_added(const TheoremPointer &thm, bool value, const size_t &previous) {
    virtual_count_added.insert_or_assign(thm, value, previous);
}

bool Simulation::should_backup() const {
    return expansions == 0;
}

TheoremPointer Simulation::parent(const size_t &hash_) const {
    if (!parent_for_theorem.contains(hash_)) {
        throw std::runtime_error("Parent not found");
    }
    return parent_for_theorem.at(hash_).first;
}

TheoremPointer Simulation::parent(const TheoremPointer &thm, const size_t &previous) const {
    return parent(get_hash(thm, previous));
}

std::vector<TheoremPointer> Simulation::get_children(const size_t &hash_) const {
    if (!children_for_theorem.contains(hash_)) {
        throw std::runtime_error("Children not found");
    }
    return children_for_theorem.at(hash_).first;
}

std::vector<TheoremPointer> Simulation::get_children(const TheoremPointer &thm, const size_t &previous) const {
    return get_children(get_hash(thm, previous));
}

std::vector<double> Simulation::child_values(const size_t &hash_) const {
    if (!children_for_theorem.contains(hash_)) {
        throw std::runtime_error("Children not found");
    }
    std::vector<double> result;
    for (const auto &child: children_for_theorem.at(hash_).first) {
        result.push_back(get_value(child, hash_));
    }
    return result;
}

std::vector<double> Simulation::child_values(const TheoremPointer &thm, const size_t &previous) const {
    return child_values(get_hash(thm, previous));
}

void Simulation::reset_expansions() {
    expansions = 0;
}

void Simulation::increment_expansions() {
    expansions += 1;
}


void Simulation::decrement_expansions() {
    expansions -= 1;
}

size_t Simulation::num_tactics() {
    return tactics.size();
}

Simulation::operator nlohmann::json() const {
    nlohmann::json j;
    j["root"] = *root;
    j["theorems"] = theorems;
    j["tactic_ids"] = tactic_ids;
    j["tactics"] = tactics;
    j["depth"] = depth;
    j["children_for_theorem"] = children_for_theorem;
    j["parent_for_theorem"] = parent_for_theorem;
    j["values"] = values;
    j["solved"] = solved;
    j["virtual_count_added"] = virtual_count_added;
    j["seen"] = seen;
    j["expansions"] = expansions;
    return j;
}

Simulation Simulation::from_json(const nlohmann::json &j) {
    Simulation s;
    s.root = j["root"];
    TheoremIncrementalMap<TheoremPointer> thms = TheoremIncrementalMap<TheoremPointer>::from_json(j["theorems"]);
    s.theorems = thms;
    s.tactic_ids = TheoremIncrementalMap<size_t>::from_json(j["tactic_ids"]);
    TheoremIncrementalMap<std::shared_ptr<tactic>> tacs = TheoremIncrementalMap<std::shared_ptr<tactic>>::from_json(j["tactics"]);
    s.tactics = tacs;
    s.depth = TheoremIncrementalMap<size_t>::from_json(j["depth"]);
    TheoremIncrementalMap<std::vector<TheoremPointer>> children;
    for (const auto &[thm_str, children_vec]: j["children_for_theorem"].items()) {
        std::vector<TheoremPointer> children_vec_;
        std::pair<nlohmann::json, std::size_t> pair;
        pair.first = children_vec["value"];
        pair.second = children_vec["previous"];
        for (const auto &child_str: pair.first) {
            children_vec_.emplace_back(child_str);
        }
        children.insert_or_assign(thm_str, children_vec_, pair.second);
    }
    s.children_for_theorem = children;
    TheoremIncrementalMap<TheoremPointer> parents;
    for (const auto &[thm_str, parent_str]: j["parent_for_theorem"].items()) {
        std::pair<nlohmann::json, std::size_t> pair;
        pair.first = parent_str["value"];
        pair.second = parent_str["previous"];
        if (parent_str.is_null()) {
            parents.insert_or_assign(thm_str, nullptr, pair.second);
            continue;
        }
        parents.insert_or_assign(thm_str, pair.first, pair.second);
    }
    s.parent_for_theorem = parents;
    if (j["values"].is_null()) {
        s.values = TheoremIncrementalMap<double>();
    }
    else {
        auto null_replacement = MIN_FLOAT; // -inf is serialized as null
        s.values = TheoremIncrementalMap<double>::from_json(j["values"], &null_replacement);
    }
    if (j["solved"].is_null()) {
        s.solved = TheoremIncrementalMap<bool>();
    }
    else {
        s.solved = TheoremIncrementalMap<bool>::from_json(j["solved"]);
    }
    s.virtual_count_added = TheoremIncrementalMap<bool>::from_json(j["virtual_count_added"]);
    s.expansions = j["expansions"];
    if (j["seen"].is_null()) {
        s.seen = TheoremIncrementalMap<TheoremSet>();
    }
    else {
        TheoremIncrementalMap<TheoremSet> seen{};
        for (const auto &[thm_str, thm_set]: j["seen"].items()) {
            std::pair<nlohmann::json, std::size_t> pair;
            pair.first = thm_set["value"];
            pair.second = thm_set["previous"];
            seen.insert_or_assign(thm_str, TheoremSet::from_json(pair.first), pair.second);
        }
        s.seen = seen;
    }
    return s;
}

void Simulation::deduplicate(const TheoremPointer &ptr) {
    for (const auto &[thm_str, thm]: theorems) {
        if (*thm.first == *ptr) {
            theorems.insert_or_assign(thm.first, ptr, thm.second);
        }
    }
    for (auto &[thm_str, children]: children_for_theorem) {
        for (size_t i = 0; i < children.first.size(); i++) {
            auto &child = children.first[i];
            if (*child == *ptr) {
                children.first[i] = ptr;
            }
        }
    }
    for (auto &[thm_str, parent]: parent_for_theorem) {
        if (parent.first && *parent.first == *ptr) {
            parent_for_theorem.insert_or_assign(parent.first, ptr, parent.second);
        }
    }
    for (auto &[thm_str, seen_set]: seen) {
        if (seen_set.first.contains(ptr)) {
            seen_set.first.insert(ptr);
        }
    }
    if (*root == *ptr) {
        root = ptr;
    }
}

size_t Simulation::get_hash(const TheoremPointer &thm, const size_t &previous) const {
    return theorems.combined_hash(thm->unique_string, previous);
}

std::pair<TheoremPointer, size_t> Simulation::parent_hash(const size_t &hash_) const {
    if (!parent_for_theorem.contains(hash_)) {
        throw std::runtime_error("Parent not found");
    }
    auto parent_pair = parent_for_theorem.at(hash_);
    size_t parent_hash = parent_pair.second;
    assert (parent_hash == theorems.at(hash_).second);
    return {parent_pair.first, parent_hash};
}

std::pair<TheoremPointer, size_t> Simulation::parent_hash(const TheoremPointer &thm, const size_t &previous) const {
    return parent_hash(get_hash(thm, previous));
}

std::size_t Simulation::previous_hash(const size_t &hash_) const {
    if (!theorems.contains(hash_)) {
        throw std::runtime_error("Previous hash not found");
    }
    return theorems.at(hash_).second;
}


void HTPSNode::reset_HTPS_stats() {
    if (error) {
        assert (tactics.empty());
        return;
    }
    // implies we will simply set logW to the first value we receive
    reset_mask = std::vector<bool>(tactics.size(), true);
    counts = std::vector<size_t>(tactics.size(), 0);
    virtual_counts = std::vector<size_t>(tactics.size(), 0);
}

bool HTPSNode::should_send(size_t count_threshold) const {
    if (error)
        return false;
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
    return HTPSSampleCritic(thm, std::exp(get_value()), is_solved(), is_bad(), log_critic_value, visit_sum);
}


void HTPSNode::get_tactics_sample_q_conditioning(size_t count_threshold,
                                                 std::vector<std::shared_ptr<tactic>> &valid_tactics,
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
    valid_targets.reserve(selected_tactics_ids.size());
    q_values.reserve(selected_tactics_ids.size());
    for (const auto &id: selected_tactics_ids) {
        valid_tactics.push_back(tactics[id]);
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
    assert (q_values.size() == valid_targets.size());
}

void HTPSNode::get_tactics_sample_regular(Metric metric, NodeMask node_mask,
                                          bool only_learn_best_tactics, double p_threshold,
                                          std::vector<std::shared_ptr<tactic>> &valid_tactics,
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
    valid_targets.reserve(selected_tactic_ids.size());

    for (const auto &id: selected_tactic_ids) {
        valid_tactics.push_back(tactics[id]);
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
    assert(selected_tactic_ids.size() == valid_tactics.size());
}

std::optional<HTPSSampleTactics> HTPSNode::get_tactics_sample(Metric metric, NodeMask node_mask,
                                                              bool only_learn_best_tactics, double p_threshold,
                                                              size_t count_threshold, bool for_q_conditioning) const {
    if (!should_send(count_threshold)) {
        return std::nullopt;
    }

    switch (node_mask) {
        case None:
            break;
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
    std::vector<double> valid_targets;
    std::vector<double> q_values;

    if (for_q_conditioning)
        get_tactics_sample_q_conditioning(count_threshold, valid_tactics,valid_targets, q_values);
    else
        get_tactics_sample_regular(metric, node_mask, only_learn_best_tactics, p_threshold, valid_tactics, valid_targets);
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
            assert(!reset_mask[i] || counts[i] == 0);
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
        if (killed(i) || (expandable_only && !tactic_expandable[i])) {
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
    if (children_for_tactic.size() != tactics.size())
        return false;
    if (children_for_tactic.size() != priors.size())
        return false;
    if (error) {
        if (log_critic_value > MIN_FLOAT)
            return false;
        if (is_solved_leaf || solved)
            return false;
        if (!tactics.empty())
            return false;
        return true;
    }
    if (tactics.empty())
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

HTPSNode HTPSNode::from_json(const nlohmann::json &j) {
    HTPSNode node;
    node.thm =j["theorem"];
    std::vector<std::shared_ptr<tactic>> tactics;
    for (const auto &tac: j["tactics"]) {
        tactics.push_back(tac);
    }
    node.tactics = tactics;
    std::vector<std::vector<TheoremPointer>> children_for_tactic;
    for (const auto &children: j["children_for_tactic"]) {
        std::vector<TheoremPointer> children_for_tactic_inner;
        for (const auto &child: children) {
            children_for_tactic_inner.push_back(child);
        }
        children_for_tactic.push_back(children_for_tactic_inner);
    }
    node.children_for_tactic = children_for_tactic;
    node.killed_tactics = j["killed_tactics"].get<std::unordered_set<size_t>>();
    node.solving_tactics = j["solving_tactics"].get<std::unordered_set<size_t>>();
    node.tactic_expandable = j["tactic_expandable"].get<std::vector<bool>>();
    node.minimum_proof_size = MinimumLengthMap::from_json(j["minimum_proof_size"]);
    node.minimum_tactics = MinimumTacticMap::from_json(j["minimum_tactics"]);
    node.minimum_tactic_length = MinimumTacticLengthMap::from_json(j["minimum_tactic_length"]);
    node.in_minimum_proof = MinimumBoolMap::from_json(j["in_minimum_proof"]);
    node.solved = j["solved"];
    node.is_solved_leaf = j["is_solved_leaf"];
    node.in_proof = j["in_proof"];
    node.old_critic_value = j["old_critic_value"];
    if (!j["log_critic_value"].is_null()) {
        node.log_critic_value = j["log_critic_value"];
    } else {
        node.log_critic_value = MIN_FLOAT;
    }
    node.priors = static_cast<std::vector<double>>(j["priors"]);
    node.q_value_solved = j["q_value_solved"];
    node.policy = j["policy"];
    node.exploration = j["exploration"];
    node.tactic_init_value = j["tactic_init_value"];
    node.log_w = static_cast<std::vector<double>>(j["log_w"]);
    node.counts = static_cast<std::vector<size_t>>(j["counts"]);
    node.virtual_counts = static_cast<std::vector<size_t>>(j["virtual_counts"]);
    node.reset_mask = static_cast<std::vector<bool>>(j["reset_mask"]);
    return node;
}

HTPSNode::operator nlohmann::json() const {
    nlohmann::json j;
    j["theorem"] = *thm;
    j["tactics"] = nlohmann::json(tactics);
    j["children_for_tactic"] = nlohmann::json(children_for_tactic);
    j["killed_tactics"] = killed_tactics;
    j["solving_tactics"] = solving_tactics;
    j["tactic_expandable"] = tactic_expandable;
    j["minimum_proof_size"] = nlohmann::json(minimum_proof_size);
    j["minimum_tactics"] = nlohmann::json(minimum_tactics);
    j["minimum_tactic_length"] = nlohmann::json(minimum_tactic_length);
    j["in_minimum_proof"] = nlohmann::json(in_minimum_proof);
    j["solved"] = solved;
    j["is_solved_leaf"] = is_solved_leaf;
    j["in_proof"] = in_proof;
    j["old_critic_value"] = old_critic_value;
    j["log_critic_value"] = log_critic_value;
    j["priors"] = priors;
    j["q_value_solved"] = q_value_solved;
    j["policy"] = nlohmann::json(*policy);
    j["exploration"] = exploration;
    j["tactic_init_value"] = tactic_init_value;
    j["log_w"] = log_w;
    j["counts"] = counts;
    j["virtual_counts"] = virtual_counts;
    j["reset_mask"] = reset_mask;
    return j;
}

std::size_t std::hash<HTPSNode>::operator()(const HTPSNode &n) const {
    return std::hash<theorem>()(*n.get_theorem());
}

bool HTPS::is_leaf(const std::shared_ptr<htps::HTPSNode> &node) const {
    return node->is_solved() && (!is_proven()) && params.early_stopping_solved_if_root_not_proven;
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

bool HTPS::is_done() const {
    return done;
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

Simulation HTPS::find_leaves_to_expand(std::vector<TheoremPointer> &terminal,
                                       std::vector<std::pair<TheoremPointer, size_t>> &to_expand) {
    Simulation sim = Simulation(root);
    std::deque<std::pair<TheoremPointer, size_t>> to_process;
    std::vector<double> node_policy;
    to_process.emplace_back(root, 0);

    while (!to_process.empty()) {
#ifdef VERBOSE_PRINTS
        printf("To process\n");
#endif
        node_policy.clear();
        auto current_elem = to_process.front();
        TheoremPointer current = current_elem.first;
        size_t previous = current_elem.second;
        size_t hash_ = sim.get_hash(current, previous);
        to_process.pop_front();
        if (!nodes.contains(current)) {
            // TODO: in theory there is some depth stuff here?
            to_expand.push_back(current_elem);
#ifdef VERBOSE_PRINTS
            printf("Adding to to_expand...\n");
            printf("Erasing theorem set\n");
#endif
            sim.erase_theorem_set(current, previous);
            continue;
        }
        auto HTPS_node = nodes.at(current);
        bool is_leaf_node = is_leaf(HTPS_node);
        if (HTPS_node->is_terminal() || is_leaf_node) {
#ifdef VERBOSE_PRINTS
            if (HTPS_node->is_terminal())
                printf("Terminal node\n");
            else
                printf("Leaf node\n");
            printf("Tactics size: %zu\n", HTPS_node->n_tactics());
#endif
            assert(!HTPS_node->all_tactics_killed());
            assert(HTPS_node->is_solved_leaf_node() || is_leaf_node);
            sim.set_value(current, HTPS_node->get_value(), previous);
            sim.set_solved(current, true, previous);
            terminal.push_back(current);
#ifdef VERBOSE_PRINTS
            printf("Erasing theorem set\n");
#endif
            sim.erase_theorem_set(current, previous);
            continue;
        }
        if (params.early_stopping && HTPS_node->is_solved()) {
            sim.set_value(current, 0.0, previous);
            sim.set_solved(current, true, previous);
            terminal.push_back(current);
#ifdef VERBOSE_PRINTS
            printf("Erasing theorem set\n");
#endif
            sim.erase_theorem_set(current, previous);
            continue;
        }
        // Select subsequent tactic
        HTPS_node->compute_policy(node_policy, true);
        size_t tactic_id;
#ifdef VERBOSE_PRINTS
        printf("Temperature %lf\n", params.policy_temperature);
#endif
        if (params.policy_temperature == 0) {
            tactic_id = std::distance(node_policy.begin(), std::max_element(node_policy.begin(), node_policy.end()));
        } else {
            // Normal softmax with temperature, i.e. exp(p / temperature)
            // But take logarithm of policy first, as done in evariste
            for (size_t i = 0; i < node_policy.size(); i++) {
                if (node_policy[i] > MIN_FLOAT)
                    node_policy[i] = std::log(node_policy[i]);
                else
                    node_policy[i] = MIN_FLOAT;
            }
            double p_sum = 0;
            for (auto &p: node_policy) {
                p = std::exp(p / params.policy_temperature);
                p_sum += p;
            }
            for (auto &p: node_policy) {
                p = p / p_sum;
            }
#ifdef VERBOSE_PRINTS
            printf("Policy: ");
            for (auto &p: node_policy) {
                printf("%lf ", p);
            }
            printf("\n");
#endif
            std::discrete_distribution<size_t> dist(node_policy.begin(), node_policy.end());
            tactic_id = dist(gen);
        }
        assert(!HTPS_node->killed(tactic_id));
        auto tactic_ptr = HTPS_node->get_tactic(tactic_id);
#ifdef VERBOSE_PRINTS
        printf("Setting tactic %zu\n", tactic_id);
#endif
        sim.set_tactic(current, tactic_ptr, previous);
        sim.set_tactic_id(current, tactic_id, previous);
        auto children = HTPS_node->get_children_for_tactic(tactic_id);
        TheoremSet &seen = sim.get_theorem_set(current, previous);
        // If any child has been seen, we have a circle, i.e. kill the tactic
        if (std::any_of(children.begin(), children.end(), [seen](const auto &thm) { return seen.contains(thm); })) {
            kill_tactic(HTPS_node, tactic_id);
            cleanup(sim);
            find_unexplored_and_propagate_expandable();
            throw FailedTacticException();
        }
        HTPS_node->add_virtual_count(tactic_id, params.virtual_loss);
        sim.set_virtual_count_added(current, true, previous);
        for (const auto &child: children) {
            size_t parent_hash = sim.get_hash(current, previous);
            sim.add_theorem(child, current, parent_hash, sim.get_depth(current, previous) + 1);
            to_process.emplace_front(child, parent_hash);
        }
        // Free up memory
        sim.erase_theorem_set(current, previous);
    }

    std::vector<TheoremPointer> all_leaves = terminal;
    for (const auto &thm: to_expand) {
        all_leaves.push_back(thm.first);
    }
    assert(!all_leaves.empty());
    assert(std::all_of(to_expand.begin(), to_expand.end(),
                       [this](const auto &thm) { return !this->nodes.contains(thm.first); }));
    assert(sim.leave_count() == all_leaves.size());
    return sim;
}


void HTPS::receive_expansion(TheoremPointer &thm, double value, bool solved) {
    // has to be log value
    assert(value <= 0);
    if (!simulations_for_theorem.contains(thm))
        throw std::runtime_error("No simulation for theorem");
    // Compare via reference, not pointer
    std::unordered_set<std::shared_ptr<Simulation>> sims{};
    for (const auto &[simulation, previous]: simulations_for_theorem.at(thm)) {
        simulation->receive_expansion(thm, value, solved, previous);
        sims.insert(simulation);
    }
    for (const auto &simulation: sims) {
        simulation->decrement_expansions();
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
                    params.q_value_solved, params.tactic_init_value, expansion->effects, true);
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
        receive_expansion(expansion->thm, expansion->log_critic, false);
        nodes.push_back(current);
    }
    add_nodes(nodes);
    expansion_count += nodes.size();
}

void HTPS::cleanup(Simulation &to_clean) {
    std::shared_ptr<HTPSNode> current;
    for (const auto &[hash_, thm]: to_clean) {
        if (!nodes.contains(thm.first))
            continue;
        current = nodes.at(thm.first);
        if (to_clean.get_virtual_count_added(hash_)) {
            current->subtract_virtual_count(to_clean.get_tactic_id(hash_), params.virtual_loss);
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
    std::queue<std::pair<TheoremPointer, size_t>> to_process;

    std::unordered_map<size_t, size_t> children_propagated;
    for (const auto &[leaf, previous_hash_]: leaves) {
        size_t hash_ = sim->get_hash(leaf, previous_hash_);
        std::shared_ptr<HTPSNode> current = nodes.at(leaf);
        if (sim->get_virtual_count_added(hash_)) {
            current->subtract_virtual_count(sim->get_tactic_id(hash_), params.virtual_loss);
        }
        assert(sim->get_value(hash_) <= 0); // log
        auto [parent, parent_hash] = sim->parent_hash(hash_);
        if (!parent) {
            assert(leaf == root);
            updated_root = true;
            continue;
        }
        if (!children_propagated.contains(parent_hash)) {
            children_propagated.insert_or_assign(parent_hash, 1);
        } else {
            children_propagated.at(parent_hash) += 1;
            assert(children_propagated.at(parent_hash) <= sim->get_children(parent_hash).size());
        }
        if (children_propagated.at(parent_hash) == sim->get_children(parent_hash).size()) {
            to_process.emplace(parent, parent_hash);
        }
    }
    while (!to_process.empty()) {
        auto cur = to_process.front();
        to_process.pop();
        std::vector<double> child_values = sim->child_values(cur.second);
        assert(std::all_of(child_values.begin(), child_values.end(), [](const auto &v) { return v <= 0; }));
        double sum_log = std::accumulate(child_values.begin(), child_values.end(), 0.0);
        std::shared_ptr<HTPSNode> current_node = nodes.at(cur.first);
        if (current_node->is_solved() && params.backup_one_for_solved) {
            sum_log = 0.0;
        }
        if (params.depth_penalty < 1) {
            sum_log += std::log(params.depth_penalty);
        }
        assert(sum_log <= 0);
#ifdef VERBOSE_PRINTS
        if (sum_log <= MIN_FLOAT) {
            printf("Sum log is min float\n");
        }
#endif
        auto [parent, parent_hash] = sim->parent_hash(cur.second);
        sim->set_value(cur.first, sum_log, parent_hash);
        if (sim->get_virtual_count_added(cur.second)) {
            current_node->subtract_virtual_count(sim->get_tactic_id(cur.second), params.virtual_loss);
        }
        if (!only_value) {
            current_node->update(sim->get_tactic_id(cur.second), sum_log);
        }
        if (!parent) {
            updated_root = true;
            continue;
        }
        if (!children_propagated.contains(parent_hash)) {
            children_propagated.insert_or_assign(parent_hash, 0);
        }
        children_propagated.at(parent_hash) += 1;
        if (children_propagated.at(parent_hash) == sim->get_children(parent_hash).size()) {
            to_process.push({parent, parent_hash});
        }
    }
    assert(updated_root);
}

void HTPS::batch_to_expand(std::vector<TheoremPointer> &theorems) {
    propagate_needed = false;
    TheoremMap<TheoremPointer> result;
    theorems.clear();
    std::vector<TheoremPointer> single_to_expand;

    for (size_t i = 0; i < params.succ_expansions; i++) {
        single_to_expand.clear();
        std::vector<TheoremPointer> terminal;
        std::vector<std::pair<TheoremPointer, size_t>> to_expand;
        Simulation sim;
        while (!dead_root()) {
            try {
#ifdef VERBOSE_PRINTS
                printf("Finding leaves to expand\n");
#endif
                sim = find_leaves_to_expand(terminal, to_expand);
                break;
            } catch (FailedTacticException &e) {
                terminal.clear();
                to_expand.clear();
                continue;
            }
        }
        if (dead_root())
            break;

        if (to_expand.empty()) {
#ifdef VERBOSE_PRINTS
            printf("To expand is empty!");
#endif
            assert (!propagate_needed);
            propagate_needed = true;
            find_unexplored_and_propagate_expandable();
            cleanup(sim);
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

void HTPS::_single_to_expand(std::vector<TheoremPointer> &theorems, Simulation &sim,
                             std::vector<std::pair<TheoremPointer, std::size_t>> &leaves_to_expand) {
    theorems.clear();
    TheoremSet seen;
    std::shared_ptr<Simulation> sim_ptr = std::make_shared<Simulation>(sim);
    sim_ptr->reset_expansions();
#ifdef VERBOSE_PRINTS
    printf("Adding simulation!");
#endif
    simulations.push_back(sim_ptr);
    for (const auto &[leaf, hash_]: leaves_to_expand) {
        if (!seen.contains(leaf)) {
            seen.insert(leaf);
            if (simulations_for_theorem.contains(leaf))
                simulations_for_theorem.at(leaf).emplace_back(sim_ptr, hash_);
            else
                simulations_for_theorem.insert(leaf, std::vector<std::pair<std::shared_ptr<Simulation>, size_t>>{{sim_ptr, hash_}});
            sim_ptr->increment_expansions();
        }
        if (!currently_expanding.contains(leaf)) {
            currently_expanding.insert(leaf);
            theorems.push_back(leaf);
        }
    }
}

std::vector<TheoremPointer> HTPS::batch_to_expand() {
    std::vector<TheoremPointer> theorems;
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

void HTPS::theorems_to_expand(std::vector<TheoremPointer> &theorems) {
    if (!currently_expanding.empty()) {
        throw std::runtime_error("Currently expanding is not empty, give results first!");
    }
    return batch_to_expand(theorems);
}

std::vector<TheoremPointer> HTPS::theorems_to_expand() {
    std::vector<TheoremPointer> theorems;
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
    std::optional<struct proof> p;
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

void HTPS::set_root(TheoremPointer &thm) {
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

HTPS HTPS::from_json(const nlohmann::json &j) {
    HTPS htps;
    htps.root = j["root"];
    TheoremMap<std::shared_ptr<HTPSNode>> nodes;
    for (const auto &node: j["nodes"]) {
        auto n = HTPSNode::from_json(node);
        nodes.insert(n.get_theorem(), std::make_shared<HTPSNode>(n));
    }
    htps.nodes = nodes;
    htps.ancestors = AncestorsMap::from_json(j["ancestors"]);
    htps.permanent_ancestors = AncestorsMap::from_json(j["permanent_ancestors"]);
    htps.unexplored_theorems = TheoremSet::from_json(j["unexplored_theorems"]);
    htps.minimum_proof_size = MinimumLengthMap::from_json(j["minimum_proof_size"]);
    htps.initial_minimum_proof_size = MinimumLengthMap::from_json(j["initial_minimum_proof_size"]);
    htps.policy = j["policy"];
    htps.params = htps_params::from_json(j["params"]);
    htps.expansion_count = j["expansion_count"];
    htps.simulations = std::vector<std::shared_ptr<Simulation>>();
    for (const auto &sim: j["simulations"]) {
        htps.simulations.push_back(sim);
    }
    for (auto &sim: htps.simulations) {
        sim->deduplicate(htps.root);
    }
    TheoremMap<std::vector<std::pair<std::shared_ptr<Simulation>, size_t>>> simulations_for_theorem;
    if (j["simulations_for_theorem"].is_null())
        simulations_for_theorem = TheoremMap<std::vector<std::pair<std::shared_ptr<Simulation>, size_t>>>();
    else {
        // Can only map a single simulation to simulations for theorem once
        std::vector<bool> sims_used(htps.simulations.size(), false);
        for (const auto &[thm_str, sim]: j["simulations_for_theorem"].items()) {
            std::vector<std::pair<std::shared_ptr<Simulation>, size_t>> sims;
            for (const auto &s: sim) {
                // Find the simulation in simulations, use that one
                std::shared_ptr<Simulation> current_sim = s[0];
                size_t hash_ = s[1];
                for (size_t i = 0; i < htps.simulations.size(); i++) {
                    if (!sims_used[i] && *htps.simulations[i] == *current_sim) {
                        current_sim = htps.simulations[i];
                        sims_used[i] = true;
                        break;
                    }
                }
                sims.emplace_back(current_sim, hash_);
            }
            simulations_for_theorem.insert(static_cast<std::string>(thm_str), sims);
        }
    }
    htps.simulations_for_theorem = simulations_for_theorem;
    htps.backedup_hashes = j["backedup_hashes"].get<std::unordered_set<size_t>>();
    htps.currently_expanding = TheoremSet::from_json(j["currently_expanding"]);
    htps.propagate_needed = j["propagate_needed"];
    htps.done = j["done"];
    htps::seed = j["seed"];
    htps::gen = setup_gen();
    return htps;
}

HTPS::operator nlohmann::json() const {
    nlohmann::json j;
    j["root"] = *root;
    std::vector<nlohmann::json> nodes_explicit;
    for (const auto &[thm, node]: nodes) {
        nodes_explicit.push_back(nlohmann::json(*node));
    }
    j["nodes"] = nodes_explicit;
    nlohmann::json ancestors_json;
    for (const auto &[thm, ancestor]: ancestors) {
        ancestors_json[thm] = ancestor.operator nlohmann::json();
    }
    j["ancestors"] = ancestors_json;
    ancestors_json = {};
    for (const auto &[thm, ancestor]: permanent_ancestors) {
        ancestors_json[thm] = ancestor.operator nlohmann::json();
    }
    j["permanent_ancestors"] = ancestors_json;
    j["unexplored_theorems"] = nlohmann::json(unexplored_theorems);
    j["minimum_proof_size"] = nlohmann::json(minimum_proof_size);
    j["initial_minimum_proof_size"] = nlohmann::json(initial_minimum_proof_size);
    j["policy"] = nlohmann::json(*policy);
    j["params"] = nlohmann::json(params);
    j["expansion_count"] = expansion_count;
    std::vector<nlohmann::json> simulations_explicit;
    for (const auto &sim: simulations) {
        simulations_explicit.push_back((*sim).operator nlohmann::json());
    }
    j["simulations"] = simulations_explicit;
    nlohmann::json simulations_for_theorem_json;
    for (const auto &[thm, sims]: simulations_for_theorem) {
        std::vector<nlohmann::json> sims_explicit;
        for (const auto &sim: sims) {
            nlohmann::json inner = nlohmann::json::array();
            inner.push_back(sim.first->operator nlohmann::json());
            inner.push_back(sim.second);
        }
        simulations_for_theorem_json[thm] = sims_explicit;
    }
    j["simulations_for_theorem"] = simulations_for_theorem_json;
    j["backedup_hashes"] = backedup_hashes;
    j["currently_expanding"] = nlohmann::json(currently_expanding);
    j["propagate_needed"] = propagate_needed;
    j["done"] = done;
    j["seed"] = htps::seed;
    return j;
}

htps::htps_params HTPS::get_params() const {
    return params;
}


std::optional<proof> HTPSResult::get_proof() const {
    return p;
}

TheoremPointer HTPSResult::get_goal() const {
    return goal;
}

std::tuple<std::vector<HTPSSampleCritic>, std::vector<HTPSSampleTactics>, std::vector<HTPSSampleEffect>, Metric, std::vector<HTPSSampleTactics>>
HTPSResult::get_samples() const {
    return {samples_critic, samples_tactic, samples_effect, metric, proof_samples_tactics};
}

std::vector<HTPSSampleTactics> HTPSResult::get_proof_samples() const {
    return proof_samples_tactics;
}

std::vector<HTPSSampleCritic> HTPSResult::get_critic_samples() const {
    return samples_critic;
}

std::vector<HTPSSampleTactics> HTPSResult::get_tactic_samples() const {
    return samples_tactic;
}

std::vector<HTPSSampleEffect> HTPSResult::get_effect_samples() const {
    return samples_effect;
}

Metric HTPSResult::get_metric() const {
    return metric;
}

htps_params::operator nlohmann::json() const {
    nlohmann::json j;
    j["num_expansions"] = num_expansions;
    j["succ_expansions"] = succ_expansions;
    j["early_stopping"] = early_stopping;
    j["no_critic"] = no_critic;
    j["early_stopping_solved_if_root_not_proven"] = early_stopping_solved_if_root_not_proven;
    j["backup_once"] = backup_once;
    j["backup_one_for_solved"] = backup_one_for_solved;
    j["exploration"] = exploration;
    j["virtual_loss"] = virtual_loss;
    j["policy_type"] = policy_type;
    j["policy_temperature"] = policy_temperature;
    j["q_value_solved"] = q_value_solved;
    j["tactic_init_value"] = tactic_init_value;
    j["effect_subsampling_rate"] = effect_subsampling_rate;
    j["critic_subsampling_rate"] = critic_subsampling_rate;
    j["tactic_p_threshold"] = tactic_p_threshold;
    j["count_threshold"] = count_threshold;
    j["node_mask"] = node_mask;
    j["only_learn_best_tactics"] = only_learn_best_tactics;
    j["tactic_sample_q_conditioning"] = tactic_sample_q_conditioning;
    j["depth_penalty"] = depth_penalty;
    j["metric"] = metric;
    return j;
}

htps_params htps_params::from_json(const nlohmann::json &j) {
    htps_params params;
    params.exploration = j["exploration"];
    params.num_expansions = j["num_expansions"];
    params.succ_expansions = j["succ_expansions"];
    params.early_stopping = j["early_stopping"];
    params.no_critic = j["no_critic"];
    params.early_stopping_solved_if_root_not_proven = j["early_stopping_solved_if_root_not_proven"];
    params.backup_once = j["backup_once"];
    params.backup_one_for_solved = j["backup_one_for_solved"];
    params.virtual_loss = j["virtual_loss"];
    params.policy_type = j["policy_type"];
    params.policy_temperature = j["policy_temperature"];
    params.q_value_solved = j["q_value_solved"];
    params.tactic_init_value = j["tactic_init_value"];
    params.effect_subsampling_rate = j["effect_subsampling_rate"];
    params.critic_subsampling_rate = j["critic_subsampling_rate"];
    params.tactic_p_threshold = j["tactic_p_threshold"];
    params.count_threshold = j["count_threshold"];
    params.node_mask = j["node_mask"];
    params.only_learn_best_tactics = j["only_learn_best_tactics"];
    params.tactic_sample_q_conditioning = j["tactic_sample_q_conditioning"];
    params.depth_penalty = j["depth_penalty"];
    params.metric = j["metric"];
    return params;
}