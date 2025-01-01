#ifndef FSM_H
#define FSM_H

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <sstream>
#include <regex>
#include <iostream>
#include <codecvt>
#include <locale>
#include <unordered_map>
#include <set>
#include <stack>
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

class ParserError : public std::runtime_error {
    size_t position;
public:
    ParserError(const std::string& msg, size_t pos) 
        : std::runtime_error(msg), position(pos) {}
    
    size_t getPosition() const { return position; }
};

class FSM {
public:
    enum class FSMState {
        Start,              // 1
        InDocument,         // 2
        // Title section states
        InTitle,            // 3
        InAuthor,           // 4
        InAffiliation,      // 5
        InDate,             // 6
        InAbstract,         // 7
        InKeywords,         // 8
        // Main content states
        InSection,          // 9
        InSubsection,       // 10
        InParagraph,        // 11
        // Math states
        InMath,             // 12
        InInlineMath,       // 13
        InEquation,         // 14
        InAlignedEquation,  // 15
        // Float states
        InFigure,           // 16
        InTable,            // 17
        InAlgorithm,        // 18
        InListing,          // 19
        // Citation states
        InCitation,         // 20
        InBibliography,     // 21
        // Special environments
        InTheorem,          // 22
        InProof,            // 23
        InDefinition,       // 24
        InLemma,            // 25
        InCorollary,        // 25
        // Command states
        InCommand,          // 26
        InText,             // 27
        InEnvironment,      // 28
        // Reference states
        InLabel,            // 29
        InRef,              // 30
        InCrossRef          // 31
    };

    FSM();
    
    void traverseAST(const std::shared_ptr<ASTNode>& node, std::string& currentChunk, 
                    std::vector<std::string>& chunks);
    nlohmann::json chunkDocumentToJson(const std::shared_ptr<ASTNode>& root);
    std::vector<std::string> chunkDocument(const std::shared_ptr<ASTNode>& root);
    DAG& getDAG();
    
    std::string getCurrentContext() const;
    FSMState getCurrentState() const;

private:
    FSMState currentState;
    static const std::unordered_map<FSMState, std::set<FSMState>> validTransitions;
    
    struct ParserContext {
        std::vector<json> semanticChunks;
        void addSemanticChunk(const json& chunk) {
            semanticChunks.push_back(chunk);
        }

        json getSemanticChunks() const {
            json result;
            result["chunks"] = semanticChunks;
            return result;
        }

        std::shared_ptr<DAGNode> currentNode;
        std::unordered_map<std::string, std::shared_ptr<DAGNode>> labeledNodes;
        std::shared_ptr<DAGNode> getCurrentNode() const { return currentNode; }
        void setCurrentNode(std::shared_ptr<DAGNode> node) { currentNode = node; }
    
        void addLabeledNode(const std::string& label, std::shared_ptr<DAGNode> node) {
            labeledNodes[label] = node;
        }
    
        std::shared_ptr<DAGNode> getLabeledNode(const std::string& label) const {
            auto it = labeledNodes.find(label);
            return it != labeledNodes.end() ? it->second : nullptr;
        }
    

        std::stack<std::string> environmentStack;
        std::unordered_map<std::string, std::shared_ptr<ASTNode>> labels;
        std::vector<std::string> citations;
        
        void pushEnvironment(const std::string& env);
        void popEnvironment();
        bool isInEnvironment(const std::string& env) const;
    };

    void handleError(const std::string& context, const std::exception& e);



    std::shared_ptr<DAGNode> currentEnvironmentNode;

    bool isTransitionValid(FSMState from, FSMState to) const;
    void validateStateTransition(FSMState newState);
    void setState(FSMState newState);
    bool isValidStructure(const std::shared_ptr<ASTNode>& node) const;

    void handleEmailCommand(const std::string& cmdArgs);
    void handleDocument(const std::shared_ptr<ASTNode>& node, std::string& currentChunk);
    void handleSection(const std::shared_ptr<ASTNode>& node, std::string& currentChunk, std::vector<std::string>& chunks);
    void handleCommand(const std::shared_ptr<ASTNode>& node, std::string& currentChunk);
    void handleText(const std::shared_ptr<ASTNode>& node, std::string& currentChunk);
    void handleEnvironment(const std::shared_ptr<ASTNode>& node, std::string& currentChunk);
    void handleMath(const std::shared_ptr<ASTNode>& node, std::string& currentChunk);

    void handleAuthorCommand(const std::string& args);
    void handleAffiliationCommand(const std::string& args);
    void handleCitationCommand(const std::string& args);
    void handleFloatEnvironment(const std::string& type, const std::shared_ptr<ASTNode>& node, std::string& currentChunk);
    void handleAbstractEnvironment(const std::shared_ptr<ASTNode>& node, std::string& currentChunk);

    std::shared_ptr<DAGNode> createOrGetDAGNode(const std::string& content, ASTNode::NodeType astType);

    std::string extractAuthorName(const std::string& authorCommand);
    std::vector<std::string> extractMultipleAuthors(const std::string& authorTexCommand);
    std::string extractAffiliation(const std::string& affiliationCommand);
    std::string extractSectionName(const std::string& sectionCommand);
    std::string extractGraphicsFile(const std::string& graphicsCommand);
    std::string extractFigureCaption(const std::string& captionCommand);
    std::string extractCitationLabel(const std::string& citationCommand);
    std::string extractContentBetweenBraces(const std::string& str, size_t start_pos);

    void parseInstitutions(const std::string& instituteBlock, 
                          std::unordered_map<std::string, std::string>& affiliationMap);
    void parseAffiliations(const std::string& affiliationBlock, std::unordered_map<std::string, std::string>& affiliationMap, std::vector<Author>& authors, std::shared_ptr<DAGNode> currentAuthorDAGNode);
    void parseAuthors(const std::string& authorBlock, std::vector<Author>& authors, const std::unordered_map<std::string, std::string>& affiliationMap);
    void processAuthorCommand(const std::string& content, Author& author);
    void parseAuthorsStyle1(const std::string& authorBlock, std::vector<Author>& authors, const std::unordered_map<std::string, std::string>& affiliationMap);
    void parseAuthorsStyle2(const std::string& content, std::vector<Author>& authors);
    void linkUnlabeledAffiliations(std::vector<Author>& authors, std::vector<std::string>& unlabeledAffiliations);


    bool isDocumentCommand(const std::string& cmd) const;
    bool isSectionCommand(const std::string& cmd) const;
    bool isMathCommand(const std::string& cmd) const;
    bool isTheoremCommand(const std::string& cmd) const;
    bool isFloatCommand(const std::string& cmd) const;
    bool isCitationCommand(const std::string& cmd) const;
    bool isReferenceCommand(const std::string& cmd) const;

    void handleDocumentCommand(const std::string& cmd, const std::vector<std::string>& args, std::string& currentChunk) ;   
    void handleSectioningCommand(const std::string& cmd, const std::vector<std::string>& args, std::string& currentChunk);
    void handleMathematicalContent(const std::string& cmd, const std::vector<std::string>& args, std::string& currentChunk);
    void handleTheoremEnvironment(const std::string& cmd, const std::vector<std::string>& args, std::string& currentChunk);
    void handleFloatEnvironment(const std::string& cmd, const std::vector<std::string>& args, std::string& currentChunk);
    void handleCitationCommand(const std::string& cmd, const std::vector<std::string>& args, std::string& currentChunk);
    void handleReferenceCommand(const std::string& cmd, const std::vector<std::string>& args, std::string& currentChunk);
    void handleGenericCommand(const std::string& cmd, const std::string& cmdOpts, const std::vector<std::string>& args, std::string& currentChunk);
    
    std::string removeInvalidUTF8(const std::string& input);
    std::string cleanAuthor(const std::string& author);
    std::string removeNewlines(const std::string& input);
    void processAffiliations(nlohmann::json& currentAuthor, const std::string& content, nlohmann::json& documentJson, bool& authorActive);

    void parseAuthorBlock(const std::string& content);
    std::string extractNextAuthor(const std::string& content, size_t& pos);
    std::shared_ptr<DAGNode> createAuthorNode(const Author& author);
    void processAffiliations(const Author& author, std::shared_ptr<DAGNode> authorNode);
    void addAffiliationNode(std::shared_ptr<DAGNode> authorNode, const std::string& label);
    void addAffiliationNode(std::shared_ptr<DAGNode> authorNode, int index);
    void handleStateChange(FSMState oldState, FSMState newState);
    std::string getStateName(FSMState state) const;

    ParserContext context;
    std::unordered_map<std::string, std::string> affiliationMap;
    std::vector<Author> authors;
    std::vector<std::string> unlabeledAffiliations;
    DAG dag;
    NER ner;
    bool insideAuthorBlock;
    std::string authorBlockContent;
    bool insideInstituteBlock;
    std::string instituteBlockContent;

    bool isInEnvironment(const std::string& env) const;

struct DAGContext {
    std::shared_ptr<DAGNode> currentSection;
    std::shared_ptr<DAGNode> currentEnvironment;
    std::vector<std::shared_ptr<DAGNode>> sectionStack;
    std::unordered_map<std::string, std::shared_ptr<DAGNode>> labelMap;
    std::shared_ptr<DAGNode> currentReference;
    
    void pushSection(std::shared_ptr<DAGNode> section) {
        currentSection = section;
        sectionStack.push_back(section);
    }
    
    void popSection() {
        if (!sectionStack.empty()) {
            sectionStack.pop_back();
            currentSection = sectionStack.empty() ? nullptr : sectionStack.back();
        }
    }
} dagContext;

};

#endif
