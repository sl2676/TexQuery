#include "dag_node.h"

DAGNode::DAGNode(const std::string& id_) : id(id_), nodeType(NodeType::Author) {}

DAGNode::DAGNode(const std::string& id_, NodeType type_) : id(id_), nodeType(type_) {}

void DAGNode::addChild(const std::shared_ptr<DAGNode>& child) {
    children.push_back(child);
}

void DAGNode::addParent(const std::shared_ptr<DAGNode>& parent) {
    parents.push_back(parent);
}


std::string DAGNode::getId() const {
    return id;
}

const std::vector<std::shared_ptr<DAGNode>>& DAGNode::getChildren() const {
    return children;
}

void DAGNode::setASTNode(const std::shared_ptr<ASTNode>& astNode_) {
    astNode = astNode_;
}

void DAG::addNode(const std::shared_ptr<DAGNode>& node) {
    if (node) {
        nodes[node->getId()] = node;
    }
}

std::shared_ptr<DAGNode> DAG::getOrCreateNode(const std::string& id, NodeType type) {
    auto it = nodes.find(id);
    if (it != nodes.end() && it->second) {  
        return it->second;
    }

    auto newNode = std::make_shared<DAGNode>(id, type);
    nodes[id] = newNode;
    return newNode;
}

std::shared_ptr<DAGNode> DAG::getNode(const std::string& id) const {
    auto it = nodes.find(id);
    if (it != nodes.end() && it->second) {  
        return it->second;
    }
    return nullptr; 
}


void DAG::addAuthorNode(const std::string& authorName) {
    auto authorNode = std::make_shared<DAGNode>(authorName, NodeType::Author);
    nodes[authorName] = authorNode;
}

void DAG::addAffiliationNode(const std::string& affiliationName) {
    auto affiliationNode = std::make_shared<DAGNode>(affiliationName, NodeType::Affiliation);
    nodes[affiliationName] = affiliationNode;
}

void DAG::linkAuthorToAffiliation(const std::string& authorName, const std::string& affiliationName) {
    auto authorNode = getNode(authorName);  
    auto affiliationNode = getNode(affiliationName);  

    if (authorNode && affiliationNode) {
        authorNode->addChild(affiliationNode);
        affiliationNode->addParent(authorNode);
    } else {
        std::cerr << "Error: Unable to link author to affiliation. One of the nodes does not exist.\n";
        if (!authorNode) {
            std::cerr << "Missing author node: " << authorName << "\n";
        }
        if (!affiliationNode) {
            std::cerr << "Missing affiliation node: " << affiliationName << "\n";
        }
    }
}


void DAG::generateDOT(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file \"" << filename << "\" for writing.\n";
        return;
    }

    file << "digraph DAG {\n";
    file << "    node [shape=ellipse, style=filled, color=lightgreen];\n";

    std::unordered_map<std::shared_ptr<DAGNode>, int> nodeIds;
    int nodeId = 0;

    for (const auto& pair : nodes) {
        const auto& dagNode = pair.second;
        if (dagNode) {
            nodeIds[dagNode] = nodeId++;
            std::string label = dagNode->getId();

            if (auto astPtr = dagNode->getASTNode().lock()) {
                label += "\\nAST Node: " + astPtr->getContent();
            }

            file << "    node" << nodeIds[dagNode] << " [label=\"" << label << "\"];\n";
        }
    }

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

