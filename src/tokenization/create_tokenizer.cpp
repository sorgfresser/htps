#include <filesystem>
#include "tokenization_utils.h"
#include <iostream>

using namespace htps;

int main(int argc, char* argv[]) {
    // Attempt to set up our tokenizer.
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <lean_files_directory> <model_output_path>" << std::endl;
        return 1;
    }

    std::filesystem::path lean_dir = argv[1];
    std::filesystem::path model_path = argv[2];

    try {
        learn_from_lean_files(lean_dir, model_path);
        std::cout << "BPE model successfully learned and saved to " << model_path << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
