//
// Created by simon on 15.02.25.
//

#ifndef HTPS_CORE_H
#define HTPS_CORE_H
#ifdef PYTHON_BINDINGS
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#endif
#include "../graph/base.h"
#include "../model/policy.h"
#include <memory>
#include <vector>
#include <optional>

namespace htps {
    struct env_effect {
#ifdef PYTHON_BINDINGS
        PyObject_HEAD
#endif
        TheoremPointer goal;
        std::shared_ptr<tactic> tac;
        std::vector<TheoremPointer> children;

        operator nlohmann::json() const {
            nlohmann::json j;
            j["goal"] = goal;
            j["tac"] = tac;
            j["children"] = children;
            return j;
        }

        static env_effect from_json(const nlohmann::json &j) {
            env_effect e;
            e.goal = j["goal"];
            e.tac = j["tac"];
            std::vector<TheoremPointer> children;
            for (const auto &child: j["children"]) {
                children.push_back(child);
            }
            e.children = children;
            return e;
        }
    };

    struct env_expansion {
        TheoremPointer thm;
        size_t expander_duration;
        size_t generation_duration;
        std::vector<size_t> env_durations;
        std::vector<std::shared_ptr<env_effect>> effects;
        double log_critic;
        std::vector<std::shared_ptr<tactic>> tactics;
        std::vector<std::vector<TheoremPointer>> children_for_tactic;
        std::vector<double> priors;
        std::optional<std::string> error;

        bool is_error() const {
            return error.has_value();
        }
        env_expansion() = default;

        env_expansion(TheoremPointer &thm, size_t expander_duration, size_t generation_duration,
                      std::vector<size_t> &env_durations, std::string &error) :
                thm(thm), expander_duration(expander_duration), generation_duration(generation_duration),
                env_durations(env_durations), log_critic(MIN_FLOAT), error(error) {}

        env_expansion(TheoremPointer &thm, size_t expander_duration, size_t generation_duration,
                      std::vector<size_t> &env_durations, std::vector<std::shared_ptr<env_effect>> &effects,
                      double log_critic, std::vector<std::shared_ptr<tactic>> &tactics,
                      std::vector<std::vector<TheoremPointer>> &children_for_tactic,
                      std::vector<double> &priors) :
                thm(thm), expander_duration(expander_duration), generation_duration(generation_duration),
                env_durations(env_durations), effects(effects), log_critic(log_critic), tactics(tactics),
                children_for_tactic(children_for_tactic), priors(priors) {}

        operator nlohmann::json() const {
            nlohmann::json j;
            j["thm"] = thm;
            j["expander_duration"] = expander_duration;
            j["generation_duration"] = generation_duration;
            j["env_durations"] = env_durations;
            std::vector<nlohmann::json> effects_json;
            for (const auto &effect: effects) {
                effects_json.push_back(nlohmann::json(*effect));
            }
            j["effects"] = effects_json;
            j["log_critic"] = log_critic;
            std::vector<nlohmann::json> tactics_json;
            for (const auto &tac: tactics) {
                tactics_json.push_back(nlohmann::json(*tac));
            }
            j["tactics"] = tactics_json;
            std::vector<std::vector<nlohmann::json>> children_for_tac;
            for (const auto &children: children_for_tactic) {
                std::vector<nlohmann::json> children_json;
                for (const auto &child: children) {
                    children_json.push_back(nlohmann::json(*child));
                }
                children_for_tac.push_back(children_json);
            }
            j["children_for_tactic"] = children_for_tac;
            j["priors"] = priors;
            if (error.has_value()) {
                j["error"] = error.value();
            }
            return j;
        }

        static env_expansion from_json(const nlohmann::json &j) {
            TheoremPointer thm = j["thm"];
            size_t expander_duration = j["expander_duration"];
            size_t generation_duration = j["generation_duration"];
            std::vector<size_t> env_durations = j["env_durations"];
            if (j.contains("error")) {
                std::string error = j["error"];
                return env_expansion(thm, expander_duration, generation_duration, env_durations, error);
            }

            std::vector<std::shared_ptr<env_effect>> effects;
            for (const auto &effect: j["effects"]) {
                effects.push_back(effect);
            }
            double log_critic = j["log_critic"];
            std::vector<std::shared_ptr<tactic>> tactics;
            for (const auto &tac: j["tactics"]) {
                tactics.push_back(tac);
            }
            std::vector<std::vector<TheoremPointer>> children_for_tactic;
            for (const auto &children: j["children_for_tactic"]) {
                std::vector<TheoremPointer> children_json;
                for (const auto &child: children) {
                    children_json.push_back(child);
                }
                children_for_tactic.push_back(children_json);
            }
            std::vector<double> priors = j["priors"];

            return {thm, expander_duration, generation_duration, env_durations, effects, log_critic,
                                 tactics, children_for_tactic, priors};
        }
    };
}
#endif //HTPS_CORE_H
