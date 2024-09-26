#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <filesystem> 
#include <vector>
#include <string>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "fsm.h"
#include "nlohmann/json.hpp"

namespace fs = std::filesystem; 

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./parser <directory>\n";
        return 1;
    }
	std::vector<std::string> paper_names;
	std::vector<std::string> paper_json;
    fs::path inputPath(argv[1]);
    if (!fs::exists(inputPath)) {
        std::cerr << "The path does not exist: " << argv[1] << "\n";
        return 1;
    }

    if (!fs::is_directory(inputPath)) {
        std::cerr << "The path is not a directory: " << argv[1] << "\n";
        return 1;
    }

    fs::path outputDir = "json_output";
    fs::create_directories(outputDir);

    for (const auto& entry : fs::recursive_directory_iterator(inputPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".tex") {
            std::cout << "Processing file: " << entry.path() << "\n";
			//paper_names.push_back(entry.path());
            std::ifstream file(entry.path());
            if (!file.is_open()) {
                std::cerr << "Could not open the file: " << entry.path() << "\n";
                continue;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string input = buffer.str();

            Lexer lexer(input);
            Parser parser(lexer);

            try {
                std::shared_ptr<AST> ast = parser.parseDocument();
                std::cout << "Printing AST structure:\n";
                ast->print();

                FSM fsm;
                nlohmann::json jsonDocument = fsm.chunkDocumentToJson(ast->root);

                fs::path jsonFilePath = outputDir / (entry.path().stem().string() + ".json");

                std::ofstream jsonFile(jsonFilePath);
                if (jsonFile.is_open()) {
                    jsonFile << jsonDocument.dump(4);
                    jsonFile.close();
                    std::cout << "Document successfully written to " << jsonFilePath << "\n";
                	paper_json.push_back(jsonFilePath);
					paper_names.push_back(entry.path());
				} else {
                    std::cerr << "Error opening file for writing JSON output: " << jsonFilePath << "\n";
                }

            } catch (const std::exception& e) {
                std::cerr << "Error processing file " << entry.path() << ": " << e.what() << "\n";
                continue;
            }
        }
    }
	for (const auto& file : paper_names) {
		std::cout << "FILE: " << file << std::endl;
	}
	for (const auto& file: paper_json) {
		std::cout << "JSON: " << file << std::endl;		
	}
	
    return 0;
}

