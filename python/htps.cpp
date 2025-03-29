#include "./htps.h"
#include <structmember.h>
#include "../src/graph/htps.h"
#include "../src/graph/lean.h"

static PyObject *PolicyTypeEnum = NULL;
static PyObject *QValueSolvedEnum = NULL;
static PyObject *MetricEnum = NULL;
static PyObject *NodeMaskEnum = NULL;
static PyObject *InProofEnum = NULL;

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

static PyObject *make_in_proof(PyObject *module, PyObject *enum_module) {
    if (InProofEnum != NULL)
        return InProofEnum;
    size_t value_size = 3;
    const char *values[value_size] = {"NotInProof", "InProof", "InMinimalProof"};
    InProofEnum = make_enum(module, enum_module, values, value_size, "InProof");
    return InProofEnum;
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


static void Theorem_dealloc(PyObject *self) {
    auto *thm = (htps::lean_theorem *) self;
    thm->conclusion.~basic_string();
    thm->hypotheses.~vector();
    thm->unique_string.~basic_string();
    thm->context.~lean_context();
    thm->past_tactics.~vector();
    Py_XDECREF(thm->py_dict);
    Py_TYPE(self)->tp_free(self);
}

static PyObject * Theorem_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    auto *self = (htps::lean_theorem *) type->tp_alloc(type, 0);
    if (!self) {
        PyErr_SetString(PyExc_MemoryError, "could not allocate memory");
        return NULL;
    }
    self->py_dict = PyDict_New();
    if (self->py_dict == NULL) {
        Py_DECREF(self);
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

static PyObject *Theorem_get_dict(PyObject *self, void *closure) {
    auto *thm = (htps::lean_theorem *) self;
    Py_INCREF(thm->py_dict);
    return thm->py_dict;
}

static int Theorem_set_dict(PyObject *self, PyObject *value, void *closure) {
    auto *thm = (htps::lean_theorem *) self;
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "new dictionary is not set");
        return -1;
    }
    if (!PyDict_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "new dictionary must be a dict");
        return -1;
    }
    Py_XDECREF(thm->py_dict);
    Py_INCREF(value);
    thm->py_dict = value;
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
    {"metadata", Theorem_get_dict, Theorem_set_dict, "Metadata dictionary", NULL},
    {NULL}
};

static PyTypeObject TheoremType = {
    PyObject_HEAD_INIT(NULL) "htps.Theorem",
sizeof(htps::lean_theorem),
0,
(destructor)Theorem_dealloc,
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
            // Check prior list
            if (priors.size() != tactics.size()) {
                PyErr_SetString(PyExc_ValueError, "priors must have the same length as tactics");
                return -1;
            }
            double sum = std::accumulate(priors.begin(), priors.end(), 0.0);
            if (sum < 0.99 || sum > 1.01) {
                PyErr_SetString(PyExc_ValueError, "priors must sum to 1");
                return -1;
            }


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
    htps::HTPSSampleEffect cpp_obj;
} PyHTPSSampleEffect;

static void PyHTPSSampleEffect_dealloc(PyHTPSSampleEffect *self) {
    self->cpp_obj.~HTPSSampleEffect();
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *PyHTPSSampleEffect_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    auto *self = (PyHTPSSampleEffect *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return PyErr_NoMemory();
    }
    new (&(self->cpp_obj)) htps::HTPSSampleEffect();
    return (PyObject *) self;
}

static int PyHTPSSampleEffect_init(PyHTPSSampleEffect *self, PyObject *args, PyObject *kwds) {
    PyObject *py_goal = NULL, *py_tac = NULL, *py_children = NULL;
    static char *kwlist[] = {"goal", "tactic", "children", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OOO", kwlist, &py_goal, &py_tac, &py_children)){
        return -1;
    }
    if (!PyObject_TypeCheck(py_goal, &TheoremType)) {
        PyErr_SetString(PyExc_TypeError, "goal must be a Theorem object");
        return -1;
    }
    if (!PyObject_TypeCheck(py_tac, &TacticType)) {
        PyErr_SetString(PyExc_TypeError, "tactic must be a Tactic object");
        return -1;
    }
    std::shared_ptr<htps::theorem> goal = std::static_pointer_cast<htps::theorem>(std::make_shared<htps::lean_theorem>(*(htps::lean_theorem*)py_goal));
    std::shared_ptr<htps::tactic> tac = std::static_pointer_cast<htps::tactic>(std::make_shared<htps::lean_tactic>(*(htps::lean_tactic*)py_tac));
    std::vector<std::shared_ptr<htps::theorem>> children;
    PyObject *iter = PyObject_GetIter(py_children);
    if (!iter) {
        PyErr_SetString(PyExc_TypeError, "children must be iterable");
        return -1;
    }
    PyObject *item;
    while ((item = PyIter_Next(iter)) != NULL) {
        if (!PyObject_TypeCheck(item, &TheoremType)) {
            PyErr_SetString(PyExc_TypeError, "each child must be a Theorem object");
            Py_DECREF(item);
            Py_DECREF(iter);
            return -1;
        }
        std::shared_ptr<htps::theorem> child = std::static_pointer_cast<htps::theorem>(std::make_shared<htps::lean_theorem>(*(htps::lean_theorem*)item));
        children.push_back(child);
        Py_DECREF(item);
    }
    Py_DECREF(iter);
    self->cpp_obj.~HTPSSampleEffect();
    new (&self->cpp_obj) htps::HTPSSampleEffect(goal, tac, children);
    return 0;
}

static PyObject* PyHTPSSampleEffect_get_goal(PyHTPSSampleEffect *self, void *closure) {
    return Theorem_NewFromShared(self->cpp_obj.get_goal());
}

static PyObject* PyHTPSSampleEffect_get_tactic(PyHTPSSampleEffect *self, void *closure) {
    return Tactic_NewFromShared(self->cpp_obj.get_tactic());
}

static PyObject* PyHTPSSampleEffect_get_children(PyHTPSSampleEffect *self, void *closure) {
    std::vector<std::shared_ptr<htps::theorem>> children = self->cpp_obj.get_children();
    PyObject *list = PyList_New(children.size());
    if (!list)
        return PyErr_NoMemory();
    for (size_t i = 0; i < children.size(); i++) {
        PyObject *child = Theorem_NewFromShared(children[i]);
        if (!child) {
            Py_DECREF(list);
            return NULL;
        }
        PyList_SET_ITEM(list, i, child);
    }
    return list;
}

static PyGetSetDef PyHTPSSampleEffect_getsetters[] = {
        {"goal", (getter)PyHTPSSampleEffect_get_goal, NULL, "Goal theorem", NULL},
        {"tactic", (getter)PyHTPSSampleEffect_get_tactic, NULL, "Tactic", NULL},
        {"children", (getter)PyHTPSSampleEffect_get_children, NULL, "Children theorems", NULL},
        {NULL}
};

static PyMethodDef PyHTPSSampleEffect_methods[] = {
        {NULL, NULL, 0, NULL}
};

static PyTypeObject PyHTPSSampleEffectType = {
        PyObject_HEAD_INIT(NULL)
        "htps.SampleEffect",
        sizeof(PyHTPSSampleEffect),
        0,
        (destructor)PyHTPSSampleEffect_dealloc,
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
        "SampleEffect object, holding effect training samples",
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        PyHTPSSampleEffect_methods,
        NULL,
        PyHTPSSampleEffect_getsetters,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        (initproc)PyHTPSSampleEffect_init,
        NULL,
        (newfunc)PyHTPSSampleEffect_new,
};

typedef struct {
#ifdef PYTHON_BINDINGS
    PyObject_HEAD
#endif
    htps::HTPSSampleCritic cpp_obj;
} PyHTPSSampleCritic;

static void PyHTPSSampleCritic_dealloc(PyHTPSSampleCritic *self) {
    self->cpp_obj.~HTPSSampleCritic();
    Py_TYPE(self)->tp_free((PyObject*)self);
}
static PyObject* PyHTPSSampleCritic_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    auto *self = (PyHTPSSampleCritic*) type->tp_alloc(type, 0);
    if (self == NULL) {
        return PyErr_NoMemory();
    }
    new (&(self->cpp_obj)) htps::HTPSSampleCritic();
    return (PyObject *) self;
}

static int PyHTPSSampleCritic_init(PyHTPSSampleCritic *self, PyObject *args, PyObject *kwds) {
    PyObject *py_goal = NULL;
    double q_estimate, critic;
    int solved, bad;
    size_t visit_count;
    static const char *kwlist[] = {"goal", "q_estimate", "solved", "bad", "critic", "visit_count", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Odppdk", const_cast<char**>(kwlist),
                                     &py_goal, &q_estimate, &solved, &bad, &critic, &visit_count)) {
        return -1;
    }
    if (!PyObject_TypeCheck(py_goal, &TheoremType)) {
        PyErr_SetString(PyExc_TypeError, "goal must be a Theorem object");
        return -1;
    }
    std::shared_ptr<htps::theorem> goal = std::static_pointer_cast<htps::theorem>(std::make_shared<htps::lean_theorem>(*(htps::lean_theorem*)py_goal));
    self->cpp_obj.~HTPSSampleCritic();
    new (&self->cpp_obj) htps::HTPSSampleCritic(goal, q_estimate, solved ? true : false, bad ? true : false, critic, visit_count);
    return 0;
}

static PyObject* PyHTPSSampleCritic_get_goal(PyHTPSSampleCritic *self, void *closure) {
    return Theorem_NewFromShared(self->cpp_obj.get_goal());
}

static PyObject* PyHTPSSampleCritic_get_q_estimate(PyHTPSSampleCritic *self, void *closure) {
    return PyFloat_FromDouble(self->cpp_obj.get_q_estimate());
}

static PyObject* PyHTPSSampleCritic_get_solved(PyHTPSSampleCritic *self, void *closure) {
    PyObject *res = self->cpp_obj.is_solved() ? Py_True : Py_False;
    Py_INCREF(res);
    return res;
}

static PyObject* PyHTPSSampleCritic_get_bad(PyHTPSSampleCritic *self, void *closure) {
    PyObject *res = self->cpp_obj.is_bad() ? Py_True : Py_False;
    Py_INCREF(res);
    return res;
}

static PyObject* PyHTPSSampleCritic_get_critic(PyHTPSSampleCritic *self, void *closure) {
    return PyFloat_FromDouble(self->cpp_obj.get_critic());
}

static PyObject* PyHTPSSampleCritic_get_visit_count(PyHTPSSampleCritic *self, void *closure) {
    return PyLong_FromSize_t(self->cpp_obj.get_visit_count());
}

static PyGetSetDef PyHTPSSampleCritic_getsetters[] = {
        {"goal", (getter)PyHTPSSampleCritic_get_goal, NULL, "Goal theorem", NULL},
        {"q_estimate", (getter)PyHTPSSampleCritic_get_q_estimate, NULL, "Q estimate", NULL},
        {"solved", (getter)PyHTPSSampleCritic_get_solved, NULL, "Solved", NULL},
        {"bad", (getter)PyHTPSSampleCritic_get_bad, NULL, "Bad", NULL},
        {"critic", (getter)PyHTPSSampleCritic_get_critic, NULL, "Critic value", NULL},
        {"visit_count", (getter)PyHTPSSampleCritic_get_visit_count, NULL, "Visit count", NULL},
        {NULL}
};


static PyMethodDef PyHTPSSampleCritic_methods[] = {
        {NULL, NULL, 0, NULL}
};



static PyTypeObject PyHTPSSampleCriticType = {
        PyObject_HEAD_INIT(NULL)
        "htps.SampleCritic",
        sizeof(PyHTPSSampleCritic),
        0,
        (destructor)PyHTPSSampleCritic_dealloc,
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
        "SampleCritic object, holding critic training samples",
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        PyHTPSSampleCritic_methods,
        NULL,
        PyHTPSSampleCritic_getsetters,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        (initproc)PyHTPSSampleCritic_init,
        NULL,
        (newfunc)PyHTPSSampleCritic_new,
};


typedef struct {
#ifdef PYTHON_BINDINGS
    PyObject_HEAD
#endif
    htps::HTPSSampleTactics cpp_obj;
} PyHTPSSampleTactics;

static void PyHTPSSampleTactics_dealloc(PyHTPSSampleTactics *self) {
    self->cpp_obj.~HTPSSampleTactics();
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* PyHTPSSampleTactics_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    auto *self = (PyHTPSSampleTactics*) type->tp_alloc(type, 0);
    if (self == NULL) {
        return PyErr_NoMemory();
    }
    new (&(self->cpp_obj)) htps::HTPSSampleTactics();
    return (PyObject *) self;
}

static int PyHTPSSampleTactics_init(PyHTPSSampleTactics *self, PyObject *args, PyObject *kwds) {
    PyObject *py_goal, *py_tactics, *py_target_pi, *py_q_estimates, *py_inproof;
    long visit_count;
    static const char *kwlist[] = {"goal", "tactics", "target_pi", "inproof", "q_estimates", "visit_count", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OOOOOn", const_cast<char**>(kwlist),
                                     &py_goal, &py_tactics, &py_target_pi, &py_inproof, &py_q_estimates, &visit_count)) {
        return -1;
    }
    if (visit_count < 0) {
        PyErr_SetString(PyExc_ValueError, "visit_count must be non-negative");
        return -1;
    }
    if (!PyObject_TypeCheck(py_goal, &TheoremType)) {
        PyErr_SetString(PyExc_TypeError, "goal must be a Theorem object");
        return -1;
    }
    std::shared_ptr<htps::theorem> goal = std::static_pointer_cast<htps::theorem>(std::make_shared<htps::lean_theorem>(*(htps::lean_theorem*)py_goal));

    std::vector<std::shared_ptr<htps::tactic>> tactics;
    PyObject *iter = PyObject_GetIter(py_tactics);
    if (!iter) {
        PyErr_SetString(PyExc_TypeError, "tactics must be iterable");
        return -1;
    }
    PyObject *item;
    while ((item = PyIter_Next(iter)) != NULL) {
        if (!PyObject_TypeCheck(item, &TacticType)) {
            PyErr_SetString(PyExc_TypeError, "each tactic must be a Tactic object");
            Py_DECREF(item);
            Py_DECREF(iter);
            return -1;
        }
        std::shared_ptr<htps::tactic> tac = std::static_pointer_cast<htps::tactic>(std::make_shared<htps::lean_tactic>(*(htps::lean_tactic*)item));
        tactics.push_back(tac);
        Py_DECREF(item);
    }
    Py_DECREF(iter);

    std::vector<double> target_pi;
    iter = PyObject_GetIter(py_target_pi);
    if (!iter) {
        PyErr_SetString(PyExc_TypeError, "target_pi must be iterable");
        return -1;
    }
    while ((item = PyIter_Next(iter)) != NULL) {
        if (!PyFloat_Check(item) && !PyLong_Check(item)) {
            PyErr_SetString(PyExc_TypeError, "each target_pi must be a number");
            Py_DECREF(item);
            Py_DECREF(iter);
            return -1;
        }
        target_pi.push_back(PyFloat_AsDouble(item));
        Py_DECREF(item);
    }
    Py_DECREF(iter);

    // Parse q_estimates: iterable of numbers (can be empty).
    std::vector<double> q_estimates;
    iter = PyObject_GetIter(py_q_estimates);
    if (iter) {
        while ((item = PyIter_Next(iter)) != NULL) {
            if (!PyFloat_Check(item) && !PyLong_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "each q_estimate must be a number");
                Py_DECREF(item);
                Py_DECREF(iter);
                return -1;
            }
            q_estimates.push_back(PyFloat_AsDouble(item));
            Py_DECREF(item);
        }
        Py_DECREF(iter);
    }

    int inproof_value = get_enum_value(py_inproof, "InProof");
    if (inproof_value < 0) {
        return -1;
    }
    auto inproof = (htps::InProof) inproof_value;
    self->cpp_obj.~HTPSSampleTactics();
    new (&self->cpp_obj) htps::HTPSSampleTactics(goal, tactics, target_pi, inproof, q_estimates, visit_count);
    return 0;
}

static PyObject* PyHTPSSampleTactics_get_goal(PyHTPSSampleTactics *self, void *closure) {
    return Theorem_NewFromShared(self->cpp_obj.get_goal());
}

static PyObject* PyHTPSSampleTactics_get_tactics(PyHTPSSampleTactics *self, void *closure) {
    const std::vector<std::shared_ptr<htps::tactic>> &tactics = self->cpp_obj.get_tactics();
    PyObject *list = PyList_New(tactics.size());
    if (!list)
        return PyErr_NoMemory();
    for (size_t i = 0; i < tactics.size(); i++) {
        PyObject *tac = Tactic_NewFromShared(tactics[i]);
        if (!tac) {
            Py_DECREF(list);
            return NULL;
        }
        PyList_SET_ITEM(list, i, tac);
    }
    return list;
}

static PyObject* PyHTPSSampleTactics_get_target_pi(PyHTPSSampleTactics *self, void *closure) {
    const std::vector<double> &pi = self->cpp_obj.get_target_pi();
    PyObject *list = PyList_New(pi.size());
    if (!list)
        return PyErr_NoMemory();
    for (size_t i = 0; i < pi.size(); i++) {
        PyObject *num = PyFloat_FromDouble(pi[i]);
        if (!num) {
            Py_DECREF(list);
            return NULL;
        }
        PyList_SET_ITEM(list, i, num);
    }
    return list;
}

static PyObject* PyHTPSSampleTactics_get_inproof(PyHTPSSampleTactics *self, void *closure) {
    int value = self->cpp_obj.get_inproof();
    return PyObject_CallFunction(InProofEnum, "i", value);
}

static PyObject* PyHTPSSampleTactics_get_q_estimates(PyHTPSSampleTactics *self, void *closure) {
    const std::vector<double> &q = self->cpp_obj.get_q_estimates();
    PyObject *list = PyList_New(q.size());
    if (!list)
        return PyErr_NoMemory();
    for (size_t i = 0; i < q.size(); i++) {
        PyObject *num = PyFloat_FromDouble(q[i]);
        if (!num) {
            Py_DECREF(list);
            return NULL;
        }
        PyList_SET_ITEM(list, i, num);
    }
    return list;
}

static PyObject* PyHTPSSampleTactics_get_visit_count(PyHTPSSampleTactics *self, void *closure) {
    return PyLong_FromSize_t(self->cpp_obj.get_visit_count());
}

static PyGetSetDef PyHTPSSampleTactics_getsetters[] = {
        {"goal", (getter)PyHTPSSampleTactics_get_goal, NULL, "theorem that was used to generate new tactics", NULL},
        {"tactics", (getter)PyHTPSSampleTactics_get_tactics, NULL, "List of tactics", NULL},
        {"target_pi", (getter)PyHTPSSampleTactics_get_target_pi, NULL, "Target probabilities", NULL},
        {"inproof", (getter)PyHTPSSampleTactics_get_inproof, NULL, "InProof enum", NULL},
        {"q_estimates", (getter)PyHTPSSampleTactics_get_q_estimates, NULL, "q estimates", NULL},
        {"visit_count", (getter)PyHTPSSampleTactics_get_visit_count, NULL, "Visit count", NULL},
        {NULL}
};

static PyMethodDef PyHTPSSampleTactics_methods[] = {
        {NULL, NULL, 0, NULL}
};

static PyTypeObject PyHTPSSampleTacticsType = {
        PyObject_HEAD_INIT(NULL)
        "htps.SampleTactics",
        sizeof(PyHTPSSampleTactics),
        0,
        (destructor) PyHTPSSampleTactics_dealloc,
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
        "SampleTactics object, holding tactic generation training samples",
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        PyHTPSSampleTactics_methods,
        NULL,
        PyHTPSSampleTactics_getsetters,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        (initproc)PyHTPSSampleTactics_init,
        NULL,
        (newfunc)PyHTPSSampleTactics_new,
};


typedef struct {
    PyObject_HEAD
    htps::proof cpp_obj;
} PyProof;



static void PyProof_dealloc(PyProof *self) {
    self->cpp_obj.~proof();
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *PyProof_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    auto *self = (PyProof *) type->tp_alloc(type, 0);
    if (self == NULL) {
        return PyErr_NoMemory();
    }
    new (&(self->cpp_obj)) htps::proof();
    return (PyObject *) self;
}

PyObject* PyProof_NewFromProof(const htps::proof &p) {
    PyObject *obj = PyProof_new(&PyProofType, NULL, NULL);
    if (obj == NULL)
        return NULL;
    auto *py_proof = (PyProof *) obj;
    py_proof->cpp_obj = p;
    return obj;
}


static int PyProof_init(PyObject *self, PyObject *args, PyObject *kwargs) {
    auto *proof = (PyProof *) self;
    PyObject *py_thm, *py_tactic, *py_children;
    static const char *kwlist[] = { "theorem", "tactic", "children",NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOO", const_cast<char**>(kwlist), &py_thm, &py_tactic, &py_children))
        return -1;
    if (!PyObject_TypeCheck(py_thm, &TheoremType)) {
        PyErr_SetString(PyExc_TypeError, "theorem must be a Theorem object");
        return -1;
    }
    if (!PyObject_TypeCheck(py_tactic, &TacticType)) {
        PyErr_SetString(PyExc_TypeError, "tactic must be a Tactic object");
        return -1;
    }
    std::shared_ptr<htps::theorem> proof_theorem = std::static_pointer_cast<htps::theorem>(std::make_shared<htps::lean_theorem>(*(htps::lean_theorem*)py_thm));
    std::shared_ptr<htps::tactic> proof_tactic = std::static_pointer_cast<htps::tactic>(std::make_shared<htps::lean_tactic>(*(htps::lean_tactic*)py_tactic));

    std::vector<htps::proof> children;
    PyObject *iter = PyObject_GetIter(py_children);
    if (!iter) {
        PyErr_SetString(PyExc_TypeError, "children must be iterable");
        return -1;
    }
    PyObject *item;
    while ((item = PyIter_Next(iter)) != NULL) {
        if (!PyObject_TypeCheck(item, &PyProofType)) {
            PyErr_SetString(PyExc_TypeError, "each child must be a Theorem object");
            Py_DECREF(item);
            Py_DECREF(iter);
            return -1;
        }
        auto *child_wrapper = (PyProof *) item;
        children.push_back(child_wrapper->cpp_obj);
        Py_DECREF(item);
    }
    Py_DECREF(iter);

    proof->cpp_obj.proof_theorem = proof_theorem;
    proof->cpp_obj.proof_tactic = proof_tactic;
    proof->cpp_obj.children = children;
    return 0;
}

static PyObject* PyProof_get_proof_theorem(PyProof *self, void *closure) {
    return Theorem_NewFromShared(self->cpp_obj.proof_theorem);
}


static PyObject* PyProof_get_proof_tactic(PyProof *self, void *closure) {
    return Tactic_NewFromShared(self->cpp_obj.proof_tactic);
}

static PyObject* PyProof_get_children(PyProof *self, void *closure) {
    const std::vector<htps::proof> &children = self->cpp_obj.children;
    PyObject *list = PyList_New(children.size());
    if (!list)
        return PyErr_NoMemory();
    for (size_t i = 0; i < children.size(); i++) {
        PyObject *child_obj = NULL;
        child_obj = PyProof_NewFromProof(children[i]);
        if (!child_obj) {
            Py_DECREF(list);
            return NULL;
        }
        PyList_SET_ITEM(list, i, child_obj);
    }
    return list;
}

static PyGetSetDef PyProof_getsetters[] = {
        {"theorem", (getter)PyProof_get_proof_theorem, NULL, "Theorem used in the proof", NULL},
        {"tactic", (getter)PyProof_get_proof_tactic, NULL, "Tactic used in the proof", NULL},
        {"children", (getter)PyProof_get_children, NULL, "Child proof objects", NULL},
        {NULL}
};

static PyMethodDef PyProof_methods[] = {
        {NULL, NULL, 0, NULL}
};

PyTypeObject PyProofType = {
        PyObject_HEAD_INIT(NULL)
        "htps.Proof",
        sizeof(PyProof),
        0,
        (destructor) PyProof_dealloc,
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
        "Proof object, holding a proof subtree",
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        PyProof_methods,
        NULL,
        PyProof_getsetters,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        (initproc)PyProof_init,
        NULL,
        (newfunc)PyProof_new,
};

typedef struct {
    PyObject_HEAD
    htps::HTPSResult cpp_obj;
} PyHTPSResult;

template<typename T, typename U>
std::vector<T> PyObject_to_vector(PyObject *obj, PyTypeObject *type, const char *error_msg) {
    auto iter = PyObject_GetIter(obj);
    if (!iter) {
        PyErr_SetString(PyExc_TypeError, "object must be iterable");
        throw std::runtime_error("Type error");
    }
    std::vector<T> vec;
    PyObject *item;
    while ((item = PyIter_Next(iter)) != NULL) {
        if (!PyObject_TypeCheck(item, type)) {
            PyErr_SetString(PyExc_TypeError, error_msg);
            Py_DECREF(item);
            Py_DECREF(iter);
            throw std::runtime_error("Type error");
        }
        auto *wrapper = (U*) item;
        vec.push_back(wrapper->cpp_obj);
        Py_DECREF(item);
    }
    Py_DECREF(iter);
    return vec;
}



static void PyHTPSResult_dealloc(PyHTPSResult *self) {
    self->cpp_obj.~HTPSResult();
    Py_TYPE(self)->tp_free((PyObject*)self);
}


static PyObject *PyHTPSResult_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    auto *self = (PyHTPSResult *) type->tp_alloc(type, 0);
    if (self == NULL) {
        return PyErr_NoMemory();
    }
    new (&(self->cpp_obj)) htps::HTPSResult();
    return (PyObject *) self;
}

static int PyHTPSResult_init(PyObject *self, PyObject *args, PyObject *kwargs) {
    auto *result = (PyHTPSResult *) self;
    PyObject *py_critic_samples, *py_tactic_samples, *py_effect_samples, *py_metric, *py_proof_samples_tactics, *py_goal, *py_proof;
    static const char *kwlist[] = { "critic_samples", "tactic_samples", "effect_samples", "metric", "proof_samples_tactics", "goal", "proof", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOOOOOO", const_cast<char**>(kwlist),
                                     &py_critic_samples, &py_tactic_samples, &py_effect_samples, &py_metric, &py_proof_samples_tactics, &py_goal, &py_proof)) {
        return -1;
    }
    std::vector<htps::HTPSSampleCritic> critic_samples;
    std::vector<htps::HTPSSampleTactics> tactic_samples;
    std::vector<htps::HTPSSampleEffect> effect_samples;
    std::vector<htps::HTPSSampleTactics> proof_samples_tactics;

    try {
        critic_samples = PyObject_to_vector<htps::HTPSSampleCritic, PyHTPSSampleCritic>(py_critic_samples, &PyHTPSSampleCriticType, "each item in critic_samples must be a SampleCritic object");
        tactic_samples = PyObject_to_vector<htps::HTPSSampleTactics, PyHTPSSampleTactics>(py_tactic_samples, &PyHTPSSampleTacticsType, "each item in tactic_samples must be a SampleTactics object");
        effect_samples = PyObject_to_vector<htps::HTPSSampleEffect, PyHTPSSampleEffect>(py_effect_samples, &PyHTPSSampleEffectType, "each item in effect_samples must be a SampleEffect object");
        proof_samples_tactics = PyObject_to_vector<htps::HTPSSampleTactics, PyHTPSSampleTactics>(py_proof_samples_tactics, &PyHTPSSampleTacticsType, "each item in proof_samples_tactics must be a SampleTactics object");
    } catch (std::runtime_error &e) {
        return -1;
    }
    int metric_value = get_enum_value(py_metric, "Metric");
    if (metric_value < 0) {
        return -1;
    }
    auto metric = (htps::Metric) metric_value;

    if (!PyObject_TypeCheck(py_goal, &TheoremType)) {
        PyErr_SetString(PyExc_TypeError, "goal must be a Theorem object");
        return -1;
    }
    if (!PyObject_TypeCheck(py_proof, &PyProofType)) {
        PyErr_SetString(PyExc_TypeError, "proof must be a Proof object");
        return -1;
    }
    auto *goal = (htps::lean_theorem *) py_goal;
    auto thm = static_cast<std::shared_ptr<htps::theorem>>(std::make_shared<htps::lean_theorem>(*goal));
    auto *proof = (PyProof *) py_proof;
    result->cpp_obj.~HTPSResult();
    new (&result->cpp_obj) htps::HTPSResult(critic_samples, tactic_samples, effect_samples, metric, proof_samples_tactics, thm, proof->cpp_obj);
    return 0;
}


static PyObject *PyHTPSResult_get_critic_samples(PyHTPSResult *self, void *closure) {
    auto vec = self->cpp_obj.get_critic_samples();
    PyObject *list = PyList_New(vec.size());
    if (!list)
        return PyErr_NoMemory();
    for (size_t i = 0; i < vec.size(); i++) {
        auto thm = Theorem_NewFromShared(vec[i].get_goal());
        if (!thm) {
            Py_DECREF(list);
            return NULL;
        }
        PyObject *solved = vec[i].is_solved() ? Py_True : Py_False;
        PyObject *bad = vec[i].is_bad() ? Py_True : Py_False;
        PyObject *args = Py_BuildValue("OdOOdk", thm, vec[i].get_q_estimate(), solved, bad, vec[i].get_critic(), vec[i].get_visit_count());
        Py_DECREF(thm);
        if (!args) {
            Py_DECREF(list);
            return NULL;
        }
        PyObject *item = PyObject_CallObject((PyObject *) &PyHTPSSampleCriticType, args);
        Py_DECREF(args);
        if (!item) {
            Py_DECREF(list);
            return NULL;
        }
        PyList_SET_ITEM(list, i, item);
    }
    return list;
}

static PyObject *PyHTPSResult_get_tactic_samples(PyHTPSResult *self, void *closure) {
    auto vec = self->cpp_obj.get_tactic_samples();
    PyObject *list = PyList_New(vec.size());
    if (!list)
        return PyErr_NoMemory();
    for (size_t i = 0; i < vec.size(); i++) {
        auto thm = Theorem_NewFromShared(vec[i].get_goal());
        if (!thm) {
            Py_DECREF(list);
            return NULL;
        }
        PyObject *tactics = PyList_New(vec[i].get_tactics().size());
        if (!tactics) {
            Py_DECREF(thm);
            Py_DECREF(list);
            return PyErr_NoMemory();
        }
        for (size_t j = 0; j < vec[i].get_tactics().size(); j++) {
            auto tac = Tactic_NewFromShared(vec[i].get_tactics()[j]);
            if (!tac) {
                Py_DECREF(thm);
                Py_DECREF(tactics);
                Py_DECREF(list);
                return NULL;
            }
            PyList_SET_ITEM(tactics, j, tac);
        }
        PyObject *target_pi = PyList_New(vec[i].get_target_pi().size());
        if (!target_pi) {
            Py_DECREF(thm);
            Py_DECREF(tactics);
            Py_DECREF(list);
            return PyErr_NoMemory();
        }
        for (size_t j = 0; j < vec[i].get_target_pi().size(); j++) {
            PyObject *num = PyFloat_FromDouble(vec[i].get_target_pi()[j]);
            if (!num) {
                Py_DECREF(thm);
                Py_DECREF(target_pi);
                Py_DECREF(tactics);
                Py_DECREF(list);
                return NULL;
            }
            PyList_SET_ITEM(target_pi, j, num);
        }
        PyObject *q_estimates = PyList_New(vec[i].get_q_estimates().size());
        if (!q_estimates) {
            Py_DECREF(thm);
            Py_DECREF(tactics);
            Py_DECREF(target_pi);
            Py_DECREF(list);
            return PyErr_NoMemory();
        }
        for (size_t j = 0; j < vec[i].get_q_estimates().size(); j++) {
            PyObject *num = PyFloat_FromDouble(vec[i].get_q_estimates()[j]);
            if (!num) {
                Py_DECREF(thm);
                Py_DECREF(tactics);
                Py_DECREF(target_pi);
                Py_DECREF(q_estimates);
                Py_DECREF(list);
                return NULL;
            }
            PyList_SET_ITEM(q_estimates, j, num);
        }

        PyObject *inproof = PyObject_CallFunction(InProofEnum, "i", vec[i].get_inproof());

        PyObject *args = Py_BuildValue("OOOOOn", thm, tactics, target_pi, inproof, q_estimates, vec[i].get_visit_count());
        Py_DECREF(thm);
        Py_DECREF(tactics);
        Py_DECREF(target_pi);
        Py_DECREF(inproof);
        Py_DECREF(q_estimates);
        if (!args) {
            Py_DECREF(list);
            return NULL;
        }
        PyObject *item = PyObject_CallObject((PyObject *) &PyHTPSSampleTacticsType, args);
        Py_DECREF(args);
        if (!item) {
            Py_DECREF(list);
            return NULL;
        }
        PyList_SET_ITEM(list, i, item);
    }
    return list;
}

static PyObject *PyHTPSResult_get_effect_samples(PyHTPSResult *self, void *closure) {
    auto vec = self->cpp_obj.get_effect_samples();
    PyObject *list = PyList_New(vec.size());
    if (!list)
        return PyErr_NoMemory();
    for (size_t i = 0; i < vec.size(); i++) {
        auto thm = Theorem_NewFromShared(vec[i].get_goal());
        if (!thm) {
            Py_DECREF(list);
            return NULL;
        }
        PyObject *tac = Tactic_NewFromShared(vec[i].get_tactic());
        if (!tac) {
            Py_DECREF(thm);
            Py_DECREF(list);
            return NULL;
        }
        PyObject *children = PyList_New(vec[i].get_children().size());
        if (!children) {
            Py_DECREF(thm);
            Py_DECREF(tac);
            Py_DECREF(list);
            return PyErr_NoMemory();
        }
        for (size_t j = 0; j < vec[i].get_children().size(); j++) {
            auto child = Theorem_NewFromShared(vec[i].get_children()[j]);
            if (!child) {
                Py_DECREF(thm);
                Py_DECREF(tac);
                Py_DECREF(children);
                Py_DECREF(list);
                return NULL;
            }
            PyList_SET_ITEM(children, j, child);
        }

        PyObject *args = Py_BuildValue("OOO", thm, tac, children);
        Py_DECREF(thm);
        Py_DECREF(tac);
        Py_DECREF(children);
        if (!args) {
            Py_DECREF(list);
            return NULL;
        }
        PyObject *item = PyObject_CallObject((PyObject *) &PyHTPSSampleEffectType, args);
        Py_DECREF(args);
        if (!item) {
            Py_DECREF(list);
            return NULL;
        }
        PyList_SET_ITEM(list, i, item);
    }
    return list;
}

static PyObject *PyHTPSResult_get_proof_samples_tactics(PyHTPSResult *self, void *closure) {
    auto vec = self->cpp_obj.get_proof_samples();
    PyObject *list = PyList_New(vec.size());
    if (!list)
        return PyErr_NoMemory();
    for (size_t i = 0; i < vec.size(); i++) {
        auto thm = Theorem_NewFromShared(vec[i].get_goal());
        if (!thm) {
            Py_DECREF(list);
            return NULL;
        }
        PyObject *tactics = PyList_New(vec[i].get_tactics().size());
        if (!tactics) {
            Py_DECREF(thm);
            Py_DECREF(list);
            return PyErr_NoMemory();
        }
        for (size_t j = 0; j < vec[i].get_tactics().size(); j++) {
            auto tac = Tactic_NewFromShared(vec[i].get_tactics()[j]);
            if (!tac) {
                Py_DECREF(thm);
                Py_DECREF(tactics);
                Py_DECREF(list);
                return NULL;
            }
            PyList_SET_ITEM(tactics, j, tac);
        }
        PyObject *target_pi = PyList_New(vec[i].get_target_pi().size());
        if (!target_pi) {
            Py_DECREF(thm);
            Py_DECREF(tactics);
            Py_DECREF(list);
            return PyErr_NoMemory();
        }
        for (size_t j = 0; j < vec[i].get_target_pi().size(); j++) {
            PyObject *num = PyFloat_FromDouble(vec[i].get_target_pi()[j]);
            if (!num) {
                Py_DECREF(thm);
                Py_DECREF(target_pi);
                Py_DECREF(tactics);
                Py_DECREF(list);
                return NULL;
            }
            PyList_SET_ITEM(target_pi, j, num);
        }
        PyObject *q_estimates = PyList_New(vec[i].get_q_estimates().size());
        if (!q_estimates) {
            Py_DECREF(thm);
            Py_DECREF(tactics);
            Py_DECREF(target_pi);
            Py_DECREF(list);
            return PyErr_NoMemory();
        }
        for (size_t j = 0; j < vec[i].get_q_estimates().size(); j++) {
            PyObject *num = PyFloat_FromDouble(vec[i].get_q_estimates()[j]);
            if (!num) {
                Py_DECREF(thm);
                Py_DECREF(tactics);
                Py_DECREF(target_pi);
                Py_DECREF(q_estimates);
                Py_DECREF(list);
                return NULL;
            }
            PyList_SET_ITEM(q_estimates, j, num);
        }
        PyObject *inproof = PyObject_CallFunction(InProofEnum, "i", vec[i].get_inproof());

        PyObject *args = Py_BuildValue("OOOOOn", thm, tactics, target_pi, inproof, q_estimates, vec[i].get_visit_count());
        Py_DECREF(thm);
        Py_DECREF(tactics);
        Py_DECREF(target_pi);
        Py_DECREF(inproof);
        Py_DECREF(q_estimates);
        if (!args) {
            Py_DECREF(list);
            return NULL;
        }
        PyObject *item = PyObject_CallObject((PyObject *) &PyHTPSSampleTacticsType, args);
        Py_DECREF(args);
        if (!item) {
            Py_DECREF(list);
            return NULL;
        }
        PyList_SET_ITEM(list, i, item);
    }
    return list;
}

static PyObject *PyHTPSResult_get_metric(PyHTPSResult *self, void *closure) {
    return PyObject_CallFunction(MetricEnum, "i", self->cpp_obj.get_metric());
}

static PyObject *PyHTPSResult_get_goal(PyHTPSResult *self, void *closure) {
    return Theorem_NewFromShared(self->cpp_obj.get_goal());
}

static PyObject *PyHTPSResult_get_proof(PyHTPSResult *self, void *closure) {
    return PyProof_NewFromProof(self->cpp_obj.get_proof());
}

static PyGetSetDef PyHTPSResult_getsetters[] = {
        {"critic_samples", (getter)PyHTPSResult_get_critic_samples, NULL, "Critic samples", NULL},
        {"tactic_samples", (getter)PyHTPSResult_get_tactic_samples, NULL, "Tactic samples", NULL},
        {"effect_samples", (getter)PyHTPSResult_get_effect_samples, NULL, "Effect samples", NULL},
        {"metric", (getter)PyHTPSResult_get_metric, NULL, "Metric", NULL},
        {"proof_samples_tactics", (getter)PyHTPSResult_get_proof_samples_tactics, NULL, "Proof samples", NULL},
        {"goal", (getter)PyHTPSResult_get_goal, NULL, "Goal theorem", NULL},
        {"proof", (getter)PyHTPSResult_get_proof, NULL, "Proof", NULL},
        {NULL}
};

static PyMethodDef PyHTPSResult_methods[] = {
        {NULL, NULL, 0, NULL}
};

PyTypeObject PyHTPSResultType = {
        PyObject_HEAD_INIT(NULL)
        "htps.Result",
        sizeof(PyHTPSResult),
        0,
        (destructor) PyHTPSResult_dealloc,
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
        "Result for a full HTPS run",
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        PyHTPSResult_methods,
        NULL,
        PyHTPSResult_getsetters,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        (initproc)PyHTPSResult_init,
        NULL,
        (newfunc)PyHTPSResult_new,
};

static PyObject* PyHTPSResult_NewFromResult(const htps::HTPSResult &result) {
    PyObject *obj = PyHTPSResult_new(&PyHTPSResultType, NULL, NULL);
    if (obj == NULL)
        return NULL;
    auto *py_result = (PyHTPSResult *) obj;
    py_result->cpp_obj = result;
    return obj;
}

typedef struct {
    PyObject_HEAD
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
    std::vector<std::shared_ptr<htps::theorem>> thms;
    try {
         thms = self->graph.theorems_to_expand();
    } catch (std::exception &e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }

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
    try {
        self->graph.expand_and_backup(expansions);
    } catch (std::exception &e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject* PyHTPS_is_proven(PyHTPS *self, PyObject *Py_UNUSED(ignored)) {
    PyObject * res = self->graph.is_proven() ? Py_True : Py_False;
    Py_INCREF(res);
    return res;
}

static PyObject* PyHTPS_is_done(PyHTPS *self, PyObject *Py_UNUSED(ignored)) {
    PyObject * res = self->graph.is_done() ? Py_True : Py_False;
    Py_INCREF(res);
    return res;
}

static void PyHTPS_dealloc(PyHTPS *self) {
    self->graph.~HTPS();
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* PyHTPS_get_result(PyHTPS *self, PyObject *Py_UNUSED(ignored)) {
    htps::HTPSResult result;
    try {
        result = self->graph.get_result();
    } catch (std::exception &e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return NULL;
    }
    return PyHTPSResult_NewFromResult(result);
}


static PyMethodDef HTPS_methods[] = {
    {"theorems_to_expand", (PyCFunction)PyHTPS_theorems_to_expand, METH_NOARGS, "Returns a list of subsequent theorems to expand"},
    {"expand_and_backup", (PyCFunction)PyHTPS_expand_and_backup, METH_VARARGS,  "Expands and backups using the provided list of EnvExpansion objects"},
    {"proven", (PyCFunction)PyHTPS_is_proven, METH_NOARGS, "Whether the start theorem is proven or not"},
    {"get_result", (PyCFunction)PyHTPS_get_result, METH_NOARGS, "Returns the result of the HTPS run"},
    {"is_done", (PyCFunction)PyHTPS_is_done, METH_NOARGS, "Whether the HTPS run is done or not"},
    {NULL, NULL, 0, NULL}
};

static PyTypeObject HTPSType = {
    PyObject_HEAD_INIT(NULL) "htps.HTPS",
    sizeof(PyHTPS),
    0,
    (destructor)PyHTPS_dealloc,
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

    PyObject *in_proof = make_in_proof(m, enum_mod);
    if (in_proof == NULL) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_XDECREF(in_proof);
        return NULL;
    }
    if (PyModule_AddObject(m, "InProof", in_proof) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(in_proof);
        return NULL;
    }

    if (PyType_Ready(&ParamsType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(in_proof);
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
        Py_DECREF(in_proof);
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
        Py_DECREF(in_proof);
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
        Py_DECREF(in_proof);
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
        Py_DECREF(in_proof);
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
        Py_DECREF(in_proof);
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
        Py_DECREF(in_proof);
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
        Py_DECREF(in_proof);
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
        Py_DECREF(in_proof);
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
        Py_DECREF(in_proof);
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
        Py_DECREF(in_proof);
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
        Py_DECREF(in_proof);
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
        Py_DECREF(in_proof);
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
        Py_DECREF(in_proof);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        Py_DECREF(&EnvEffectType);
        Py_XDECREF(&EnvExpansionType);
        return NULL;
    }

    if (PyType_Ready(&PyHTPSSampleEffectType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(in_proof);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        Py_DECREF(&EnvEffectType);
        Py_DECREF(&EnvExpansionType);
        return NULL;
    }

    Py_INCREF(&PyHTPSSampleEffectType);
    if (PyModule_AddObject(m, "SampleEffect", (PyObject *) &PyHTPSSampleEffectType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(in_proof);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        Py_DECREF(&EnvEffectType);
        Py_DECREF(&EnvExpansionType);
        Py_XDECREF(&PyHTPSSampleEffectType);
        return NULL;
    }

    if (PyType_Ready(&PyHTPSSampleCriticType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(in_proof);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        Py_DECREF(&EnvEffectType);
        Py_DECREF(&EnvExpansionType);
        Py_DECREF(&PyHTPSSampleEffectType);
        return NULL;
    }

    Py_INCREF(&PyHTPSSampleCriticType);
    if (PyModule_AddObject(m, "SampleCritic", (PyObject *) &PyHTPSSampleCriticType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(in_proof);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        Py_DECREF(&EnvEffectType);
        Py_DECREF(&EnvExpansionType);
        Py_DECREF(&PyHTPSSampleEffectType);
        Py_XDECREF(&PyHTPSSampleCriticType);
        return NULL;
    }

    if (PyType_Ready(&PyHTPSSampleTacticsType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(in_proof);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        Py_DECREF(&EnvEffectType);
        Py_DECREF(&EnvExpansionType);
        Py_DECREF(&PyHTPSSampleEffectType);
        Py_DECREF(&PyHTPSSampleCriticType);
        return NULL;
    }

    Py_INCREF(&PyHTPSSampleTacticsType);
    if (PyModule_AddObject(m, "SampleTactics", (PyObject *) &PyHTPSSampleTacticsType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(in_proof);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        Py_DECREF(&EnvEffectType);
        Py_DECREF(&EnvExpansionType);
        Py_DECREF(&PyHTPSSampleEffectType);
        Py_DECREF(&PyHTPSSampleCriticType);
        Py_XDECREF(&PyHTPSSampleTacticsType);
        return NULL;
    }

    if (PyType_Ready(&PyProofType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(in_proof);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        Py_DECREF(&EnvEffectType);
        Py_DECREF(&EnvExpansionType);
        Py_DECREF(&PyHTPSSampleEffectType);
        Py_DECREF(&PyHTPSSampleCriticType);
        Py_DECREF(&PyHTPSSampleTacticsType);
        return NULL;
    }

    Py_INCREF(&PyProofType);
    if (PyModule_AddObject(m, "Proof", (PyObject *) &PyProofType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(in_proof);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        Py_DECREF(&EnvEffectType);
        Py_DECREF(&EnvExpansionType);
        Py_DECREF(&PyHTPSSampleEffectType);
        Py_DECREF(&PyHTPSSampleCriticType);
        Py_DECREF(&PyHTPSSampleTacticsType);
        Py_XDECREF(&PyProofType);
        return NULL;
    }

    if (PyType_Ready(&PyHTPSResultType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(in_proof);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        Py_DECREF(&EnvEffectType);
        Py_DECREF(&EnvExpansionType);
        Py_DECREF(&PyHTPSSampleEffectType);
        Py_DECREF(&PyHTPSSampleCriticType);
        Py_DECREF(&PyHTPSSampleTacticsType);
        Py_DECREF(&PyProofType);
        return NULL;
    }

    Py_INCREF(&PyHTPSResultType);
    if (PyModule_AddObject(m, "Result", (PyObject *) &PyHTPSResultType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(in_proof);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        Py_DECREF(&EnvEffectType);
        Py_DECREF(&EnvExpansionType);
        Py_DECREF(&PyHTPSSampleEffectType);
        Py_DECREF(&PyHTPSSampleCriticType);
        Py_DECREF(&PyHTPSSampleTacticsType);
        Py_DECREF(&PyProofType);
        Py_XDECREF(&PyHTPSResultType);
        return NULL;
    }

    if (PyType_Ready(&HTPSType) < 0) {
        Py_DECREF(m);
        Py_DECREF(enum_mod);
        Py_DECREF(policy_type);
        Py_DECREF(q_value_solved);
        Py_DECREF(node_mask);
        Py_DECREF(metric);
        Py_DECREF(in_proof);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        Py_DECREF(&EnvEffectType);
        Py_DECREF(&EnvExpansionType);
        Py_DECREF(&PyHTPSSampleEffectType);
        Py_DECREF(&PyHTPSSampleCriticType);
        Py_DECREF(&PyHTPSSampleTacticsType);
        Py_DECREF(&PyProofType);
        Py_DECREF(&PyHTPSResultType);
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
        Py_DECREF(in_proof);
        Py_DECREF(&ParamsType);
        Py_DECREF(&HypothesisType);
        Py_DECREF(&TacticType);
        Py_DECREF(&ContextType);
        Py_DECREF(&TheoremType);
        Py_DECREF(&EnvEffectType);
        Py_DECREF(&EnvExpansionType);
        Py_DECREF(&PyHTPSSampleEffectType);
        Py_DECREF(&PyHTPSSampleCriticType);
        Py_DECREF(&PyHTPSSampleTacticsType);
        Py_DECREF(&PyProofType);
        Py_DECREF(&PyHTPSResultType);
        Py_XDECREF(&HTPSType);
        return NULL;
    }

    return m;
}


int main() {
    return 0;
}
