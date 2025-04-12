//
// Created by simon on 14.02.25.
//

#include "policy.h"
#include <vector>
#include <stdexcept>
#include <array>
#include <numeric>
#include <cmath>
#include <algorithm>


using namespace htps;

constexpr size_t MAX_DEPTH = 50;
constexpr double TOLERANCE = 1e-3;
constexpr double EPSILON = 1e-10;

void Policy::get_policy(const std::vector<double> &q_values, const std::vector<double> &pi_values,
                        const std::vector<size_t> &counts, std::vector<double> &result) const {
    size_t valid_count = 0;
    result.clear();
    result.insert(result.begin(), q_values.size(), 0);
    std::array<size_t, 2> valid_indices{0, 0};
    for (size_t i = 0; i < q_values.size(); i++) {
        if (q_values[i] > MIN_FLOAT) {
            valid_indices[valid_count] = i;
            valid_count++;
            if (valid_count >= 2) {
                break;
            }
        }
    }
    if (valid_count < 1) {
        throw std::invalid_argument("No valid q-values");
    }
    if (valid_count == 1) {
        std::fill(result.begin(), result.end(), MIN_FLOAT);
        result[valid_indices[0]] = 1;
        return;
    }
    switch (type) {
        case AlphaZero:
            alpha_zero(q_values, pi_values, counts, result);
            break;
        case RPO:
            mcts_rpo(q_values, pi_values, counts, result);
            break;
        default:
            throw std::invalid_argument("Invalid policy type");
    }
    bool is_nan = std::any_of(result.begin(), result.end(), [](double d) { return std::isnan(d); });
    assert (!is_nan);
    assert (q_values.size() == result.size());
    double sum = 0.0;
    for (size_t i = 0; i < result.size(); i++) {
        if (result[i] > MIN_FLOAT)
            sum += result[i];
    }
    assert (sum > 0.99 && sum < 1.01);
}


void Policy::alpha_zero(const std::vector<double> &q_values, const std::vector<double> &pi_values,
                        const std::vector<size_t> &counts, std::vector<double> &result) const {
    std::vector<double> scores(q_values.size());
    auto count_sum = static_cast<double>(std::accumulate(counts.begin(), counts.end(), static_cast<size_t>(0)));
    std::vector<double> counts_d(counts.begin(), counts.end());
    double score_sum = 0;
    size_t valid_count = 0;
    for (size_t i = 0; i < q_values.size(); i++) {
        if (pi_values[i] > MIN_FLOAT && q_values[i] > MIN_FLOAT) {
            scores[i] = q_values[i] + exploration * pi_values[i] * std::sqrt(count_sum) / (1 + counts_d[i]);
            valid_count++;
            score_sum += scores[i];
        } else {
            scores[i] = MIN_FLOAT;
        }
    }
    assert (valid_count > 0);
    // If score sum is 0, simply return the uniform distribution over valid actions
    if (1e-10 > score_sum && score_sum > -1e-10) {
        for (size_t i = 0; i < q_values.size(); i++) {
            if (q_values[i] > MIN_FLOAT && pi_values[i] > MIN_FLOAT) {
                result[i] = 1.0 / static_cast<double>(valid_count);
            } else {
                result[i] = MIN_FLOAT;
            }
        }
        return;
    }
    // Normalize the scores
    for (size_t i = 0; i < q_values.size(); i++) {
        if (scores[i] > MIN_FLOAT)
            result[i] = scores[i] / score_sum;
        else
            result[i] = MIN_FLOAT;
    }
}

double Policy::find_rpo_alpha(double alpha_min, double alpha_max, const std::vector<double> &q_values,
                              const std::vector<double> &scaled_pi_values) {
    double alpha_mid;
    double pi_difference_sum;
    double diff;

    for (size_t j = 0; j < MAX_DEPTH; j++) {
        assert(alpha_min < alpha_max);
        alpha_mid = (alpha_min + alpha_max) / 2;
        pi_difference_sum = 0;
        for (size_t i = 0; i < q_values.size(); i++) {
            diff = alpha_mid - q_values[i];
            if (diff == 0) {
                throw std::runtime_error("Precision error");
            }
            pi_difference_sum += scaled_pi_values[i] / diff;
        }
        if (std::abs(pi_difference_sum - 1) < TOLERANCE) {
            return alpha_mid;
        }
        if (pi_difference_sum > 1)
            alpha_min = alpha_mid;
        else
            alpha_max = alpha_mid;
    }
    throw std::runtime_error("Max depth reached");
}


void Policy::mcts_rpo(const std::vector<double> &q_values, const std::vector<double> &pi_values,
                      const std::vector<size_t> &counts, std::vector<double> &result) const {
    auto count_sum = static_cast<double>(std::accumulate(counts.begin(), counts.end(), static_cast<size_t>(0)));
    double multiplier = std::sqrt(count_sum) / (count_sum + static_cast<double>(counts.size())) * exploration;
    if (multiplier <= 0) {
        size_t valid_count = 0;
        double q_sum = 0;
        for (size_t i = 0; i < q_values.size(); i++) {
            if (q_values[i] > MIN_FLOAT) {
                result[i] = q_values[i];
                q_sum += q_values[i];
                valid_count++;
            } else {
                result[i] = MIN_FLOAT;
            }
        }
        // If q sum is 0, simply return the uniform distribution over valid actions
        assert (valid_count > 0);
        if (1e-10 > q_sum && q_sum > -1e-10) {
            for (size_t i = 0; i < q_values.size(); i++) {
                if (q_values[i] > MIN_FLOAT && pi_values[i] > MIN_FLOAT) {
                    result[i] = 1.0 / static_cast<double>(valid_count);
                } else {
                    result[i] = MIN_FLOAT;
                }
            }
            return;
        }

        for (size_t i = 0; i < q_values.size(); i++) {
            if (q_values[i] > MIN_FLOAT) {
                result[i] = result[i] / q_sum;
            } else {
                result[i] = MIN_FLOAT;
            }
        }
        return;
    }

    std::vector<double> scaled_pi_values(q_values.size());

    double alpha_min = 0, alpha_max = 0;

    for (size_t i = 0; i < q_values.size(); i++) {
        scaled_pi_values[i] = pi_values[i] * multiplier;
        alpha_min = std::max(alpha_min, q_values[i] + multiplier * pi_values[i]);
        alpha_max = std::max(alpha_max, q_values[i] + multiplier);
    }
    double alpha = find_rpo_alpha(alpha_min, alpha_max, q_values, scaled_pi_values);
    double result_sum = 0;
    for (size_t i = 0; i < q_values.size(); i++) {
        if (q_values[i] > MIN_FLOAT) {
            result[i] = scaled_pi_values[i] / std::max((alpha - q_values[i]), EPSILON);
            result_sum += result[i];
        }
        else {
            result[i] = MIN_FLOAT;
        }
    }
    for (size_t i = 0; i < q_values.size(); i++) {
        if (result[i] > MIN_FLOAT)
            result[i] = result[i] / result_sum;
        else
            result[i] = MIN_FLOAT;
    }
}

Policy Policy::from_json(const nlohmann::json &j) {
    PolicyType type = static_cast<PolicyType>(j["type"]);
    double exploration = j["exploration"];
    return Policy(type, exploration);
}

Policy::operator nlohmann::json() const {
    nlohmann::json j;
    j["type"] = type;
    j["exploration"] = exploration;
    return j;
}
