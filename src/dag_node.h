#ifndef DAG_NODE_H
#define DAG_NODE_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <fstream>


class ASTNode;



class DAGNode {
public:
	DAGNode(const std::string& id_);
	void addParent(const std::shared_ptr<DAGNode>& parent);
	void addChild(const std::shared_ptr<DAGNode>& child);
	std::string getId() const;
	const std::vector<std::shared_ptr<DAGNode>>& getChildren() const;
	std::weak_ptr<ASTNode> astNode;

private:
	std::string id;
	std::vector<std::shared_ptr<DAGNode>> children;
	std::vector<std::shared_ptr<DAGNode>> parents;
};

class DAG {
public:
	void addNode(const std::shared_ptr<DAGNode>& node) {
		nodes[node->astNode.lock()] = node;
	}
	void generateDOT(const std::string& filename) const;

private:
	std::unordered_map<std::shared_ptr<ASTNode>, std::shared_ptr<DAGNode>> nodes;
};

#endif
