//
// Created by simon on 14.02.25.
//

#include "lean.h"

using namespace htps;

bool lean_tactic::operator==(const tactic &t) const {
    return unique_string == t.unique_string;
}

bool lean_theorem::operator==(const theorem &t) const {
    return unique_string == t.unique_string;
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
