#ifndef FSM_H
#define FSM_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <stack>
#include <regex>
#include <algorithm>
#include "nlohmann/json.hpp"
#include "ast.h"
#include "fsm.h"

class FSM {
public:
	FSM();
	nlohmann::json chunkDocumentToJson(const std::shared_ptr<ASTNode>& root);
	std::vector<std::string> chunkDocument(const std::shared_ptr<ASTNode>& root);

private:
	enum class FSMState { Start, ProcessingDocument, ProcessingSection, ProcessingCommand, ProcessingText, ProcessingEnvironment };
	FSMState currentState;
	void traverseAST(const std::shared_ptr<ASTNode>& node, std::string& currentChunk, std::vector<std::string>& chunks);
	void handleDocument(const std::shared_ptr<ASTNode>& node, std::string& currentChunk);
	void handleSection(const std::shared_ptr<ASTNode>& node, std::string& currentChunk, std::vector<std::string>& chunks);
	void handleCommand(const std::shared_ptr<ASTNode>& node, std::string& currentChunk);
	void handleText(const std::shared_ptr<ASTNode>& node, std::string& currentChunk);
	void handleEnvironment(const std::shared_ptr<ASTNode>& node, std::string& currentChunk);
	
	std::string extractAuthorName(const std::string& authorCommand);
	std::string extractAffiliation(const std::string& affiliationCommand);
	std::string extractSectionName(const std::string& sectionCommand);
	std::vector<std::string> extractMultipleAuthors(const std::string& authorCommand);
	
	std::string extractMathContent(const std::shared_ptr<ASTNode>& node);

	std::string extractGraphicsFile(const std::string& graphicsCommand); 
	std::string extractFigureCaption(const std::string& captionCommand);
	std::string extractCitationLabel(const std::string& citationCommand);	
	
};

#endif
