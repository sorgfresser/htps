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
#include "../json.hpp"

namespace htps {
    struct tactic {
#ifdef PYTHON_BINDINGS
        PyObject_HEAD
#endif
        std::string unique_string;
        bool is_valid;
        size_t duration; // duration in milliseconds

        static tactic from_json(const nlohmann::json &j);

        operator nlohmann::json() const;

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

        static hypothesis from_json(const nlohmann::json &j);

        operator nlohmann::json() const;

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

        static context from_json(const nlohmann::json &j);

        operator nlohmann::json() const;
    };

    struct theorem {
        std::string conclusion;
        std::vector<hypothesis> hypotheses;
        std::string unique_string;
        context ctx;
        std::vector<tactic> past_tactics;
        std::any metadata;

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

        static theorem from_json(const nlohmann::json &j);

        operator nlohmann::json() const;

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

        static proof from_json(const nlohmann::json &json);

        operator nlohmann::json() const;
    };
}

namespace nlohmann
{
    template <typename T>
    struct adl_serializer<std::shared_ptr<T>>
    {
        static void to_json(json& j, const std::shared_ptr<T>& opt)
        {
            if (opt)
            {
                j = *opt;
            }
            else
            {
                j = nullptr;
            }
        }

        static void from_json(const json& j, std::shared_ptr<T>& opt)
        {
            if (j.is_null()) {
                opt = nullptr;
            }
            else {
                opt = std::make_shared<T>(j.get<T>());
            }
        }
    };
}
#endif // HTPS_BASE_H
