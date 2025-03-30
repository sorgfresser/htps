#ifndef HTPS_BASE_H
#define HTPS_BASE_H
#ifdef PYTHON_BINDINGS
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#endif
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <unordered_set>
#include <memory>
#include <set>
#include <any>

namespace htps {
    struct tactic {
#ifdef PYTHON_BINDINGS
        PyObject_HEAD
#endif
        std::string unique_string;
        bool is_valid;
        size_t duration; // duration in milliseconds

        bool operator==(const tactic &t) const;

        ~tactic() = default;
    };
}

template<>
struct std::hash<htps::tactic> {
    std::size_t operator()(const htps::tactic &t) const;
};

namespace htps {
    struct hypothesis {
#ifdef PYTHON_BINDINGS
        PyObject_HEAD
#endif
        std::string identifier;
        std::string type;

        static hypothesis from_json(const std::string &json);

        bool operator==(const hypothesis &h) const;
    };

    struct context {
#ifdef PYTHON_BINDINGS
        PyObject_HEAD
#endif
        std::set<std::string> namespaces{}; // we need a sorted set because the order should not influence tokenization

        context() = default;

        context(const context &) = default;

        context(context &) = default;

        context(context &&) = default;

        explicit context(std::set<std::string> namespaces) : namespaces(std::move(namespaces)) {}
    };

    struct theorem {
#ifdef PYTHON_BINDINGS
        PyObject_HEAD
#endif
        std::string conclusion;
        std::vector<hypothesis> hypotheses;
        std::string unique_string;
        context ctx;
        std::vector<tactic> past_tactics;
        std::unordered_map<std::string, std::any> metadata;

        theorem() = default;

        theorem(const std::string &conclusion, const std::vector<hypothesis> &hypotheses) : conclusion(conclusion),
                                                                                            hypotheses(hypotheses),
                                                                                            ctx(),
                                                                                            past_tactics(),
                                                                                            metadata() {
            unique_string = get_unique_string(conclusion, hypotheses);
        }

        ~theorem() = default;

        bool operator==(const theorem &t) const;

        void set_context(const context& ctx);

        void reset_tactics();

        void set_tactics(std::vector<tactic> &tactics);

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
