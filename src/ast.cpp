#include "ast.h"

ASTNode::ASTNode(NodeType type, const std::string& content, size_t position, ParserState state)  
    : type(type), content(content), position(position), state(state) {
    try {
        validateNode();
    } catch (const std::exception& e) {
        std::cerr << "Node validation error: " << e.what() << std::endl;
    }
}

void ASTNode::validateNode() {
    if (content.empty() && type != NodeType::Document) {
        throw std::runtime_error("Empty content for non-document node");
    }

    switch (type) {
        case NodeType::Math:
            validateMathContent();
            break;
        case NodeType::Section:
            validateSectionContent();
            break;
        case NodeType::Command:
            validateCommandContent();
            break;
        default:
            break;
    }
}

void ASTNode::validateMathContent() {
    if (content.find("$") == std::string::npos && 
        content.find("\\begin{equation}") == std::string::npos) {
        throw std::runtime_error("Invalid math content format");
    }
}

void ASTNode::validateSectionContent() {
    if (content.find("\\section") == std::string::npos && 
        content.find("\\subsection") == std::string::npos) {
        throw std::runtime_error("Invalid section content format");
    }
}

void ASTNode::validateCommandContent() {
    if (content.empty() || content[0] != '\\') {
        throw std::runtime_error("Invalid command format");
    }
}

std::string ASTNode::getContent() const {
    return content;
}

ASTNode::NodeType ASTNode::getType() const {
    return type;
}

ParserState ASTNode::getState() const {
    return state;
}

size_t ASTNode::getPosition() const {
    return position;
}

std::string ASTNode::getNodeTypeName(ASTNode::NodeType type) const {
    static const std::unordered_map<NodeType, std::string> nodeTypeNames = {
        {NodeType::Command, "Command"},
        {NodeType::Text, "Text"},
        {NodeType::Section, "Section"},
        {NodeType::Math, "Math"},
        {NodeType::Document, "Document"},
        {NodeType::Environment, "Environment"},
        {NodeType::Author, "Author"},
        {NodeType::Affiliation, "Affiliation"},
        {NodeType::Abstract, "Abstract"},
        {NodeType::Bibliography, "Bibliography"}
    };

    auto it = nodeTypeNames.find(type);
    return it != nodeTypeNames.end() ? it->second : "Unknown";
}

const std::vector<std::shared_ptr<ASTNode>>& ASTNode::getChildren() const {
    return children;
}

void ASTNode::addChild(const std::shared_ptr<ASTNode>& child) {
    try {
        if (!child) {
            throw std::runtime_error("Attempted to add null child to node");
        }

        if (!isValidChild(child)) {
            throw std::runtime_error("Invalid child type for current node");
        }

        children.push_back(child);
    } catch (const std::exception& e) {
        std::cerr << "Error adding child to node \"" << content << "\": " 
                  << e.what() << std::endl;
    }
}

bool ASTNode::isValidChild(const std::shared_ptr<ASTNode>& child) const {
    static const std::unordered_map<NodeType, std::set<NodeType>> validChildren = {
        {NodeType::Document, {
            NodeType::Section, 
            NodeType::Command, 
            NodeType::Text, 
            NodeType::Environment, 
            NodeType::Author,
            NodeType::Affiliation,  
            NodeType::Abstract,     
            NodeType::Bibliography, 
            NodeType::Math,        
            NodeType::Label,       
            NodeType::Reference,   
            NodeType::EnvironmentContent 
        }},
        {NodeType::Section, {NodeType::Text, NodeType::Command, NodeType::Math}},
        {NodeType::Math, {NodeType::Text}},
        {NodeType::Environment, {NodeType::Text, NodeType::Command, NodeType::Math}},
        {NodeType::Author, {NodeType::Affiliation, NodeType::Text}}
    };

    auto it = validChildren.find(type);
    if (it != validChildren.end()) {
        if (it->second.find(child->getType()) == it->second.end()) {
            std::cerr << "Invalid child type: " << getNodeTypeName(child->getType()) 
                      << " for parent type: " << getNodeTypeName(type) << std::endl;
        }
        return it->second.find(child->getType()) != it->second.end();
    }
    return true; 
}

void ASTNode::addReference(const std::shared_ptr<ASTNode>& node) {
    try {
        if (!node) {
            throw std::runtime_error("Attempted to add null reference");
        }
        references.push_back(node);
    } catch (const std::exception& e) {
        std::cerr << "Error adding reference to node \"" << content << "\": " 
                  << e.what() << std::endl;
    }
}

void ASTNode::setDAGNode(const std::shared_ptr<DAGNode>& dagNode_) {
    try {
        if (!dagNode_) {
            throw std::runtime_error("Attempted to set null DAG node");
        }
        dagNode = dagNode_;
    } catch (const std::exception& e) {
        std::cerr << "Error setting DAG node: " << e.what() << std::endl;
    }
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

    else if ((inAuthorGroup && content.find("\\affiliation") != std::string::npos) || (inAuthorGroup && content.find("\\institute") != std::string::npos)) {
        std::cout << indentStr << "  Node Type: " << getNodeTypeName(type)
                  << ", Content: \"" << content << "\"\n";
    }

    else if ((inAuthorGroup && content.find("\\affiliation") == std::string::npos && content.find("\\author") == std::string::npos) || (inAuthorGroup && content.find("\\institute") == std::string::npos && content.find("\\author") == std::string::npos)) {
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

AST::AST() {
    try {
        root = std::make_shared<ASTNode>(ASTNode::NodeType::Document, "Document Root", 
                                       0, ParserState::DefaultState);
    } catch (const std::exception& e) {
        std::cerr << "Error creating AST root node: " << e.what() << std::endl;
        throw;
    }
}

void AST::print() const {
    if (root) {
        root->print();
    } else {
        std::cerr << "Warning: Attempting to print empty AST" << std::endl;
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
				if (auto linkedAstNode = childDagNode->getASTNode().lock()) {
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
