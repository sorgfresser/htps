//
// Created by simon on 14.02.25.
//

#ifndef HTPS_POLICY_H
#define HTPS_POLICY_H

#include <vector>
#include <cassert>
#include <limits>
#include "../json.hpp"

namespace htps {
    constexpr double MIN_FLOAT = -std::numeric_limits<double>::infinity(); // Avoid underflow, use float min
    enum PolicyType {
        AlphaZero, RPO, PolicyTypeCount
    };


/* Tree policy for MCTS */
    class Policy {
    private:
        PolicyType type;
        double exploration;

    public:
        virtual /* Get the policy for a given set of q-values and pi-values
         * q_values: The q-values for each action
         * pi_values: The prior values for each action
         * counts: The number of times each action has been taken
         * result: The resulting policy vector
         * */
        void get_policy(const std::vector<double> &q_values, const std::vector<double> &pi_values,
                        const std::vector<size_t> &counts, std::vector<double> &result) const;

        Policy(PolicyType type, double exploration) : type(type), exploration(exploration) {
            assert(type != PolicyTypeCount);
        }

        static Policy from_json(const nlohmann::json &j);

        explicit operator nlohmann::json() const;

    protected:
        void alpha_zero(const std::vector<double> &q_values, const std::vector<double> &pi_values,
                        const std::vector<size_t> &counts, std::vector<double> &result) const;

        void mcts_rpo(const std::vector<double> &q_values, const std::vector<double> &pi_values,
                      const std::vector<size_t> &counts, std::vector<double> &result) const;

        static double find_rpo_alpha(double alpha_min, double alpha_max, const std::vector<double> &q_values,
                                     const std::vector<double> &scaled_pi_values);
    };
}
#endif //HTPS_POLICY_H
