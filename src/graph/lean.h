//
// Created by simon on 14.02.25.
//

#ifndef HTPS_LEAN_H
#define HTPS_LEAN_H

#include "base.h"
#include <set>
#include <string>

namespace htps {
    struct lean_context {
#ifdef PYTHON_BINDINGS
        PyObject_HEAD
#endif
        std::set<std::string> namespaces; // we need a sorted set because the order should not influence tokenization

        lean_context() = default;

        lean_context(const lean_context &) = default;

        lean_context(lean_context &&) = default;

        lean_context(std::set<std::string> namespaces) : namespaces(std::move(namespaces)) {}

        std::vector<std::string> tokenize() const;

        void tokenize(std::vector<std::string> &tokens) const; //inplace
    };

    struct lean_tactic : public tactic {
        std::vector<std::string> tokenize() const override;

        void tokenize(std::vector<std::string> &tokens) const override;
    };


    struct lean_theorem : public theorem {
    public:
        /* Tokenize a theorem. Will have the structure <Namespace-Info> <Goal> <PastTactics> */
        std::vector<std::string> tokenize() const override;

        void tokenize(std::vector<std::string> &tokens) const override; //inplace

    private:
        lean_context context;
        std::vector<lean_tactic> past_tactics;
    };
}


#endif //HTPS_LEAN_H
