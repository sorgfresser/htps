//
// Created by simon on 15.02.25.
//

#ifndef HTPS_CORE_H
#define HTPS_CORE_H

#include "../graph/base.h"
#include <memory>
#include <vector>
#include <optional>

namespace htps {
    struct env_effect {
        std::shared_ptr<theorem> goal;
        std::shared_ptr<tactic> tac;
        std::vector<std::shared_ptr<theorem>> children;
    };

    struct env_expansion {
        std::shared_ptr<theorem> thm;
        size_t expander_duration;
        size_t generation_duration;
        std::vector<size_t> env_durations;
        std::vector<std::shared_ptr<env_effect>> effects;
        double log_critic;
        std::vector<std::shared_ptr<tactic>> tactics;
        std::vector<std::vector<std::shared_ptr<theorem>>> children_for_tactic;
        std::vector<double> priors;
        std::optional<std::string> error;

        bool is_error() const {
            return error.has_value();
        }

        env_expansion(std::shared_ptr<theorem> &thm, size_t expander_duration, size_t generation_duration,
                      std::vector<size_t> &env_durations, std::string &error) :
                thm(thm), expander_duration(expander_duration), generation_duration(generation_duration),
                env_durations(env_durations), log_critic(MIN_FLOAT), error(error) {}

        env_expansion(std::shared_ptr<theorem> &thm, size_t expander_duration, size_t generation_duration,
                      std::vector<size_t> &env_durations, std::vector<std::shared_ptr<env_effect>> &effects,
                      double log_critic, std::vector<std::shared_ptr<tactic>> &tactics,
                      std::vector<std::vector<std::shared_ptr<theorem>>> &children_for_tactic,
                      std::vector<double> &priors) :
                thm(thm), expander_duration(expander_duration), generation_duration(generation_duration),
                env_durations(env_durations), effects(effects), log_critic(log_critic), tactics(tactics),
                children_for_tactic(children_for_tactic), priors(priors) {}

    };
}
#endif //HTPS_CORE_H
