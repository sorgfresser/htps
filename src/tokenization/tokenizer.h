//
// Created by simon on 12.02.25.
//

#ifndef HTPS_TOKENIZER_H
#define HTPS_TOKENIZER_H

#include <onmt/Tokenizer.h>
#include <filesystem>

namespace htps {
    extern onmt::Tokenizer global_tokenizer;

    onmt::Tokenizer get_tokenizer_from_env();
}

#endif //HTPS_TOKENIZER_H
