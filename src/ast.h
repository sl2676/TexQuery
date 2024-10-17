#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_set>
#include <iostream>
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
		EnvironmentContent
	};

	ASTNode(NodeType type, const std::string& content, size_t position, ParserState state);
	
	std::string getContent() const;

	NodeType getType() const {
		return type;
	}

	std::string getNodeTypeName(ASTNode::NodeType type) const;

	const std::vector<std::shared_ptr<ASTNode>>& getChildren() const;

	void addChild(const std::shared_ptr<ASTNode>& child);
	void addReference(const std::shared_ptr<ASTNode>& node);

	void setDAGNode(const std::shared_ptr<DAGNode>& dagNode);
	std::shared_ptr<DAGNode> getDAGNode() const;

	void print(int indent = 0) const;

private:
	NodeType type;
	std::string content;
	size_t position;
	ParserState state;

	std::vector<std::shared_ptr<ASTNode>> children;
	std::vector<std::weak_ptr<ASTNode>> references;

	std::weak_ptr<DAGNode> dagNode;  

	void printHelper(int indent, std::unordered_set<const ASTNode*>& visitedNodes) const;
};

class AST {
public:
	AST();
	void print() const;
	std::vector<std::string> chunk() const;
	std::shared_ptr<ASTNode> root;
};

#endif

