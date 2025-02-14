//
// Created by simon on 14.02.25.
//

#include "lean.h"
#include "../tokenization/tokenizer.h"
#include "../model/specialtokens.h"

using namespace htps;


void lean_tactic::tokenize(std::vector<std::string> &tokens) const {
    global_tokenizer.tokenize(unique_string, tokens);
}


std::vector<std::string> lean_tactic::tokenize() const {
    std::vector<std::string> tokens;
    tokenize(tokens);
    return tokens;
}


void lean_context::tokenize(std::vector<std::string> &tokens) const {
    std::vector<std::string> ns_tokens;
    for (const auto &ns: namespaces) {
        ns_tokens.clear();
        global_tokenizer.tokenize(ns, ns_tokens);
        tokens.insert(tokens.end(), ns_tokens.begin(), ns_tokens.end());
    }
}

std::vector<std::string> lean_context::tokenize() const {
    std::vector<std::string> tokens;
    tokenize(tokens);
    return tokens;
}

std::vector<std::string> lean_theorem::tokenize() const {
    std::vector<std::string> context_tokens = {B_NS_WORD};
    context.tokenize(context_tokens);
    context_tokens.push_back(E_NS_WORD);

    std::vector<std::string> result = {B_GOAL_WORD};
    result.insert(result.begin(), context_tokens.begin(), context_tokens.end());
    std::vector<std::string> conclusion_tokens;
    global_tokenizer.tokenize(conclusion, conclusion_tokens);
    result.insert(result.end(), conclusion_tokens.begin(), conclusion_tokens.end());
    result.push_back(E_GOAL_WORD);

    // Past tactics
    std::vector<std::string> tactic_tokens;
    for (const auto &tactic: past_tactics) {
        result.push_back(M_STACK_WORD);
        tactic.tokenize(tactic_tokens);
        result.insert(result.end(), tactic_tokens.begin(), tactic_tokens.end());
    }

    return result;
}