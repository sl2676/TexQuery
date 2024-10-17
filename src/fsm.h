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
#include <unordered_map>

#include <nlohmann/json.hpp>
#include "ast.h" 
#include "dag_node.h" 
#include "ner.h"

class ASTNode;

struct Author {
    std::string name;
    std::string email;
    std::string orcid;
    std::vector<int> affiliationIndices;
    std::vector<std::string> affiliationLabels; 
    std::vector<std::string> affiliations;     
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
	DAG& getDAG();

private:
    FSMState currentState;
    
    std::unordered_map<std::string, std::string> affiliationMap;
    std::vector<Author> authors;
    std::vector<std::string> unlabeledAffiliations;

    DAG dag;
	NER ner;  

	bool insideAuthorBlock = false;
    std::string authorBlockContent;
    bool insideInstituteBlock = false;
    std::string instituteBlockContent;

	std::shared_ptr<DAGNode> createOrGetDAGNode(const std::string& content, NodeType type);

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

//    void parseAffiliations(const std::string& instituteBlock, std::unordered_map<std::string, std::string>& affiliationMap, std::vector<std::string>& unlabeledAffiliations);
	void parseAffiliations(const std::string& affiliationBlock, std::unordered_map<std::string, std::string>& affiliationMap, std::vector<Author>     & authors, std::shared_ptr<DAGNode> currentAuthorDAGNode);	 
    void parseAuthors(const std::string& authorBlock, std::vector<Author>& authors, const std::unordered_map<std::string, std::string>& affiliationMap);
    void processAuthorCommand(const std::string& content, Author& author);
    void parseAuthorsStyle1(const std::string& authorBlock, std::vector<Author>& authors, const std::unordered_map<std::string, std::string>& affiliationMap);
    void parseAuthorsStyle2(const std::string& content, std::vector<Author>& authors);
	void linkUnlabeledAffiliations(std::vector<Author>& authors, std::vector<std::string>& unlabeledAffiliations);


    std::string extractContentBetweenBraces(const std::string& str, size_t start_pos);
    std::string removeInvalidUTF8(const std::string& input);
    std::string cleanAuthor(const std::string& author);
    std::string removeNewlines(const std::string& input);

    void processAffiliations(nlohmann::json& currentAuthor, const std::string& content, nlohmann::json& documentJson, bool& authorActive);
};

#endif

