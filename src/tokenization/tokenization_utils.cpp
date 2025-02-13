#include "tokenization_utils.h"
#include <onmt/BPELearner.h>
#include <onmt/BPE.h>
#include <onmt/Tokenizer.h>
#include <glob/glob.h>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace onmt;

BPELearner learn_from_lean_files(const std::filesystem::path &path, const std::filesystem::path &model_path) {
    BPELearner learner = {true, 65536, 5, false, true};

    Tokenizer tokenizer(Tokenizer::Mode::Conservative);
    for (auto const &p: glob::glob(path.string() + "/**/*.lean")) {
        std::ifstream file(p);
        learner.ingest(file, nullptr);
    }

    const char *description = "Initial BPE Model test";
    std::ofstream out(model_path);
    learner.learn(out, description, true);
    return learner;
}

BPE bpe_from_model_file(const std::filesystem::path &path) {
    return {path.string()};
}

Tokenizer tokenizer_from_model_file(const std::filesystem::path &path) {
    return {Tokenizer::Mode::Conservative, 0, path.string()};
}
