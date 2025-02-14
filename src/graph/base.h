#ifndef HTPS_BASE_H
#define HTPS_BASE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <unordered_set>
#include <memory>

namespace htps {
    struct tactic {
        std::string unique_string;
        bool is_valid;
        size_t duration; // duration in milliseconds

        bool operator==(const tactic &t) const;

        virtual std::vector<std::string> tokenize() const = 0;

        virtual void tokenize(std::vector<std::string> &tokens) const = 0;
    };
}

template<>
struct std::hash<htps::tactic> {
    std::size_t operator()(const htps::tactic &t) const;
};

namespace htps {
    struct hypothesis {
        std::string identifier;
        std::string type;

        static hypothesis from_json(const std::string &json);

        bool operator==(const hypothesis &h) const = default;
    };

    struct theorem {
        std::string conclusion;
        std::vector<hypothesis> hypotheses;
        std::string unique_string;

        bool operator==(const theorem &t) const = default;

        virtual std::vector<std::string> tokenize() const = 0;

        virtual void tokenize(std::vector<std::string> &tokens) const = 0;

        theorem(const std::string &conclusion, const std::vector<hypothesis> &hypotheses) : conclusion(conclusion),
                                                                                            hypotheses(hypotheses) {
            unique_string = get_unique_string(conclusion, hypotheses);
        }

    protected:
        /* Create a unique string for a theorem using its conclusion and hypotheses.
         * Is invariant to hypothesis order.
         * */
        static std::string get_unique_string(const std::string &conclusion, const std::vector<hypothesis> &hypotheses);
    };
}
template<>
struct std::hash<htps::theorem> {
    std::size_t operator()(const htps::theorem &t) const;
};

//template<>
//struct std::hash<std::pair<std::optional<htps::theorem>, int>> {
//    std::size_t operator()(const std::pair<std::optional<htps::theorem>, int> &p) const {
//        return std::hash<std::optional<htps::theorem>>()(p.first) ^ std::hash<int>()(p.second);
//    }
//};
//
//
//template<>
//struct std::hash<std::pair<std::optional<htps::theorem>, size_t>> {
//    std::size_t operator()(const std::pair<std::optional<htps::theorem>, size_t> &p) const {
//        return std::hash<std::optional<htps::theorem>>()(p.first) ^ std::hash<size_t>()(p.second);
//    }
//};

// Hash std::pair<std::optional<struct theorem*>, size_t>
//template<>
//struct std::hash<std::pair<std::optional<htps::theorem*>, size_t>> {
//    std::size_t operator()(const std::pair<std::optional<htps::theorem*>, size_t> &p) const {
//        std::size_t h1 = p.first ? std::hash<htps::theorem*>()(p.first.value()) : 0;
//        std::size_t h2 = std::hash<size_t>()(p.second);
//        return h1 ^ (h2 << 1);
//    }
//};

template<>
struct std::hash<std::pair<std::shared_ptr<htps::theorem>, size_t>> {
    std::size_t operator()(const std::pair<std::shared_ptr<htps::theorem>, size_t> &p) const;
};

namespace htps {

    struct proof {
        std::shared_ptr<theorem> proof_theorem;
        std::shared_ptr<tactic> proof_tactic;
        std::vector<proof> children;
    };

//    struct proof_node {
//        theorem *proof_theorem;
//        tactic *proof_tactic;
//        std::vector<theorem> children;

//        static proof_node from_json(const std::string &json);
//    };

//    template<typename T>
//    class TacticToChildrenMap {
//    private:
//        std::unordered_map<std::string, std::vector<T>> string_to_children;
//    public:
//        bool contains(const std::string &s) {
//            return string_to_children.contains(s);
//        }
//    };
//
//    struct hyper_tree_node {
//        std::size_t id;
//        bool _proven;
//        tactic *proven_by = nullptr;
//
//        std::vector<tactic *> tactics;
//        std::unordered_map<tactic, std::vector<hyper_tree_node>> tactic_to_children;
//
//        bool is_proven() const;
//
//        bool operator==(const hyper_tree_node &n) const = default;
//
//        static hyper_tree_node from_json(const std::string &json);
//    };
//
//    struct hyper_tree {
//        std::unordered_map<theorem, hyper_tree_node> theorem_to_node;
//        std::vector<theorem> id_to_theorem;
//        std::vector<theorem> leaves;
//        theorem *root;
//        std::unordered_set<theorem> proven;
//
//        hyper_tree_node add_node(const theorem &t);
//
//        void expand_with_tactic(const theorem &src, const tactic &tactic, const std::vector<theorem> &children);
//
//        bool propagate_proven(const theorem &t);
//
//        proof get_proof();
//
//    protected:
//        bool walk(hyper_tree_node &src, std::vector<hyper_tree_node> &seen, std::vector<hyper_tree_node> &seen_proven);
//
//        proof build_proof(size_t id);
//    };
}

#endif // HTPS_BASE_H
