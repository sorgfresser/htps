//
// Created by simon on 14.02.25.
//

#ifndef HTPS_LEAN_H
#define HTPS_LEAN_H
#ifdef PYTHON_BINDINGS
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#endif
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

        lean_context(lean_context &) = default;

        lean_context(lean_context &&) = default;

        lean_context(std::set<std::string> namespaces) : namespaces(std::move(namespaces)) {}
    };

    struct lean_tactic : public tactic {
#ifdef PYTHON_BINDINGS
        PyObject_HEAD
#endif
        bool operator==(const tactic &t) const override;

        ~lean_tactic() = default;
    };


    struct lean_theorem : public theorem {
#ifdef PYTHON_BINDINGS
        PyObject_HEAD
#endif
    public:
        lean_context context;
        std::vector<lean_tactic> past_tactics;
#ifdef PYTHON_BINDINGS
        PyObject *py_dict;
#endif

        void set_context(const lean_context& ctx);

        void reset_tactics();

        void set_tactics(std::vector<lean_tactic> &tactics);

        lean_theorem() = default;

//        ~lean_theorem() {
//            past_tactics.clear();
//#ifdef PYTHON_BINDINGS
//            Py_XDECREF(py_dict);
//#endif
//        }

        bool operator==(const theorem &t) const override;
    };
}


#endif //HTPS_LEAN_H
