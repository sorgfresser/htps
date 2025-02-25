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
#include <limits>
#include <optional>
#include <deque>
#include <cassert>
#include <queue>
#include <memory>

namespace htps {
    constexpr size_t MAXIMUM_PROOF_LENGTH =
            std::numeric_limits<size_t>::max() / 2; // Is still large enough, but prevents overflow

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

        bool has_value(Metric metric) const {
            return minimum_length[metric] != MAXIMUM_PROOF_LENGTH;
        }

        bool has_value() const {
            return std::any_of(minimum_length.begin(), minimum_length.end(),
                               [](size_t v) { return v != MAXIMUM_PROOF_LENGTH; });
        }

        // Print all values
        void print() const {
            std::cout << "DEPTH: " << minimum_length[DEPTH] << '\n';
            std::cout << "SIZE: " << minimum_length[SIZE] << '\n';
            std::cout << "TIME: " << minimum_length[TIME] << '\n';
        }

        MinimumLengthMap() : minimum_length{MAXIMUM_PROOF_LENGTH, MAXIMUM_PROOF_LENGTH, MAXIMUM_PROOF_LENGTH} {}
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
            for (size_t t: minimum_tactics[DEPTH]) std::cout << t << ' ';
            std::cout << '\n';

            std::cout << "SIZE tactics: ";
            for (size_t t: minimum_tactics[SIZE]) std::cout << t << ' ';
            std::cout << '\n';

            std::cout << "TIME tactics: ";
            for (size_t t: minimum_tactics[TIME]) std::cout << t << ' ';
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
                minimum_tactic_length[metric].resize(tactic_id + 1, MAXIMUM_PROOF_LENGTH);
            }
            minimum_tactic_length[metric][tactic_id] = value;
        }

        // Get value for a specific metric
        size_t get(Metric metric, size_t tactic_id) const {
            if (tactic_id >= minimum_tactic_length[metric].size()) {
                return MAXIMUM_PROOF_LENGTH;
            }
            return minimum_tactic_length[metric][tactic_id];
        }

        bool has_value(Metric metric, size_t tactic_id) const {
            return tactic_id < minimum_tactic_length[metric].size() &&
                   minimum_tactic_length[metric][tactic_id] != MAXIMUM_PROOF_LENGTH;
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
        std::shared_ptr<theorem> thm;
        std::vector<std::shared_ptr<tactic>> tactics;
        std::vector<bool> tactic_expandable;
        std::vector<std::vector<std::shared_ptr<theorem>>> children_for_tactic;
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

        /* Returns whether two nodes are equal or not.
         * Since the Graphs only consist of one Node per theorem, we can simply compare the theorems.
         */
        bool operator==(const Node &n) const {
            return *thm == *n.thm;
        }

        Node(const std::shared_ptr<theorem> &thm, const std::vector<std::shared_ptr<tactic>> &tactics,
             const std::vector<std::vector<std::shared_ptr<theorem>>> &children_for_tactic) :
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
                if (children_for_tactic[i].empty() && tactics[i]->is_valid) {
                    is_solved[i] = true;
                }
            }
            const bool all_solved = std::all_of(is_solved.begin(), is_solved.end(), [](bool b) { return b; });
            const bool none_solved = std::none_of(is_solved.begin(), is_solved.end(), [](bool b) { return b; });

            if (!(all_solved || none_solved || n_tactics() == 0)) {
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
                if (!tactics[i]->is_valid) {
                    kill_tactic(i);
                }
            }
        }

        Node() = default;

        bool all_tactics_killed() const {
            return killed_tactics.size() == tactics.size();
        }

        virtual bool kill_tactic(size_t i) {
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

        std::shared_ptr<theorem> get_theorem() const {
            return thm;
        }

        std::vector<std::shared_ptr<theorem>> get_children_for_tactic(size_t i) const {
            if (i >= children_for_tactic.size()) {
                throw std::invalid_argument("Invalid tactic");
            }
            return children_for_tactic[i];
        }

        std::vector<std::shared_ptr<theorem>> &get_children_for_tactic(size_t i) {
            if (i >= children_for_tactic.size()) {
                throw std::invalid_argument("Invalid tactic");
            }
            return children_for_tactic[i];
        }

        std::vector<std::vector<std::shared_ptr<theorem>>> get_children_for_tactic() const {
            return children_for_tactic;
        }


        std::vector<std::vector<std::shared_ptr<theorem>>> &get_children_for_tactic() {
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
            return tactics[i]->is_valid;
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

        bool is_in_proof() const {
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

        std::shared_ptr<tactic> get_tactic(size_t tactic_id) const {
            return tactics[tactic_id];
        }

        void set_minimum_length(Metric metric, size_t value) {
            minimum_proof_size.set(metric, value);
        }

        bool has_minimum_length(Metric metric) const {
            return minimum_proof_size.get(metric) != MAXIMUM_PROOF_LENGTH;
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

        bool is_in_minimum_proof(Metric metric) const {
            return in_minimum_proof.get(metric);
        }

    };
}

template<>
struct std::hash<htps::Node> {
    std::size_t operator()(const htps::Node &n) const {
        return std::hash<std::shared_ptr<htps::theorem>>{}(n.get_theorem());
    }
};

namespace htps {
    /* A node, tactic pair with a priority, the priority is given based on one of our Metrics
     * The tactic solves the given node, and the priority is used to describe the relevance of the solution
     * */
    class PrioritizedNode {
    public:
        std::shared_ptr<Node> node;
        size_t priority;
        size_t tactic_id;

        PrioritizedNode(Node *node, size_t priority, size_t tactic_id) : node(node), priority(priority),
                                                                         tactic_id(tactic_id) {}

        bool operator<(const PrioritizedNode &other) const {
            return priority < other.priority;
        }
    };


    class TheoremSet {
    private:
        std::unordered_set<std::string> _set;
    public:
        auto begin() const noexcept {
            return _set.begin();
        }

        auto begin() noexcept {
            return _set.begin();
        }

        auto end() const noexcept {
            return _set.end();
        }

        auto end() noexcept {
            return _set.end();
        }

        bool contains(const std::string &s) const {
            return _set.contains(s);
        }

        bool contains(const theorem &thm) const {
            return contains(thm.unique_string);
        }

        bool contains(const std::shared_ptr<theorem> &thm) const {
            return contains(*thm);
        }

        void insert(const std::string &s) {
            _set.insert(s);
        }

        void insert(const theorem &thm) {
            insert(thm.unique_string);
        }

        void insert(const std::shared_ptr<theorem> &thm) {
            insert(*thm);
        }

        bool erase(const std::string &s) {
            return _set.erase(s) > 0;
        }

        bool erase(const theorem &thm) {
            return erase(thm.unique_string);
        }

        bool erase(const std::shared_ptr<theorem> &thm) {
            return erase(*thm);
        }

        size_t size() const {
            return _set.size();
        }

        auto find(const std::string &s) const {
            return _set.find(s);
        }

        auto find(const theorem &thm) const {
            return find(thm.unique_string);
        }

        auto find(const std::shared_ptr<theorem> &thm) const {
            return find(*thm);
        }

        void clear() {
            _set.clear();
        }

        auto empty() const {
            return _set.empty();
        }
    };

    struct PairHash {
        template<typename T>
        std::size_t operator()(const std::pair<std::string, T> &p) const {
            std::size_t h1 = std::hash<std::string>{}(p.first);
            std::size_t h2 = std::hash<T>{}(p.second);
            // Combine the two hash values.
            return h1 ^ (h2 << 1);  // simple combination; you can choose a different method if needed.
        }
    };

    template
            <typename T>
    class TheoremPairSet {
    private:
        std::unordered_set<std::pair<std::string, T>, PairHash> _set;
    public:
        auto begin() const noexcept {
            return _set.begin();
        }

        auto begin() noexcept {
            return _set.begin();
        }

        auto end() const noexcept {
            return _set.end();
        }

        auto end() noexcept {
            return _set.end();
        }

        bool contains(const std::string &s, const T &t) const {
            return _set.contains(std::make_pair(s, t));
        }

        bool contains(const theorem &thm, const T &t) const {
            return contains(thm.unique_string, t);
        }

        bool contains(const std::shared_ptr<theorem> &thm, const T &t) const {
            if (!thm)
                return contains("", t);
            return contains(*thm, t);
        }

        void insert(const std::string &s, const T &t) {
            _set.insert(std::make_pair(s, t));
        }

        void insert(const theorem &thm, const T &t) {
            insert(thm.unique_string, t);
        }

        void insert(const std::shared_ptr<theorem> &thm, const T &t) {
            if (!thm)
                insert("", t);
            insert(*thm, t);
        }

        bool erase(const std::string &s, const T &t) {
            return _set.erase(std::make_pair(s, t)) > 0;
        }

        bool erase(const theorem &thm, const T &t) {
            return erase(thm.unique_string, t);
        }

        bool erase(const std::shared_ptr<theorem> &thm, const T &t) {
            if (!thm)
                return erase("", t);
            return erase(*thm, t);
        }

        size_t size() const {
            return _set.size();
        }

        void clear() {
            _set.clear();
        }

        auto empty() const {
            return _set.empty();
        }
    };

    // Map a theorem to type T
    template<typename T>
    class TheoremMap {
    protected:
        std::unordered_map<std::string, T> _map;
    public:
        auto begin() const noexcept {
            return _map.begin();
        }

        auto begin() noexcept {
            return _map.begin();
        }

        auto end() const noexcept {
            return _map.end();
        }

        auto end() noexcept {
            return _map.end();
        }

        bool contains(const std::string &s) const {
            return _map.contains(s);
        }

        bool contains(const theorem &thm) const {
            return contains(thm.unique_string);
        }

        bool contains(const std::shared_ptr<theorem> &thm) const {
            return contains(*thm);
        }

        T &at(const std::string &s) {
            return _map.at(s);
        }

        T &at(const theorem &thm) {
            return at(thm.unique_string);
        }

        T &at(const std::shared_ptr<theorem> &thm) {
            return at(*thm);
        }

        T at(const std::string &s) const {
            return _map.at(s);
        }

        T at(const theorem &thm) const {
            return at(thm.unique_string);
        }

        T at(const std::shared_ptr<theorem> &thm) const {
            return at(*thm);
        }

        auto insert(const std::string &s, const T &t) {
            return _map.insert({s, t});
        }

        auto insert(const theorem &thm, const T &t) {
            return insert(thm.unique_string, t);
        }

        auto insert(const std::shared_ptr<theorem> &thm, const T &t) {
            return insert(*thm, t);
        }

        auto insert_or_assign(const std::string &s, const T &t) {
            return _map.insert_or_assign(s, t);
        }

        auto insert_or_assign(const theorem &thm, const T &t) {
            return insert_or_assign(thm.unique_string, t);
        }

        auto insert_or_assign(const std::shared_ptr<theorem> &thm, const T &t) {
            return insert_or_assign(*thm, t);
        }

        bool erase(const std::string &s) {
            return _map.erase(s) > 0;
        }

        bool erase(const theorem &thm) {
            return erase(thm.unique_string);
        }

        bool erase(const std::shared_ptr<theorem> &thm) {
            return erase(*thm);
        }

        size_t size() const {
            return _map.size();
        }

        auto find(const std::string &s) const {
            return _map.find(s);
        }

        auto find(const theorem &thm) const {
            return find(thm.unique_string);
        }

        auto find(const std::shared_ptr<theorem> &thm) const {
            return find(*thm);
        }

        void set(const std::string &s, const T &t) {
            _map[s] = t;
        }

        void set(const theorem &thm, const T &t) {
            set(thm.unique_string, t);
        }

        void set(const std::shared_ptr<theorem> &thm, const T &t) {
            set(*thm, t);
        }

        bool empty() const noexcept {
            return _map.empty();
        }
    };


    using AncestorSet = TheoremPairSet<size_t>;

    class AncestorsMap : public TheoremMap<AncestorSet> {
    public:
        using TheoremMap<AncestorSet>::contains;
        using TheoremMap<AncestorSet>::at;
        using TheoremMap<AncestorSet>::insert;
        using TheoremMap<AncestorSet>::erase;
        using TheoremMap<AncestorSet>::find;
        using TheoremMap<AncestorSet>::size;

        // Add an ancestor relationship
        void
        add_ancestor(const std::shared_ptr<theorem> &thm, const std::shared_ptr<theorem> &parent, size_t tactic_id) {
            at(thm).insert(parent, tactic_id);
        }

        // Get ancestors for a theorem
        const AncestorSet &get_ancestors(const std::string &thm) const {
            static const AncestorSet empty_set;
            auto it = find(thm);
            return it != end() ? it->second : empty_set;
        }

        const AncestorSet &get_ancestors(const theorem &thm) const {
            return get_ancestors(thm.unique_string);
        }

        const AncestorSet &get_ancestors(const std::shared_ptr<theorem> &thm) const {
            return get_ancestors(*thm);
        }


        // Print ancestors
        void print() const {
            for (const auto &[thm, ancestor_set]: _map) {
                std::cout << "Theorem string: " << thm << '\n';
                for (const auto &[parent, tactic_id]: ancestor_set) {
                    std::cout << "(Parent: " << parent << ", Tactic ID: " << tactic_id << ") ";
                }
                std::cout << '\n';
            }
        }

        bool contains(const std::string &thm, const std::shared_ptr<theorem> &parent, size_t tactic_id) {
            if (!contains(thm)) {
                return false;
            }
            return get_ancestors(thm).contains(parent, tactic_id);
        }

        bool contains(const theorem &thm, const std::shared_ptr<theorem> &parent, size_t tactic_id) {
            return contains(thm.unique_string, parent, tactic_id);
        }

        bool contains(const std::shared_ptr<theorem> &thm, const std::shared_ptr<theorem> &parent, size_t tactic_id) {
            return contains(*thm, parent, tactic_id);
        }

        size_t size(const std::string &thm) const {
            return get_ancestors(thm).size();
        }

        size_t size(const theorem &thm) const {
            return size(thm.unique_string);
        }

        size_t size(const std::shared_ptr<theorem> &thm) const {
            return size(*thm);
        }

        bool erase(const theorem &thm, const std::shared_ptr<theorem> &parent, size_t tactic_id) {
            if (!contains(thm)) {
                return false;
            }
            return at(thm).erase(parent, tactic_id);
        }

        bool erase(const std::shared_ptr<theorem> &thm, const std::shared_ptr<theorem> &parent, size_t tactic_id) {
            return erase(*thm, parent, tactic_id);
        }
    };

    // Nodes of type T, prioritized nodes of type PT
    template<typename T, typename PT>
    class Graph {
    protected:
        std::shared_ptr<theorem> root;
        TheoremMap<T> nodes;
        AncestorsMap ancestors;
        AncestorsMap permanent_ancestors;
        TheoremSet unexplored_theorems;
        MinimumLengthMap minimum_proof_size;
        MinimumLengthMap initial_minimum_proof_size;

    public:
        explicit Graph(std::shared_ptr<theorem> &root) : root(root), nodes(), ancestors(), permanent_ancestors(),
                                                         unexplored_theorems(),
                                                         minimum_proof_size(), initial_minimum_proof_size() {
            ancestors.add_ancestor(root, nullptr, 0);
            permanent_ancestors.add_ancestor(root, nullptr, 0);
            unexplored_theorems.insert(*root);
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

        virtual bool dead_root() const {
            return nodes.contains(root) && nodes.at(root).all_tactics_killed();
        }

        void add_nodes(std::vector<T> &node_list) {
            std::vector<T> to_check_solved;
            std::vector<T> newly_solved;
            for (auto &node: node_list) {
                std::shared_ptr<theorem> th = node.get_theorem();
                if (!ancestors.contains(th) || permanent_ancestors.contains(th)) {
                    throw std::invalid_argument("Invalid node");
                }
                if (nodes.contains(th)) {
                    throw std::invalid_argument("Node already exists");
                }
                nodes.set(th, node);
                if (node.is_bad()) {
                    for (const auto &[parent_th, tactic_id]: ancestors.get_ancestors(th)) {
                        if (!parent_th.empty()) {
                            nodes.at(parent_th).kill_tactic(tactic_id);
                        }
                    }
                    continue;
                }
                if (node.is_solved()) {
                    newly_solved.push_back(node);
                    continue;
                }

                std::unordered_set<size_t> bad_tactic_ids;
                auto children_for_tactic = node.get_children_for_tactic();
                for (size_t i = 0; i < children_for_tactic.size(); i++) {
                    auto children = children_for_tactic[i];
                    for (const auto &child: children) {
                        permanent_ancestors.add_ancestor(child, th, i);
                        if (nodes.contains(child) && nodes.at(child).is_bad()) {
                            bad_tactic_ids.insert(i);
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
            std::shared_ptr<theorem> thm;
            to_kill.push_back({node, tactic_id});
            while (!to_kill.empty()) {
                auto [current, tid] = to_kill.front();
                to_kill.pop_front();
                if (current.killed(tid)) {
                    continue;
                }
                thm = current.get_theorem();
                for (const auto &child: current.get_children_for_tactic(tid)) {
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
                        if (!parent.empty()) {
                            to_kill.push_front({nodes.at(parent), parent_tid});
                        }
                    }
                }
            }
        }

        // Rebuild the unexplored_theorems set
        void find_unexplored(bool ignore_solved) {
            unexplored_theorems.clear();
            if (!nodes.contains(root)) {
                unexplored_theorems.insert(*root);
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
                if (current.is_solved() && ignore_solved) {
                    continue;
                }
                std::vector<std::vector<std::shared_ptr<theorem>>> children = current.get_children_for_tactic();
                for (size_t i = 0; i < children.size(); i++) {
                    if (current.killed(i)) {
                        continue;
                    }
                    for (const auto &child: children[i]) {
                        if (nodes.contains(child))
                            to_explore.push_front(nodes.at(child));
                        else
                            unexplored_theorems.insert(*child);
                    }
                }
            }
        }

        /* If a theorem is unexplored, we mark the tactics leading from the root to this theorem as expandable.
         * This function propagates the expandable flag to all ancestors of the theorem until we reach the root.
         * It is a bit more complicated than usual since a theorem can have multiple parents.
         */
        void propagate_expandable() {
            for (auto &[thm, node]: nodes) {
                node.set_expandable(false);
            }
            std::vector<std::string> to_propagate;
            for (const auto &thm: unexplored_theorems) {
                for (const auto &[parent, tactic_id]: ancestors.get_ancestors(thm)) {
                    if (parent.empty()) {
                        continue;
                    }
                    T &node = nodes.at(parent);
                    if (node.killed(tactic_id)) {
                        continue;
                    }
                    node.set_expandable(tactic_id, true);
                    to_propagate.push_back(parent);
                }
            }
            std::deque<std::string> propagate_queue(to_propagate.begin(), to_propagate.end());
            TheoremSet seen;
            while (!propagate_queue.empty()) {
                std::string current = propagate_queue.front();
                propagate_queue.pop_front();
                if (seen.contains(current)) {
                    continue;
                }
                seen.insert(current);
                for (const auto &[parent, tactic_id]: ancestors.get_ancestors(current)) {
                    if (parent.empty()) {
                        continue;
                    }
                    T &node = nodes.at(parent);
                    if (node.killed(tactic_id)) {
                        continue;
                    }
                    node.set_expandable(tactic_id, true);
                    propagate_queue.push_front(parent);
                }
            }
        }

        // Simple assertion to make sure there are no expandable and killed tactics
        void expandable_check() {
            for (const auto &[thm, node]: nodes) {
                for (size_t i = 0; i < node.n_tactics(); i++) {
                    assert(!(node.killed(i) && node.expandable(i)));
                }
            }
        }

        // Find unexplored theorems and propagate expandable tactics, and some sanity checks
        void find_unexplored_and_propagate_expandable(bool ignore_solved) {
            find_unexplored(ignore_solved);
            propagate_expandable();
            // If just starting
            if (unexplored_theorems.size() == 1 && unexplored_theorems.contains(*root)) {
                expandable_check();
                return;
            }
            if (unexplored_theorems.empty()) {
                expandable_check();
                return;
            }
            if (nodes.at(root).expandable()) {
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
                        if (parent.empty()) {
                            continue;
                        }
                        to_check.push_back({nodes.at(parent), tactic_id});
                    }
                }
                auto [current, tid] = to_check.front();
                to_check.pop_front();
                if (!current.is_valid(tid)) {
                    continue;
                }
                auto children = current.get_children_for_tactic(tid);
                if (std::all_of(children.begin(), children.end(), [this](std::shared_ptr<theorem> &thm) {
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
                return node.is_solved() == (node.n_solving_tactics() > 0);
            });
            if (!solved_requires_tactic) {
                throw std::runtime_error(
                        "Solved consistency check failed, at least one node is solved without a solving tactic or vice versa");
            }
            for (const auto &[thm, node]: nodes) {
                const auto &children_for_tactic = node.get_children_for_tactic();
                for (size_t tactic_id = 0; tactic_id < children_for_tactic.size(); tactic_id++) {
                    const auto &children = children_for_tactic[tactic_id];
                    for (const auto &child: children) {
                        if (node.killed(tactic_id))
                            continue;
                        if (!ancestors.get_ancestors(child).contains(thm, tactic_id)) {
                            throw std::runtime_error("Ancestor consistency check failed, ancestor not found");
                        }
                    }
                }
            }
            for (const auto &[_, node]: nodes) {
                bool should_be_solved = false;
                const auto &children_for_tactic = node.get_children_for_tactic();
                for (size_t tactic_id = 0; tactic_id < children_for_tactic.size(); tactic_id++) {
                    const auto &children = children_for_tactic[tactic_id];
                    should_be_solved |= std::all_of(children.begin(), children.end(), [this](const std::shared_ptr<theorem> &thm) {
                        return nodes.contains(thm) && nodes.at(thm).is_solved();
                    }) && node.is_valid(tactic_id);
                }
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
            TheoremSet seen;
            std::deque<std::shared_ptr<theorem>> to_visit;
            while (!to_visit.empty()) {
                std::shared_ptr<theorem> current = to_visit.front();
                to_visit.pop_front();
                if (!nodes.contains(current)) {
                    continue;
                }
                if (seen.contains(*current)) {
                    continue;
                }
                seen.insert(*current);
                T &node = nodes.at(current);
                assert (node.n_solving_tactics() > 0);
                node.set_in_proof();
                for (const auto &tactic_id: node.solving_range())
                    for (const auto &child: node.get_children_for_tactic(tactic_id))
                        to_visit.push_back(child);
            }
        }

        // Get all proofs for a theorem
        std::vector<struct proof> all_proofs(std::shared_ptr<theorem> &thm) {
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
                for (auto &[thm, node]: nodes) {
                    if (!node.is_solved_leaf_node()) {
                        continue;
                    }
                    for (const auto &tactic_id: node.solving_range()) {
                        size_t priority = 1;
                        if (metric == TIME)
                            priority = node.get_tactic(tactic_id)->duration;
                        to_process.push_back({&node, priority, tactic_id});
                    }
                }

                std::priority_queue<PT> pq(to_process.begin(), to_process.end());
                // Set minimum length for each node that has a minimum length
                while (!pq.empty()) {
                    PT pnode = pq.top();
                    pq.pop();
                    auto [node, priority, tactic_id] = pnode;
                    if (!node->has_minimum_tactic_length(metric, tactic_id)) {
                        node->set_minimum_tactic_length(metric, priority, tactic_id);
                        if (priority <= node->minimum_length(metric)) {
                            assert(!node->has_minimum_tactic(metric, tactic_id));
                            node->set_minimum_tactic(metric, tactic_id);
                        }
                    }
                    if (node->has_minimum_length(metric)) {
                        assert(node->minimum_length(metric) <= priority);
                        continue;
                    }
                    node->set_minimum_length(metric, priority);
                    // Also for each parent, if all children have a minimum (i.e. are all solved)
                    // Then propagate the minimum length to the parent
                    for (const auto &[parent, parent_tactic]: permanent_ancestors.get_ancestors(node->get_theorem())) {
                        if (parent.empty()) {
                            continue;
                        }
                        T &parent_node = nodes.at(parent);
                        size_t added_priority = 1;
                        if (metric == TIME)
                            added_priority = parent_node.get_tactic(parent_tactic)->duration;
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
                        if (new_priority < MAXIMUM_PROOF_LENGTH) {
                            pq.push({&parent_node, new_priority, parent_tactic});
                        }
                    }
                }
            }
            for (const auto metric: {DEPTH, SIZE, TIME}) {
                if (!is_proven())
                    return;

                assert(nodes.contains(root));
                assert(nodes.at(root).minimum_length(metric) < MAXIMUM_PROOF_LENGTH);
                minimum_proof_size.set(metric, nodes.at(root).minimum_length(metric));
                std::deque<std::shared_ptr<theorem>> to_visit;
                to_visit.push_back(root);
                TheoremSet seen;
                while (!to_visit.empty()) {
                    std::shared_ptr<theorem> current = to_visit.front();
                    to_visit.pop_front();
                    if (seen.contains(*current)) {
                        continue;
                    }
                    seen.insert(*current);
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
        size_t depth_for_children(const std::vector<std::shared_ptr<theorem>> &children) const {
            size_t base = 0;
            for (const auto &child: children) {
                if (!nodes.contains(child)) {
                    return MAXIMUM_PROOF_LENGTH;
                }
                const T &node = nodes.at(child);
                base = std::max(base, node.minimum_length(DEPTH));
            }
            return base;
        }

        size_t size_for_children(const std::vector<std::shared_ptr<theorem>> &children) const {
            size_t base = 0;
            for (const auto &child: children) {
                if (!nodes.contains(child)) {
                    return MAXIMUM_PROOF_LENGTH;
                }
                const T &node = nodes.at(child);
                base += node.minimum_length(SIZE);
            }
            return base;
        }

        size_t time_for_children(const std::vector<std::shared_ptr<theorem>> &children) const {
            size_t base = 0;
            for (const auto &child: children) {
                if (!nodes.contains(child)) {
                    return MAXIMUM_PROOF_LENGTH;
                }
                const T &node = nodes.at(child);
                base += node.minimum_length(TIME);
            }
            return base;
        }
    };

}

#endif //HTPS_GRAPHCORRECT_H