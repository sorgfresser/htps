#include "base.h"
#include <string>
#include <vector>
#include <algorithm>
#include "../json.hpp"
#include "../model/specialtokens.h"

using namespace htps;

bool hypothesis::operator==(const hypothesis &h) const {
    return identifier == h.identifier && type == h.type;
}


bool theorem::operator==(const theorem &t) const {
    return unique_string == t.unique_string;
}

bool tactic::operator==(const tactic &t) const {
    return unique_string == t.unique_string;
}

std::size_t std::hash<tactic>::operator()(const tactic &t) const {
    return std::hash<std::string>{}(t.unique_string);
}

std::vector<std::string> tactic::tokenize() const {
    throw std::invalid_argument("Not implemented");
}


hypothesis hypothesis::from_json(const std::string &json) {
    nlohmann::json j = nlohmann::json::parse(json);
    hypothesis h;
    h.identifier = j["identifier"];
    h.type = j["type"];
    return h;
}


std::vector<std::string> theorem::tokenize() const {
    throw std::invalid_argument("Not implemented");
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


//
//
//proof_node proof_node::from_json(const std::string &json) {
//    nlohmann::json j = nlohmann::json::parse(json);
//    proof_node node;
//    node.proof_theorem = theorem::from_json(j["proof_theorem"].dump());
//    node.proof_tactic.unique_string = j["proof_tactic"]["unique_string"];
//    for (auto &child: j["children"]) {
//        node.children.push_back(theorem::from_json(child.dump()));
//    }
//    return node;
//}
//
//bool hyper_tree_node::is_proven() const {
//    if (_proven) return true;
//    return (proven_by != nullptr);
//}
//
//hyper_tree_node hyper_tree_node::from_json(const std::string &json) {
//    nlohmann::json j = nlohmann::json::parse(json);
//    hyper_tree_node node;
//    node.id = j["id"];
//    node._proven = j["_proven"];
//    for (auto &tactic: j["tactics"]) {
//        node.tactics.push_back({tactic["unique_string"]});
//    }
//    for (auto &[tactic, children]: j["tactic_to_children"].items()) {
//        std::vector<hyper_tree_node> children_nodes;
//        for (auto &child: children) {
//            children_nodes.push_back(hyper_tree_node::from_json(child.dump()));
//        }
//        node.tactic_to_children[{tactic}] = children_nodes;
//    }
//    return node;
//}
//
//hyper_tree_node hyper_tree::add_node(const theorem &t) {
//    if (auto it = theorem_to_node.find(t); it != theorem_to_node.end()) {
//        return it->second;
//    }
//    hyper_tree_node node;
//    node.id = id_to_theorem.size();
//    node._proven = false; // default value
//    id_to_theorem.push_back(t);
//    theorem_to_node[t] = node;
//    return node;
//}
//
//void hyper_tree::expand_with_tactic(const theorem &src, const tactic &tactic, const std::vector<theorem> &children) {
//    hyper_tree_node &src_node = theorem_to_node[src];
//    std::vector<hyper_tree_node> children_nodes = std::vector<hyper_tree_node>(children.size());
//    for (std::size_t i = 0; i < children.size(); i++) {
//        children_nodes[i] = add_node(children[i]);
//    }
//
//    src_node.tactics.push_back(tactic);
//    src_node.tactic_to_children[tactic] = children_nodes;
//    if (children_nodes.empty() && (src_node.proven_by == nullptr)) {
//        src_node.proven_by = &src_node.tactics.back();
//    }
//}
//
//
//bool hyper_tree::propagate_proven(const theorem &t) {
//    std::vector<hyper_tree_node> seen_proven = std::vector<hyper_tree_node>(proven.size());
//    for (auto it = proven.begin(); it != proven.end(); it++) {
//        seen_proven.push_back(theorem_to_node[*it]);
//    }
//    std::vector<hyper_tree_node> seen = std::vector<hyper_tree_node>();
//    bool p = walk(theorem_to_node[t], seen, seen_proven);
//    for (auto &node: seen_proven) {
//        proven.insert(id_to_theorem[node.id]);
//    }
//    return p;
//}
//
//bool hyper_tree::walk(hyper_tree_node &src, std::vector<hyper_tree_node> &seen,
//                      std::vector<hyper_tree_node> &seen_proven) {
//    if (src.is_proven()) {
//        seen_proven.push_back(src);
//        return true;
//    }
//    if (std::find(seen_proven.begin(), seen_proven.end(), src) != seen_proven.end()) {
//        return true;
//    }
//    if (std::find(seen.begin(), seen.end(), src) != seen.end()) {
//        return false;
//    }
//    seen.push_back(src);
//    auto tactics = src.tactics;
//
//    // Copy the vector to avoid modifying the original
//    std::vector<hyper_tree_node> _seen = seen;
//    bool children_proven;
//    for (auto &tactic: tactics) {
//        auto children = src.tactic_to_children[tactic];
//        for (auto &child: children) {
//            children_proven = walk(child, seen, seen_proven) && children_proven;
//        }
//        // Only if all children are proven, the node is proven
//        if (children_proven) {
//            src.proven_by = &tactic;
//            seen_proven.push_back(src);
//            return true;
//        }
//        seen = _seen;
//    }
//
//    return false;
//}
//
//proof hyper_tree::get_proof() {
//    return build_proof(0);
//}
//
//proof hyper_tree::build_proof(size_t id) {
//    proof p;
//    p.proof_theorem = id_to_theorem[id];
//    hyper_tree_node &node = theorem_to_node[p.proof_theorem];
//
//    if (node.proven_by != nullptr) {
//        tactic t = *node.proven_by;
//        p.proof_tactic = t;
//        for (auto &child: node.tactic_to_children[t]) {
//            p.children.push_back(build_proof(child.id));
//        }
//    }
//    return p;
//}
