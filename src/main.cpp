#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "fsm.h"
#include "nlohmann/json.hpp"

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cerr << "Usage: ./parser <file.tex>\n";
		return 1;
	}

	std::ifstream file(argv[1]);
	if (!file.is_open()) {
		std::cerr << "Could not open the file: " << argv[1] << "\n";
		return 1;
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
		
		std::ofstream jsonFile("document.json");
		if (jsonFile.is_open()) {
			jsonFile << jsonDocument.dump(4);
			jsonFile.close();
			std::cout << "Document successfully written to document.json\n";
		} else {
			std::cerr << "Error opening file for writing JSON output.\n";
		}
	
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	}

	return 0;
}


	
