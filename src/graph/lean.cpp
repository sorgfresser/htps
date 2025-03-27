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

void lean_theorem::tokenize(std::vector<std::string> &tokens) const {
    std::vector<std::string> context_tokens = {B_NS_WORD};
    context.tokenize(context_tokens);
    context_tokens.push_back(E_NS_WORD);

    tokens = {B_GOAL_WORD};
    tokens.insert(tokens.begin(), context_tokens.begin(), context_tokens.end());
    std::vector<std::string> conclusion_tokens;
    global_tokenizer.tokenize(conclusion, conclusion_tokens);
    tokens.insert(tokens.end(), conclusion_tokens.begin(), conclusion_tokens.end());
    tokens.push_back(E_GOAL_WORD);

    // Past tactics
    std::vector<std::string> tactic_tokens;
    for (const auto &tactic: past_tactics) {
        tokens.push_back(M_STACK_WORD);
        tactic.tokenize(tactic_tokens);
        tokens.insert(tokens.end(), tactic_tokens.begin(), tactic_tokens.end());
    }
}


std::vector<std::string> lean_theorem::tokenize() const {
    std::vector<std::string> result;
    tokenize(result);
    return result;
}


void lean_theorem::set_context(const lean_context& ctx) {
    context.namespaces = ctx.namespaces;
}

void lean_theorem::reset_tactics() {
    past_tactics.clear();
}


void lean_theorem::set_tactics(std::vector<lean_tactic> &tactics) {
    past_tactics = tactics;
}
