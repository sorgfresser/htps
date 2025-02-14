#ifndef HTPS_TOKENIZATION_UTILS_H
#define HTPS_TOKENIZATION_UTILS_H
#include <onmt/BPELearner.h>
#include <onmt/BPE.h>
#include <onmt/Tokenizer.h>
#include <filesystem>


namespace htps {
    onmt::BPELearner learn_from_lean_files(const std::filesystem::path &path, const std::filesystem::path &model_path);
    onmt::BPE bpe_from_model_file(const std::filesystem::path &path);
    onmt::Tokenizer tokenizer_from_model_file(const std::filesystem::path &path);
}

#endif //HTPS_TOKENIZATION_UTILS_H
