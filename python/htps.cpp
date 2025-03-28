#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
#include "../src/graph/htps.h"
#include "../src/graph/lean.h"

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
    {"exploration",         T_DOUBLE,  offsetof(htps::htps_params, exploration),         0, "exploration parameter"},
    {"num_expansions",      T_ULONG,   offsetof(htps::htps_params, num_expansions),      0, "max number of expansions"},
    {"succ_expansions",     T_ULONG,   offsetof(htps::htps_params, succ_expansions),     0, "expansions per batch"},
    {"early_stopping",      T_BOOL,     offsetof(htps::htps_params, early_stopping),      0, "early stopping flag"},
    {"no_critic",           T_BOOL,     offsetof(htps::htps_params, no_critic),           0, "no critic flag"},
    {"backup_once",         T_BOOL,     offsetof(htps::htps_params, backup_once),         0, "backup once flag"},
    {"backup_one_for_solved",T_BOOL,    offsetof(htps::htps_params, backup_one_for_solved),0, "backup one for solved flag"},
    {"depth_penalty",       T_DOUBLE,  offsetof(htps::htps_params, depth_penalty),       0, "depth penalty"},
    {"count_threshold",     T_ULONG,   offsetof(htps::htps_params, count_threshold),     0, "count threshold"},
    {"tactic_p_threshold",  T_BOOL,     offsetof(htps::htps_params, tactic_p_threshold),  0, "tactic p threshold flag"},
    {"tactic_sample_q_conditioning", T_BOOL, offsetof(htps::htps_params, tactic_sample_q_conditioning), 0, "tactic sample q conditioning flag"},
    {"only_learn_best_tactics", T_BOOL, offsetof(htps::htps_params, only_learn_best_tactics), 0, "only learn best tactics flag"},
    {"tactic_init_value",   T_DOUBLE,  offsetof(htps::htps_params, tactic_init_value),   0, "tactic initial value"},
    {"policy_temperature",  T_DOUBLE,  offsetof(htps::htps_params, policy_temperature),  0, "policy temperature"},
    {"effect_subsampling_rate", T_DOUBLE, offsetof(htps::htps_params, effect_subsampling_rate), 0, "effect subsampling rate"},
    {"critic_subsampling_rate", T_DOUBLE, offsetof(htps::htps_params, critic_subsampling_rate), 0, "critic subsampling rate"},
    {"early_stopping_solved_if_root_not_proven", T_BOOL, offsetof(htps::htps_params, early_stopping_solved_if_root_not_proven), 0, "early stopping solved flag"},
    {"virtual_loss",        T_ULONG,   offsetof(htps::htps_params, virtual_loss),        0, "virtual loss"},
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

static PyObject * Hypothesis_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    auto *self = (htps::hypothesis*) type->tp_alloc(type, 0);
    if (!self) {
        PyErr_SetString(PyExc_MemoryError, "could not allocate memory");
        return NULL;
    }
    new (&(self->identifier)) std::string();
    new (&(self->type)) std::string();
    return (PyObject *) self;
}

static int Hypothesis_init(PyObject *self, PyObject *args, PyObject *kwargs) {
    auto* h = (htps::hypothesis *) self;
    const char *identifier = nullptr;
    const char *type_str = nullptr;
    static const char *kwlist[] = { "identifier", "value", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss", const_cast<char**>(kwlist), &identifier, &type_str))
        return -1;
    h->identifier = identifier;
    h->type = type_str;
    return 0;
}

static PyMemberDef HypothesisMembers[] = {
    {"identifier", T_STRING, offsetof(htps::hypothesis, identifier), READONLY, "Identifier for a hypothesis"},
    {"value", T_STRING, offsetof(htps::hypothesis, type), READONLY, "Type of the hypothesis"},
    {NULL}
};

static PyTypeObject HypothesisType = {
PyObject_HEAD_INIT(NULL) "htps.Hypothesis",
    sizeof(htps::hypothesis),
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
    "A single hypothesis for HyperTreeProofSearch",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    HypothesisMembers,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    (initproc)Hypothesis_init,
    NULL,
    (newfunc)Hypothesis_new,
};



static PyObject *Tactic_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    auto *self = (htps::lean_tactic*) type->tp_alloc(type, 0);
    if (!self) {
        PyErr_SetString(PyExc_MemoryError, "could not allocate memory");
        return NULL;
    }
    new (&(self->unique_string)) std::string();
    return (PyObject *) self;
}

static int Tactic_init(PyObject *self, PyObject *args, PyObject *kwargs) {
    auto *t = (htps::lean_tactic *) self;
    const char *unique_str = nullptr;
    int is_valid;
    size_t duration;
    static char *kwlist[] = { (char*)"unique_string", (char*)"is_valid", (char*)"duration", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "spn", kwlist, &unique_str, &is_valid, &duration))
        return -1;
    t->unique_string = unique_str;
    t->is_valid = is_valid ? true : false;
    t->duration = duration;
    return 0;
}


static PyMemberDef TacticMembers[] = {
    {"unique_string", T_STRING, offsetof(htps::lean_tactic, unique_string), READONLY, "Unique identifier for a tactic"},
    {"is_valid", T_BOOL, offsetof(htps::lean_tactic, is_valid), READONLY, "Whether the tactic is valid or not"},
    {"duration", T_LONG, offsetof(htps::lean_tactic, duration), READONLY, "Duration in milliseconds"},
    {NULL}
};


static PyTypeObject TacticType = {
    PyObject_HEAD_INIT(NULL) "htps.Tactic",
    sizeof(htps::lean_tactic),
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
    "A single tactic for HyperTreeProofSearch",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    TacticMembers,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    (initproc)Tactic_init,
    NULL,
    (newfunc)Tactic_new,
    };


static PyObject *Context_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    auto *self = (htps::lean_context*) type->tp_alloc(type, 0);
    if (!self) {
        PyErr_SetString(PyExc_MemoryError, "could not allocate memory");
        return NULL;
    }
    new (&(self->namespaces)) std::set<std::string>();
    return (PyObject *) self;
}

static int Context_init(PyObject *self, PyObject *args, PyObject *kwargs) {
    auto *c = (htps::lean_context *) self;
    static char *kwlist[] = { (char*)"namespaces", NULL };
    PyObject *namespace_set, *iterator;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &namespace_set))
        return -1;
    iterator = PyObject_GetIter(namespace_set);
    if (!iterator) {
        PyErr_SetString(PyExc_TypeError, "namespaces must be iterable");
        return -1;
    }

    std::set<std::string> namespaces;
    PyObject *item;
    while ((item = PyIter_Next(iterator))) {
        if (!PyUnicode_Check(item)) {
            PyErr_SetString(PyExc_TypeError, "each namespace must be a string");
            Py_DECREF(item);
            Py_DECREF(iterator);
            return -1;
        }
        const char *namespace_str = PyUnicode_AsUTF8(item);
        if (namespace_str)
            namespaces.insert(namespace_str);
        Py_DECREF(item);
    }
    Py_DECREF(iterator);
    c->namespaces = std::move(namespaces);
    return 0;
}

static PyObject* Context_get_namespaces(PyObject *self, void* closure) {
    auto *context = (htps::lean_context *) self;
    PyObject *py_list = PyList_New(context->namespaces.size());
    if (!py_list)
        return NULL;
    size_t i = 0;
    for (const auto &ns : context->namespaces) {
        PyObject *py_str = PyUnicode_FromString(ns.c_str());
        if (!py_str) {
            Py_DECREF(py_list);
            return NULL;
        }
        PyList_SetItem(py_list, i++, py_str);
    }
    return py_list;
}

static int Context_set_namespaces(PyObject *self, PyObject *value, void *closure) {
    auto *context = (htps::lean_context *) self;
    PyObject * iterator = PyObject_GetIter(value);
    if (!iterator) {
        PyErr_SetString(PyExc_TypeError, "namespaces must be iterable");
        return -1;
    }

    std::set<std::string> namespaces;
    PyObject *item;
    while ((item = PyIter_Next(iterator))) {
        if (!PyUnicode_Check(item)) {
            PyErr_SetString(PyExc_TypeError, "each namespace must be a string");
            Py_DECREF(item);
            Py_DECREF(iterator);
            return -1;
        }
        const char *namespace_str = PyUnicode_AsUTF8(item);
        if (namespace_str)
            namespaces.insert(namespace_str);
        Py_DECREF(item);
    }
    Py_DECREF(iterator);
    context->namespaces = std::move(namespaces);
    return 0;
}

static PyGetSetDef Context_getsetters[] = {
    {"namespaces", (getter)Context_get_namespaces, (setter)Context_set_namespaces, "Namespaces set", NULL},
    {NULL}
};

static void Context_dealloc(PyObject* self) {
    auto *ctx = (htps::lean_context*) self;
     ctx->namespaces.~set<std::string>();
    Py_TYPE(self)->tp_free(self);
}

static PyTypeObject ContextType = {
    PyObject_HEAD_INIT(NULL) "htps.Context",
sizeof(htps::lean_context),
0,
Context_dealloc,
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
"The underlying context for HyperTreeProofSearch",
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
Context_getsetters,
NULL,
NULL,
NULL,
NULL,
NULL,
(initproc)Context_init,
NULL,
(newfunc)Context_new,
};


static PyObject * Theorem_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    auto *self = (htps::lean_theorem *) type->tp_alloc(type, 0);
    if (!self) {
        PyErr_SetString(PyExc_MemoryError, "could not allocate memory");
        return NULL;
    }
    new (&(self->conclusion)) std::string();
    new (&(self->hypotheses)) std::vector<htps::hypothesis>();
    new (&(self->unique_string)) std::string();
    new (&(self->context)) htps::lean_context();
    new (&(self->past_tactics)) std::vector<htps::lean_tactic>();

    return (PyObject *) self;
}

static int Theorem_init(PyObject* self, PyObject * args, PyObject *kwargs) {
    auto *t = (htps::lean_theorem *) self;
    const char *unique_str = nullptr;
    const char *conclusion = nullptr;
    PyObject *context, *hypotheses, *past_tactics;
    static char *kwlist[] = { (char*)"conclusion", (char*)"unique_string", (char*)"hypotheses", (char*)"context", (char*)"past_tactics", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ssOOO", kwlist, &conclusion, &unique_str, &hypotheses, &context, &past_tactics))
        return -1;
    t->unique_string = unique_str;
    t->conclusion = conclusion;

    if (!PyObject_TypeCheck(context, &ContextType)) {
        PyErr_SetString(PyExc_TypeError, "context must be a Context object");
        return -1;
    }
    auto *c_ctx = (htps::lean_context*) context;
    // Copy underlying Lean context
    t->set_context(*c_ctx);

    PyObject *iterator = PyObject_GetIter(hypotheses);
    if (!iterator) {
    PyErr_SetString(PyExc_TypeError, "hypotheses must be iterable!");
    return -1;
    }
    std::vector<htps::hypothesis> theses;
    PyObject *item;
    t->hypotheses.clear();
    while ((item = PyIter_Next(iterator))) {
        if (!PyObject_TypeCheck(item, &HypothesisType)) {
            PyErr_SetString(PyExc_TypeError, "each hypothesis must be a Hypothesis object");
            Py_DECREF(item);
            Py_DECREF(iterator);
            return -1;
        }
        auto *h = (htps::hypothesis*) item;
        theses.push_back(*h);
        Py_DECREF(item);
    }
    Py_DECREF(iterator);
    t->hypotheses = theses;

    iterator = PyObject_GetIter(past_tactics);
    if (!iterator) {
        PyErr_SetString(PyExc_TypeError, "past_tactics must be iterable!");
        return -1;
    }
    std::vector<htps::lean_tactic> tacs;
    t->reset_tactics();
    while ((item = PyIter_Next(iterator))) {
        if (!PyObject_TypeCheck(item, &TacticType)) {
            PyErr_SetString(PyExc_TypeError, "each tactic must be a Tactic object");
            Py_DECREF(item);
            Py_DECREF(iterator);
            return -1;
        }
        auto *tactic = (htps::lean_tactic*) item;
        tacs.push_back(*tactic);
        Py_DECREF(item);
    }
    Py_DECREF(iterator);
    t->set_tactics(tacs);
    return 0;
}

static PyObject *Theorem_get_context(PyObject *self, void *closure) {
    auto *thm = (htps::lean_theorem *) self;
    auto *new_ctx = (htps::lean_context *) Context_new(&ContextType, NULL, NULL);
    if (!new_ctx)
        return PyErr_NoMemory();
    new_ctx->namespaces = thm->context.namespaces;
    return (PyObject*) new_ctx;
}

static int Theorem_set_context(PyObject *self, PyObject *value, void *closure) {
    auto *thm = (htps::lean_theorem *) self;
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "new context is not set");
        return -1;
    }
    if (!PyObject_TypeCheck(value, &ContextType)) {
        PyErr_SetString(PyExc_TypeError, "context must be a Context object");
        return -1;
    }
    auto *new_ctx = (htps::lean_context *) value;
    thm->set_context(*new_ctx);
    return 0;
}

static PyObject *Theorem_get_hypotheses(PyObject *self, void *closure) {
    auto *thm = (htps::lean_theorem *) self;
    PyObject *list = PyList_New(thm->hypotheses.size());
    if (!list)
        return PyErr_NoMemory();

    for (size_t i = 0; i < thm->hypotheses.size(); i++) {
        // Call the class to get a new object
        PyObject *args = Py_BuildValue("ss", thm->hypotheses[i].identifier.c_str(), thm->hypotheses[i].type.c_str());
        if (!args) {
            return NULL;
        }
        PyObject *hyp_obj = PyObject_CallObject((PyObject *) &HypothesisType, args);
        if (!hyp_obj) {
            Py_DECREF(list);
            Py_DECREF(args);
            return NULL;
        }
        Py_DECREF(args);
        PyList_SET_ITEM(list, i, hyp_obj);
    }
    return list;
}


static int Theorem_set_hypotheses(PyObject *self, PyObject *value, void *closure) {
    auto *thm = (htps::lean_theorem *) self;
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "new hypotheses is not set");
        return -1;
    }
    PyObject *iterator = PyObject_GetIter(value);
    if (!iterator) {
        PyErr_SetString(PyExc_TypeError, "hypotheses must be an iterable");
        return -1;
    }

    std::vector<htps::hypothesis> new_hypotheses;
    PyObject *item;
    while ((item = PyIter_Next(iterator)) != NULL) {
        // Check that each element is a hypothesis
        if (!PyObject_TypeCheck(item, &HypothesisType)) {
            PyErr_SetString(PyExc_TypeError, "Each hypothesis must be a Hypothesis object");
            Py_DECREF(item);
            Py_DECREF(iterator);
            return -1;
        }
        auto *hyp = (htps::hypothesis*) item;
        new_hypotheses.push_back(*hyp);
        Py_DECREF(item);
    }
    Py_DECREF(iterator);
    thm->hypotheses = new_hypotheses;
    return 0;
}

static PyObject *Theorem_get_past_tactics(PyObject *self, void *closure) {
    auto *thm = (htps::lean_theorem *) self;
    PyObject *list = PyList_New(thm->past_tactics.size());
    if (!list)
        return PyErr_NoMemory();

    for (size_t i = 0; i < thm->past_tactics.size(); i++) {
        PyObject *args = Py_BuildValue("sOn", thm->past_tactics[i].unique_string.c_str(), thm->past_tactics[i].is_valid ? Py_True : Py_False, (Py_ssize_t) thm->past_tactics[i].duration);
        if (!args) {
            return NULL;
        }
        PyObject *tac_obj = PyObject_CallObject((PyObject *) &TacticType, args);
        if (!tac_obj) {
            Py_DECREF(list);
            Py_DECREF(args);
            return NULL;
        }
        Py_DECREF(args);
        PyList_SET_ITEM(list, i, tac_obj);
    }
    return list;
}

static int Theorem_set_past_tactics(PyObject *self, PyObject *value, void *closure) {
    auto *thm = (htps::lean_theorem *) self;
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "new past tactics is not set");
        return -1;
    }
    PyObject *iterator = PyObject_GetIter(value);
    if (!iterator) {
        PyErr_SetString(PyExc_TypeError, "past_tactics must be an iterable");
        return -1;
    }
    std::vector<htps::lean_tactic> new_tactics;
    PyObject *item;
    while ((item = PyIter_Next(iterator)) != NULL) {
        if (!PyObject_TypeCheck(item, &TacticType)) {
            PyErr_SetString(PyExc_TypeError, "Each tactic must be a Tactic object");
            Py_DECREF(item);
            Py_DECREF(iterator);
            return -1;
        }
        auto *tactic = (htps::lean_tactic*) item;
        new_tactics.push_back(*tactic);
        Py_DECREF(item);
    }
    Py_DECREF(iterator);
    thm->set_tactics(new_tactics);
    return 0;
}



static PyMemberDef Theorem_members[] = {
    {"conclusion", T_STRING, offsetof(htps::lean_theorem, conclusion), READONLY, "Conclusion of the theorem"},
    {"unique_string", T_STRING, offsetof(htps::lean_theorem, unique_string), READONLY, "Unique string of the theorem"},
    {NULL}
};

static PyGetSetDef Theorem_getsetters[] = {
    {"context", Theorem_get_context, Theorem_set_context, "Context of the theorem", NULL},
    {"hypotheses", Theorem_get_hypotheses, Theorem_set_hypotheses, "Hypotheses of theorem", NULL},
    {"past_tactics", Theorem_get_past_tactics, Theorem_set_past_tactics, "Past tactics of theorem", NULL},
    {NULL}
};

static PyTypeObject TheoremType = {
    PyObject_HEAD_INIT(NULL) "htps.Theorem",
sizeof(htps::lean_theorem),
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
"A single theorem for HyperTreeProofSearch",
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
Theorem_members,
Theorem_getsetters,
NULL,
NULL,
NULL,
NULL,
NULL,
(initproc)Theorem_init,
NULL,
(newfunc)Theorem_new,
};


PyObject* Theorem_NewFromShared(const std::shared_ptr<htps::theorem>& thm_ptr) {
    auto thm = std::dynamic_pointer_cast<htps::lean_theorem>(thm_ptr);
    PyObject *obj = Theorem_new(&TheoremType, NULL, NULL);
    if (obj == NULL)
        return NULL;
    auto *c_obj = (htps::lean_theorem *) obj;
    c_obj->conclusion = thm->conclusion;
    c_obj->unique_string  = thm->unique_string;
    c_obj->set_context(thm->context);
    c_obj->hypotheses = thm->hypotheses;
    c_obj->past_tactics = thm->past_tactics;
    return obj;
}

PyObject* Tactic_NewFromShared(const std::shared_ptr<htps::tactic>& tac_ptr) {
    auto tac = std::dynamic_pointer_cast<htps::lean_tactic>(tac_ptr);
    PyObject *obj = Tactic_new(&TacticType, NULL, NULL);
    if (obj == NULL)
        return NULL;
    auto *c_obj = (htps::lean_tactic *) obj;
    c_obj->unique_string = tac->unique_string;
    c_obj->is_valid = tac->is_valid;
    c_obj->duration = tac->duration;
    return obj;
}




static PyObject *EnvEffect_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    auto *self = (htps::env_effect *)type->tp_alloc(type, 0);
    if (self == NULL)
        return PyErr_NoMemory();
    auto shared_thm = static_pointer_cast<htps::lean_theorem>(std::make_shared<htps::lean_theorem>());
    auto shared_tactic = std::static_pointer_cast<htps::tactic>(std::make_shared<htps::lean_tactic>());
    self->goal = shared_thm;
    self->children = std::vector<std::shared_ptr<htps::theorem>>();
    self->tac = shared_tactic;
    return (PyObject *)self;
}

static int EnvEffect_init(PyObject *self, PyObject *args, PyObject *kwargs) {
    PyObject *py_goal, *py_tac, *py_children;
    static const char *kwlist[] = {"goal", "tactic", "children", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOO", const_cast<char**>(kwlist), &py_goal, &py_tac, &py_children))
        return -1;

    if (!PyObject_TypeCheck(py_goal, &TheoremType)) {
        PyErr_SetString(PyExc_TypeError, "goal must be a Theorem object");
        return -1;
    }
    if (!PyObject_TypeCheck(py_tac, &TacticType)) {
        PyErr_SetString(PyExc_TypeError, "tac must be a Tactic object");
        return -1;
    }
    PyObject *iterator = PyObject_GetIter(py_children);
    if (!iterator) {
        PyErr_SetString(PyExc_TypeError, "children must be iterable");
        return -1;
    }
    std::vector<std::shared_ptr<htps::theorem>> children_vec;
    PyObject *item;
    while ((item = PyIter_Next(iterator)) != NULL) {
        if (!PyObject_TypeCheck(item, &TheoremType)) {
            PyErr_SetString(PyExc_TypeError, "each child must be a Theorem object");
            Py_DECREF(item);
            Py_DECREF(iterator);
            return -1;
        }
        auto lean_thm = (htps::lean_theorem *) item;
        auto shared_thm = std::static_pointer_cast<htps::theorem>(std::make_shared<htps::lean_theorem>(*lean_thm));
        children_vec.push_back(shared_thm);
        Py_DECREF(item);
    }
    Py_DECREF(iterator);
    auto *eff = (htps::env_effect *) self;
    auto lean_thm = (htps::lean_theorem *) py_goal;
    auto shared_thm = std::static_pointer_cast<htps::theorem>(std::make_shared<htps::lean_theorem>(*lean_thm));
    auto lean_tactic = (htps::lean_tactic *) py_tac;
    auto shared_tactic = std::static_pointer_cast<htps::tactic>(std::make_shared<htps::lean_tactic>(*lean_tactic));
    eff->goal = shared_thm;
    eff->tac = shared_tactic;
    eff->children = children_vec;
    return 0;
}

static PyObject *EnvEffect_get_goal(PyObject *self, void *closure) {
    auto *effect = (htps::env_effect *) self;
    return Theorem_NewFromShared(effect->goal);
}

static int EnvEffect_set_goal(PyObject *self, PyObject *value, void *closure) {
    auto *effect = (htps::env_effect *) self;
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "no value provided for the goal attribute");
        return -1;
    }
    if (!PyObject_TypeCheck(value, &TheoremType)) {
        PyErr_SetString(PyExc_TypeError, "goal must be a Theorem object");
        return -1;
    }
    auto lean_thm = (htps::lean_theorem *) value;
    auto shared_thm = std::static_pointer_cast<htps::theorem>(std::make_shared<htps::lean_theorem>(*lean_thm));
    effect->goal = shared_thm;
    return 0;
}

static PyObject *EnvEffect_get_tac(PyObject *self, void *closure) {
    auto *effect = (htps::env_effect *) self;
    return Tactic_NewFromShared(effect->tac);
}

static int EnvEffect_set_tac(PyObject *self, PyObject *value, void *closure) {
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "no value provided for the tac attribute");
        return -1;
    }
    if (!PyObject_TypeCheck(value, &TacticType)) {
        PyErr_SetString(PyExc_TypeError, "tac must be a Tactic object");
        return -1;
    }
    auto *effect = (htps::env_effect *) self;
    auto lean_tac = (htps::lean_tactic *) value;
    auto shared_tac = std::static_pointer_cast<htps::tactic>(std::make_shared<htps::lean_tactic>(*lean_tac));
    effect->tac = shared_tac;
    return 0;
}


static PyObject *EnvEffect_get_children(PyObject *self, void *closure){
    auto *effect = (htps::env_effect *) self;
    const std::vector<std::shared_ptr<htps::theorem>> &children = effect->children;
    PyObject *list = PyList_New(children.size());
    if (!list)
        return PyErr_NoMemory();
    for (size_t i = 0; i < children.size(); i++) {
        PyObject *child_obj = Theorem_NewFromShared(children[i]);
        if (!child_obj) {
            Py_DECREF(list);
            return NULL;
        }
        PyList_SET_ITEM(list, i, child_obj);
    }
    return list;
}

static int EnvEffect_set_children(PyObject *self, PyObject *value, void *closure) {
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "no value provided for the children attribute");
        return -1;
    }
    PyObject *iter = PyObject_GetIter(value);
    if (!iter) {
        PyErr_SetString(PyExc_TypeError, "children must be iterable");
        return -1;
    }
    std::vector<std::shared_ptr<htps::theorem>> new_children;
    PyObject *item;
    while ((item = PyIter_Next(iter)) != NULL) {
        if (!PyObject_TypeCheck(item, &TheoremType)) {
            PyErr_SetString(PyExc_TypeError, "each child must be a Theorem object");
            Py_DECREF(item);
            Py_DECREF(iter);
            return -1;
        }
        auto lean_thm = (htps::lean_theorem *) item;
        auto shared_thm = std::static_pointer_cast<htps::theorem>(std::make_shared<htps::lean_theorem>(*lean_thm));
        new_children.push_back(shared_thm);
        Py_DECREF(item);
    }
    Py_DECREF(iter);
    auto *effect = (htps::env_effect *) self;
    effect->children = new_children;
    return 0;
}
static PyGetSetDef EnvEffect_getsetters[] = {
    {"goal", (getter)EnvEffect_get_goal, (setter)EnvEffect_set_goal, "Goal theorem", NULL},
    {"tactic", (getter)EnvEffect_get_tac, (setter)EnvEffect_set_tac, "Tactic", NULL},
    {"children", (getter)EnvEffect_get_children, (setter)EnvEffect_set_children, "Children theorems", NULL},
    {NULL}
};

static PyTypeObject EnvEffectType = {
    PyObject_HEAD_INIT(NULL) "htps.EnvEffect",
sizeof(htps::env_effect),
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
"A single EnvEffect, i.e. a tactic applied on a goal, for HyperTreeProofSearch",
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
EnvEffect_getsetters,
NULL,
NULL,
NULL,
NULL,
NULL,
(initproc)EnvEffect_init,
NULL,
(newfunc)EnvEffect_new,
};

PyObject* EnvEffect_NewFromShared(const std::shared_ptr<htps::env_effect>& eff) {
    PyObject *obj = EnvEffect_new(&EnvEffectType, NULL, NULL);
    if (obj == NULL)
        return NULL;
    auto *c_obj = (htps::env_effect *) obj;
    c_obj->goal = eff->goal;
    c_obj->tac = eff->tac;
    c_obj->children = eff->children;
    return obj;
}

typedef struct {
    PyObject_HEAD
    htps::env_expansion expansion;
} PyEnvExpansion;

static void EnvExpansion_dealloc(PyObject *self) {
    ((PyEnvExpansion *)self)->expansion.~env_expansion();
    Py_TYPE(self)->tp_free(self);
}

static PyObject *EnvExpansion_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    auto *self = (PyEnvExpansion *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return PyErr_NoMemory();
    }
    new (&(self->expansion)) htps::env_expansion();
    return (PyObject *)self;
}

static PyObject *EnvExpansion_get_thm(PyObject *self, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    return Theorem_NewFromShared(obj->expansion.thm);
}

static int EnvExpansion_set_thm(PyObject *self, PyObject *value, void *closure) {
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "received empty thm attribute");
        return -1;
    }
    if (!PyObject_TypeCheck(value, &TheoremType)) {
        PyErr_SetString(PyExc_TypeError, "thm must be a Theorem object");
        return -1;
    }
    auto *obj = (PyEnvExpansion *)self;
    auto *c_thm = (htps::lean_theorem *) value;
    obj->expansion.thm = std::static_pointer_cast<htps::theorem>(std::make_shared<htps::lean_theorem>(*c_thm));
    return 0;
}

static PyObject *EnvExpansion_get_expander_duration(PyObject *self, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    return PyLong_FromSize_t(obj->expansion.expander_duration);
}

static int EnvExpansion_set_expander_duration(PyObject *self, PyObject *value, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    if (!PyLong_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "expander_duration must be an integer");
        return -1;
    }
    obj->expansion.expander_duration = PyLong_AsSize_t(value);
    return 0;
}

static PyObject *EnvExpansion_get_generation_duration(PyObject *self, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    return PyLong_FromSize_t(obj->expansion.generation_duration);
}

static int EnvExpansion_set_generation_duration(PyObject *self, PyObject *value, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    if (!PyLong_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "generation_duration must be an integer");
        return -1;
    }
    obj->expansion.generation_duration = PyLong_AsSize_t(value);
    return 0;
}

static PyObject *EnvExpansion_get_env_durations(PyObject *self, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    const std::vector<size_t>& vec = obj->expansion.env_durations;
    PyObject *list = PyList_New(vec.size());
    if (!list)
        return PyErr_NoMemory();
    for (size_t i = 0; i < vec.size(); i++) {
        PyObject *num = PyLong_FromSize_t(vec[i]);
        if (!num) {
            Py_DECREF(list);
            return PyErr_NoMemory();
        }
        PyList_SET_ITEM(list, i, num);
    }
    return list;
}

static int EnvExpansion_set_env_durations(PyObject *self, PyObject *value, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    PyObject *iter = PyObject_GetIter(value);
    if (!iter) {
        PyErr_SetString(PyExc_TypeError, "env_durations must be iterable");
        return -1;
    }
    std::vector<size_t> vec;
    PyObject *item;
    while ((item = PyIter_Next(iter)) != NULL) {
        if (!PyLong_Check(item)) {
            PyErr_SetString(PyExc_TypeError, "each env duration must be an integer");
            Py_DECREF(item);
            Py_DECREF(iter);
            return -1;
        }
        vec.push_back(PyLong_AsSize_t(item));
        Py_DECREF(item);
    }
    Py_DECREF(iter);
    obj->expansion.env_durations = vec;
    return 0;
}

static PyObject *EnvExpansion_get_effects(PyObject *self, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    const std::vector<std::shared_ptr<htps::env_effect>> &effects = obj->expansion.effects;
    PyObject *list = PyList_New(effects.size());
    if (!list)
        return PyErr_NoMemory();
    for (size_t i = 0; i < effects.size(); i++) {
        PyObject *eff_obj = EnvEffect_NewFromShared(effects[i]);
        if (!eff_obj) {
            Py_DECREF(list);
            return NULL;
        }
        PyList_SET_ITEM(list, i, eff_obj);
    }
    return list;
}

static int EnvExpansion_set_effects(PyObject *self, PyObject *value, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    PyObject *iter = PyObject_GetIter(value);
    if (!iter) {
        PyErr_SetString(PyExc_TypeError, "effects must be iterable");
        return -1;
    }
    std::vector<std::shared_ptr<htps::env_effect>> effects;
    PyObject *item;
    while ((item = PyIter_Next(iter)) != NULL) {
        if (!PyObject_TypeCheck(item, &EnvEffectType)) {
            PyErr_SetString(PyExc_TypeError, "each effect must be an EnvEffect object");
            Py_DECREF(item);
            Py_DECREF(iter);
            return -1;
        }
        auto *eff = (htps::env_effect *) item;
        auto shared_eff = std::static_pointer_cast<htps::env_effect>(std::make_shared<htps::env_effect>(*eff));
        effects.push_back(shared_eff);
        Py_DECREF(item);
    }
    Py_DECREF(iter);
    obj->expansion.effects = effects;
    return 0;
}

static PyObject *EnvExpansion_get_log_critic(PyObject *self, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    return PyFloat_FromDouble(obj->expansion.log_critic);
}

static int EnvExpansion_set_log_critic(PyObject *self, PyObject *value, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    if (!PyFloat_Check(value) && !PyLong_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "log_critic must be a number");
        return -1;
    }
    obj->expansion.log_critic = PyFloat_AsDouble(value);
    return 0;
}

static PyObject *EnvExpansion_get_tactics(PyObject *self, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    const std::vector<std::shared_ptr<htps::tactic>> &tactics = obj->expansion.tactics;
    PyObject *list = PyList_New(tactics.size());
    if (!list)
        return PyErr_NoMemory();
    for (size_t i = 0; i < tactics.size(); i++) {
        PyObject *tac_obj = Tactic_NewFromShared(tactics[i]);
        if (!tac_obj) {
            Py_DECREF(list);
            return NULL;
        }
        PyList_SET_ITEM(list, i, tac_obj);
    }
    return list;
}

static int EnvExpansion_set_tactics(PyObject *self, PyObject *value, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    PyObject *iter = PyObject_GetIter(value);
    if (!iter) {
        PyErr_SetString(PyExc_TypeError, "tactics must be iterable");
        return -1;
    }
    std::vector<std::shared_ptr<htps::tactic>> tactics;
    PyObject *item;
    while ((item = PyIter_Next(iter)) != NULL) {
        if (!PyObject_TypeCheck(item, &TacticType)) {
            PyErr_SetString(PyExc_TypeError, "each tactic must be a Tactic object");
            Py_DECREF(item);
            Py_DECREF(iter);
            return -1;
        }
        auto *tac = (htps::lean_tactic *) item;
        auto shared_tac = std::static_pointer_cast<htps::tactic>(std::make_shared<htps::lean_tactic>(*tac));
        tactics.push_back(shared_tac);
        Py_DECREF(item);
    }
    Py_DECREF(iter);
    obj->expansion.tactics = tactics;
    return 0;
}

static PyObject *EnvExpansion_get_children_for_tactic(PyObject *self, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    const auto &outer = obj->expansion.children_for_tactic;
    PyObject *outer_list = PyList_New(outer.size());
    if (!outer_list)
        return PyErr_NoMemory();
    for (size_t i = 0; i < outer.size(); i++) {
        const auto &inner = outer[i];
        PyObject *inner_list = PyList_New(inner.size());
        if (!inner_list) {
            Py_DECREF(outer_list);
            return PyErr_NoMemory();
        }
        for (size_t j = 0; j < inner.size(); j++) {
            PyObject *thm_obj = Theorem_NewFromShared(inner[j]);
            if (!thm_obj) {
                Py_DECREF(inner_list);
                Py_DECREF(outer_list);
                return NULL;
            }
            PyList_SET_ITEM(inner_list, j, thm_obj);
        }
        PyList_SET_ITEM(outer_list, i, inner_list);
    }
    return outer_list;
}

static int EnvExpansion_set_children_for_tactic(PyObject *self, PyObject *value, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    PyObject *iter_outer = PyObject_GetIter(value);
    if (!iter_outer) {
        PyErr_SetString(PyExc_TypeError, "children_for_tactic must be iterable");
        return -1;
    }
    std::vector<std::vector<std::shared_ptr<htps::theorem>>> outer;
    PyObject *item_outer;
    while ((item_outer = PyIter_Next(iter_outer)) != NULL) {
        PyObject *iter_inner = PyObject_GetIter(item_outer);
        if (!iter_inner) {
            PyErr_SetString(PyExc_TypeError, "each element of children_for_tactic must be iterable");
            Py_DECREF(item_outer);
            Py_DECREF(iter_outer);
            return -1;
        }
        std::vector<std::shared_ptr<htps::theorem>> inner;
        PyObject *item_inner;
        while ((item_inner = PyIter_Next(iter_inner)) != NULL) {
            if (!PyObject_TypeCheck(item_inner, &TheoremType)) {
                PyErr_SetString(PyExc_TypeError, "each child must be a Theorem object");
                Py_DECREF(item_inner);
                Py_DECREF(iter_inner);
                Py_DECREF(item_outer);
                Py_DECREF(iter_outer);
                return -1;
            }
            auto *thm = (htps::lean_theorem *) item_inner;
            auto shared_thm = std::static_pointer_cast<htps::theorem>(std::make_shared<htps::lean_theorem>(*thm));
            inner.push_back(shared_thm);
            Py_DECREF(item_inner);
        }
        Py_DECREF(iter_inner);
        outer.push_back(inner);
        Py_DECREF(item_outer);
    }
    Py_DECREF(iter_outer);
    obj->expansion.children_for_tactic = outer;
    return 0;
}

static PyObject *EnvExpansion_get_priors(PyObject *self, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    const auto &priors = obj->expansion.priors;
    PyObject *list = PyList_New(priors.size());
    if (!list)
        return PyErr_NoMemory();
    for (size_t i = 0; i < priors.size(); i++) {
        PyObject *num = PyFloat_FromDouble(priors[i]);
        if (!num) {
            Py_DECREF(list);
            return PyErr_NoMemory();
        }
        PyList_SET_ITEM(list, i, num);
    }
    return list;
}

static int EnvExpansion_set_priors(PyObject *self, PyObject *value, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    PyObject *iter = PyObject_GetIter(value);
    if (!iter) {
        PyErr_SetString(PyExc_TypeError, "priors must be iterable");
        return -1;
    }
    std::vector<double> priors;
    PyObject *item;
    while ((item = PyIter_Next(iter)) != NULL) {
        if (!PyFloat_Check(item) && !PyLong_Check(item)) {
            PyErr_SetString(PyExc_TypeError, "each prior must be a number");
            Py_DECREF(item);
            Py_DECREF(iter);
            return -1;
        }
        priors.push_back(PyFloat_AsDouble(item));
        Py_DECREF(item);
    }
    Py_DECREF(iter);
    obj->expansion.priors = priors;
    return 0;
}

static PyObject *PyObject_from_string(const std::string &str) {
    return PyUnicode_FromString(str.c_str());
}

static std::string PyObject_to_string(PyObject *obj) {
    if (!PyUnicode_Check(obj)) {
        throw std::invalid_argument("Expected a Unicode object");
    }
    const char *utf8_str = PyUnicode_AsUTF8(obj);
    if (!utf8_str) {
        // PyUnicode_AsUTF8 returns NULL and sets an exception if conversion fails.
        throw std::runtime_error("Failed to convert PyObject to a UTF-8 string");
    }
    return {utf8_str};
}

static PyObject *EnvExpansion_get_error(PyObject *self, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    if (obj->expansion.error.has_value()) {
        return PyObject_from_string(obj->expansion.error.value());
    }

    Py_RETURN_NONE;
}

static int EnvExpansion_set_error(PyObject *self, PyObject *value, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    if (value == NULL || value == Py_None) {
        obj->expansion.error.reset();
        return 0;
    }
    if (!PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "error must be a string or None");
        return -1;
    }
    try {
        obj->expansion.error = PyObject_to_string(value);
    } catch (std::runtime_error &) {
        return -1;
    }

    return 0;
}

static PyObject *EnvExpansion_get_is_error(PyObject *self, void *closure) {
    auto *obj = (PyEnvExpansion *)self;
    if (obj->expansion.is_error())
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static int EnvExpansion_init(PyObject *self, PyObject *args, PyObject *kwargs) {
    static const char *kwlist_full[] = {
        "thm", "expander_duration", "generation_duration", "env_durations",
        "effects", "log_critic", "tactics", "children_for_tactic", "priors", NULL
    };
    static const char *kwlist_error[] = {
        "thm", "expander_duration", "generation_duration", "env_durations", "error", NULL
    };

    PyObject *py_thm = NULL, *py_env_durations = NULL;
    unsigned long expander_duration, generation_duration;
    // Two constructors, we try both
    {
        PyObject *py_effects = NULL, *py_tactics = NULL, *py_children = NULL, *py_priors = NULL;
        double log_critic;
        if (PyArg_ParseTupleAndKeywords(args, kwargs, "OkkOOdOOO", const_cast<char**>(kwlist_full),
                                        &py_thm, &expander_duration, &generation_duration, &py_env_durations,
                                        &py_effects, &log_critic, &py_tactics, &py_children, &py_priors)) {
            if (!PyObject_TypeCheck(py_thm, &TheoremType)) {
                PyErr_SetString(PyExc_TypeError, "thm must be a Theorem object");
                return -1;
            }
            auto *c_thm = (htps::lean_theorem *)py_thm;
            auto shared_thm = std::static_pointer_cast<htps::theorem>(std::make_shared<htps::lean_theorem>(*c_thm));

            std::vector<size_t> env_durations;
            PyObject *iter = PyObject_GetIter(py_env_durations);
            if (!iter) {
                PyErr_SetString(PyExc_TypeError, "env_durations must be iterable");
                return -1;
            }
            PyObject *item;
            while ((item = PyIter_Next(iter)) != NULL) {
                if (!PyLong_Check(item)) {
                    PyErr_SetString(PyExc_TypeError, "env_durations must contain integers");
                    Py_DECREF(item);
                    Py_DECREF(iter);
                    return -1;
                }
                env_durations.push_back(PyLong_AsSize_t(item));
                Py_DECREF(item);
            }
            Py_DECREF(iter);
            std::vector<std::shared_ptr<htps::env_effect>> effects;
            iter = PyObject_GetIter(py_effects);
            if (!iter) {
                PyErr_SetString(PyExc_TypeError, "effects must be iterable");
                return -1;
            }
            while ((item = PyIter_Next(iter)) != NULL) {
                if (!PyObject_TypeCheck(item, &EnvEffectType)) {
                    PyErr_SetString(PyExc_TypeError, "each effect must be an EnvEffect object");
                    Py_DECREF(item);
                    Py_DECREF(iter);
                    return -1;
                }
                auto *eff = (htps::env_effect *) item;
                auto shared_eff = std::static_pointer_cast<htps::env_effect>(std::make_shared<htps::env_effect>(*eff));
                effects.push_back(shared_eff);
                Py_DECREF(item);
            }
            Py_DECREF(iter);
            std::vector<std::shared_ptr<htps::tactic>> tactics;
            iter = PyObject_GetIter(py_tactics);
            if (!iter) {
                PyErr_SetString(PyExc_TypeError, "tactics must be iterable");
                return -1;
            }
            while ((item = PyIter_Next(iter)) != NULL) {
                if (!PyObject_TypeCheck(item, &TacticType)) {
                    PyErr_SetString(PyExc_TypeError, "each tactic must be a Tactic object");
                    Py_DECREF(item);
                    Py_DECREF(iter);
                    return -1;
                }
                auto *tac = (htps::lean_tactic *) item;
                auto shared_tac = std::static_pointer_cast<htps::tactic>(std::make_shared<htps::lean_tactic>(*tac));
                tactics.push_back(shared_tac);
                Py_DECREF(item);
            }
            Py_DECREF(iter);
            std::vector<std::vector<std::shared_ptr<htps::theorem>>> children_for_tactic;
            iter = PyObject_GetIter(py_children);
            if (!iter) {
                PyErr_SetString(PyExc_TypeError, "children_for_tactic must be iterable");
                return -1;
            }
            while ((item = PyIter_Next(iter)) != NULL) {
                std::vector<std::shared_ptr<htps::theorem>> inner;
                PyObject *iter_inner = PyObject_GetIter(item);
                if (!iter_inner) {
                    PyErr_SetString(PyExc_TypeError, "each element of children_for_tactic must be iterable");
                    Py_DECREF(item);
                    Py_DECREF(iter);
                    return -1;
                }
                PyObject *inner_item;
                while ((inner_item = PyIter_Next(iter_inner)) != NULL) {
                    if (!PyObject_TypeCheck(inner_item, &TheoremType)) {
                        PyErr_SetString(PyExc_TypeError, "each child must be a Theorem object");
                        Py_DECREF(inner_item);
                        Py_DECREF(iter_inner);
                        Py_DECREF(item);
                        Py_DECREF(iter);
                        return -1;
                    }
                    auto *child = (htps::lean_theorem *) inner_item;
                    auto shared_child = std::static_pointer_cast<htps::theorem>(std::make_shared<htps::lean_theorem>(*child));
                    inner.push_back(shared_child);
                    Py_DECREF(inner_item);
                }
                Py_DECREF(iter_inner);
                children_for_tactic.push_back(inner);
                Py_DECREF(item);
            }
            Py_DECREF(iter);
            std::vector<double> priors;
            iter = PyObject_GetIter(py_priors);
            if (!iter) {
                PyErr_SetString(PyExc_TypeError, "priors must be iterable");
                return -1;
            }
            while ((item = PyIter_Next(iter)) != NULL) {
                if (!PyFloat_Check(item) && !PyLong_Check(item)) {
                    PyErr_SetString(PyExc_TypeError, "each prior must be a number");
                    Py_DECREF(item);
                    Py_DECREF(iter);
                    return -1;
                }
                priors.push_back(PyFloat_AsDouble(item));
                Py_DECREF(item);
            }
            Py_DECREF(iter);
            new (&(((PyEnvExpansion *)self)->expansion)) htps::env_expansion(
                shared_thm, expander_duration, generation_duration, env_durations,
                effects, log_critic, tactics, children_for_tactic, priors
            );
            return 0;
        }
    }
    // Try the error one

        PyObject *py_error = NULL;
        if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OkkOO", const_cast<char**>(kwlist_error),
                                         &py_thm, &expander_duration, &generation_duration, &py_env_durations, &py_error))
        {
            return -1;
        }
        if (!PyObject_TypeCheck(py_thm, &TheoremType)) {
            PyErr_SetString(PyExc_TypeError, "thm must be a Theorem object");
            return -1;
        }
        auto *c_thm = (htps::lean_theorem *)py_thm;
        auto shared_thm = std::static_pointer_cast<htps::theorem>(std::make_shared<htps::lean_theorem>(*c_thm));
        std::vector<size_t> env_durations;
        PyObject *iter = PyObject_GetIter(py_env_durations);
        if (!iter) {
            PyErr_SetString(PyExc_TypeError, "env_durations must be iterable");
            return -1;
        }
        PyObject *item;
        while ((item = PyIter_Next(iter)) != NULL) {
            if (!PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "env_durations must contain integers");
                Py_DECREF(item);
                Py_DECREF(iter);
                return -1;
            }
            env_durations.push_back(PyLong_AsSize_t(item));
            Py_DECREF(item);
        }
        Py_DECREF(iter);

        std::string error;

        if (py_error == Py_None) {
            PyErr_SetString(PyExc_TypeError, "Error must be set for this constructor to work!");
            return -1;
        }
        if (!PyUnicode_Check(py_error)) {
                PyErr_SetString(PyExc_TypeError, "error must be a string or None");
                return -1;
        }
        try {
            error = PyObject_to_string(py_error);
        }
        catch (std::runtime_error &) {
            return -1;
        }
        new (&(((PyEnvExpansion *)self)->expansion)) htps::env_expansion(
            shared_thm, expander_duration, generation_duration, env_durations, error
        );
        return 0;

}

static PyGetSetDef EnvExpansion_getsetters[] = {
    {"thm", (getter)EnvExpansion_get_thm, (setter)EnvExpansion_set_thm, "Theorem for expansion", NULL},
    {"expander_duration", (getter)EnvExpansion_get_expander_duration, (setter)EnvExpansion_set_expander_duration, "Expander duration", NULL},
    {"generation_duration", (getter)EnvExpansion_get_generation_duration, (setter)EnvExpansion_set_generation_duration, "Generation duration", NULL},
    {"env_durations", (getter)EnvExpansion_get_env_durations, (setter)EnvExpansion_set_env_durations, "Environment durations", NULL},
    {"effects", (getter)EnvExpansion_get_effects, (setter)EnvExpansion_set_effects, "EnvEffects", NULL},
    {"log_critic", (getter)EnvExpansion_get_log_critic, (setter)EnvExpansion_set_log_critic, "Log critic value", NULL},
    {"tactics", (getter)EnvExpansion_get_tactics, (setter)EnvExpansion_set_tactics, "Tactics", NULL},
    {"children_for_tactic", (getter)EnvExpansion_get_children_for_tactic, (setter)EnvExpansion_set_children_for_tactic, "Children for each tactic", NULL},
    {"priors", (getter)EnvExpansion_get_priors, (setter)EnvExpansion_set_priors, "Priors", NULL},
    {"error", (getter)EnvExpansion_get_error, (setter)EnvExpansion_set_error, "Error string (optional)", NULL},
    {"is_error", (getter)EnvExpansion_get_is_error, NULL, "Returns True if error is set", NULL},
    {NULL}
};


static PyTypeObject EnvExpansionType = {
    PyObject_HEAD_INIT(NULL) "htps.EnvExpansion",
sizeof(PyEnvExpansion),
0,
(destructor)EnvExpansion_dealloc,
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
"A full EnvExpansion for HyperTreeProofSearch",
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
EnvExpansion_getsetters,
NULL,
NULL,
NULL,
NULL,
NULL,
(initproc)EnvExpansion_init,
NULL,
(newfunc)EnvExpansion_new,
};



typedef struct {
#ifdef PYTHON_BINDINGS
    PyObject_HEAD
#endif
    htps::HTPS graph;
} PyHTPS;


static PyObject *HTPS_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    auto *self = (PyHTPS *) type->tp_alloc(type, 0);
    if (!self) {
        PyErr_SetString(PyExc_MemoryError, "could not allocate memory");
        return NULL;
    }
    new (&(self->graph)) htps::HTPS();
    return (PyObject *) self;
}

static int HTPS_init(PyObject *self, PyObject *args, PyObject *kwargs) {
    auto *tree = (PyHTPS *) self;
    PyObject *thm, *params;
    static char *kwlist[] = { (char*)"theorem", (char*)"params", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist, &thm, &params))
        return -1;
    if (!PyObject_TypeCheck(thm, &TheoremType)) {
        PyErr_SetString(PyExc_TypeError, "theorem must be a Theorem object");
        return -1;
    }
    auto *c_thm = (htps::lean_theorem *) thm;
    if (!PyObject_TypeCheck(params, &ParamsType)) {
        PyErr_SetString(PyExc_TypeError, "params must be a SearchParams object");
        return -1;
    }
    auto *c_params = (htps::htps_params *) params;
    auto shared_thm = (std::shared_ptr<htps::theorem>) std::make_shared<htps::lean_theorem>(*c_thm);
    tree->graph.set_root(shared_thm);
    tree->graph.set_params(*c_params);
    return 0;
}

static PyObject* PyHTPS_theorems_to_expand(PyHTPS *self, PyObject *Py_UNUSED(ignored)) {
    std::vector<std::shared_ptr<htps::theorem>> thms = self->graph.theorems_to_expand();
    PyObject *list = PyList_New(thms.size());
    if (!list)
        return PyErr_NoMemory();

    for (size_t i = 0; i < thms.size(); i++) {
        PyObject *pythm = Theorem_NewFromShared(thms[i]);
        if (!pythm) {
            Py_DECREF(list);
            return NULL;
        }
        PyList_SET_ITEM(list, i, pythm);
    }
    return list;
}

static PyObject* PyHTPS_expand_and_backup(PyHTPS *self, PyObject *args) {
    PyObject *py_expansions;
    if (!PyArg_ParseTuple(args, "O", &py_expansions)) {
        PyErr_SetString(PyExc_TypeError, "expand_and_backup expects an iterable of EnvExpansion objects");
        return NULL;
    }
    std::vector<std::shared_ptr<htps::env_expansion>> expansions;
    PyObject *iterator = PyObject_GetIter(py_expansions);
    if (!iterator) {
    PyErr_SetString(PyExc_TypeError, "Provided object is not iterable");
    return NULL;
    }
    PyObject *item;
    while ((item = PyIter_Next(iterator)) != NULL) {
        if (!PyObject_TypeCheck(item, &EnvExpansionType)) {
            PyErr_SetString(PyExc_TypeError, "each item in the iterable must be an EnvExpansion object");
            Py_DECREF(item);
            Py_DECREF(iterator);
            return NULL;
        }
        auto *exp = (PyEnvExpansion *) item;
        std::shared_ptr<htps::env_expansion> shared_exp(
            &exp->expansion,
            [](htps::env_expansion*) {
                // no deletion: the memory is owned by the Python object.
            }
        );
        //expansions.push_back(std::make_shared<htps::env_expansion>(exp->expansion));
        expansions.push_back(shared_exp);
        Py_DECREF(item);
    }
    Py_DECREF(iterator);
    self->graph.expand_and_backup(expansions);
    Py_RETURN_NONE;
}

static PyObject* PyHTPS_is_proven(PyHTPS *self, PyObject *Py_UNUSED(ignored)) {
    return self->graph.is_proven() ? Py_True : Py_False;
}


static PyMethodDef HTPS_methods[] = {
    {"theorems_to_expand", (PyCFunction)PyHTPS_theorems_to_expand, METH_NOARGS, "Returns a list of subsequent theorems to expand"},
    {"expand_and_backup", (PyCFunction)PyHTPS_expand_and_backup, METH_VARARGS,  "Expands and backups using the provided list of EnvExpansion objects"},
    {"proven", (PyCFunction)PyHTPS_is_proven, METH_NOARGS, "Whether the start theorem is proven or not"},
    {NULL, NULL, 0, NULL}
};

static PyTypeObject HTPSType = {
    PyObject_HEAD_INIT(NULL) "htps.HTPS",
    sizeof(PyHTPS),
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
    "The HyperTreeProofSearch",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    HTPS_methods,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    (initproc)HTPS_init,
    NULL,
    (newfunc)HTPS_new,
    };


extern "C"
PyMODINIT_FUNC
PyInit_htps(void) {
    PyObject *m = PyModule_Create(&htps_module);
    if (m == NULL) {
        Py_XDECREF(m);
        return NULL;
    }

    HTPSError = PyErr_NewException("htps.error", NULL, NULL);
    if (PyModule_AddObject(m, "error", HTPSError) < 0) {
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

    if (PyType_Ready(&HypothesisType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(&ParamsType);
        return NULL;
    }

    Py_INCREF(&HypothesisType);
    if (PyModule_AddObject(m, "Hypothesis", (PyObject *) &HypothesisType) < 0) {
        Py_DECREF(m); Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(&ParamsType);
        Py_XDECREF(&HypothesisType);
        return NULL;
    }

    if (PyType_Ready(&TacticType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        return NULL;
    }

    Py_INCREF(&TacticType);
    if (PyModule_AddObject(m, "Tactic", (PyObject *) &TacticType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_XDECREF(&TacticType);
        return NULL;
    }

    if (PyType_Ready(&ContextType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        return NULL;
    }

    Py_INCREF(&ContextType);
    if (PyModule_AddObject(m, "Context", (PyObject *) &ContextType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_XDECREF(&ContextType);
        return NULL;
    }

    if (PyType_Ready(&TheoremType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        return NULL;
    }

    Py_INCREF(&TheoremType);
    if (PyModule_AddObject(m, "Theorem", (PyObject *) &TheoremType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_XDECREF(&TheoremType);
        return NULL;
    }

    if (PyType_Ready(&EnvEffectType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        return NULL;
    }

    Py_INCREF(&EnvEffectType);
    if (PyModule_AddObject(m, "EnvEffect", (PyObject *) &EnvEffectType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        Py_XDECREF(&EnvEffectType);
        return NULL;
    }


    if (PyType_Ready(&EnvExpansionType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        Py_DECREF(&EnvEffectType);
        return NULL;
    }

    Py_INCREF(&EnvExpansionType);
    if (PyModule_AddObject(m, "EnvExpansion", (PyObject *) &EnvExpansionType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        Py_DECREF(&EnvEffectType);
        Py_XDECREF(&EnvExpansionType);
        return NULL;
    }

    if (PyType_Ready(&HTPSType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        Py_DECREF(&EnvEffectType);
        Py_DECREF(&EnvExpansionType);
        return NULL;
    }

    Py_INCREF(&HTPSType);
    if (PyModule_AddObject(m, "HTPS", (PyObject *) &HTPSType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        Py_DECREF(&EnvEffectType);
        Py_DECREF(&EnvExpansionType);
        Py_XDECREF(&HTPSType);
        return NULL;
    }

    return m;
}


int main() {
    return 0;
}
