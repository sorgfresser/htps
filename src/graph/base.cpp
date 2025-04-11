#include "base.h"
#include <string>
#include <vector>
#include <algorithm>

using namespace htps;

tactic tactic::from_json(const nlohmann::json &j) {
    tactic t;
    t.unique_string = j["unique_string"];
    t.is_valid = j["is_valid"];
    t.duration = j["duration"];
    return t;
}

tactic::operator nlohmann::json() const {
    nlohmann::json j;
    j["unique_string"] = unique_string;
    j["is_valid"] = is_valid;
    j["duration"] = duration;
    return j;
}

bool tactic::operator==(const tactic &t) const {
    return unique_string == t.unique_string;
}

bool hypothesis::operator==(const hypothesis &h) const {
    return identifier == h.identifier && type == h.type;
}


std::size_t std::hash<tactic>::operator()(const tactic &t) const {
    return std::hash<std::string>{}(t.unique_string);
}


hypothesis hypothesis::from_json(const nlohmann::json &j) {
    hypothesis h;
    h.identifier = j["identifier"];
    h.type = j["type"];
    return h;
}

hypothesis::operator nlohmann::json() const {
    nlohmann::json j;
    j["identifier"] = identifier;
    j["type"] = type;
    return j;
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

theorem theorem::from_json(const nlohmann::json &j) {
    theorem t;
    t.conclusion = j["conclusion"];
    std::vector<hypothesis> hypotheses;
    for (const auto &h: j["hypotheses"]) {
        hypotheses.push_back(hypothesis::from_json(h));
    }
    t.unique_string = j["unique_string"];
    t.set_context(context::from_json(j["ctx"]));
    std::vector<tactic> past_tactics;
    for (const auto &tac: j["past_tactics"]) {
        past_tactics.push_back(tactic::from_json(tac));
    }
    t.set_tactics(past_tactics);
    return t;
}

theorem::operator nlohmann::json() const {
    nlohmann::json j;
    j["conclusion"] = conclusion;
    j["hypotheses"] = hypotheses;
    j["unique_string"] = unique_string;
    j["ctx"] = ctx;
    j["past_tactics"] = past_tactics;
    return j;
}

proof proof::from_json(const nlohmann::json &j) {
    proof p;
    p.proof_theorem = j["proof_theorem"];
    p.proof_tactic = j["proof_tactic"];
    std::vector<proof> children;
    for (const auto &child: j["children"]) {
        children.push_back(proof::from_json(child));
    }
    p.children = children;
    return p;
}

proof::operator nlohmann::json() const {
    nlohmann::json j;
    j["proof_theorem"] = proof_theorem;
    j["proof_tactic"] = proof_tactic;
    j["children"] = children;
    return j;
}

std::size_t std::hash<theorem>::operator()(const theorem &t) const {
    return std::hash<std::string>{}(t.unique_string);
}

std::size_t std::hash<std::pair<htps::TheoremPointer, size_t> >::operator()(
    const std::pair<htps::TheoremPointer, size_t> &p) const {
    const auto second_hash = std::hash<size_t>{}(p.second);
    const auto first = p.first;
    const auto first_hash = first ? std::hash<htps::theorem>{}(*first) : 0;
    return first_hash ^ (second_hash);
}

context context::from_json(const nlohmann::json &j) {
    context ctx;
    ctx.namespaces = j["namespaces"];
    return ctx;
}

context::operator nlohmann::json() const {
    nlohmann::json j;
    j["namespaces"] = namespaces;
    return j;
}
