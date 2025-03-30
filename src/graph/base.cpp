#include "base.h"
#include <string>
#include <vector>
#include <algorithm>
#include "../json.hpp"

using namespace htps;

bool tactic::operator==(const tactic &t) const {
    return unique_string == t.unique_string;
}

bool hypothesis::operator==(const hypothesis &h) const {
    return identifier == h.identifier && type == h.type;
}


std::size_t std::hash<tactic>::operator()(const tactic &t) const {
    return std::hash<std::string>{}(t.unique_string);
}


hypothesis hypothesis::from_json(const std::string &json) {
    nlohmann::json j = nlohmann::json::parse(json);
    hypothesis h;
    h.identifier = j["identifier"];
    h.type = j["type"];
    return h;
}

std::string theorem::get_unique_string(const std::string &conclusion, const std::vector<hypothesis> &hypotheses) {
    // Sort hypotheses by identifier
    // We sort such that two theorems with the same hypotheses are equal, even if the hypotheses are in different order
    std::vector<hypothesis> hypotheses_sorted = hypotheses;
    std::sort(hypotheses_sorted.begin(), hypotheses_sorted.end(), [](const hypothesis &a, const hypothesis &b) {
        return a.identifier < b.identifier;
    });
    std::string unique_string;
    for (auto &hypothesis: hypotheses_sorted) {
        unique_string += hypothesis.identifier;
        unique_string += "|||";
        unique_string += hypothesis.type;
        unique_string += "|||";
    }
    unique_string += conclusion;
    return unique_string;
}

bool theorem::operator==(const theorem &t) const {
    return unique_string == t.unique_string;
}



void theorem::set_context(const context& other_ctx) {
    ctx.namespaces = other_ctx.namespaces;
}

void theorem::reset_tactics() {
    past_tactics.clear();
}


void theorem::set_tactics(std::vector<tactic> &tactics) {
    past_tactics = tactics;
}


std::size_t std::hash<theorem>::operator()(const theorem &t) const {
    return std::hash<std::string>{}(t.unique_string);
}

std::size_t std::hash<std::pair<std::shared_ptr<htps::theorem>, size_t> >::operator()(
    const std::pair<std::shared_ptr<htps::theorem>, size_t> &p) const {
    const auto second_hash = std::hash<size_t>{}(p.second);
    const auto first = p.first;
    const auto first_hash = first ? std::hash<htps::theorem>{}(*first) : 0;
    return first_hash ^ (second_hash);
}

