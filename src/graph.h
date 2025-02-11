#ifndef HTPS_GRAPH_H
#define HTPS_GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <unordered_set>

namespace htps {
    struct tactic {
        std::string unique_string;
        bool is_valid;

        static tactic from_json(const std::string &json);

        bool operator==(const tactic &t) const = default;
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

        static theorem from_json(const std::string &json);

        bool operator==(const theorem &t) const = default;
    };
}
template<>
struct std::hash<htps::theorem> {
    std::size_t operator()(const htps::theorem &t) const;
};


template<>
struct std::hash<std::pair<std::optional<htps::theorem>, int>> {
    std::size_t operator()(const std::pair<std::optional<htps::theorem>, int> &p) const {
        return std::hash<std::optional<htps::theorem>>()(p.first) ^ std::hash<int>()(p.second);
    }
};

namespace htps {

    struct proof {
        theorem proof_theorem;
        std::optional<tactic> proof_tactic;
        std::vector<proof> children;
    };

    struct proof_node {
        theorem proof_theorem;
        tactic proof_tactic;
        std::vector<theorem> children;

        static proof_node from_json(const std::string &json);
    };

    struct hyper_tree_node {
        std::size_t id;
        bool _proven;
        tactic *proven_by = nullptr;

        std::vector<tactic> tactics;
        std::unordered_map<tactic, std::vector<hyper_tree_node>> tactic_to_children;

        bool is_proven() const;

        bool operator==(const hyper_tree_node &n) const = default;

        static hyper_tree_node from_json(const std::string &json);
    };

    struct hyper_tree {
        std::unordered_map<theorem, hyper_tree_node> theorem_to_node;
        std::vector<theorem> id_to_theorem;
        std::vector<theorem> leaves;
        theorem root;
        std::unordered_set<theorem> proven;

        hyper_tree_node add_node(const theorem &t);

        void expand_with_tactic(const theorem &src, const tactic &tactic, const std::vector<theorem> &children);

        bool propagate_proven(const theorem &t);

        proof get_proof();

    protected:
        bool walk(hyper_tree_node &src, std::vector<hyper_tree_node> &seen, std::vector<hyper_tree_node> &seen_proven);

        proof build_proof(int id);
    };
}

#endif // HTPS_GRAPH_H
