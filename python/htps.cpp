#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "../src/graph/htps.h"


static PyObject *make_enum(PyObject *module, PyObject *enum_module, const char ** values, size_t value_size, const char *name) {
    PyObject *key, *val, *name_py, *attrs, *args, *modname, *enum_type, *sub_enum_type, *kwargs;
    attrs = PyDict_New();

    for (long i = 0; i < value_size; i++) {
        key = PyUnicode_FromString(values[i]);
        val = PyLong_FromLong(i);
        PyObject_SetItem(attrs, key, val);
        Py_DECREF(key);
        Py_DECREF(val);
    }
    name_py = PyUnicode_FromString(name);
    args = PyTuple_Pack(2, name_py, attrs);
    Py_DECREF(attrs);
    Py_DECREF(name_py);

    // the module name might need to be passed as keyword argument
    kwargs = PyDict_New();
    key = PyUnicode_FromString("module");
    modname = PyModule_GetNameObject(module);
    PyObject_SetItem(kwargs, key, modname);
    Py_DECREF(key);
    Py_DECREF(modname);

    enum_type = PyObject_GetAttrString(enum_module, "Enum");
    sub_enum_type = PyObject_Call(enum_type, args, kwargs);
    Py_DECREF(enum_type);
    Py_DECREF(args);
    Py_DECREF(kwargs);
    return sub_enum_type;
}



static PyObject *make_policy_type(PyObject *module, PyObject *enum_module) {
    size_t value_size = 2;
    const char * values[value_size] = {"AlphaZero", "RPO"};
    return make_enum(module, enum_module, values, value_size, "PolicyType");
}

static PyObject *make_q_value_solved(PyObject *module, PyObject *enum_module) {
    size_t value_size = 5;
    const char * values[value_size] = {"OneOverCounts", "CountOverCounts", "One", "OneOverVirtualCounts", "OneOverCountsNoFPU", "CountOverCountsNoFPU"};
    return make_enum(module, enum_module, values, value_size, "QValueSolved");
}

static PyObject *HTPSError;

static PyMethodDef htps_methods[] = {
    {NULL, NULL, 0, NULL}
};

static PyModuleDef htps_module = {
    PyModuleDef_HEAD_INIT,
    "htps",
    "HyperTreeProofSearch",
    -1,
    htps_methods,
};

// static PyObject *PolicyType_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
//     htps::PolicyType *self;
//     self = (htps::PolicyType *)type->tp_alloc(type, 0);
//     if (!self) {
//         PyErr_SetString(PyExc_MemoryError, "could not allocate memory");
//         return NULL;
//     }
//     return (PyObject *) self;
// }
//
// static int PolicyType_init(htps::PolicyType *self, PyObject *args, PyObject *kwargs) {
//     int value = 0;
//     if (!PyArg_ParseTuple(args, "i", &value)) {
//         return -1;
//     }
//     *self = (htps::PolicyType) value;
//     return 0;
// }
//
// static PyMethodDef PolicyType_methods[] = { //
//     {NULL, NULL, 0, NULL}};
//
// static PyTypeObject PolicyTypeType = {
//     PyObject_HEAD_INIT(NULL, 0).tp_name = "htps.PolicyType",
//     .tp_doc = "Policy type",
//     .tp_basicsize = sizeof(htps::PolicyType),
//     .tp_flags = Py_TPFLAGS_DEFAULT,
//     .tp_methods = PolicyType_methods,
//     .tp_new = (newfunc)PolicyType_new,
//     .tp_init = (initproc)PolicyType_init,
// };

static PyObject *Params_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    auto *self = (htps::htps_params *) type->tp_alloc(type, 0);
    if (!self) {
        PyErr_SetString(PyExc_MemoryError, "could not allocate memory");
        return NULL;
    }
    return (PyObject *) self;
}

static PyObject *Params_init(PyObject *self, PyObject *args, PyObject *kwargs) {
    auto *params = (htps::htps_params *) self;
    double exploration, depth_penalty, tactic_init_value, policy_temperature, effect_subsampling_rate, critic_subsampling_rate;
    size_t num_expansions, succ_expansions, count_threshold, virtual_loss;
    bool early_stopping, no_critic, backup_once, backup_one_for_solved, tactic_p_threshold, tactic_sample_q_conditiong, only_learn_best_tactics, early_stopping_solved_if_root_not_proven;
    htps::PolicyType type;
    htps::QValueSolved q_value_solved;
    htps::Metric metric;
    htps::NodeMask node_mask;

}

// static PyTypeObject HTPSParamsType = {
//     PyObject_HEAD_INIT(NULL).tp_name = "htps.SearchParams",
//     .tp_doc = "Parameters for HyperTreeProofSearch",
//     .tp_basicsize = sizeof(htps::htps_params),
//     .tp_new = Params_new
// };

PyMODINIT_FUNC
PyInit_htps(void) {
    PyObject *m = PyModule_Create(&htps_module);
    if (m == NULL) {
        Py_XDECREF(m);
        return NULL;
    }

    HTPSError = PyErr_NewException("htps.error", NULL, NULL);
    if (PyModule_AddObjectRef(m, "error", HTPSError) < 0) {
        Py_CLEAR(HTPSError);
        Py_DECREF(m);
        return NULL;
    }

    PyObject *enum_mod = PyImport_ImportModule("enum");
    if (enum_mod == NULL) {
        Py_DECREF(m);
        Py_XDECREF(enum_mod);
        return NULL;
    }
    PyObject *policy_type = make_policy_type(m, enum_mod);
    if (policy_type == NULL) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_XDECREF(policy_type);
        return NULL;
    }

    if (PyModule_AddObject(m, "PolicyType", policy_type) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        return NULL;
    }

    return m;
}


int main() {
    return 0;
}
