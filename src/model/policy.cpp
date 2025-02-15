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
                        const std::vector<size_t> counts, std::vector<double> &result) const {
    size_t valid_count = 0;
    result.clear();
    result.reserve(q_values.size());
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
        std::fill(result.begin(), result.end(), 0);
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
}


void Policy::alpha_zero(const std::vector<double> &q_values, const std::vector<double> &pi_values,
                        const std::vector<size_t> counts, std::vector<double> &result) const {
    std::vector<double> scores(q_values.size());
    result.reserve(q_values.size());
    double count_sum = static_cast<double>(std::accumulate(counts.begin(), counts.end(), 0));
    std::vector<double> counts_d(counts.begin(), counts.end());
    double score_sum = 0;
    for (size_t i = 0; i < q_values.size(); i++) {
        if (pi_values[i] > MIN_FLOAT && q_values[i] > MIN_FLOAT) {
            scores[i] = q_values[i] + exploration * pi_values[i] * std::sqrt(count_sum) / (1 + counts_d[i]);
        } else {
            scores[i] = 0;
        }
        score_sum += scores[i];
    }
    for (size_t i = 0; i < q_values.size(); i++) {
        result[i] = scores[i] / score_sum;
    }
}

double Policy::find_rpo_alpha(double alpha_min, double alpha_max, const std::vector<double> &q_values,
                              const std::vector<double> &scaled_pi_values) const {
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
        if ((pi_difference_sum - 1) < TOLERANCE) {
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
                      const std::vector<size_t> counts, std::vector<double> &result) const {
    result.reserve(q_values.size());
    double count_sum = static_cast<double>(std::accumulate(counts.begin(), counts.end(), 0));
    double multiplier = std::sqrt(count_sum) / (count_sum + counts.size()) * exploration;
    if (multiplier <= 0) {
        double q_sum = 0;
        for (size_t i = 0; i < q_values.size(); i++) {
            if (q_values[i] > MIN_FLOAT) {
                result[i] = q_values[i];
                q_sum += q_values[i];
            } else {
                result[i] = 0;
            }
        }
        for (size_t i = 0; i < q_values.size(); i++) {
            result[i] /= q_sum;
        }
        return;
    }

    std::vector<double> scaled_pi_values(q_values.size());

    double alpha_min, alpha_max = 0;

    for (size_t i = 0; i < q_values.size(); i++) {
        scaled_pi_values[i] = pi_values[i] * multiplier;
        alpha_min = std::max(alpha_min, q_values[i] + multiplier * pi_values[i]);
        alpha_max = std::max(alpha_max, q_values[i] + multiplier);
    }
    double alpha = find_rpo_alpha(alpha_min, alpha_max, q_values, scaled_pi_values);
    double result_sum = 0;
    for (size_t i = 0; i < q_values.size(); i++) {
        result[i] = scaled_pi_values[i] / std::max((alpha - q_values[i]), EPSILON);
        result_sum += result[i];
    }
    for (size_t i = 0; i < q_values.size(); i++) {
        result[i] /= result_sum;
    }
}
