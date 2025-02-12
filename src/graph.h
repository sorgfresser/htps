//
// Created by simon on 11.02.25.
//

#ifndef HTPS_GRAPHCORRECT_H
#define HTPS_GRAPHCORRECT_H

#include "base.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>
#include <array>
#include <iostream>
#include <vector>
#include <climits>
#include <optional>
#include <deque>
#include <cassert>
#include <queue>

namespace htps {
    enum Metric {
        DEPTH, SIZE, TIME, METRIC_COUNT
    };

    class MinimumLengthMap {
    private:
        std::array<size_t, METRIC_COUNT> minimum_length{};

    public:
        // Set value for a specific metric
        void set(Metric metric, size_t value) {
            minimum_length[metric] = value;
        }

        // Get value for a specific metric
        size_t get(Metric metric) const {
            return minimum_length[metric];
        }

        // Print all values
        void print() const {
            std::cout << "DEPTH: " << minimum_length[DEPTH] << '\n';
            std::cout << "SIZE: " << minimum_length[SIZE] << '\n';
            std::cout << "TIME: " << minimum_length[TIME] << '\n';
        }

        MinimumLengthMap() : minimum_length{INT_MAX, INT_MAX, INT_MAX} {}
    };

    class MinimumTacticMap {
    private:
        std::array<std::vector<size_t>, METRIC_COUNT> minimum_tactics;

    public:
        // Add a tactic for a specific metric
        void add_tactic(Metric metric, size_t tactic_id) {
            minimum_tactics[metric].push_back(tactic_id);
        }

        // Get tactics for a specific metric
        const std::vector<size_t> &get_tactics(Metric metric) const {
            return minimum_tactics[metric];
        }

        // Print all tactics
        void print() const {
            std::cout << "DEPTH tactics: ";
            for (int t: minimum_tactics[DEPTH]) std::cout << t << ' ';
            std::cout << '\n';

            std::cout << "SIZE tactics: ";
            for (int t: minimum_tactics[SIZE]) std::cout << t << ' ';
            std::cout << '\n';

            std::cout << "TIME tactics: ";
            for (int t: minimum_tactics[TIME]) std::cout << t << ' ';
            std::cout << '\n';
        }

        MinimumTacticMap() : minimum_tactics{std::vector<size_t>(), std::vector<size_t>(), std::vector<size_t>()} {}
    };

    class MinimumTacticLengthMap {
    private:
        std::array<std::vector<size_t>, METRIC_COUNT> minimum_tactic_length;


    public:
        void set_tactic(Metric metric, size_t value, size_t tactic_id) {
            if (tactic_id >= minimum_tactic_length[metric].size()) {
                minimum_tactic_length[metric].resize(tactic_id + 1, INT_MAX);
            }
            minimum_tactic_length[metric][tactic_id] = value;
        }

        // Get value for a specific metric
        size_t get(Metric metric, size_t tactic_id) const {
            if (tactic_id >= minimum_tactic_length[metric].size()) {
                return INT_MAX;
            }
            return minimum_tactic_length[metric][tactic_id];
        }

        bool has_value(Metric metric, size_t tactic_id) const {
            return tactic_id < minimum_tactic_length[metric].size() &&
                   minimum_tactic_length[metric][tactic_id] != INT_MAX;
        }

        // Print all values
        void print() const {
            std::cout << "DEPTH: ";
            for (size_t t: minimum_tactic_length[DEPTH]) std::cout << t << ' ';
            std::cout << '\n';

            std::cout << "SIZE: ";
            for (size_t t: minimum_tactic_length[SIZE]) std::cout << t << ' ';
            std::cout << '\n';

            std::cout << "TIME: ";
            for (size_t t: minimum_tactic_length[TIME]) std::cout << t << ' ';
            std::cout << '\n';
        }

    };

    class MinimumBoolMap {
    private:
        std::array<bool, METRIC_COUNT> minimum_bool{};
    public:
        void set(Metric metric, bool value) {
            minimum_bool[metric] = value;
        }

        bool get(Metric metric) const {
            return minimum_bool[metric];
        }

        void print() const {
            std::cout << "DEPTH: " << minimum_bool[DEPTH] << '\n';
            std::cout << "SIZE: " << minimum_bool[SIZE] << '\n';
            std::cout << "TIME: " << minimum_bool[TIME] << '\n';
        }

        MinimumBoolMap() : minimum_bool{false, false, false} {}
    };

    class Node {


    protected:
        theorem thm;
        std::vector<tactic> tactics;
        std::vector<bool> tactic_expandable;
        std::vector<std::vector<theorem>> children_for_tactic;
        std::unordered_set<size_t> killed_tactics;
        std::unordered_set<size_t> solving_tactics;
        MinimumLengthMap minimum_proof_size;
        MinimumTacticMap minimum_tactics;
        MinimumTacticLengthMap minimum_tactic_length;
        MinimumBoolMap in_minimum_proof;
        bool solved;
        bool is_solved_leaf;
        bool in_proof;

    public:
        size_t n_tactics() const {
            return tactics.size();
        }

        size_t n_solving_tactics() const {
            return solving_tactics.size();
        }

        bool is_solved() const {
            return solved;
        }

        Node(theorem thm, std::vector<tactic> tactics, std::vector<std::vector<theorem>> children_for_tactic) :
                thm(thm),
                tactics(tactics),
                tactic_expandable(tactics.size(), true),
                children_for_tactic(children_for_tactic),
                killed_tactics(),
                solving_tactics(),
                minimum_proof_size(),
                minimum_tactics(),
                minimum_tactic_length(),
                in_minimum_proof(),
                solved(false),
                is_solved_leaf(false),
                in_proof(false) {
            std::vector<bool> is_solved(tactics.size(), false);
            for (size_t i = 0; i < tactics.size(); i++) {
                if (children_for_tactic[i].empty() && tactics[i].is_valid) {
                    is_solved[i] = true;
                }
            }
            const bool all_solved = std::all_of(is_solved.begin(), is_solved.end(), [](bool b) { return b; });
            const bool none_solved = std::none_of(is_solved.begin(), is_solved.end(), [](bool b) { return b; });

            if (!(all_solved || none_solved || this->n_tactics() == 0)) {
                throw std::invalid_argument("Invalid tactics");
            }

            if (n_tactics() > 0 && all_solved) {
                solved = true;
                is_solved_leaf = true;
                for (size_t i = 0; i < tactics.size(); i++) {
                    solving_tactics.insert(i);
                }
            }
            // Kill fake tactics
            for (size_t i = 0; i < tactics.size(); i++) {
                if (!tactics[i].is_valid) {
                    kill_tactic(i);
                }
            }
        }

        bool all_tactics_killed() const {
            return killed_tactics.size() == tactics.size();
        }

        bool kill_tactic(size_t i) {
            if (i >= tactics.size()) {
                return false;
            }
            if (killed_tactics.find(i) != killed_tactics.end()) {
                assert(!tactic_expandable[i]);
                return false;
            }
            killed_tactics.insert(i);
            return all_tactics_killed();
        }

        bool killed(size_t tactic_id) const {
            return killed_tactics.find(tactic_id) != killed_tactics.end();
        }

        void reset_minimum_proof_stats() {
            minimum_proof_size = MinimumLengthMap();
            minimum_tactics = MinimumTacticMap();
            minimum_tactic_length = MinimumTacticLengthMap();
            in_minimum_proof = MinimumBoolMap();
            in_proof = false;
        }

        bool is_terminal() const {
            return is_solved_leaf || children_for_tactic.empty() || all_tactics_killed();
        }

        bool is_bad() const {
            return is_terminal() && !is_solved_leaf;
        }

        bool is_solved_leaf_node() const {
            return is_solved_leaf;
        }

        theorem get_theorem() const {
            return thm;
        }

        std::vector<theorem> get_children_for_tactic(size_t i) const {
            if (i >= children_for_tactic.size()) {
                throw std::invalid_argument("Invalid tactic");
            }
            return children_for_tactic[i];
        }

        std::vector<std::vector<theorem>> get_children_for_tactic() const {
            return children_for_tactic;
        }

        void set_expandable(bool expandable) {
            for (size_t i = 0; i < tactics.size(); i++) {
                tactic_expandable[i] = expandable;
            }
        }

        void set_expandable(size_t i, bool expandable) {
            tactic_expandable[i] = expandable;
        }

        bool expandable(size_t i) const {
            return tactic_expandable[i];
        }

        bool expandable() const {
            return std::any_of(tactic_expandable.begin(), tactic_expandable.end(), [](bool b) { return b; });
        }

        bool is_valid(size_t i) const {
            return tactics[i].is_valid;
        }

        // Returns true if this was the first tactic to solve the theorem
        bool solved_by(size_t i) {
            solving_tactics.insert(i);
            bool old_solved = solved;
            solved = true;
            return !old_solved;
        }

        void set_in_proof() {
            in_proof = true;
        }

        bool is_in_proof() {
            return in_proof;
        }

        auto solving_range() const {
            return std::ranges::subrange(solving_tactics.begin(), solving_tactics.end());
        }

        auto minimum_tactics_range(Metric metric) const {
            return std::ranges::subrange(minimum_tactics.get_tactics(metric).begin(),
                                         minimum_tactics.get_tactics(metric).end());
        }

        size_t minimum_tactic(Metric metric) const {
            return minimum_tactics.get_tactics(metric).front();
        }

        bool has_minimum_tactic(Metric metric, size_t tactic_id) {
            return std::find(minimum_tactics.get_tactics(metric).begin(), minimum_tactics.get_tactics(metric).end(),
                             tactic_id) != minimum_tactics.get_tactics(metric).end();
        }

        size_t minimum_length(Metric metric) const {
            return minimum_proof_size.get(metric);
        }

        tactic get_tactic(size_t tactic_id) const {
            return tactics[tactic_id];
        }

        void set_minimum_length(Metric metric, size_t value) {
            minimum_proof_size.set(metric, value);
        }

        bool has_minimum_length(Metric metric) const {
            return minimum_proof_size.get(metric) != INT_MAX;
        }

        void set_minimum_tactic(Metric metric, size_t tactic_id) {
            minimum_tactics.add_tactic(metric, tactic_id);
        }

        bool has_minimum_tactic_length(Metric metric, size_t tactic_id) const {
            return minimum_tactic_length.has_value(metric, tactic_id);
        }

        size_t get_minimum_tactic_length(Metric metric, size_t tactic_id) const {
            return minimum_tactic_length.get(metric, tactic_id);
        }

        void set_minimum_tactic_length(Metric metric, size_t value, size_t tactic_id) {
            minimum_tactic_length.set_tactic(metric, value, tactic_id);
        }

        void set_in_minimum_proof(Metric metric, bool value) {
            in_minimum_proof.set(metric, value);
        }

        bool get_in_minimum_proof(Metric metric) {
            return in_minimum_proof.get(metric);
        }

    };

    /* A node, tactic pair with a priority, the priority is given based on one of our Metrics
     * The tactic solves the given node, and the priority is used to describe the relevance of the solution
     * */
    class PrioritizedNode {
    public:
        Node *node;
        size_t priority;
        size_t tactic_id;

        PrioritizedNode(Node *node, size_t priority, size_t tactic_id) : node(node), priority(priority),
                                                                         tactic_id(tactic_id) {}

        bool operator<(const PrioritizedNode &other) const {
            return priority < other.priority;
        }
    };

    class AncestorsMap {

    protected:
        using AncestorSet = std::unordered_set<std::pair<std::optional<struct theorem>, size_t>>;
        std::unordered_map<theorem, AncestorSet> ancestors;
    public:
        // Add an ancestor relationship
        void add_ancestor(const theorem &theorem, const std::optional<struct theorem> &parent, size_t tactic_id) {
            ancestors[theorem].insert(std::make_tuple(parent, tactic_id));
        }

        // Get ancestors for a theorem
        const AncestorSet &get_ancestors(const theorem &theorem) const {
            static const AncestorSet empty_set;
            auto it = ancestors.find(theorem);
            return it != ancestors.end() ? it->second : empty_set;
        }

        // Print ancestors
        void print() const {
            for (const auto &[theorem, ancestor_set]: ancestors) {
                std::cout << "Theorem string: " << theorem.unique_string << '\n';
                for (const auto &[parent, tactic_id]: ancestor_set) {
                    if (parent) {
                        std::cout << "(Parent: " << parent->unique_string << ", Tactic ID: " << tactic_id << ") ";
                    } else {
                        std::cout << "(Parent: None, Tactic ID: " << tactic_id << ") ";
                    }
                }
                std::cout << '\n';
            }
        }

        auto begin() noexcept {
            return ancestors.begin();
        }

        auto end() noexcept {
            return ancestors.end();
        }

        bool contains(theorem thm) {
            return ancestors.contains(thm);
        }

        bool contains(theorem thm, std::optional<theorem> parent, size_t tactic_id) {
            if (!ancestors.contains(thm)) {
                return false;
            }
            return ancestors.at(thm).contains({parent, tactic_id});
        }

        bool size() {
            return ancestors.size();
        }

        bool size(theorem thm) {
            return ancestors.at(thm).size();
        }

        bool erase(theorem thm) {
            return ancestors.erase(thm) > 0;
        }

        bool erase(theorem thm, std::optional<theorem> parent, size_t tactic_id) {
            if (!ancestors.contains(thm)) {
                return false;
            }
            return ancestors.at(thm).erase({parent, tactic_id}) > 0;
        }
    };

    // Nodes of type T, prioritized nodes of type PT
    template<typename T, typename PT>
    class Graph {
    protected:
        theorem root;
        std::unordered_map<theorem, T> nodes;
        AncestorsMap ancestors;
        AncestorsMap permanent_ancestors;
        std::unordered_set<theorem> unexplored_theorems;
        MinimumLengthMap minimum_proof_size;
        MinimumLengthMap initial_minimum_proof_size;

    public:
        Graph(theorem &root) : root(root), nodes(), ancestors(), permanent_ancestors(), unexplored_theorems(),
                               minimum_proof_size(), initial_minimum_proof_size() {
            ancestors.add_ancestor(root, std::nullopt, 0);
            permanent_ancestors.add_ancestor(root, std::nullopt, 0);
            unexplored_theorems.insert(root);
        }

        void reset_minimum_proof_stats() {
            minimum_proof_size = initial_minimum_proof_size;
            for (auto &[_, node]: nodes) {
                node.reset_minimum_proof_stats();
            }
        }

        bool is_proven() const {
            return nodes.contains(root) && nodes.at(root).is_solved();
        }

        bool dead_root() const {
            return nodes.contains(root) && nodes.at(root).all_tactics_killed();
        }

        void add_nodes(std::vector<T> &node_list) {
            std::vector<T> to_check_solved;
            std::vector<T> newly_solved;
            for (const auto &node: node_list) {
                theorem th = node.get_theorem();
                if (!ancestors.contains(th) || permanent_ancestors.contains(th)) {
                    throw std::invalid_argument("Invalid node");
                }
                if (nodes.contains(th)) {
                    throw std::invalid_argument("Node already exists");
                }
                nodes[th] = node;
                if (node.is_bad()) {
                    for (const auto &[parent_th, tactic_id]: ancestors.get_ancestors(th)) {
                        if (parent_th) {
                            nodes.at(parent_th.value()).kill_tactic(tactic_id);
                        }
                    }
                    continue;
                }
                if (node.is_solved()) {
                    newly_solved.push_back(node);
                    continue;
                }

                std::unordered_set<size_t> bad_tactic_ids;
                for (const auto &[tactic_id, children]: node.get_children_for_tactic()) {
                    for (const auto &child: children) {
                        permanent_ancestors.add_ancestor(child, th, tactic_id);
                        if (nodes.contains(child) && nodes.at(child).is_bad()) {
                            bad_tactic_ids.insert(tactic_id);
                        }
                    }
                }
                for (size_t bad_tactic_id: bad_tactic_ids) {
                    node.kill_tactic(bad_tactic_id);
                }
            }
            propagate_check_and_solved(newly_solved, to_check_solved);
        }

        void kill_tactic(T node, size_t tactic_id) {
            std::deque<std::pair<T, size_t>> to_kill;
            theorem thm;
            to_kill.push_back({node, tactic_id});
            while (!to_kill.empty()) {
                auto [current, tid] = to_kill.front();
                to_kill.pop_front();
                if (current.killed(tid)) {
                    continue;
                }
                thm = current.get_theorem();
                for (const auto child: current.get_children_for_tactic(tid)) {
                    if (ancestors.contains(child, thm, tid)) {
                        ancestors.erase(child, thm, tid);
                        if (!ancestors.size(child)) {
                            // We can rebuild unexplored_theorems from scratch, so we can erase the theorem
                            if (!nodes.contains(child) && unexplored_theorems.contains(child)) {
                                unexplored_theorems.erase(child);
                            }
                        }
                    }
                }
                // If killing the tactics leads to all tactics killed, we need to kill all tactics leading to this node
                // Since this node has become bad.
                if (current.kill_tactic(tid)) {
                    for (const auto &[parent, parent_tid]: ancestors.get_ancestors(thm)) {
                        if (parent) {
                            to_kill.push_front({nodes.at(parent.value()), parent_tid});
                        }
                    }
                }
            }
        }

        // Rebuild the unexplored_theorems set
        void find_unexplored() {
            unexplored_theorems.clear();
            if (!nodes.contains(root)) {
                unexplored_theorems.insert(root);
                return;
            }
            std::deque<T> to_explore;
            to_explore.push_back(nodes.at(root));
            std::unordered_set<T> seen;
            while (!to_explore.empty()) {
                T current = to_explore.front();
                to_explore.pop_front();
                if (seen.contains(current)) {
                    continue;
                }
                seen.insert(current);
                if (current.is_solved()) {
                    continue;
                }
                std::vector<std::vector<theorem>> &children = current.get_children_for_tactic();
                for (size_t i = 0; i < children.size(); i++) {
                    if (current.killed(i)) {
                        continue;
                    }
                    for (const auto &child: children[i]) {
                        if (nodes.contains(child))
                            to_explore.push_front(nodes.at(child));
                        else
                            unexplored_theorems.insert(child);
                    }
                }
            }
        }

        /* If a theorem is unexplored, we mark the tactics leading from the root to this theorem as expandable.
         * This function propagates the expandable flag to all ancestors of the theorem until we reach the root.
         * It is a bit more complicated than usual since a theorem can have multiple parents.
         */
        void propagate_expandable() {
            for (const auto &[thm, node]: nodes) {
                node.set_expandable(false);
            }
            std::vector<theorem> to_propagate;
            for (const auto &thm: unexplored_theorems) {
                for (const auto &[parent, tactic_id]: ancestors.get_ancestors(thm)) {
                    if (!parent) {
                        continue;
                    }
                    T &node = nodes.at(parent.value());
                    if (node.killed(tactic_id)) {
                        continue;
                    }
                    node.set_expandable(tactic_id, true);
                    to_propagate.push_back(parent.value());
                }
            }
            std::deque<theorem> propagate_queue(to_propagate.begin(), to_propagate.end());
            std::unordered_set<theorem> seen;
            while (!propagate_queue.empty()) {
                theorem current = propagate_queue.front();
                propagate_queue.pop_front();
                if (seen.contains(current)) {
                    continue;
                }
                seen.insert(current);
                for (const auto &[parent, tactic_id]: ancestors.get_ancestors(current)) {
                    if (!parent) {
                        continue;
                    }
                    T &node = nodes.at(parent.value());
                    if (node.killed(tactic_id)) {
                        continue;
                    }
                    node.set_expandable(tactic_id, true);
                    propagate_queue.push_front(parent.value());
                }
            }
        }

        // Simple assertion to make sure there are no expandable and killed tactics
        void expandable_check() {
            for (const auto &[thm, node]: nodes) {
                for (size_t i = 0; i < node.n_tactics(); i++) {
                    assert(!(node.killed(i) && node.tactic_expandable[i]));
                }
            }
        }

        // Find unexplored theorems and propagate expandable tactics, and some sanity checks
        void find_unexplored_and_propagate_expandable() {
            find_unexplored();
            propagate_expandable();
            // If just starting
            if (unexplored_theorems.size() == 1 and unexplored_theorems.contains(root)) {
                expandable_check();
                return;
            }
            if (unexplored_theorems.empty()) {
                expandable_check();
                return;
            }
            if (nodes[root].expandable()) {
                expandable_check();
                return;
            }
            throw std::runtime_error("Good propagate failure!");
        }


        /* Given a set of newly solved nodes, propagate the solved status to all ancestors
         * This entails setting ancestors to solved if there is a tactic that solves all children
         */
        void propagate_check_and_solved(const std::vector<T> &newly_solved, std::vector<T> &to_check_solved) {
            std::deque<std::pair<T, size_t >> to_check;
            std::deque<T> newly_solved_deque(newly_solved.begin(), newly_solved.end());
            for (T &node: to_check_solved) {
                for (size_t i = 0; i < node.n_tactics(); i++) {
                    to_check.push_back({node, i});
                }
            }
            while (!to_check.empty()) {
                while (!newly_solved_deque.empty()) {
                    T &node = newly_solved_deque.front();
                    newly_solved_deque.pop_front();
                    assert(node.is_solved());
                    for (const auto &[parent, tactic_id]: permanent_ancestors.get_ancestors(node.get_theorem())) {
                        if (!parent) {
                            continue;
                        }
                        to_check.push_back({nodes.at(parent.value()), tactic_id});
                    }
                }
                auto [current, tid] = to_check.front();
                to_check.pop_front();
                if (!current.is_valid(tid)) {
                    continue;
                }
                auto children = current.get_children_for_tactic(tid);
                if (std::all_of(children.begin(), children.end(), [this](const theorem &thm) {
                    return nodes.contains(thm) && nodes.at(thm).is_solved();
                })) {
                    if (current.solved_by(tid))
                        newly_solved_deque.push_back(current);
                }
            }
        }

        // A sanity check to assert that each node is solved if there is any tactic that solves all children
        void check_solved_consistency() const {
            bool solved_requires_tactic = std::all_of(nodes.begin(), nodes.end(), [this](const auto &pair) {
                const auto &node = pair.second;
                return node.is_solved() == (node.n_solving_tactics > 0);
            });
            if (!solved_requires_tactic) {
                throw std::runtime_error(
                        "Solved consistency check failed, at least one node is solved without a solving tactic or vice versa");
            }
            for (const auto &[thm, node]: nodes) {
                for (const auto &[tactic_id, children]: node.get_children_for_tactic()) {
                    for (const auto &child: children) {
                        if (node.killed(tactic_id))
                            continue;
                        if (!ancestors.get_ancestors(child).contains({thm, tactic_id})) {
                            throw std::runtime_error("Ancestor consistency check failed, ancestor not found");
                        }
                    }
                }
            }
            for (const auto &[_, node]: nodes) {
                bool should_be_solved = std::any_of(node.get_children_for_tactic().begin(),
                                                    node.get_children_for_tactic().end(),
                                                    [this, node](const auto &pair) {
                                                        const auto &[tactic_id, children] = pair;
                                                        return std::all_of(children.begin(), children.end(),
                                                                           [this](const theorem &thm) {
                                                                               return nodes.contains(thm) &&
                                                                                      nodes.at(thm).is_solved();
                                                                           }) && node.is_valid(tactic_id);
                                                    });
                if (should_be_solved != node.is_solved()) {
                    throw std::runtime_error(
                            "Solved consistency check failed, node should be solved but is not or vice versa");
                }
            }
        }

        void build_in_proof() {
            if (!nodes.contains(root)) {
                return;
            }
            if (!nodes.at(root).is_solved()) {
                return;
            }
            std::unordered_set<theorem> seen;
            std::deque<theorem> to_visit;
            while (!to_visit.empty()) {
                theorem current = to_visit.front();
                to_visit.pop_front();
                if (!nodes.contains(current)) {
                    continue;
                }
                if (seen.contains(current)) {
                    continue;
                }
                seen.insert(current);
                T &node = nodes.at(current);
                assert (node.n_solving_tactics() > 0);
                node.set_in_proof();
                for (const auto &tactic_id: node.solving_range())
                    for (const auto &child: node.get_children_for_tactic(tactic_id))
                        to_visit.push_back(child);
            }
        }

        // Get all proofs for a theorem
        std::vector<struct proof> all_proofs(theorem thm) {
            T node = nodes.at(thm);
            if (!node.is_solved()) {
                throw std::invalid_argument("Theorem not solved");
            }

            std::vector<struct proof> proofs;
            std::vector<struct proof> child_proofs;
            for (const auto &tactic_id: node.solving_range()) {
                child_proofs.clear();
                for (const auto &child: node.get_children_for_tactic(tactic_id)) {
                    child_proofs = all_proofs(child);
                }
                proofs.push_back({thm, node.get_tactic(tactic_id), child_proofs});
            }
        }

        struct proof minimal_proof(Metric metric, theorem &thm) {
            if (!nodes.contains(thm)) {
                throw std::invalid_argument("Theorem not found");
            }
            T &node = nodes.at(thm);
            if (!node.is_solved()) {
                throw std::invalid_argument("Theorem not solved");
            }
            if (!node.is_in_proof()) {
                throw std::invalid_argument("Theorem not in proof");
            }
            size_t min_tac = node.minimum_tactic(metric);
            std::vector<proof> proofs;
            for (const auto &child: node.get_children_for_tactic(min_tac)) {
                proofs.push_back(minimal_proof(metric, child));
            }
            return {thm, node.tactics[min_tac], proofs};
        }

        /* Get minimal proof sizes for all nodes in the graph
         */
        void get_node_proof_sizes_and_depths() {
            std::vector<PT> to_process;
            for (auto const metric: {DEPTH, SIZE, TIME}) {
                to_process.clear();
                // For each solved leaf, create priority nodes for each tactic that solves the leaf
                for (const auto &[thm, node]: nodes) {
                    if (!node.is_solved_leaf()) {
                        continue;
                    }
                    for (const auto &tactic_id: node.solving_range()) {
                        size_t priority = 1;
                        if (metric == TIME)
                            priority = node.get_tactic(tactic_id).duration;
                        to_process.push_back({&node, priority, tactic_id});
                    }
                }

                std::priority_queue<PT> pq(to_process.begin(), to_process.end());
                // Set minimum length for each node that has a minimum length
                while (!pq.empty()) {
                    PT pnode = pq.top();
                    pq.pop();
                    auto [node, priority, tactic_id] = pnode;
                    if (!node.has_minimum_tactic_length(metric, tactic_id)) {
                        node.set_minimum_tactic_length(metric, priority, tactic_id);
                        if (priority <= node.minimum_length(metric)) {
                            assert(!node.has_minimum_tactic(metric, tactic_id));
                            node.set_minimum_tactic(metric, tactic_id);
                        }
                    }
                    if (node.has_minimum_length(metric)) {
                        assert(node.minimum_length(metric) <= priority);
                        continue;
                    }
                    node.set_minimum_length(metric, priority);
                    // Also for each parent, if all children have a minimum (i.e. are all solved)
                    // Then propagate the minimum length to the parent
                    for (const auto &[parent, parent_tactic]: permanent_ancestors.get_ancestors(node.get_theorem())) {
                        if (!parent) {
                            continue;
                        }
                        T &parent_node = nodes.at(parent.value());
                        size_t added_priority = 1;
                        if (metric == TIME)
                            added_priority = parent_node.get_tactic(parent_tactic).duration;
                        size_t new_priority = added_priority;
                        switch (metric) {
                            case DEPTH:
                                new_priority += depth_for_children(parent_node.get_children_for_tactic(parent_tactic));
                                break;
                            case SIZE:
                                new_priority += size_for_children(parent_node.get_children_for_tactic(parent_tactic));
                                break;
                            case TIME:
                                new_priority += time_for_children(parent_node.get_children_for_tactic(parent_tactic));
                                break;
                            default:
                                throw std::invalid_argument("Invalid metric");
                        }
                        if (new_priority < INT_MAX) {
                            pq.push({&parent_node, new_priority, parent_tactic});
                        }
                    }
                }
            }
            for (const auto metric: {DEPTH, SIZE, TIME}) {
                if (!is_proven())
                    return;

                assert(nodes.contains(root));
                assert(nodes.at(root).minimum_length(metric) < INT_MAX);
                minimum_proof_size.set(metric, nodes.at(root).minimum_length(metric));
                std::deque<theorem> to_visit;
                to_visit.push_back(root);
                std::unordered_set<theorem> seen;
                while (!to_visit.empty()) {
                    theorem current = to_visit.front();
                    to_visit.pop_front();
                    if (seen.contains(current)) {
                        continue;
                    }
                    seen.insert(current);
                    T &node = nodes.at(current);
                    node.set_in_minimum_proof(metric, true);
                    assert(node.is_in_proof());
                    assert(!node.minimum_tactics_range(metric).empty());
                    for (const auto &tactic_id: node.minimum_tactics_range(metric)) {
                        for (const auto &child: node.get_children_for_tactic(tactic_id)) {
                            to_visit.push_back(child);
                        }
                    }
                }
            }

        }


    protected:
        size_t depth_for_children(const std::vector<theorem> &children) const {
            size_t base = 0;
            for (const auto &child: children) {
                if (!nodes.contains(child)) {
                    return INT_MAX;
                }
                const T &node = nodes.at(child);
                base = max(base, node.minimum_length(DEPTH));
            }
            return base;
        }

        size_t size_for_children(const std::vector<theorem> &children) const {
            size_t base = 0;
            for (const auto &child: children) {
                if (!nodes.contains(child)) {
                    return INT_MAX;
                }
                const T &node = nodes.at(child);
                base += node.minimum_length(SIZE);
            }
            return base;
        }

        size_t time_for_children(const std::vector<theorem> &children) const {
            size_t base = 0;
            for (const auto &child: children) {
                if (!nodes.contains(child)) {
                    return INT_MAX;
                }
                const T &node = nodes.at(child);
                base += node.minimum_length(TIME);
            }
            return base;
        }
    };

}

#endif //HTPS_GRAPHCORRECT_H