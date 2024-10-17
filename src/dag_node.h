#ifndef DAG_NODE_H
#define DAG_NODE_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include "ast.h"

class ASTNode;

enum class NodeType {
	Author,
	Affiliation
};

class DAGNode {
public:
	DAGNode(const std::string& id_);
	DAGNode(const std::string& id_, NodeType type_);

	std::weak_ptr<ASTNode> getASTNode() const {
        return astNode;
    }

	void addParent(const std::shared_ptr<DAGNode>& parent);
	void addChild(const std::shared_ptr<DAGNode>& child);

	std::string getId() const;
	NodeType getNodeType() const { return nodeType; }

	const std::vector<std::shared_ptr<DAGNode>>& getChildren() const;


	void setASTNode(const std::shared_ptr<ASTNode>& astNode);

private:
	std::string id;                        
	NodeType nodeType;                     
	std::vector<std::shared_ptr<DAGNode>> children;  
	std::vector<std::weak_ptr<DAGNode>> parents;   
	std::weak_ptr<ASTNode> astNode;        
};

class DAG {
public:
	std::shared_ptr<DAGNode> getOrCreateNode(const std::string& id, NodeType type);
	std::shared_ptr<DAGNode> getNode(const std::string& id) const;

	void addAuthorNode(const std::string& authorName);
	void addAffiliationNode(const std::string& affiliationName);

	void linkAuthorToAffiliation(const std::string& authorName, const std::string& affiliationName);

	void generateDOT(const std::string& filename) const;

	void addNode(const std::shared_ptr<DAGNode>& node);
	

private:
	std::unordered_map<std::string, std::shared_ptr<DAGNode>> nodes;
};

#endif

