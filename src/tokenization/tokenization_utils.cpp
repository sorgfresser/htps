#include "tokenization_utils.h"
#include <onmt/BPELearner.h>
#include <onmt/BPE.h>
#include <onmt/Tokenizer.h>
#include <glob/glob.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include "../model/specialtokens.h"


using namespace onmt;
using namespace htps;

constexpr size_t N_SYMBOLS = 65536;

BPELearner htps::learn_from_lean_files(const std::filesystem::path &path, const std::filesystem::path &model_path) {
    BPELearner learner = {true, N_SYMBOLS, 5, false, true};

    for (const auto special_token: SPECIAL_TOKENS) {
        learner.add_special_token(special_token);
    }

    for (auto const &p: glob::glob(path.string() + "/**/*.lean")) {
        std::ifstream file(p);
        learner.ingest(file, nullptr);
    }

    const char *description = "Initial BPE Model test";
    std::ofstream out(model_path);
    learner.learn(out, description, true);
    return learner;
}

BPE htps::bpe_from_model_file(const std::filesystem::path &path) {
    BPE bpe = {path.string()};
    return bpe;
}

Tokenizer htps::tokenizer_from_model_file(const std::filesystem::path &path) {
    BPE *bpe = new BPE(bpe_from_model_file(path));
    return {Tokenizer::Mode::Space, bpe, Tokenizer::Flags::NoSubstitution};
}
