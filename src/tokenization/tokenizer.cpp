#include "tokenizer.h"
#include "tokenization_utils.h"
#include <onmt/Tokenizer.h>
#include <iostream>

using namespace onmt;

onmt::Tokenizer get_tokenizer_from_env() {
    const char *tokenizer_path = std::getenv("TOKENIZER_PATH");
    if (!tokenizer_path || std::string(tokenizer_path).empty()) {
        throw std::runtime_error("TOKENIZER_PATH not set");
    }
    return tokenizer_from_model_file(tokenizer_path);
}

// We'll create a global_tokenizer but initialize it in main with fallback logic.
onmt::Tokenizer global_tokenizer = get_tokenizer_from_env();
