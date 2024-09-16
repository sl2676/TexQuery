#include "dag_node.h"
#include "ast.h"
#include <iostream>
#include <fstream>

// Constructor initializes the DAG node with a unique identifier
DAGNode::DAGNode(const std::string& id_) : id(id_) {}

// Adds a parent DAG node to this node
void DAGNode::addParent(const std::shared_ptr<DAGNode>& parent) {
    if (parent) {
        parents.push_back(parent);
    } else {
        std::cerr << "Warning: Attempted to add a null parent to DAGNode \"" << id << "\"\n";
    }
}

// Adds a child DAG node to this node
void DAGNode::addChild(const std::shared_ptr<DAGNode>& child) {
    if (child) {
        children.push_back(child);
    } else {
        std::cerr << "Warning: Attempted to add a null child to DAGNode \"" << id << "\"\n";
    }
}

// Getter for id
std::string DAGNode::getId() const {
    return id;
}

// Getter for children
const std::vector<std::shared_ptr<DAGNode>>& DAGNode::getChildren() const {
    return children;
}

// Generates a DOT file representing the DAG
void DAG::generateDOT(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file \"" << filename << "\" for writing.\n";
        return;
    }

    file << "digraph DAG {\n";
    file << "    node [shape=ellipse, style=filled, color=lightgreen];\n";

    // Assign unique integer IDs to DAGNodes
    std::unordered_map<std::shared_ptr<DAGNode>, int> nodeIds;
    int nodeId = 0;

    for (const auto& pair : nodes) {
        const auto& dagNode = pair.second;
        if (dagNode) {
            nodeIds[dagNode] = nodeId++;
            std::string label = dagNode->getId();

            // Optionally, include ASTNode content
            if (auto astPtr = dagNode->astNode.lock()) {
                label += "\\nAST Node: " + astPtr->getContent();
            }

            file << "    node" << nodeIds[dagNode] << " [label=\"" << label << "\"];\n";
        }
    }

    // Add edges
    for (const auto& pair : nodes) {
        const auto& dagNode = pair.second;
        if (dagNode) {
            int fromId = nodeIds[dagNode];
            for (const auto& child : dagNode->getChildren()) {
                if (child && nodeIds.find(child) != nodeIds.end()) {
                    int toId = nodeIds[child];
                    file << "    node" << fromId << " -> node" << toId << ";\n";
                }
            }
        }
    }

    file << "}\n";
    file.close();
    std::cout << "DAG DOT file generated: " << filename << "\n";
}

