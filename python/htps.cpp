#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "../src/graph/htps.h"
#include <descrobject.h>

static PyObject *PolicyTypeEnum = NULL;
static PyObject *QValueSolvedEnum = NULL;
static PyObject *MetricEnum = NULL;
static PyObject *NodeMaskEnum = NULL;

static PyObject *make_enum(PyObject *module, PyObject *enum_module, const char **values, size_t value_size,
                           const char *name) {
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
    if (PolicyTypeEnum != NULL)
        return PolicyTypeEnum;
    size_t value_size = 2;
    const char *values[value_size] = {"AlphaZero", "RPO"};
    PolicyTypeEnum = make_enum(module, enum_module, values, value_size, "PolicyType");
    return PolicyTypeEnum;
}

static PyObject *make_q_value_solved(PyObject *module, PyObject *enum_module) {
    if (QValueSolvedEnum != NULL)
        return QValueSolvedEnum;
    size_t value_size = 5;
    const char *values[value_size] = {
        "OneOverCounts", "CountOverCounts", "One", "OneOverVirtualCounts", "OneOverCountsNoFPU", "CountOverCountsNoFPU"
    };
    QValueSolvedEnum = make_enum(module, enum_module, values, value_size, "QValueSolved");
    return QValueSolvedEnum;
}

static PyObject *make_node_mask(PyObject *module, PyObject *enum_module) {
    if (NodeMaskEnum != NULL)
        return NodeMaskEnum;
    size_t value_size = 5;
    const char *values[value_size] = {"NoMask", "Solving", "Proof", "MinimalProof", "MinimalProofSolving"};
    NodeMaskEnum =  make_enum(module, enum_module, values, value_size, "NodeMask");
    return NodeMaskEnum;
}

static PyObject *make_metric(PyObject *module, PyObject *enum_module) {
    if (MetricEnum != NULL)
        return MetricEnum;
    size_t value_size = 3;
    const char *values[value_size] = {"Depth", "Size", "Time"};
    MetricEnum = make_enum(module, enum_module, values, value_size, "Metric");
    return MetricEnum;
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

int get_enum_value(PyObject *obj, const char *name) {
    int value;
    if (PyLong_Check(obj)) {
        value = (int) PyLong_AsLong(obj);
    } else {
        PyObject * val = PyObject_GetAttrString(obj, "value");
        if (val == NULL) {
            PyErr_Format(PyExc_TypeError, "%s must be an int or an enum with a 'value' attribute", name);
            return -1;
        }
        if (!PyLong_Check(val)) {
            Py_DECREF(val);
            PyErr_Format(PyExc_TypeError, "The 'value' attribute of %s must be an int", name);
            return -1;
        }
        value = (int) PyLong_AsLong(val);
        Py_DECREF(val);
    }
return value;
}




static int Params_init(PyObject *self, PyObject *args, PyObject *kwargs) {
    auto *params = (htps::htps_params *) self;
    double exploration, depth_penalty, tactic_init_value, policy_temperature, effect_subsampling_rate,
            critic_subsampling_rate;
    size_t num_expansions, succ_expansions, count_threshold, virtual_loss;
    int early_stopping, no_critic, backup_once, backup_one_for_solved, tactic_p_threshold, tactic_sample_q_conditioning,
            only_learn_best_tactics, early_stopping_solved_if_root_not_proven;
    PyObject *policy_obj, *q_value_solved_obj, *metric_obj, *node_mask_obj;
    static char *kwlist[] = {
        "exploration",
        "policy_type",
        "num_expansions",
        "succ_expansions",
        "early_stopping",
        "no_critic",
        "backup_once",
        "backup_one_for_solved",
        "depth_penalty",
        "count_threshold",
        "tactic_p_threshold",
        "tactic_sample_q_conditioning",
        "only_learn_best_tactics",
        "tactic_init_value",
        "q_value_solved",
        "policy_temperature",
        "metric",
        "node_mask",
        "effect_subsampling_rate",
        "critic_subsampling_rate",
        "early_stopping_solved_if_root_not_proven",
        "virtual_loss",
        NULL
    };
    const char *format = "dO" "nn" "pppp" "d" "n" "ppp" "d" "O" "d" "OO" "dd" "pn";
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, format, kwlist,
                                     &exploration,
                                     &policy_obj,
                                     &num_expansions,
                                     &succ_expansions,
                                     &early_stopping,
                                     &no_critic,
                                     &backup_once,
                                     &backup_one_for_solved,
                                     &depth_penalty,
                                     &count_threshold,
                                     &tactic_p_threshold,
                                     &tactic_sample_q_conditioning,
                                     &only_learn_best_tactics,
                                     &tactic_init_value,
                                     &q_value_solved_obj,
                                     &policy_temperature,
                                     &metric_obj,
                                     &node_mask_obj,
                                     &effect_subsampling_rate,
                                     &critic_subsampling_rate,
                                     &early_stopping_solved_if_root_not_proven,
                                     &virtual_loss)) {
        return -1;
    }


    params->exploration = exploration;
    int enum_value = get_enum_value(policy_obj, "PolicyType");
    if (enum_value < 0) {
        return -1;
    }
    params->policy_type = (htps::PolicyType) enum_value;
    params->num_expansions = num_expansions;
    params->succ_expansions = succ_expansions;
    params->early_stopping = early_stopping ? true : false;
    params->no_critic = no_critic ? true : false;
    params->backup_once = backup_once ? true : false;
    params->backup_one_for_solved = backup_one_for_solved ? true : false;
    params->depth_penalty = depth_penalty;
    params->count_threshold = count_threshold;
    params->tactic_p_threshold = tactic_p_threshold ? true : false;
    params->tactic_sample_q_conditioning = tactic_sample_q_conditioning ? true : false;
    params->only_learn_best_tactics = only_learn_best_tactics ? true : false;
    params->tactic_init_value = tactic_init_value;
    enum_value = get_enum_value(q_value_solved_obj, "QValueSolved");
    if (enum_value < 0) {
        return -1;
    }
    params->q_value_solved = (htps::QValueSolved) enum_value;
    params->policy_temperature = policy_temperature;
    params->metric = (htps::Metric) get_enum_value(metric_obj, "Metric");
    params->node_mask = (htps::NodeMask) get_enum_value(node_mask_obj, "NodeMask");
    params->effect_subsampling_rate = effect_subsampling_rate;
    params->critic_subsampling_rate = critic_subsampling_rate;
    params->early_stopping_solved_if_root_not_proven = early_stopping_solved_if_root_not_proven ? true : false;
    params->virtual_loss = virtual_loss;
    return 0;
}

static PyObject* Params_get_policy_type(PyObject* self, void* closure) {
    htps::htps_params *params = (htps::htps_params*) self;
    int value = params->policy_type;
    // Call the Python enum type with the integer value.
    return PyObject_CallFunction(PolicyTypeEnum, "i", value);
}

static int Params_set_policy_type(PyObject * self, PyObject * value, void* closure) {
    htps::htps_params *params = (htps::htps_params*) self;
    int enum_value = get_enum_value(value, "PolicyType");
    // Check that it is a valid enum value
    PyObject *enum_obj = PyObject_CallFunction(PolicyTypeEnum, "i", enum_value);
    if (!enum_obj) {
        Py_XDECREF(enum_obj);
        return -1;
    }
    Py_DECREF(enum_obj);
    params->policy_type = (htps::PolicyType) enum_value;
    return 0;
}

static PyObject* Params_get_q_value_solved(PyObject* self, void* closure) {
    htps::htps_params *params = (htps::htps_params*) self;
    int value = params->q_value_solved;
    return PyObject_CallFunction(QValueSolvedEnum, "i", value);
}

static int Params_set_q_value_solved(PyObject * self, PyObject * value, void* closure) {
    htps::htps_params *params = (htps::htps_params*) self;
    int enum_value = get_enum_value(value, "QValueSolved");
    PyObject *enum_obj = PyObject_CallFunction(QValueSolvedEnum, "i", enum_value);
    if (!enum_obj) {
        Py_XDECREF(enum_obj);
        return -1;
    }
    Py_DECREF(enum_obj);
    params->q_value_solved = (htps::QValueSolved) enum_value;
    return 0;
}

static PyObject* Params_get_metric(PyObject* self, void* closure) {
    htps::htps_params *params = (htps::htps_params*) self;
    int value = params->metric;
    return PyObject_CallFunction(MetricEnum, "i", value);
}

static int Params_set_metric(PyObject * self, PyObject * value, void* closure) {
    htps::htps_params *params = (htps::htps_params*) self;
    int enum_value = get_enum_value(value, "Metric");
    PyObject *enum_obj = PyObject_CallFunction(MetricEnum, "i", enum_value);
    if (!enum_obj) {
        Py_XDECREF(enum_obj);
        return -1;
    }
    Py_DECREF(enum_obj);
    params->metric = (htps::Metric) enum_value;
    return 0;
}

static PyObject* Params_get_node_mask(PyObject* self, void* closure) {
    htps::htps_params *params = (htps::htps_params*) self;
    int value = params->node_mask;
    return PyObject_CallFunction(NodeMaskEnum, "i", value);
}

static int Params_set_node_mask(PyObject * self, PyObject * value, void* closure) {
    htps::htps_params *params = (htps::htps_params*) self;
    int enum_value = get_enum_value(value, "NodeMask");
    PyObject *enum_obj = PyObject_CallFunction(NodeMaskEnum, "i", enum_value);
    if (!enum_obj) {
        Py_XDECREF(enum_obj);
        return -1;
    }
    Py_DECREF(enum_obj);
    params->node_mask = (htps::NodeMask) enum_value;
    return 0;
}

static PyMemberDef Params_members[] = {
    {"exploration",         Py_T_DOUBLE,  offsetof(htps::htps_params, exploration),         0, "exploration parameter"},
    {"num_expansions",      Py_T_ULONG,   offsetof(htps::htps_params, num_expansions),      0, "max number of expansions"},
    {"succ_expansions",     Py_T_ULONG,   offsetof(htps::htps_params, succ_expansions),     0, "expansions per batch"},
    {"early_stopping",      Py_T_BOOL,     offsetof(htps::htps_params, early_stopping),      0, "early stopping flag"},
    {"no_critic",           Py_T_BOOL,     offsetof(htps::htps_params, no_critic),           0, "no critic flag"},
    {"backup_once",         Py_T_BOOL,     offsetof(htps::htps_params, backup_once),         0, "backup once flag"},
    {"backup_one_for_solved",Py_T_BOOL,    offsetof(htps::htps_params, backup_one_for_solved),0, "backup one for solved flag"},
    {"depth_penalty",       Py_T_DOUBLE,  offsetof(htps::htps_params, depth_penalty),       0, "depth penalty"},
    {"count_threshold",     Py_T_ULONG,   offsetof(htps::htps_params, count_threshold),     0, "count threshold"},
    {"tactic_p_threshold",  Py_T_BOOL,     offsetof(htps::htps_params, tactic_p_threshold),  0, "tactic p threshold flag"},
    {"tactic_sample_q_conditioning", Py_T_BOOL, offsetof(htps::htps_params, tactic_sample_q_conditioning), 0, "tactic sample q conditioning flag"},
    {"only_learn_best_tactics", Py_T_BOOL, offsetof(htps::htps_params, only_learn_best_tactics), 0, "only learn best tactics flag"},
    {"tactic_init_value",   Py_T_DOUBLE,  offsetof(htps::htps_params, tactic_init_value),   0, "tactic initial value"},
    {"policy_temperature",  Py_T_DOUBLE,  offsetof(htps::htps_params, policy_temperature),  0, "policy temperature"},
    {"effect_subsampling_rate", Py_T_DOUBLE, offsetof(htps::htps_params, effect_subsampling_rate), 0, "effect subsampling rate"},
    {"critic_subsampling_rate", Py_T_DOUBLE, offsetof(htps::htps_params, critic_subsampling_rate), 0, "critic subsampling rate"},
    {"early_stopping_solved_if_root_not_proven", Py_T_BOOL, offsetof(htps::htps_params, early_stopping_solved_if_root_not_proven), 0, "early stopping solved flag"},
    {"virtual_loss",        Py_T_ULONG,   offsetof(htps::htps_params, virtual_loss),        0, "virtual loss"},
    {NULL}
};

static PyGetSetDef Params_getsetters[] = {
    {"policy_type",     (getter)Params_get_policy_type,     (setter)Params_set_policy_type,     "PolicyType enum", NULL},
    {"q_value_solved",  (getter)Params_get_q_value_solved,  (setter)Params_set_q_value_solved,  "QValueSolved enum", NULL},
    {"metric",          (getter)Params_get_metric,          (setter)Params_set_metric,          "Metric enum", NULL},
    {"node_mask",       (getter)Params_get_node_mask,       (setter)Params_set_node_mask,       "NodeMask enum", NULL},
    {NULL}
};

static PyMethodDef Params_methods[] = {
    {NULL, NULL, 0, NULL}
};

static PyTypeObject ParamsType = {
    PyObject_HEAD_INIT(NULL) "htps.SearchParams",
    sizeof(htps::htps_params),
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    Py_TPFLAGS_DEFAULT,
    "Parameters for HyperTreeProofSearch",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    Params_methods,
    Params_members,
    Params_getsetters,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    (initproc)Params_init,
    NULL,
    (newfunc)Params_new,
};

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

    PyObject *q_value_solved = make_q_value_solved(m, enum_mod);
    if (q_value_solved == NULL) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_XDECREF(q_value_solved);
        return NULL;
    }
    if (PyModule_AddObject(m, "QValueSolved", q_value_solved) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        return NULL;
    }

    PyObject *node_mask = make_node_mask(m, enum_mod);
    if (node_mask == NULL) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_XDECREF(node_mask);
        return NULL;
    }
    if (PyModule_AddObject(m, "NodeMask", node_mask) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        return NULL;
    }

    PyObject *metric = make_metric(m, enum_mod);
    if (metric == NULL) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_XDECREF(metric);
        return NULL;
    }
    if (PyModule_AddObject(m, "Metric", metric) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        return NULL;
    }

    if (PyType_Ready(&ParamsType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        return NULL;
    }
    Py_INCREF(&ParamsType);
    if (PyModule_AddObject(m, "SearchParams", (PyObject *)&ParamsType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_XDECREF(&ParamsType);
        return NULL;
    }


    return m;
}


int main() {
    return 0;
}
