//
// Created by simon on 11.02.25.
//

#ifndef HTPS_GRAPHCORRECT_H
#define HTPS_GRAPHCORRECT_H

#include "graph.h"
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

namespace htps {
    enum Metric {
        DEPTH, SIZE, TIME, METRIC_COUNT
    };

    class MinimumLengthMap {
    private:
        std::array<int, METRIC_COUNT> minimum_length{};

    public:
        // Set value for a specific metric
        void set(Metric metric, int value) {
            minimum_length[metric] = value;
        }

        // Get value for a specific metric
        int get(Metric metric) const {
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
        std::array<std::vector<int>, METRIC_COUNT> minimum_tactics;

    public:
        // Add a tactic for a specific metric
        void add_tactic(Metric metric, int tactic) {
            minimum_tactics[metric].push_back(tactic);
        }

        // Get tactics for a specific metric
        const std::vector<int> &getTactics(Metric metric) const {
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

        MinimumTacticMap() : minimum_tactics{std::vector<int>(), std::vector<int>(), std::vector<int>()} {}
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
        bool solved;
        bool is_solved_leaf;
        bool in_proof;

    public:
        size_t n_tactics() const {
            return tactics.size();
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
            in_proof = false;
        }

        bool is_terminal() const {
            return is_solved_leaf || children_for_tactic.empty() || all_tactics_killed();
        }

        bool is_bad() const {
            return is_terminal() && !is_solved_leaf;
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

    };

    class PrioritizedNode {
    public:
        Node *node;
        size_t priority;

        PrioritizedNode(Node *node, size_t priority) : node(node), priority(priority) {}

        bool operator<(const PrioritizedNode &other) const {
            return priority < other.priority;
        }
    };

    class AncestorsMap {

    protected:
        using AncestorSet = std::unordered_set<std::pair<std::optional<struct theorem>, int>>;
        std::unordered_map<theorem, AncestorSet> ancestors;
    public:
        // Add an ancestor relationship
        void add_ancestor(const theorem &theorem, const std::optional<struct theorem> &parent, int tactic_id) {
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

    template<typename T>
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


        void propagate_check_and_solved(std::vector<T> &newly_solved, std::vector<T> &to_check_solved) {
            // TODO
        }
    };
}

#endif //HTPS_GRAPHCORRECT_H
