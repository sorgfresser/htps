#include "tokenizer.h"
#include "tokenization_utils.h"
#include <onmt/Tokenizer.h>
#include <iostream>

using namespace onmt;

onmt::Tokenizer htps::get_tokenizer_from_env() {
    const char *tokenizer_path = std::getenv("TOKENIZER_PATH");
    // If we are running tests, we don't want to throw an error if the env var is not set
    if (!tokenizer_path || std::string(tokenizer_path).empty()) {
#ifdef VERBOSE_PRINTS
        printf("TOKENIZER_PATH not set, using default tokenizer");
#endif
        return {Tokenizer::Mode::None, Tokenizer::Flags::NoSubstitution};
    }
    return tokenizer_from_model_file(tokenizer_path);
}

// We'll create a global_tokenizer using the TOKENIZER_PATH env var
onmt::Tokenizer htps::global_tokenizer = htps::get_tokenizer_from_env();
