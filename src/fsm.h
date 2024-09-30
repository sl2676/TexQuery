#ifndef FSM_H
#define FSM_H

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <sstream>
#include <regex>
#include <iostream>
#include <codecvt>
#include <locale>

#include <nlohmann/json.hpp>
#include "ast.h" 



class ASTNode;

struct Author {
    std::string name;
    std::vector<int> affiliationIndices;
    std::string email;
    std::string orcid;
};

struct Affiliation {
    int index;
    std::string details;
};

class FSM {
public:
    enum class FSMState {
        Start,
        InDocument,
        InSection,
        InCommand,
        InText,
        InEnvironment,
        InEquation
    };

    FSM();

    void traverseAST(const std::shared_ptr<ASTNode>& node, std::string& currentChunk, std::vector<std::string>& chunks);

    nlohmann::json chunkDocumentToJson(const std::shared_ptr<ASTNode>& root);

    std::vector<std::string> chunkDocument(const std::shared_ptr<ASTNode>& root);

private:
    FSMState currentState;

    void handleDocument(const std::shared_ptr<ASTNode>& node, std::string& currentChunk);
    void handleSection(const std::shared_ptr<ASTNode>& node, std::string& currentChunk, std::vector<std::string>& chunks);
    void handleCommand(const std::shared_ptr<ASTNode>& node, std::string& currentChunk);
    void handleText(const std::shared_ptr<ASTNode>& node, std::string& currentChunk);
    void handleEnvironment(const std::shared_ptr<ASTNode>& node, std::string& currentChunk);

    std::string extractAuthorName(const std::string& authorCommand);
    std::vector<std::string> extractMultipleAuthors(const std::string& authorTexCommand);
    std::string extractAffiliation(const std::string& affiliationCommand);
    std::string extractSectionName(const std::string& sectionCommand);
    std::string extractGraphicsFile(const std::string& graphicsCommand);
    std::string extractFigureCaption(const std::string& captionCommand);
    std::string extractCitationLabel(const std::string& citationCommand);

	void parseAffiliations(const std::string& instituteBlock, std::vector<Affiliation>& affiliations);
	void parseAuthors(const std::string& authorBlock, std::vector<Author>& authors);

    std::string extractContentBetweenBraces(const std::string& str, size_t start_pos);
    std::string removeInvalidUTF8(const std::string& input);
    std::string cleanAuthor(const std::string& author);
    std::string removeNewlines(const std::string& input);

    void processAffiliations(nlohmann::json& currentAuthor, const std::string& content, nlohmann::json& documentJson, bool& authorActive);
};

#endif

