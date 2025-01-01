#include "dag_node.h"
#include <sstream>
#include <queue>
#include <stack>
#include <iostream>
#include <fstream>
#include <algorithm>

std::atomic<size_t> DAGNode::nodeCounter(0);


std::shared_ptr<DAGNode> DAGNode::create(const std::string& id, NodeType type) {
    try {
        return std::shared_ptr<DAGNode>(new DAGNode(id, type));
    } catch (const std::exception& e) {
        std::cerr << "Error creating DAGNode: " << e.what() << std::endl;
        return nullptr;
    }
}



DAGNode::DAGNode(const std::string& id, NodeType type) 
    : id(id)
    , nodeType(type)
    , content()
    , children()
    , parents()
    , outgoingEdges()
    , incomingEdges()
    , astNode() 
{

    children.reserve(8);
    parents.reserve(8);
    outgoingEdges.reserve(8);
    incomingEdges.reserve(8);
}



void DAGNode::addEdge(const std::shared_ptr<DAGNode>& target, EdgeType type,
                     const std::string& label) {
    if (!target) {
        std::cerr << "Cannot add edge to null target from node " << id << std::endl;
        return;
    }
    
    try {
        if (!validateEdge(target, type)) {
            std::cerr << "Invalid edge type " << static_cast<int>(type) 
                     << " between " << id << " and " << target->getId() << std::endl;
            return;
        }
        
        if (type == EdgeType::Hierarchical && wouldCreateCycle(target)) {
            std::cerr << "Cannot add hierarchical edge from " << id 
                     << " to " << target->getId() << " - would create cycle" << std::endl;
            return;
        }
        

        for (const auto& edge : outgoingEdges) {
            if (auto existing = edge.target.lock()) {
                if (existing == target && edge.type == type) {
                    std::cerr << "Edge already exists from " << id 
                             << " to " << target->getId() << std::endl;
                    return;
                }
            }
        }
        
        Edge newOutEdge{target, type, label};
        Edge newInEdge{shared_from_this(), type, label};
        
        outgoingEdges.push_back(std::move(newOutEdge));
        target->incomingEdges.push_back(std::move(newInEdge));
        
        std::cerr << "Added edge from " << id << " to " << target->getId() 
                  << " of type " << static_cast<int>(type) << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error adding edge: " << e.what() << std::endl;
    }
}

bool DAGNode::validateEdge(const std::shared_ptr<DAGNode>& target, EdgeType type) const {
    static const std::unordered_map<NodeType, 
           std::unordered_map<NodeType, std::set<EdgeType>>> validEdges = {
        {NodeType::Author, {
            {NodeType::Affiliation, {EdgeType::AuthorAffiliation}},
            {NodeType::Citation, {EdgeType::Citation}}
        }},
        {NodeType::Section, {
            {NodeType::Section, {EdgeType::CrossReference}},
            {NodeType::Figure, {EdgeType::FigureReference}},
            {NodeType::Table, {EdgeType::TableReference}},
            {NodeType::Equation, {EdgeType::EquationReference}}
        }}
    };
    
    auto sourceIt = validEdges.find(nodeType);
    if (sourceIt != validEdges.end()) {
        auto targetIt = sourceIt->second.find(target->getType());
        if (targetIt != sourceIt->second.end()) {
            return targetIt->second.find(type) != targetIt->second.end();
        }
    }
    
    return type == EdgeType::Hierarchical;
}

bool DAGNode::wouldCreateCycle(const std::shared_ptr<DAGNode>& target) const {
    if (target.get() == this) return true;
    
    std::set<const DAGNode*> visited;
    std::stack<const DAGNode*> stack;
    stack.push(target.get());
    
    while (!stack.empty()) {
        const DAGNode* current = stack.top();
        stack.pop();
        
        if (current == this) return true;
        if (visited.insert(current).second) {
            for (const auto& edge : current->outgoingEdges) {
                if (edge.type == EdgeType::Hierarchical) {
                    if (auto node = edge.target.lock()) {
                        stack.push(node.get());
                    }
                }
            }
        }
    }
    
    return false;
}

std::shared_ptr<DAGNode> DAG::createNode(NodeType type, const std::string& content) {
    static size_t nodeCounter = 0;
    std::string id = std::to_string(nodeCounter++) + "_" + 
                    std::to_string(static_cast<int>(type)) + "_" +
                    content.substr(0, 30);
    
    auto node = DAGNode::create(id, type);
    node->setContent(content);
    nodes[id] = node;
    return node;
}

void DAG::buildFromAST(const std::shared_ptr<ASTNode>& root) {
    if (!root) return;
    auto documentNode = createNode(NodeType::Document);
    processASTNode(root, documentNode);
}

void DAG::processASTNode(const std::shared_ptr<ASTNode>& astNode,
                        const std::shared_ptr<DAGNode>& parentDagNode) {
    if (!astNode || !parentDagNode) return;
    

    auto sanitizeContent = [](const std::string& content) {
        std::string sanitized;
        bool inCommand = false;
        bool inBrace = false;
        
        for (size_t i = 0; i < content.length(); ++i) {
            char c = content[i];
            

            if (c == '\\') {
                inCommand = true;
                continue;
            }
            

            if (c == '{') {
                inBrace = true;
                continue;
            }
            if (c == '}') {
                inBrace = false;
                continue;
            }
            

            if (inCommand && (std::isspace(c) || c == '{' || c == '}' || c == '_' || c == '^')) {
                inCommand = false;
            }
            

            if (!inCommand && !inBrace && !std::isspace(c)) {
                sanitized += c;
            }
        }
        return sanitized;
    };


    static const std::unordered_map<ASTNode::NodeType, NodeType> nodeTypeMap = {
        {ASTNode::NodeType::Document, NodeType::Document},
        {ASTNode::NodeType::Author, NodeType::Author},
        {ASTNode::NodeType::Affiliation, NodeType::Affiliation},
        {ASTNode::NodeType::Section, NodeType::Section},
        {ASTNode::NodeType::Math, NodeType::Math},
        {ASTNode::NodeType::Command, NodeType::Command},
        {ASTNode::NodeType::Environment, NodeType::Environment},
        {ASTNode::NodeType::Text, NodeType::Text},
        {ASTNode::NodeType::Reference, NodeType::Reference}
    };


    auto it = nodeTypeMap.find(astNode->getType());
    NodeType dagType = (it != nodeTypeMap.end()) ? it->second : NodeType::Text;
    

    std::string content = sanitizeContent(astNode->getContent());
    auto dagNode = createNode(dagType, content);
    
    if (!dagNode) {
        std::cerr << "Failed to create DAG node for content: " << content << std::endl;
        return;
    }

    dagNode->setASTNode(astNode);
    

    if (parentDagNode != dagNode) {
        try {
            parentDagNode->addEdge(dagNode, EdgeType::Hierarchical);
        } catch (const std::exception& e) {
            std::cerr << "Error adding edge: " << e.what() << std::endl;
        }
    }
    

    processSpecialRelationships(astNode, dagNode);
    

    for (const auto& child : astNode->getChildren()) {
        processASTNode(child, dagNode);
    }
}



void DAG::processSpecialRelationships(const std::shared_ptr<ASTNode>& astNode,
                                    const std::shared_ptr<DAGNode>& dagNode) {
    if (astNode->getType() == ASTNode::NodeType::Reference) {
        std::string content = astNode->getContent();
        if (content.find("\\cite") != std::string::npos ||
            content.find("\\citep") != std::string::npos ||
            content.find("\\citet") != std::string::npos) {
            auto citationNode = createNode(NodeType::Citation, content);
            dagNode->addEdge(citationNode, EdgeType::Citation);
        }
    }
}

std::string DAG::generateSubgraph(const std::vector<std::shared_ptr<DAGNode>>& nodes,
                                const std::string& name) const {
    std::stringstream ss;
    ss << "  subgraph cluster_" << name << " {\n";
    ss << "    style=filled;\n";
    ss << "    color=lightgrey;\n";
    ss << "    label=\"" << name << "\";\n";
    
    for (const auto& node : nodes) {
        ss << "    \"" << node->getId() << "\" [label=\"" << node->getContent() << "\"];\n";
    }
    
    ss << "  }\n";
    return ss.str();
}

void DAG::addNode(const std::shared_ptr<DAGNode>& node) {
    if (!node) return;
    nodes[node->getId()] = node;
}

std::shared_ptr<DAGNode> DAG::getNode(const std::string& id) const {
    auto it = nodes.find(id);
    return (it != nodes.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<DAGNode>> DAG::findNodesByType(NodeType type) const {
    std::vector<std::shared_ptr<DAGNode>> result;
    for (const auto& [_, node] : nodes) {
        if (node->getType() == type) {
            result.push_back(node);
        }
    }
    return result;
}

std::vector<std::shared_ptr<DAGNode>> DAG::findConnectedNodes(
    const std::string& nodeId, EdgeType type) const {
    std::vector<std::shared_ptr<DAGNode>> result;
    auto sourceNode = getNode(nodeId);
    if (!sourceNode) return result;
    
    for (const auto& edge : sourceNode->getOutgoingEdges()) {
        if (edge.type == type) {
            if (auto target = edge.target.lock()) {
                result.push_back(target);
            }
        }
    }
    return result;
}


std::weak_ptr<ASTNode> DAGNode::getASTNode() const {
    return astNode;
}

const std::vector<std::shared_ptr<DAGNode>>& DAGNode::getChildren() const {
    return children;
}

NodeType DAGNode::getNodeType() const {
    return nodeType;
}

void DAGNode::addChild(const std::shared_ptr<DAGNode>& child) {
    if (!child) return;
    

    auto it = std::find(children.begin(), children.end(), child);
    if (it == children.end()) {
        children.push_back(child);
        child->addParent(shared_from_this());
    }
    

    addEdge(child, EdgeType::Hierarchical);
}

void DAGNode::addParent(const std::shared_ptr<DAGNode>& parent) {
    if (!parent) return;
    auto it = std::find(parents.begin(), parents.end(), parent);
    if (it == parents.end()) {
        parents.push_back(parent);
    }
}

std::shared_ptr<DAGNode> DAG::getOrCreateNode(const std::string& id, NodeType type) {
    auto it = nodes.find(id);
    if (it != nodes.end()) {
        return it->second;
    }
    
    auto node = DAGNode::create(id, type);
    nodes[id] = node;
    return node;
}


void DAG::generateDOT(const std::string& filename) const {
    if (!validate()) {
        std::cerr << "Warning: DAG validation failed. DOT output may be incomplete.\n";
    }

    std::ofstream file(filename);
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    try {
        file << "digraph ResearchPaper {\n";
        file << "  rankdir=LR;\n";
        file << "  node [shape=box, style=filled, fontname=\"Arial\"];\n";
        file << "  concentrate=true;\n";
        file << "  compound=true;\n";


        std::unordered_map<std::string, std::string> cleanLabels;
        for (const auto& [id, node] : nodes) {
            if (!node) continue;

            std::string content = node->getContent();
            

            if (content.empty()) {
                content = "[" + id + "]"; 
            }


            std::string cleanContent = content;
            bool inCommand = false;
            std::string currentCommand;
            std::string result;

            for (size_t i = 0; i < cleanContent.length(); ++i) {
                char c = cleanContent[i];
                if (c == '\\') {
                    inCommand = true;
                    currentCommand = "\\";
                    continue;
                }

                if (inCommand) {
                    if (std::isalpha(c)) {
                        currentCommand += c;
                    } else {
                        if (currentCommand == "\\cite" || currentCommand == "\\ref") {
                            result += "[REF]";
                        }
                        inCommand = false;
                        if (!std::isspace(c) && c != '{' && c != '}') {
                            result += c;
                        }
                    }
                    continue;
                }

                if (!std::isspace(c) && c != '{' && c != '}') {
                    result += c;
                } else if (std::isspace(c) && !result.empty() && result.back() != ' ') {
                    result += ' ';
                }
            }


            result = result.substr(0, result.find_last_not_of(" \n\r\t") + 1);
            if (result.length() > 40) {
                result = result.substr(0, 37) + "...";
            }

            cleanLabels[id] = result.empty() ? "[" + id + "]" : result;
        }


        std::unordered_map<NodeType, std::vector<std::shared_ptr<DAGNode>>> nodesByType;
        for (const auto& [id, node] : nodes) {
            if (!node) continue;
            nodesByType[node->getType()].push_back(node);
        }


        for (const auto& [type, nodeGroup] : nodesByType) {
            std::string typeName = getNodeTypeName(type);

            file << "  subgraph cluster_" << typeName << " {\n";
            file << "    style=filled;\n";
            file << "    color=lightgrey;\n";
            file << "    label=\"" << typeName << "\";\n";

            for (const auto& node : nodeGroup) {
                file << "    \"" << node->getId() << "\" [label=\""
                     << cleanLabels[node->getId()] << "\"];\n";
            }

            file << "  }\n";
        }


        for (const auto& [id, node] : nodes) {
            if (!node) continue;

            for (const auto& edge : node->getOutgoingEdges()) {
                if (auto target = edge.target.lock()) {
                    file << "  \"" << node->getId() << "\" -> \""
                         << target->getId() << "\" [" << getEdgeStyle(edge.type);
                    
                    if (!edge.label.empty()) {
                        file << ",label=\"" << edge.label << "\"";
                    }
                    file << "];\n";
                }
            }
        }

        file << "}\n";

    } catch (const std::exception& e) {
        std::cerr << "Error generating DOT file: " << e.what() << std::endl;
        dumpStructure();
    }
}

std::string DAG::getNodeTypeName(NodeType type) const {
    static const std::unordered_map<NodeType, std::string> typeNames = {
        {NodeType::Document, "Document"},
        {NodeType::Section, "Sections"},
        {NodeType::Author, "Authors"},
        {NodeType::Affiliation, "Affiliations"},
        {NodeType::Citation, "Citations"},
        {NodeType::Reference, "References"},
        {NodeType::Math, "Math"},
        {NodeType::Command, "Commands"},
        {NodeType::Environment, "Environments"},
        {NodeType::Text, "Text"}
    };

    auto it = typeNames.find(type);
    return it != typeNames.end() ? 
           it->second : 
           "Unknown_" + std::to_string(static_cast<int>(type));
}

std::string DAG::getEdgeStyle(EdgeType type) const {
    switch (type) {
        case EdgeType::Hierarchical:
            return "style=solid,weight=2";
        case EdgeType::Citation:
            return "style=dashed,color=blue,weight=1";
        case EdgeType::CrossReference:
            return "style=dotted,color=red,weight=1";
        case EdgeType::AuthorAffiliation:
            return "style=bold,color=green,weight=1";
        case EdgeType::FigureReference:
            return "style=dashed,color=purple,weight=1";
        case EdgeType::TableReference:
            return "style=dashed,color=orange,weight=1";
        case EdgeType::EquationReference:
            return "style=dashed,color=brown,weight=1";
        case EdgeType::RelatedWork:
            return "style=dotted,color=gray,weight=1";
        default:
            return "style=solid";
    }
}


bool DAG::validate() const {
    bool isValid = true;
    std::cerr << "Validating DAG structure...\n";
    std::cerr << "Total nodes: " << nodes.size() << "\n";


    for (const auto& [id, node] : nodes) {
        if (!node) {
            std::cerr << "Error: Null node found with id: " << id << "\n";
            isValid = false;
            continue;
        }


        if (!validateNode(node)) {
            isValid = false;
        }
    }


    std::set<std::string> reachableNodes;
    std::queue<std::shared_ptr<DAGNode>> queue;


    for (const auto& [id, node] : nodes) {
        if (node && node->getIncomingEdges().empty()) {
            queue.push(node);
        }
    }

    while (!queue.empty()) {
        auto current = queue.front();
        queue.pop();

        if (reachableNodes.insert(current->getId()).second) {
            for (const auto& edge : current->getOutgoingEdges()) {
                if (auto target = edge.target.lock()) {
                    queue.push(target);
                }
            }
        }
    }

    if (reachableNodes.size() != nodes.size()) {
        std::cerr << "Warning: DAG contains disconnected components\n";
        std::cerr << "Reachable nodes: " << reachableNodes.size() << "\n";
        std::cerr << "Total nodes: " << nodes.size() << "\n";
        isValid = false;
    }

    return isValid;
}

bool DAG::validateNode(const std::shared_ptr<DAGNode>& node) const {
    bool isValid = true;


    if (node->getContent().empty()) {
        std::cerr << "Warning: Empty content in node " << node->getId() << "\n";
    }


    for (const auto& edge : node->getOutgoingEdges()) {
        auto target = edge.target.lock();
        if (!target) {
            std::cerr << "Error: Dead edge found in node " << node->getId() << "\n";
            isValid = false;
            continue;
        }


        if (nodes.find(target->getId()) == nodes.end()) {
            std::cerr << "Error: Edge points to non-existent node " << target->getId() << "\n";
            isValid = false;
        }
    }

    return isValid;
}

void DAG::dumpStructure() const {
    std::cerr << "\nDAG Structure Dump:\n";
    std::cerr << "================\n";
    std::cerr << "Total nodes: " << nodes.size() << "\n\n";


    std::vector<std::shared_ptr<DAGNode>> roots;
    for (const auto& [id, node] : nodes) {
        if (node && node->getIncomingEdges().empty()) {
            roots.push_back(node);
        }
    }


    for (const auto& root : roots) {
        dumpNode(root);
    }
}

void DAG::dumpNode(const std::shared_ptr<DAGNode>& node, int depth) const {
    std::string indent(depth * 2, ' ');
    std::cerr << indent << "Node: " << node->getId() << "\n";
    std::cerr << indent << "Type: " << static_cast<int>(node->getNodeType()) << "\n";
    std::cerr << indent << "Content: " << node->getContent() << "\n";
    std::cerr << indent << "Outgoing edges: " << node->getOutgoingEdges().size() << "\n";

    for (const auto& edge : node->getOutgoingEdges()) {
        if (auto target = edge.target.lock()) {
            std::cerr << indent << "  -> " << target->getId()
                     << " (type: " << static_cast<int>(edge.type) << ")\n";
        }
    }
    std::cerr << "\n";


    for (const auto& edge : node->getOutgoingEdges()) {
        if (edge.type == EdgeType::Hierarchical) {
            if (auto target = edge.target.lock()) {
                dumpNode(target, depth + 1);
            }
        }
    }
}


DAG::PaperStructureAnalysis DAG::analyzePaperStructure() const {
    PaperStructureAnalysis analysis;
    std::unordered_map<std::string, double> conceptFrequency;


    for (const auto& [_, node] : nodes) {

        for (const auto& edge : node->getOutgoingEdges()) {
            if (edge.type == EdgeType::MainContribution) {
                if (auto target = edge.target.lock()) {
                    analysis.mainContributions.push_back(target);
                }
            } else if (edge.type == EdgeType::ResultSupports ||
                      edge.type == EdgeType::EvidenceSupport) {
                if (auto target = edge.target.lock()) {
                    analysis.supportingEvidence.push_back(target);
                }
            }
        }


        if (node->getType() == NodeType::Section) {
            bool isMethodology = false;
            std::string content = node->getContent();
            std::transform(content.begin(), content.end(), content.begin(), ::tolower);

            if (content.find("method") != std::string::npos ||
                content.find("approach") != std::string::npos ||
                content.find("implementation") != std::string::npos) {
                isMethodology = true;
                analysis.methodologySteps.push_back(node);
            }
        }


        if (node->getType() == NodeType::Text ||
            node->getType() == NodeType::Math ||
            node->getType() == NodeType::Environment) {
            for (const auto& edge : node->getOutgoingEdges()) {
                if (edge.type == EdgeType::Assumption) {
                    if (auto target = edge.target.lock()) {
                        analysis.criticalAssumptions.push_back(target);
                    }
                }
            }

            std::string content = node->getContent();
            std::istringstream iss(content);
            std::string word;
            while (iss >> word) {
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                if (word.length() > 3) { 
                    conceptFrequency[word]++;
                }
            }
        }
    }

    double total = 0;
    for (const auto& [concept, freq] : conceptFrequency) {
        total += freq;
    }
    for (const auto& [concept, freq] : conceptFrequency) {
        analysis.topicDistribution[concept] = freq / total;
    }

    return analysis;
}


DAG::CitationAnalysis DAG::analyzeCitations() const {
    CitationAnalysis analysis;
    std::unordered_map<std::string, int> citationMap;
    std::unordered_map<std::string, std::vector<std::string>> contextMap;
    std::unordered_map<int, std::set<std::string>> yearMap;

    for (const auto& [_, node] : nodes) {
        if (node->getType() == NodeType::Citation) {

            std::string citation = node->getContent();
            citationMap[citation]++;


            for (const auto& edge : node->getIncomingEdges()) {
                if (auto source = edge.target.lock()) {
                    if (source->getType() == NodeType::Text) {
                        contextMap[citation].push_back(source->getContent());
                    }
                }
            }


            std::regex yearRegex(R"(\b(19|20)\d{2}\b)");
            std::smatch match;
            if (std::regex_search(citation, match, yearRegex)) {
                int year = std::stoi(match[0].str());
                yearMap[year].insert(citation);
            }
        }
    }


    for (const auto& [citation, count] : citationMap) {
        analysis.citationCounts.push_back({citation, count});
    }


    std::sort(analysis.citationCounts.begin(), analysis.citationCounts.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });


    for (size_t i = 0; i < std::min(size_t(10), analysis.citationCounts.size()); i++) {
        analysis.mostInfluentialPapers.push_back(analysis.citationCounts[i].first);
    }


    analysis.citationContexts = std::map<std::string, std::vector<std::string>>(
        contextMap.begin(), contextMap.end());


    for (const auto& [year, citations] : yearMap) {
        analysis.citationsByYear[year] = std::vector<std::string>(
            citations.begin(), citations.end());
    }

    return analysis;
}

std::vector<std::string> DAG::identifyResearchGaps() const {
    std::vector<std::string> gaps;
    std::set<std::string> coveredTopics;
    std::set<std::string> mentionedGaps;


    for (const auto& [_, node] : nodes) {
        if (node->getType() == NodeType::Section ||
            node->getType() == NodeType::Text) {
            std::string content = node->getContent();
            std::transform(content.begin(), content.end(), content.begin(), ::tolower);


            std::istringstream iss(content);
            std::string word;
            while (iss >> word) {
                if (word.length() > 3) {
                    coveredTopics.insert(word);
                }
            }
        }
    }


    for (const auto& [_, node] : nodes) {

        for (const auto& edge : node->getOutgoingEdges()) {
            if (edge.type == EdgeType::Limitation || edge.type == EdgeType::FutureWork) {
                if (auto target = edge.target.lock()) {
                    std::string content = target->getContent();
                    gaps.push_back("Explicit gap: " + content);
                }
            }
        }


        if (node->getType() == NodeType::Section) {
            bool hasValidation = false;
            bool hasComparison = false;

            for (const auto& edge : node->getOutgoingEdges()) {
                if (edge.type == EdgeType::ValidationMethod) hasValidation = true;
                if (edge.type == EdgeType::ComparisonLink) hasComparison = true;
            }

            if (!hasValidation) {
                gaps.push_back("Missing validation for section: " + node->getContent());
            }
            if (!hasComparison) {
                gaps.push_back("Missing comparison analysis for: " + node->getContent());
            }
        }
    }

    return gaps;
}
std::vector<std::vector<std::shared_ptr<DAGNode>>>
DAG::findStronglyConnectedComponents() const {
    std::vector<std::vector<std::shared_ptr<DAGNode>>> components;
    std::unordered_map<std::string, int> indices;
    std::unordered_map<std::string, int> lowLink;
    std::stack<std::shared_ptr<DAGNode>> stack;
    std::unordered_set<std::string> onStack;
    int index = 0;

    std::function<void(const std::shared_ptr<DAGNode>&)> strongConnect =
        [&](const std::shared_ptr<DAGNode>& node) {
        indices[node->getId()] = index;
        lowLink[node->getId()] = index;
        index++;
        stack.push(node);
        onStack.insert(node->getId());

        for (const auto& edge : node->getOutgoingEdges()) {
            if (auto successor = edge.target.lock()) {
                if (indices.find(successor->getId()) == indices.end()) {
                    strongConnect(successor);
                    lowLink[node->getId()] = std::min(lowLink[node->getId()],
                                                    lowLink[successor->getId()]);
                } else if (onStack.find(successor->getId()) != onStack.end()) {
                    lowLink[node->getId()] = std::min(lowLink[node->getId()],
                                                    indices[successor->getId()]);
                }
            }
        }

        if (lowLink[node->getId()] == indices[node->getId()]) {
            std::vector<std::shared_ptr<DAGNode>> component;
            std::shared_ptr<DAGNode> w;
            do {
                w = stack.top();
                stack.pop();
                onStack.erase(w->getId());
                component.push_back(w);
            } while (w != node);
            components.push_back(component);
        }
    };

    for (const auto& [_, node] : nodes) {
        if (indices.find(node->getId()) == indices.end()) {
            strongConnect(node);
        }
    }

    return components;
}


std::map<std::shared_ptr<DAGNode>, double> DAG::calculateNodeCentrality() const {
    std::map<std::shared_ptr<DAGNode>, double> centrality;
    size_t n = nodes.size();


    for (const auto& [_, node] : nodes) {
        centrality[node] = 1.0 / n;
    }


    double damping = 0.85;
    int maxIterations = 100;
    double threshold = 1e-6;

    for (int iter = 0; iter < maxIterations; iter++) {
        std::map<std::shared_ptr<DAGNode>, double> newScores;
        double diff = 0.0;

        for (const auto& [_, node] : nodes) {
            double sum = 0.0;
            for (const auto& edge : node->getIncomingEdges()) {
                if (auto source = edge.target.lock()) {
                    sum += centrality[source] / source->getOutgoingEdges().size();
                }
            }
            newScores[node] = (1 - damping) / n + damping * sum;
            diff += std::abs(newScores[node] - centrality[node]);
        }

        centrality = newScores;
        if (diff < threshold) break;
    }

    return centrality;
}


std::vector<std::pair<std::string, double>> DAG::extractKeyThemes() const {
    std::unordered_map<std::string, double> themeScores;
    std::unordered_map<std::string, std::set<std::string>> themeContexts;


    for (const auto& [_, node] : nodes) {
        std::string content = node->getContent();
        std::transform(content.begin(), content.end(), content.begin(), ::tolower);


        if (node->getType() == NodeType::Math ||
            node->getType() == NodeType::Command) {
            continue;
        }


        std::istringstream iss(content);
        std::string word;
        std::set<std::string> localThemes;

        while (iss >> word) {
            if (word.length() > 3) { 
                localThemes.insert(word);


                std::string context;
                for (const auto& edge : node->getOutgoingEdges()) {
                    if (auto target = edge.target.lock()) {
                        context += " " + target->getContent();
                    }
                }
                themeContexts[word].insert(context);
            }
        }


        for (const auto& theme : localThemes) {
            double score = 1.0;


            switch (node->getType()) {
                case NodeType::Section:
                    score *= 2.0;
                    break;
                case NodeType::Abstract:
                    score *= 3.0;
                    break;
                case NodeType::Author:
                    score *= 0.5;
                    break;
                default:
                    break;
            }


            for (const auto& edge : node->getOutgoingEdges()) {
                if (edge.type == EdgeType::MainContribution) score *= 2.0;
                if (edge.type == EdgeType::RelatedWork) score *= 1.5;
                if (edge.type == EdgeType::ResultSupports) score *= 1.3;
            }

            themeScores[theme] += score;
        }
    }


    std::vector<std::pair<std::string, double>> themes;
    double maxScore = 0.0;
    for (const auto& [theme, score] : themeScores) {
        maxScore = std::max(maxScore, score);
    }

    for (const auto& [theme, score] : themeScores) {

        double contextDiversity = static_cast<double>(themeContexts[theme].size());
        double normalizedScore = (score / maxScore) * (1.0 + std::log(1.0 + contextDiversity));
        themes.push_back({theme, normalizedScore});
    }


    std::sort(themes.begin(), themes.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    return themes;
}

std::map<std::string, std::vector<std::string>> DAG::buildConceptHierarchy() const {
    std::map<std::string, std::vector<std::string>> hierarchy;
    std::unordered_map<std::string, std::set<std::string>> dependencies;


    for (const auto& [_, node] : nodes) {
        for (const auto& edge : node->getOutgoingEdges()) {
            if (edge.type == EdgeType::ConceptDependency ||
                edge.type == EdgeType::Definition) {
                if (auto target = edge.target.lock()) {
                    dependencies[node->getContent()].insert(target->getContent());
                }
            }
        }
    }


    std::set<std::string> visited;
    std::set<std::string> current;

    std::function<void(const std::string&)> buildHierarchyDFS =
        [&](const std::string& concept) {
        if (current.find(concept) != current.end()) {
            return;  
        }
        if (visited.find(concept) != visited.end()) {
            return; 
        }

        current.insert(concept);


        for (const auto& dep : dependencies[concept]) {
            buildHierarchyDFS(dep);
            hierarchy[concept].push_back(dep);
        }

        current.erase(concept);
        visited.insert(concept);
    };


    for (const auto& [concept, _] : dependencies) {
        if (visited.find(concept) == visited.end()) {
            buildHierarchyDFS(concept);
        }
    }

    return hierarchy;
}


bool DAG::validateMethodologyCompleteness() const {
    bool isValid = true;
    std::vector<std::shared_ptr<DAGNode>> methodNodes;


    for (const auto& [_, node] : nodes) {
        if (node->getType() == NodeType::Section) {
            std::string content = node->getContent();
            std::transform(content.begin(), content.end(), content.begin(), ::tolower);

            if (content.find("method") != std::string::npos ||
                content.find("approach") != std::string::npos) {
                methodNodes.push_back(node);
            }
        }
    }


    for (const auto& node : methodNodes) {
        bool hasInputDescription = false;
        bool hasProcessDescription = false;
        bool hasOutputDescription = false;
        bool hasValidation = false;


        for (const auto& edge : node->getOutgoingEdges()) {
            if (auto target = edge.target.lock()) {
                std::string content = target->getContent();
                std::transform(content.begin(), content.end(), content.begin(), ::tolower);

                if (edge.type == EdgeType::DataDependency) hasInputDescription = true;
                if (edge.type == EdgeType::MethodologyFlow) hasProcessDescription = true;
                if (edge.type == EdgeType::ResultSupports) hasOutputDescription = true;
                if (edge.type == EdgeType::ValidationMethod) hasValidation = true;
            }
        }


        if (!hasInputDescription) {
            std::cerr << "Warning: Missing input description in methodology section: "
                     << node->getContent() << std::endl;
            isValid = false;
        }
        if (!hasProcessDescription) {
            std::cerr << "Warning: Missing process description in methodology section: "
                     << node->getContent() << std::endl;
            isValid = false;
        }
        if (!hasOutputDescription) {
            std::cerr << "Warning: Missing output description in methodology section: "
                     << node->getContent() << std::endl;
            isValid = false;
        }
        if (!hasValidation) {
            std::cerr << "Warning: Missing validation in methodology section: "
                     << node->getContent() << std::endl;
            isValid = false;
        }
    }

    return isValid;
}

bool DAG::validateEvidenceChain() const {
    bool isValid = true;


    std::vector<std::shared_ptr<DAGNode>> claims;
    for (const auto& [_, node] : nodes) {
        for (const auto& edge : node->getOutgoingEdges()) {
            if (edge.type == EdgeType::MainContribution ||
                edge.type == EdgeType::SubContribution) {
                claims.push_back(node);
                break;
            }
        }
    }


    for (const auto& claim : claims) {
        bool hasDirectEvidence = false;
        bool hasMethodologicalSupport = false;
        bool hasValidation = false;
        std::set<std::shared_ptr<DAGNode>> evidenceNodes;


        std::function<void(const std::shared_ptr<DAGNode>&)> collectEvidence =
            [&](const std::shared_ptr<DAGNode>& node) {
            if (evidenceNodes.find(node) != evidenceNodes.end()) return;
            evidenceNodes.insert(node);

            for (const auto& edge : node->getOutgoingEdges()) {
                if (edge.type == EdgeType::EvidenceSupport ||
                    edge.type == EdgeType::ResultSupports ||
                    edge.type == EdgeType::ValidationMethod) {
                    if (auto target = edge.target.lock()) {
                        collectEvidence(target);

                        if (edge.type == EdgeType::EvidenceSupport) hasDirectEvidence = true;
                        if (edge.type == EdgeType::ResultSupports) hasMethodologicalSupport = true;
                        if (edge.type == EdgeType::ValidationMethod) hasValidation = true;
                    }
                }
            }
        };

        collectEvidence(claim);


        if (!hasDirectEvidence) {
            std::cerr << "Warning: Claim lacks direct evidence: "
                     << claim->getContent() << std::endl;
            isValid = false;
        }
        if (!hasMethodologicalSupport) {
            std::cerr << "Warning: Claim lacks methodological support: "
                     << claim->getContent() << std::endl;
            isValid = false;
        }
        if (!hasValidation) {
            std::cerr << "Warning: Claim lacks validation: "
                     << claim->getContent() << std::endl;
            isValid = false;
        }


        if (!validateRelationshipChain(
                std::vector<std::shared_ptr<DAGNode>>(evidenceNodes.begin(),
                                                     evidenceNodes.end()),
                EdgeType::EvidenceSupport)) {
            std::cerr << "Warning: Broken evidence chain for claim: "
                     << claim->getContent() << std::endl;
            isValid = false;
        }
    }

    return isValid;
}


std::vector<std::shared_ptr<DAGNode>> DAG::findBridgingConcepts() const {
    std::vector<std::shared_ptr<DAGNode>> bridges;
    std::map<std::shared_ptr<DAGNode>, double> betweenness = calculateNodeCentrality();


    for (const auto& [_, node] : nodes) {
        std::set<NodeType> connectedTypes;
        int totalConnections = 0;


        for (const auto& edge : node->getOutgoingEdges()) {
            if (auto target = edge.target.lock()) {
                connectedTypes.insert(target->getType());
                totalConnections++;
            }
        }


        for (const auto& edge : node->getIncomingEdges()) {
            if (auto source = edge.target.lock()) {
                connectedTypes.insert(source->getType());
                totalConnections++;
            }
        }

        double diversityScore = static_cast<double>(connectedTypes.size());
        double densityScore = static_cast<double>(totalConnections) / nodes.size();
        double bridgeScore = betweenness[node] * diversityScore * densityScore;

        if (bridgeScore > 0.5) { 
            bridges.push_back(node);
        }
    }

    std::sort(bridges.begin(), bridges.end(),
              [&betweenness](const auto& a, const auto& b) {
                  return betweenness[a] > betweenness[b];
              });

    return bridges;
}


void DAG::exportToKnowledgeGraph(const std::string& filename) const {
    json graph;
    graph["metadata"]["type"] = "ResearchPaperKnowledgeGraph";
    graph["metadata"]["nodes"] = nodes.size();


    graph["nodes"] = json::array();
    std::unordered_map<std::string, size_t> nodeIndices;
    size_t index = 0;

    for (const auto& [id, node] : nodes) {
        json nodeJson;
        nodeJson["id"] = id;
        nodeJson["type"] = getNodeTypeName(node->getType());
        nodeJson["content"] = node->getContent();


        if (auto semanticInfo = node->getSemanticInfo()) {
            nodeJson["semantic"] = {
                {"type", semanticInfo->conceptType},
                {"keywords", semanticInfo->keywords},
                {"topics", semanticInfo->topics},
                {"importance", semanticInfo->importance}
            };
        }

        graph["nodes"].push_back(nodeJson);
        nodeIndices[id] = index++;
    }


    graph["edges"] = json::array();
    for (const auto& [_, node] : nodes) {
        for (const auto& edge : node->getOutgoingEdges()) {
            if (auto target = edge.target.lock()) {
                json edgeJson;
                edgeJson["source"] = nodeIndices[node->getId()];
                edgeJson["target"] = nodeIndices[target->getId()];
                edgeJson["type"] = static_cast<int>(edge.type);
                edgeJson["label"] = edge.label;


                if (auto metadata = node->getRelationshipMetadata(target, edge.type)) {
                    edgeJson["metadata"] = {
                        {"confidence", metadata->confidence},
                        {"evidence", metadata->evidence},
                        {"context", metadata->context},
                        {"timestamp", metadata->timestamp.time_since_epoch().count()}
                    };
                }

                graph["edges"].push_back(edgeJson);
            }
        }
    }


    std::ofstream file(filename);
    if (file) {
        file << graph.dump(2); 
    } else {
        std::cerr << "Failed to open file for knowledge graph export: "
                  << filename << std::endl;
    }
}

void DAG::generateSemanticMap(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file) {
        std::cerr << "Failed to open file for semantic map: " << filename << std::endl;
        return;
    }


    file << "digraph SemanticMap {\n";
    file << "  rankdir=TB;\n";
    file << "  node [shape=box, style=filled, fontname=\"Arial\"];\n";
    file << "  concentrate=true;\n";


    std::unordered_map<std::string, std::vector<std::shared_ptr<DAGNode>>> conceptGroups;
    std::unordered_map<std::string, double> conceptImportance;

    for (const auto& [_, node] : nodes) {
        if (auto semanticInfo = node->getSemanticInfo()) {
            conceptGroups[semanticInfo->conceptType].push_back(node);
            conceptImportance[node->getId()] = semanticInfo->importance;
        }
    }


    for (const auto& [conceptType, groupNodes] : conceptGroups) {
        file << "  subgraph cluster_" << conceptType << " {\n";
        file << "    label=\"" << conceptType << "\";\n";
        file << "    color=lightgrey;\n";


        for (const auto& node : groupNodes) {
            double importance = conceptImportance[node->getId()];
            int colorIntensity = static_cast<int>(255 * (1.0 - importance));

            file << "    \"" << node->getId() << "\" [label=\""
                 << node->getContent() << "\", fillcolor=\"#"
                 << std::hex << colorIntensity << colorIntensity << "ff\"];\n";
        }

        file << "  }\n";
    }


    for (const auto& [_, node] : nodes) {
        for (const auto& edge : node->getOutgoingEdges()) {
            if (auto target = edge.target.lock()) {

                if (isSemanticEdgeType(edge.type)) {
                    file << "  \"" << node->getId() << "\" -> \""
                         << target->getId() << "\" ["
                         << getSemanticEdgeStyle(edge.type) << "];\n";
                }
            }
        }
    }

    file << "}\n";
}

void DAG::generateMethodologyFlow(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file) {
        std::cerr << "Failed to open file for methodology flow: " << filename << std::endl;
        return;
    }

    file << "digraph MethodologyFlow {\n";
    file << "  rankdir=LR;\n";
    file << "  node [shape=box, style=filled, fontname=\"Arial\"];\n";


    std::vector<std::shared_ptr<DAGNode>> methodNodes;
    std::vector<std::vector<std::shared_ptr<DAGNode>>> phases;

    for (const auto& [_, node] : nodes) {
        if (isMethodologyComponent(node)) {
            methodNodes.push_back(node);
        }
    }


    std::unordered_set<std::string> processed;
    std::function<void(std::shared_ptr<DAGNode>, int)> assignPhase =
        [&](std::shared_ptr<DAGNode> node, int phase) {
        if (processed.find(node->getId()) != processed.end()) return;
        processed.insert(node->getId());


        while (phases.size() <= static_cast<size_t>(phase)) {
            phases.push_back({});
        }
        phases[phase].push_back(node);


        for (const auto& edge : node->getOutgoingEdges()) {
            if (edge.type == EdgeType::MethodologyFlow) {
                if (auto target = edge.target.lock()) {
                    assignPhase(target, phase + 1);
                }
            }
        }
    };


    for (const auto& node : methodNodes) {
        bool isStart = true;
        for (const auto& edge : node->getIncomingEdges()) {
            if (edge.type == EdgeType::MethodologyFlow) {
                isStart = false;
                break;
            }
        }
        if (isStart) {
            assignPhase(node, 0);
        }
    }


    for (size_t i = 0; i < phases.size(); ++i) {
        file << "  subgraph cluster_phase" << i << " {\n";
        file << "    label=\"Phase " << i + 1 << "\";\n";
        file << "    color=lightgrey;\n";

        for (const auto& node : phases[i]) {
            file << "    \"" << node->getId() << "\" [label=\""
                 << node->getContent() << "\"];\n";
        }

        file << "  }\n";
    }


    for (const auto& [_, node] : nodes) {
        for (const auto& edge : node->getOutgoingEdges()) {
            if (edge.type == EdgeType::MethodologyFlow) {
                if (auto target = edge.target.lock()) {
                    file << "  \"" << node->getId() << "\" -> \""
                         << target->getId() << "\" [color=\"blue\"];\n";
                }
            } else if (edge.type == EdgeType::DataDependency) {
                if (auto target = edge.target.lock()) {
                    file << "  \"" << node->getId() << "\" -> \""
                         << target->getId() << "\" [style=\"dashed\", color=\"red\"];\n";
                }
            }
        }
    }

    file << "}\n";
}

std::vector<std::string> DAG::identifyLogicalGaps() const {
    std::vector<std::string> gaps;


    struct LogicalPattern {
        EdgeType prerequisite;
        EdgeType conclusion;
        std::string description;
    };

    std::vector<LogicalPattern> patterns = {
        {EdgeType::Assumption, EdgeType::ResultSupports,
         "Missing connection between assumption and results"},
        {EdgeType::MethodologyFlow, EdgeType::ValidationMethod,
         "Missing validation for methodology step"},
        {EdgeType::MainContribution, EdgeType::EvidenceSupport,
         "Missing evidence for main contribution"},
        {EdgeType::DataDependency, EdgeType::ResultSupports,
         "Missing connection between data and results"}
    };


    for (const auto& [_, node] : nodes) {
        std::set<EdgeType> outgoingTypes;
        for (const auto& edge : node->getOutgoingEdges()) {
            outgoingTypes.insert(edge.type);
        }

        for (const auto& pattern : patterns) {
            if (outgoingTypes.find(pattern.prerequisite) != outgoingTypes.end() &&
                outgoingTypes.find(pattern.conclusion) == outgoingTypes.end()) {
                gaps.push_back(pattern.description + " in: " + node->getContent());
            }
        }


        if (node->getType() == NodeType::Section) {
            bool hasClaim = false;
            bool hasSupport = false;

            for (const auto& edge : node->getOutgoingEdges()) {
                if (edge.type == EdgeType::MainContribution ||
                    edge.type == EdgeType::SubContribution) {
                    hasClaim = true;
                }
                if (edge.type == EdgeType::EvidenceSupport ||
                    edge.type == EdgeType::ResultSupports) {
                    hasSupport = true;
                }
            }

            if (hasClaim && !hasSupport) {
                gaps.push_back("Unsupported claim in section: " + node->getContent());
            }
        }
    }

    return gaps;
}


bool DAG::isMethodologyComponent(const std::shared_ptr<DAGNode>& node) const {
    if (!node) return false;


    if (node->getType() == NodeType::Section) {
        std::string content = node->getContent();
        std::transform(content.begin(), content.end(), content.begin(), ::tolower);


        if (content.find("method") != std::string::npos ||
            content.find("approach") != std::string::npos ||
            content.find("implementation") != std::string::npos ||
            content.find("procedure") != std::string::npos ||
            content.find("algorithm") != std::string::npos) {
            return true;
        }
    }


    for (const auto& edge : node->getOutgoingEdges()) {
        if (edge.type == EdgeType::MethodologyFlow ||
            edge.type == EdgeType::ExperimentalSetup) {
            return true;
        }
    }

    return false;
}

bool DAG::isSemanticEdgeType(EdgeType type) const {
    static const std::set<EdgeType> semanticTypes = {
        EdgeType::ConceptDependency,
        EdgeType::Definition,
        EdgeType::RelatedWork,
        EdgeType::ResultSupports,
        EdgeType::TheoremDependency
    };

    return semanticTypes.find(type) != semanticTypes.end();
}

std::string DAG::getSemanticEdgeStyle(EdgeType type) const {
    switch (type) {
        case EdgeType::ConceptDependency:
            return "color=\"blue\", style=\"solid\", weight=2";
        case EdgeType::Definition:
            return "color=\"green\", style=\"dashed\"";
        case EdgeType::RelatedWork:
            return "color=\"purple\", style=\"dotted\"";
        case EdgeType::ResultSupports:
            return "color=\"red\", style=\"bold\"";
        case EdgeType::TheoremDependency:
            return "color=\"orange\", style=\"solid\"";
        default:
            return "color=\"black\", style=\"solid\"";
    }
}

bool DAG::validateRelationshipChain(const std::vector<std::shared_ptr<DAGNode>>& chain,
                                  EdgeType relationType) const {
    if (chain.empty()) return true;


    for (size_t i = 0; i < chain.size() - 1; ++i) {
        bool connected = false;
        for (const auto& edge : chain[i]->getOutgoingEdges()) {
            if (edge.type == relationType) {
                if (auto target = edge.target.lock()) {
                    if (target == chain[i + 1]) {
                        connected = true;
                        break;
                    }
                }
            }
        }
        if (!connected) return false;
    }

    return true;
}

std::shared_ptr<RelationshipMetadata> DAGNode::getRelationshipMetadata(
    const std::shared_ptr<DAGNode>& target, EdgeType type) const {
    

    const auto& relationships = relationshipManager.getRelationships();
    

    auto typeIt = relationships.find(type);
    if (typeIt != relationships.end()) {

        auto targetIt = typeIt->second.find(target);
        if (targetIt != typeIt->second.end()) {

            return std::make_shared<RelationshipMetadata>(targetIt->second);
        }
    }
    
    return nullptr;
}
