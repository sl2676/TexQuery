#include "ast.h"


ASTNode::ASTNode(NodeType type, const std::string& content, size_t position, ParserState state)  
    : type(type), content(content), position(position), state(state) {}


std::string ASTNode::getContent() const {
    return content;
}

std::string ASTNode::getNodeTypeName(ASTNode::NodeType type) const {
    switch (type) {
        case ASTNode::NodeType::Command:
            return "Command";
        case ASTNode::NodeType::Text:
            return "Text";
        case ASTNode::NodeType::Section:
            return "Section";
        case ASTNode::NodeType::Math:
            return "Math";
        case ASTNode::NodeType::Document:
            return "Document";
        default:
            return "Unknown";
    }
}

const std::vector<std::shared_ptr<ASTNode>>& ASTNode::getChildren() const {
    return children;
}

void ASTNode::addChild(const std::shared_ptr<ASTNode>& child) {
    if (child) {
        children.push_back(child);
    } else {
        std::cerr << "Warning: Attempted to add a null child to node \"" << content << "\"\n";
    }
}

void ASTNode::addReference(const std::shared_ptr<ASTNode>& node) {
    if (node) {
        references.push_back(node);
    } else {
        std::cerr << "Warning: Attempted to add a null reference to node \"" << content << "\"\n";
    }
}

void ASTNode::setDAGNode(const std::shared_ptr<DAGNode>& dagNode_) {
    dagNode = dagNode_;
}

std::shared_ptr<DAGNode> ASTNode::getDAGNode() const {
    return dagNode.lock();
}

void ASTNode::print(int indent) const {
    std::unordered_set<const ASTNode*> visitedNodes;
    printHelper(indent, visitedNodes);
}

void ASTNode::printHelper(int indent, std::unordered_set<const ASTNode*>& visitedNodes) const {
    if (visitedNodes.find(this) != visitedNodes.end()) {
        std::string indentStr(indent, ' ');
        std::cout << indentStr << "[Already printed node: \"" << content << "\"]\n";
        return;
    }

    visitedNodes.insert(this);
    std::string indentStr(indent, ' ');

    static bool inAuthorGroup = false;

    if (content.find("\\author") != std::string::npos) {
        if (inAuthorGroup) {
            std::cout << indentStr << "[End of Author-Affiliation Group]\n";
        }

        std::cout << indentStr << "[Author-Affiliation Group]\n";
        inAuthorGroup = true;

        std::cout << indentStr << "  Node Type: " << getNodeTypeName(type)
                  << ", Content: \"" << content << "\"\n";
    }

    else if (inAuthorGroup && content.find("\\affiliation") != std::string::npos) {
        std::cout << indentStr << "  Node Type: " << getNodeTypeName(type)
                  << ", Content: \"" << content << "\"\n";
    }

    else if (inAuthorGroup && content.find("\\affiliation") == std::string::npos && content.find("\\author") == std::string::npos) {
        std::cout << indentStr << "[End of Author-Affiliation Group]\n";
        inAuthorGroup = false;

        std::cout << indentStr << "Node Type: " << getNodeTypeName(type)
                  << ", Content: \"" << content << "\"\n";
    }

    else if (!inAuthorGroup) {
        std::cout << indentStr << "Node Type: " << getNodeTypeName(type)
                  << ", Content: \"" << content << "\"\n";
    }

    for (const auto& child : children) {
        child->printHelper(indent + 2, visitedNodes);
    }

    visitedNodes.erase(this);
}

// Define the AST class constructor and member functions
AST::AST() {
    root = std::make_shared<ASTNode>(ASTNode::NodeType::Document, "Document Root", 0, ParserState::DefaultState);
}

void AST::print() const {
    if (root) {
        root->print();
    }
}

std::vector<std::string> AST::chunk() const {
    std::vector<std::string> chunks;

    std::function<void(const std::shared_ptr<ASTNode>&, std::string&)> traverse;
    traverse = [&](const std::shared_ptr<ASTNode>& node, std::string& currentChunk) {
        if (node->getContent().find("\\author") != std::string::npos ||
            node->getContent().find("\\affiliation") != std::string::npos) {
            currentChunk += "[Author-Affiliation Group] " + node->getContent() + " ";
        }
        else if (node->getContent().find("\\begin{abstract}") != std::string::npos) {
            currentChunk += "[Abstract Start] " + node->getContent() + " ";
            for (const auto& child : node->getChildren()) {
                currentChunk += child->getContent() + " ";
            }
            currentChunk += "[Abstract End] ";
        }
        else if (node->getContent().find("\\begin{equation}") != std::string::npos ||
                 node->getContent().find("$") != std::string::npos) {
            currentChunk += "[Math Start] " + node->getContent() + " ";
            for (const auto& child : node->getChildren()) {
                currentChunk += child->getContent() + " ";
            }
            currentChunk += "[Math End] ";
        }
        else if (node->getContent().find("\\begin{thebibliography}") != std::string::npos) {
            currentChunk += "[Bibliography Start] " + node->getContent() + " ";
            for (const auto& child : node->getChildren()) {
                if (child->getContent().find("\\bibitem") != std::string::npos) {
                    currentChunk += "[Citation: " + child->getContent() + "] ";
                } else {
                    currentChunk += child->getContent() + " ";
                }
            }
            currentChunk += "[Bibliography End] ";
        }
        else if (node->getContent().find("\\section") != std::string::npos ||
                 node->getContent().find("\\subsection") != std::string::npos) {
            currentChunk += "[Section] " + node->getContent() + " ";
        }
        else {
            if (!currentChunk.empty()) {
                chunks.push_back(currentChunk);
                currentChunk.clear();
            }
            currentChunk += node->getContent() + " ";
        }

        for (const auto& child : node->getChildren()) {
            traverse(child, currentChunk);
        }

        if (auto dagNode = node->getDAGNode()) {
            for (const auto& childDagNode : dagNode->getChildren()) {
                if (auto linkedAstNode = childDagNode->astNode.lock()) {
                    traverse(linkedAstNode, currentChunk);
                }
            }
        }
    };

    std::string currentChunk;
    traverse(root, currentChunk);

    if (!currentChunk.empty()) {
        chunks.push_back(currentChunk);
    }

    return chunks;
}

