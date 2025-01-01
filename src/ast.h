#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <iostream>
#include <stdexcept>
#include "parser_state.h"
#include "dag_node.h"

class DAGNode;

class ASTNode {
public:
    enum class NodeType {
        Document,
        Section,
        Command,
        Environment,
        Math,
        Text,
        Label,
        Reference,
        EnvironmentContent,
        Author,
        Affiliation,
        Abstract,
        Citation,
        Bibliography
    };

    ASTNode(NodeType type, const std::string& content, size_t position, ParserState state);
    
    std::string getContent() const;
    NodeType getType() const;
    ParserState getState() const;
    size_t getPosition() const;
    
    std::string getNodeTypeName(ASTNode::NodeType type) const;

    const std::vector<std::shared_ptr<ASTNode>>& getChildren() const;

    void addChild(const std::shared_ptr<ASTNode>& child);
    void addReference(const std::shared_ptr<ASTNode>& node);
    
    void setDAGNode(const std::shared_ptr<DAGNode>& dagNode);
    std::shared_ptr<DAGNode> getDAGNode() const;
    
    void print(int indent = 0) const;

    int getSectionLevel() const { 
        if (type == NodeType::Section) {
            if (content.find("\\section") != std::string::npos) return 1;
            if (content.find("\\subsection") != std::string::npos) return 2;
            if (content.find("\\subsubsection") != std::string::npos) return 3;
        }
        return 0;
    }

protected:
    void validateNode();
    void validateMathContent();
    void validateSectionContent();
    void validateCommandContent();
    bool isValidChild(const std::shared_ptr<ASTNode>& child) const;

private:
    NodeType type;
    std::string content;
    size_t position;
    ParserState state;
    std::vector<std::shared_ptr<ASTNode>> children;
    std::vector<std::weak_ptr<ASTNode>> references;
    std::weak_ptr<DAGNode> dagNode;
    
    void printHelper(int indent, std::unordered_set<const ASTNode*>& visitedNodes) const;
    
    static const std::unordered_map<NodeType, std::set<NodeType>> validChildTypes;
};


class AST {
public:
    AST();
    void print() const;
    std::vector<std::string> chunk() const;
    std::shared_ptr<ASTNode> root;
};

class ASTError : public std::runtime_error {
public:
    ASTError(const std::string& msg, ASTNode::NodeType type, size_t pos) 
        : std::runtime_error(msg), nodeType(type), position(pos) {}
    
    ASTNode::NodeType getNodeType() const { return nodeType; }
    size_t getPosition() const { return position; }

private:
    ASTNode::NodeType nodeType;
    size_t position;
};

#endif
