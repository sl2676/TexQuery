#include "fsm.h"
#include <cmath>
#include <exception>
#include <vector>
using json = nlohmann::json;

const std::unordered_map<FSM::FSMState, std::set<FSM::FSMState>> FSM::validTransitions = {
    {FSMState::Start, {FSMState::InDocument, FSMState::InCommand}},
    {FSMState::InDocument, {FSMState::InDocument, FSMState::InSection, FSMState::InCommand, 
                           FSMState::InText, FSMState::InMath, FSMState::InAuthor, 
                           FSMState::InAbstract, FSMState::InEnvironment}},
    {FSMState::InSection, {FSMState::InSection, FSMState::InText, FSMState::InCommand, 
                          FSMState::InMath, FSMState::InEnvironment}},
    {FSMState::InCommand, {FSMState::InCommand, FSMState::InDocument, FSMState::InSection, 
                          FSMState::InText, FSMState::InAuthor, FSMState::InAffiliation,
                          FSMState::InMath, FSMState::InEnvironment, FSMState::InEquation,
                           FSMState::InAbstract}},
    {FSMState::InText, {FSMState::InText, FSMState::InCommand, FSMState::InMath, 
                       FSMState::InSection, FSMState::InEnvironment}},
    {FSMState::InEnvironment, {FSMState::InEnvironment, FSMState::InText, FSMState::InCommand, 
                              FSMState::InMath, FSMState::InFigure, FSMState::InTable}},
    {FSMState::InEquation, {FSMState::InEquation, FSMState::InText, FSMState::InMath}},
    {FSMState::InMath, {FSMState::InMath, FSMState::InText, FSMState::InDocument, 
                        FSMState::InEquation, FSMState::InCommand}},
    {FSMState::InFigure, {FSMState::InFigure, FSMState::InText, FSMState::InCommand, 
                          FSMState::InEnvironment}},
    {FSMState::InTable, {FSMState::InTable, FSMState::InText, FSMState::InCommand, 
                         FSMState::InEnvironment}},
    {FSMState::InBibliography, {FSMState::InBibliography, FSMState::InText, 
                               FSMState::InCommand}},
    {FSMState::InAuthor, {FSMState::InAuthor, FSMState::InAffiliation, FSMState::InText, 
                         FSMState::InCommand}},
    {FSMState::InAffiliation, {FSMState::InAffiliation, FSMState::InAuthor, FSMState::InText, 
                              FSMState::InCommand}},
    {FSMState::InAbstract, {FSMState::InAbstract, FSMState::InText, FSMState::InCommand, 
                           FSMState::InMath}}
};

std::string FSM::getStateName(FSMState state) const {
    static const std::unordered_map<FSMState, std::string> stateNames = {
        {FSMState::Start, "Start"},
        {FSMState::InDocument, "InDocument"},
        {FSMState::InSection, "InSection"},
        {FSMState::InCommand, "InCommand"},
        {FSMState::InText, "InText"},
        {FSMState::InEnvironment, "InEnvironment"},
        {FSMState::InEquation, "InEquation"},
        {FSMState::InMath, "InMath"},
        {FSMState::InFigure, "InFigure"},
        {FSMState::InTable, "InTable"},
        {FSMState::InBibliography, "InBibliography"},
        {FSMState::InAuthor, "InAuthor"},
        {FSMState::InAffiliation, "InAffiliation"},
        {FSMState::InAbstract, "InAbstract"}
    };
    
    auto it = stateNames.find(state);
    if (it != stateNames.end()) {
        return it->second;
    }
    return "Unknown";
}

FSM::FSM() : currentState(FSMState::Start), ner() {}

void FSM::validateStateTransition(FSMState newState) {
    try {
        if (!isTransitionValid(currentState, newState)) {
            std::stringstream ss;
            ss << "Invalid state transition from " << static_cast<int>(currentState) 
            << " to " << static_cast<int>(newState);
            throw ParserError(ss.str(), 0);
        }
    } catch (const std::exception& e) {
            std::throw_with_nested(ParserError("State transition validation failed", 0));
    }
}



void FSM::setState(FSMState newState) {
    FSMState prevState = currentState;
    
    validateStateTransition(newState);
    
    currentState = newState;
    
    handleStateChange(prevState, newState);
}

void FSM::handleStateChange(FSMState oldState, FSMState newState) {
    switch (oldState) {
        case FSMState::InAuthor:
            if (newState != FSMState::InAffiliation) {
                insideAuthorBlock = false;
            }
            break;
        case FSMState::InAffiliation:
            if (newState != FSMState::InAuthor) {
                insideInstituteBlock = false;
            }
            break;
        default:
            break;
    }

    switch (newState) {
        case FSMState::InAuthor:
            insideAuthorBlock = true;
            break;
        case FSMState::InAffiliation:
            insideInstituteBlock = true;
            break;
        default:
            break;
    }
}

bool FSM::isValidStructure(const std::shared_ptr<ASTNode>& node) const {
    if (!node) return false;
    
    switch (currentState) {
        case FSMState::InMath:
            return node->getType() == ASTNode::NodeType::Math ||
                   node->getType() == ASTNode::NodeType::Text ||
                   node->getType() == ASTNode::NodeType::Command ||
                   node->getType() == ASTNode::NodeType::Environment;
        case FSMState::InSection:
            return node->getType() != ASTNode::NodeType::Math;
        case FSMState::InAuthor:
            return node->getType() == ASTNode::NodeType::Command ||
                   node->getType() == ASTNode::NodeType::Text;
        default:
            return true;
    }
}

void FSM::traverseAST(const std::shared_ptr<ASTNode>& node, std::string& currentChunk, 
                      std::vector<std::string>& chunks) {
    if (!node) return;

    try {
        if (!isValidStructure(node)) {
            std::stringstream ss;
            ss << "Invalid node structure in state " << static_cast<int>(currentState);
            throw ParserError(ss.str(), 0);
        }

        switch (node->getType()) {
            case ASTNode::NodeType::Document:
                setState(FSMState::InDocument);
                handleDocument(node, currentChunk);
                break;
            case ASTNode::NodeType::Section:
                setState(FSMState::InSection);
                handleSection(node, currentChunk, chunks);
                break;
            case ASTNode::NodeType::Command: {
                setState(FSMState::InCommand);
                std::string cmd = node->getContent();
                if (cmd.find("\\author") != std::string::npos) {
                    setState(FSMState::InAuthor);
                    insideAuthorBlock = true;
                } else if (cmd.find("\\institute") != std::string::npos || 
                         cmd.find("\\affiliation") != std::string::npos) {
                    setState(FSMState::InAffiliation);
                    insideInstituteBlock = true;
                }
                handleCommand(node, currentChunk);
                break;
            }
            case ASTNode::NodeType::Text:
                setState(FSMState::InText);
                handleText(node, currentChunk);
                break;
            case ASTNode::NodeType::Math:
                try {
                    setState(FSMState::InMath);
                    handleMath(node, currentChunk);
                } catch (const std::exception& e) {
                    std::cerr << "Error handling math node: " << e.what() << std::endl;
                    setState(FSMState::InText);
                }
                break; 
            case ASTNode::NodeType::Environment:
                setState(FSMState::InEnvironment);
                handleEnvironment(node, currentChunk);
                break;
            default:
                std::cerr << "Error: Unknown node type encountered: " 
                      << static_cast<int>(node->getType()) 
                      << " (" << node->getNodeTypeName(node->getType()) << ")" << std::endl;
            throw ParserError("Unknown node type", 0);
        }

        for (const auto& child : node->getChildren()) {
            if (child) {
                traverseAST(child, currentChunk, chunks);
            }
        }

        if (auto dagNode = node->getDAGNode()) {
            for (const auto& childDagNode : dagNode->getChildren()) {
                if (auto linkedAstNode = childDagNode->getASTNode().lock()) {
                    traverseAST(linkedAstNode, currentChunk, chunks);
                }
            }
        }

        if (node->getType() == ASTNode::NodeType::Command) {
            std::string cmd = node->getContent();
            if (cmd.find("\\author") != std::string::npos) {
                insideAuthorBlock = false;
            } else if (cmd.find("\\institute") != std::string::npos ||
                      cmd.find("\\affiliation") != std::string::npos) {
                insideInstituteBlock = false;
            }
        }

    } catch (const ParserError& e) {
        std::cerr << "Parser error at position " << e.getPosition() << ": " 
                  << e.what() << "\n" << getCurrentContext() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << "\n" 
                  << getCurrentContext() << std::endl;
    }
}

std::optional<NodeType> convertASTtoDAGNodeType(ASTNode::NodeType type) {
    static const std::unordered_map<ASTNode::NodeType, NodeType> typeMap = {
        {ASTNode::NodeType::Document, NodeType::Document},
        {ASTNode::NodeType::Author, NodeType::Author},
        {ASTNode::NodeType::Affiliation, NodeType::Affiliation},
        {ASTNode::NodeType::Section, NodeType::Section},
        {ASTNode::NodeType::Reference, NodeType::Reference},
        {ASTNode::NodeType::Command, NodeType::Command},
        {ASTNode::NodeType::Text, NodeType::Text},
        {ASTNode::NodeType::Math, NodeType::Math},
        {ASTNode::NodeType::Environment, NodeType::Environment},
        {ASTNode::NodeType::Abstract, NodeType::Environment},
        {ASTNode::NodeType::Bibliography, NodeType::Section},
        {ASTNode::NodeType::Label, NodeType::Reference},
        {ASTNode::NodeType::EnvironmentContent, NodeType::Text}
    };
    
    auto it = typeMap.find(type);
    if (it != typeMap.end()) {
        return it->second;
    }
    
    std::cerr << "Warning: No DAG node type mapping for AST type " 
              << static_cast<int>(type) << std::endl;
    return std::nullopt;
}

std::string FSM::cleanAuthor(const std::string& authorStr) {
    std::string cleaned = authorStr;

    std::regex bracesRegex(R"(\{[^}]*\})");
    cleaned = std::regex_replace(cleaned, bracesRegex, "");

    auto start = cleaned.find_first_not_of(" \t\n\r");
    auto end = cleaned.find_last_not_of(" \t\n\r");
    if (start != std::string::npos && end != std::string::npos) {
        cleaned = cleaned.substr(start, end - start + 1);
    }

    std::transform(cleaned.begin(), cleaned.end(), cleaned.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return cleaned;
}


void FSM::ParserContext::pushEnvironment(const std::string& env) {
    environmentStack.push(env);
}

void FSM::ParserContext::popEnvironment() {
    if (!environmentStack.empty()) {
        environmentStack.pop();
    }
}

bool FSM::ParserContext::isInEnvironment(const std::string& env) const {
    return !environmentStack.empty() && environmentStack.top() == env;
}

bool FSM::isTransitionValid(FSMState from, FSMState to) const {
    if (validTransitions.find(from) == validTransitions.end()) {
        return false;
    }
    const auto& validStates = validTransitions.at(from);
    return validStates.find(to) != validStates.end();
}

std::string FSM::getCurrentContext() const {
    std::stringstream ss;
    ss << "Current State: " << static_cast<int>(currentState) << "\n";
    ss << "Current State name: " << getStateName(currentState) << "\n";
    ss << "Inside Author Block: " << (insideAuthorBlock ? "Yes" : "No") << "\n";
    ss << "Inside Institute Block: " << (insideInstituteBlock ? "Yes" : "No") << "\n";
    if (!context.environmentStack.empty()) {
        ss << "Current Environment: " << context.environmentStack.top() << "\n";
    }
    return ss.str();
}

void FSM::processAuthorCommand(const std::string& content, Author& author) {
    std::regex emailRegex(R"(\{email:\s*([^}]+)\})");
    std::regex orcidRegex(R"(\{orcid:\s*([^}]+)\})");
    std::regex affRegex(R"(\{aff(?:iliation)?:\s*([^}]+)\})");
    
    std::smatch emailMatch;
    if (std::regex_search(content, emailMatch, emailRegex)) {
        author.email = emailMatch[1].str();
    }
    
    std::smatch orcidMatch;
    if (std::regex_search(content, orcidMatch, orcidRegex)) {
        author.orcid = orcidMatch[1].str();
    }
    
    std::string remaining = content;
    std::smatch affMatch;
    while (std::regex_search(remaining, affMatch, affRegex)) {
        author.affiliationLabels.push_back(affMatch[1].str());
        remaining = affMatch.suffix().str();
    }
    
    author.name = cleanAuthor(content);
}


void FSM::handleCitationCommand(const std::string& cmd, const std::vector<std::string>& args,
                              std::string& currentChunk) {
    try {
        auto citationNode = createOrGetDAGNode(args[0], ASTNode::NodeType::Citation);
        auto sourceNode = context.getCurrentNode();
        
        if (sourceNode) {
            citationNode->addEdge(sourceNode, EdgeType::Citation);
        }
        
        json citationJson;
        citationJson["type"] = "citation";
        citationJson["command"] = cmd;
        citationJson["sourceNode"] = sourceNode ? sourceNode->getId() : "";
        citationJson["keys"] = json::array();
        citationJson["context"] = currentChunk; 
        
        for (const auto& arg : args) {
            size_t pos = 0;
            
            if (pos < arg.length() && arg[pos] == '[') {
                size_t optEnd = arg.find("]", pos);
                if (optEnd != std::string::npos) {
                    std::string options = arg.substr(pos + 1, optEnd - pos - 1);
                    citationJson["options"] = options;
                    pos = optEnd + 1;
                }
            }
            
            while (pos < arg.length()) {
                pos = arg.find("{", pos);
                if (pos == std::string::npos) break;
                
                std::string key = extractContentBetweenBraces(arg, pos);
                if (!key.empty()) {
                    context.citations.push_back(key);
                    citationJson["keys"].push_back({
                        {"key", key},
                        {"node_id", citationNode->getId()},
                        {"source_location", currentChunk} 
                    });
                }
                pos += key.length() + 2;
            }
        }
        
        context.semanticChunks.push_back(citationJson);
        
        currentChunk += "<citation type=\"" + cmd + "\">\n";
        for (const auto& key : citationJson["keys"]) {
            currentChunk += "  <key>" + key["key"].get<std::string>() + "</key>\n";
        }
        currentChunk += "</citation>\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error in handleCitationCommand: " << e.what() << std::endl;
        throw ParserError("Citation handling failed", 0);
    }
}

std::string FSM::extractContentBetweenBraces(const std::string& str, size_t start_pos) {
    size_t open = str.find('{', start_pos);
    if (open == std::string::npos) return "";
    
    int depth = 1;
    size_t pos = open + 1;
    
    while (pos < str.length() && depth > 0) {
        if (str[pos] == '{') depth++;
        else if (str[pos] == '}') depth--;
        pos++;
    }
    
    if (depth == 0) {
        return str.substr(open + 1, pos - open - 2);
    }
    
    return "";
}

void FSM::handleText(const std::shared_ptr<ASTNode>& node, std::string& currentChunk) {
    if (!node) return;
    
    try {
        std::string textContent = node->getContent();

        if (insideAuthorBlock) {
            std::stringstream ss;
            ss << "<author_content>\n"
               << "  <text>" << textContent << "</text>\n";
            
            std::regex emailRegex(R"(\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}\b)");
            std::smatch matches;
            if (std::regex_search(textContent, matches, emailRegex)) {
                ss << "  <email>" << matches[0].str() << "</email>\n";
            }
            
            ss << "</author_content>\n";
            authorBlockContent += ss.str();
            
            auto authorNode = createOrGetDAGNode(textContent, ASTNode::NodeType::Author);
            node->setDAGNode(authorNode);
            return;
        }

        if (insideInstituteBlock) {
            std::stringstream ss;
            ss << "<affiliation_content>\n"
               << "  <text>" << textContent << "</text>\n";
            
            if (textContent.find("Department") != std::string::npos ||
                textContent.find("School") != std::string::npos) {
                ss << "  <department>true</department>\n";
            }
            if (textContent.find("University") != std::string::npos ||
                textContent.find("Institute") != std::string::npos) {
                ss << "  <institution>true</institution>\n";
            }
            
            ss << "</affiliation_content>\n";
            instituteBlockContent += ss.str();
            
            auto affiliationNode = createOrGetDAGNode(textContent, ASTNode::NodeType::Affiliation);
            node->setDAGNode(affiliationNode);
            return;
        }

        std::stringstream contentStream;
        std::string remaining = textContent;
        std::regex inlineMathRegex(R"(\$(.*?)\$)");
        std::smatch matches;
        
        while (std::regex_search(remaining, matches, inlineMathRegex)) {
            if (matches.prefix().length() > 0) {
                contentStream << "<text_content>" << matches.prefix().str() << "</text_content>\n";
            }
            
            contentStream << "<inline_math>" << matches[1].str() << "</inline_math>\n";
            
            remaining = matches.suffix();
        }
        
        if (!remaining.empty()) {
            contentStream << "<text_content>" << remaining << "</text_content>\n";
        }
        
        currentChunk += contentStream.str();

    } catch (const std::exception& e) {
        std::cerr << "Error in handleText: " << e.what() << std::endl;
        throw ParserError("Text handling failed", 0);
    }
}

void FSM::handleSection(const std::shared_ptr<ASTNode>& node, std::string& currentChunk, 
                       std::vector<std::string>& chunks) {
    if (!node) return;

    try {
        std::string sectionContent = node->getContent();
        
        currentChunk += "<section_header>\n";
        currentChunk += "  <section_title>" + sectionContent + "</section_title>\n";
        currentChunk += "  <section_level>" + 
                       std::to_string(node->getSectionLevel()) + 
                       "</section_level>\n";
        currentChunk += "</section_header>\n";

        auto sectionNode = createOrGetDAGNode(sectionContent, ASTNode::NodeType::Section);
        node->setDAGNode(sectionNode);

        if (!currentChunk.empty()) {
            chunks.push_back(currentChunk);
            currentChunk.clear();
        }

    } catch (const std::exception& e) {
        std::cerr << "Error in handleSection: " << e.what() << std::endl;
        throw ParserError("Section handling failed", 0);
    }
}

void FSM::handleDocument(const std::shared_ptr<ASTNode>&, std::string& currentChunk) {
    currentChunk += "Document Root\n";
}


void FSM::handleEnvironment(const std::shared_ptr<ASTNode>& node, std::string& currentChunk) {
    if (!node) return;

    try {
        std::string envContent = node->getContent();
        
        if (currentState != FSMState::InEnvironment) {
            setState(FSMState::InEnvironment);
        }

        std::regex envRegex(R"(\\(begin|end)\{(\w+)\})");
        std::smatch matches;

        if (std::regex_search(envContent, matches, envRegex)) {
            std::string envAction = matches[1].str();
            std::string envType = matches[2].str();

            if (envAction == "begin") {
                context.pushEnvironment(envType);
                currentChunk += "<environment_start type=\"" + envType + "\">\n";
                
                if (envType == "figure" || envType == "table") {
                    handleFloatEnvironment(envType, node, currentChunk);
                } else if (envType == "abstract") {
                    setState(FSMState::InAbstract);
                    handleAbstractEnvironment(node, currentChunk);
                }
            } else if (envAction == "end") {
                context.popEnvironment();
                currentChunk += "</environment_end>\n";
                setState(FSMState::InDocument);  
            }
        }

        auto envNode = createOrGetDAGNode(envContent, ASTNode::NodeType::Environment);
        if (envNode) {
            node->setDAGNode(envNode);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error in handleEnvironment: " << e.what() << std::endl;
        setState(FSMState::InDocument);  
        throw ParserError("Environment handling failed", 0);
    }
}

std::vector<std::string> FSM::chunkDocument(const std::shared_ptr<ASTNode>& root) {
    std::vector<std::string> chunks;
    std::string currentChunk;

    traverseAST(root, currentChunk, chunks);

    if (!currentChunk.empty()) {
        chunks.push_back(currentChunk);
    }

    return chunks;
}

json FSM::chunkDocumentToJson(const std::shared_ptr<ASTNode>& root) {
    json documentJson;
    documentJson["document"]["metadata"]["authors"] = json::array();
    documentJson["document"]["metadata"]["affiliations"] = json::array();
    documentJson["document"]["content"] = json::array();

    std::string currentChunk;
    std::vector<std::string> chunks;

    traverseAST(root, currentChunk, chunks);

    const auto& entities = ner.getEntities();

    std::unordered_map<std::string, int> affiliationIds;
    int affiliationCounter = 1;

    if (entities.find("authors") != entities.end()) {
        for (const auto& authorName : entities.at("authors")) {
            json authorJson;
            authorJson["name"] = authorName;
            authorJson["affiliations"] = json::array();

            auto authorNode = dag.getNode(authorName);
            if (authorNode) {
                for (const auto& child : authorNode->getChildren()) {
                    if (child->getNodeType() == NodeType::Affiliation) {
                        std::string affiliation = child->getId();

                        if (affiliationIds.find(affiliation) == affiliationIds.end()) {
                            affiliationIds[affiliation] = affiliationCounter++;

                            json affiliationJson;
                            affiliationJson["id"] = affiliationIds[affiliation];
                            affiliationJson["details"] = affiliation;
                            documentJson["document"]["metadata"]["affiliations"].push_back(affiliationJson);
                        }

                        authorJson["affiliations"].push_back(affiliationIds[affiliation]);
                    }
                }
            }

            documentJson["document"]["metadata"]["authors"].push_back(authorJson);
        }
    } else {
        std::cerr << "Warning: No authors found in the NER entities." << std::endl;
    }

    for (const auto& chunk : chunks) {
        documentJson["document"]["content"].push_back(chunk);
    }

    return documentJson;
}

DAG& FSM::getDAG() {
    return dag;
}


void FSM::handleMath(const std::shared_ptr<ASTNode>& node, std::string& currentChunk) {
    if (!node) return;
    
    try {
        std::string mathContent = node->getContent();
        std::stringstream ss;
        ss << "<math_content>\n"
           << "  <math_type>" 
           << (context.isInEnvironment("equation") ? "display" : "inline") 
           << "</math_type>\n"
           << "  <math_expression>" << mathContent << "</math_expression>\n"
           << "</math_content>\n";
        
        currentChunk += ss.str();
        
        auto mathNode = createOrGetDAGNode(mathContent, ASTNode::NodeType::Math);
        if (mathNode) {
            node->setDAGNode(mathNode);
        }
        
        if (currentState == FSMState::InMath) {
            setState(FSMState::InText);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error in handleMath: " << e.what() << std::endl;
        setState(FSMState::InText);  
        throw ParserError("Math handling failed", 0);
    }
}

void FSM::handleCommand(const std::shared_ptr<ASTNode>& node, std::string& currentChunk) {
    if (!node) return;

    try {
        std::string commandContent = node->getContent();
        std::regex cmdRegex(R"(\\(\w+)(?:\[([^\]]*)\])?(?:\{([^}]*)\})*)");
        std::smatch matches;

        if (std::regex_search(commandContent, matches, cmdRegex)) {
            std::string cmdName = matches[1].str();
            std::string cmdOpts = matches[2].str();
            std::vector<std::string> args;
            
            std::string remaining = matches.suffix().str();
            std::regex argRegex(R"(\{([^}]*)\})");
            std::smatch argMatch;
            while (std::regex_search(remaining, argMatch, argRegex)) {
                args.push_back(argMatch[1].str());
                remaining = argMatch.suffix().str();
            }

            auto cmdNode = createOrGetDAGNode(commandContent, ASTNode::NodeType::Command);
            node->setDAGNode(cmdNode);

            if (isDocumentCommand(cmdName)) {
                handleDocumentCommand(cmdName, args, currentChunk);
            } else if (isSectionCommand(cmdName)) {
                handleSectioningCommand(cmdName, args, currentChunk);
            } else if (isMathCommand(cmdName)) {
                handleMathematicalContent(cmdName, args, currentChunk);
            } else if (isTheoremCommand(cmdName)) {
                handleTheoremEnvironment(cmdName, args, currentChunk);
            } else if (isFloatCommand(cmdName)) {
                handleFloatEnvironment(cmdName, args, currentChunk);
            } else if (isCitationCommand(cmdName)) {
                handleCitationCommand(cmdName, args, currentChunk);
            } else if (isReferenceCommand(cmdName)) {
                handleReferenceCommand(cmdName, args, currentChunk);
            } else {
                handleGenericCommand(cmdName, cmdOpts, args, currentChunk);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in handleCommand: " << e.what() << std::endl;
        handleError("Command handling failed", e);
    }
}


void FSM::handleDocumentCommand(const std::string& cmd, const std::vector<std::string>& args,
                              std::string& currentChunk) {
    try {
        FSMState prevState = currentState;
        
        currentChunk += "<document_command type=\"" + cmd + "\">\n";
        
        if (cmd == "documentclass") {
            setState(FSMState::InDocument);  
            currentChunk += "  <class>";
            if (!args.empty()) currentChunk += args[0];
            currentChunk += "</class>\n";
        }
        else if (cmd == "title") {
            setState(FSMState::InDocument);  
            currentChunk += "  <title>";
            if (!args.empty()) currentChunk += args[0];
            currentChunk += "</title>\n";
        }
        else if (cmd == "author") {
            if (!args.empty()) {
                setState(FSMState::InAuthor);  
                handleAuthorCommand(args[0]);
            }
        }
        else if (cmd == "abstract") {
            setState(FSMState::InAbstract); 
            currentChunk += "  <abstract>";
            if (!args.empty()) currentChunk += args[0];
            currentChunk += "</abstract>\n";
        }
        else if (cmd == "date") {
            setState(FSMState::InDocument);  
            currentChunk += "  <date>";
            if (!args.empty()) currentChunk += args[0];
            currentChunk += "</date>\n";
        }
        else if (cmd == "thanks") {
            setState(FSMState::InDocument);  
            currentChunk += "  <acknowledgment>";
            if (!args.empty()) currentChunk += args[0];
            currentChunk += "</acknowledgment>\n";
        }
        
        currentChunk += "</document_command>\n";
        
        if (cmd != "author" && cmd != "abstract") {  
            setState(prevState);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error in handleDocumentCommand: " << e.what() << std::endl;
        setState(FSMState::InDocument);  // Recovery state
        throw ParserError("Document command handling failed", 0);
    }
}


void FSM::handleSectioningCommand(const std::string& cmd, const std::vector<std::string>& args,
                                std::string& currentChunk) {
    try {
        setState(FSMState::InSection);
        
        int level = 0;
        if (cmd == "chapter") level = 1;
        else if (cmd == "section") level = 2;
        else if (cmd == "subsection") level = 3;
        else if (cmd == "subsubsection") level = 4;
        else if (cmd == "paragraph") level = 5;
        else if (cmd == "subparagraph") level = 6;
        
        currentChunk += "<section_header>\n";
        currentChunk += "  <level>" + std::to_string(level) + "</level>\n";
        
        if (!args.empty()) {
            currentChunk += "  <title>" + args[0] + "</title>\n";
            
            size_t labelPos = args[0].find("\\label");
            if (labelPos != std::string::npos) {
                std::string label = extractContentBetweenBraces(args[0], labelPos + 6);
                if (!label.empty()) {
                    currentChunk += "  <label>" + label + "</label>\n";
                }
            }
        }
        
        currentChunk += "</section_header>\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error in handleSectioningCommand: " << e.what() << std::endl;
        throw ParserError("Section command handling failed", 0);
    }
}


void FSM::handleMathematicalContent(const std::string& cmd, const std::vector<std::string>& args,
                                  std::string& currentChunk) {
    try {
        if (cmd == "equation" || cmd == "align" || cmd == "gather" || cmd == "eq") {
            setState(FSMState::InEquation);
            currentChunk += "<display_math type=\"" + cmd + "\">\n";
            for (const auto& arg : args) {
                currentChunk += "  <math_content>" + arg + "</math_content>\n";
            }
            currentChunk += "</display_math>\n";
        } else if (isMathCommand(cmd)) {
            setState(FSMState::InMath);
            currentChunk += "<math_command type=\"" + cmd + "\">\n";
            for (size_t i = 0; i < args.size(); i++) {
                currentChunk += "  <arg" + std::to_string(i+1) + ">" + 
                              args[i] + "</arg" + std::to_string(i+1) + ">\n";
            }
            currentChunk += "</math_command>\n";
        }
        
        if (currentState == FSMState::InMath || currentState == FSMState::InEquation) {
            setState(FSMState::InText);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in handleMathematicalContent: " << e.what() << std::endl;
        setState(FSMState::InText);  
        throw ParserError("Math content handling failed", 0);
    }
}



void FSM::handleTheoremEnvironment(const std::string& cmd, const std::vector<std::string>& args,
                                 std::string& currentChunk) {
    try {
        setState(FSMState::InEnvironment);

        currentChunk += "<theorem type=\"" + cmd + "\">\n";

        size_t startPos = 0;

        if (!args.empty()) {
            size_t labelPos = args[0].find("\\label");
            if (labelPos != std::string::npos) {
                std::string label = extractContentBetweenBraces(args[0], labelPos + 6);
                if (!label.empty()) {
                    currentChunk += "  <label>" + label + "</label>\n";
                }
                startPos = labelPos + 6;
            }

            if (startPos < args[0].length()) {
                size_t titlePos = args[0].find("\\name", startPos);
                if (titlePos != std::string::npos) {
                    std::string title = extractContentBetweenBraces(args[0], titlePos + 5);
                    if (!title.empty()) {
                        currentChunk += "  <title>" + title + "</title>\n";
                    }
                }
            }

            currentChunk += "  <content>" + args[0] + "</content>\n";
        }

        currentChunk += "</theorem>\n";

    } catch (const std::exception& e) {
        std::cerr << "Error in handleTheoremEnvironment: " << e.what() << std::endl;
        throw ParserError("Theorem environment handling failed", 0);
    }
}



void FSM::handleFloatEnvironment(const std::string& cmd, const std::vector<std::string>& args,
                               std::string& currentChunk) {
    try {
        setState(FSMState::InEnvironment);

        currentChunk += "<float type=\"" + cmd + "\">\n";

        for (const auto& arg : args) {
            size_t pos = 0;

            pos = arg.find("\\caption");
            if (pos != std::string::npos) {
                std::string caption = extractContentBetweenBraces(arg, pos + 8);
                if (!caption.empty()) {
                    currentChunk += "  <caption>" + caption + "</caption>\n";
                }
            }

            pos = arg.find("\\label");
            if (pos != std::string::npos) {
                std::string label = extractContentBetweenBraces(arg, pos + 6);
                if (!label.empty()) {
                    currentChunk += "  <label>" + label + "</label>\n";
                }
            }

            pos = arg.find("\\includegraphics");
            if (pos != std::string::npos) {
                size_t optPos = arg.find("[", pos);
                std::string options;
                if (optPos != std::string::npos && optPos < arg.find("{", pos)) {
                    size_t optEnd = arg.find("]", optPos);
                    if (optEnd != std::string::npos) {
                        options = arg.substr(optPos + 1, optEnd - optPos - 1);
                        pos = optEnd + 1;
                    }
                }

                std::string path = extractContentBetweenBraces(arg, pos);
                if (!path.empty()) {
                    currentChunk += "  <graphics";
                    if (!options.empty()) {
                        currentChunk += " options=\"" + options + "\"";
                    }
                    currentChunk += " path=\"" + path + "\"/>\n";
                }
            }
        }

        currentChunk += "</float>\n";

    } catch (const std::exception& e) {
        std::cerr << "Error in handleFloatEnvironment: " << e.what() << std::endl;
        throw ParserError("Float environment handling failed", 0);
    }
}

void FSM::handleReferenceCommand(const std::string& cmd, const std::vector<std::string>& args,
                               std::string& currentChunk) {
    try {
        EdgeType edgeType;
        std::string refType;
        if (cmd == "eqref") {
            edgeType = EdgeType::EquationReference;
            refType = "equation";
        } else if (cmd == "ref") {
            edgeType = EdgeType::CrossReference;
            refType = "standard";
        } else if (cmd == "autoref") {
            edgeType = EdgeType::CrossReference;
            refType = "auto";
        }
        
        auto referenceNode = createOrGetDAGNode(args[0], ASTNode::NodeType::Reference);
        
        json referenceJson;
        referenceJson["type"] = "reference";
        referenceJson["reference_type"] = refType;
        referenceJson["node_id"] = referenceNode->getId();
        referenceJson["context"] = currentChunk;
        
        if (!args.empty()) {
            size_t pos = 0;
            if (args[0][0] == '[') {
                size_t optEnd = args[0].find("]");
                if (optEnd != std::string::npos) {
                    std::string text = args[0].substr(1, optEnd - 1);
                    referenceJson["description"] = text;
                    pos = optEnd + 1;
                }
            }
            
            std::string label = extractContentBetweenBraces(args[0], pos);
            if (!label.empty()) {
                if (auto targetNode = context.getLabeledNode(label)) {
                    referenceNode->addEdge(targetNode, edgeType);
                    referenceJson["target"] = {
                        {"label", label},
                        {"node_id", targetNode->getId()},
                        {"node_type", static_cast<int>(targetNode->getType())}
                    };
                }
            }
        }
        
        context.semanticChunks.push_back(referenceJson);
        
        currentChunk += "<reference type=\"" + refType + "\">\n";
        if (referenceJson.contains("description")) {
            currentChunk += "  <text>" + referenceJson["description"].get<std::string>() + "</text>\n";
        }
        if (referenceJson.contains("target")) {
            currentChunk += "  <label>" + referenceJson["target"]["label"].get<std::string>() + "</label>\n";
        }
        currentChunk += "</reference>\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error in handleReferenceCommand: " << e.what() << std::endl;
        throw ParserError("Reference handling failed", 0);
    }
}

void FSM::handleGenericCommand(const std::string& cmd, const std::string& cmdOpts,
                             const std::vector<std::string>& args, std::string& currentChunk) {
    try {
        currentChunk += "<command name=\"" + cmd + "\">\n";
        
        if (!cmdOpts.empty()) {
            currentChunk += "  <options>" + cmdOpts + "</options>\n";
        }
        
        for (size_t i = 0; i < args.size(); i++) {
            currentChunk += "  <arg" + std::to_string(i+1) + ">";
            size_t pos = 0;
            while (pos < args[i].length()) {
                size_t bracePos = args[i].find("{", pos);
                if (bracePos == std::string::npos) {
                    currentChunk += args[i].substr(pos);
                    break;
                }
                
                currentChunk += args[i].substr(pos, bracePos - pos);
                std::string nested = extractContentBetweenBraces(args[i], bracePos);
                currentChunk += nested;
                pos = bracePos + nested.length() + 2; 
            }
            currentChunk += "</arg" + std::to_string(i+1) + ">\n";
        }
        
        currentChunk += "</command>\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error in handleGenericCommand: " << e.what() << std::endl;
        throw ParserError("Generic command handling failed", 0);
    }
}

void FSM::handleError(const std::string& context, const std::exception& e) {
    std::cerr << "Error in " << context << ": " << e.what() << "\n"
              << "Current state: " << static_cast<int>(currentState) << "\n"
              << "Attempting recovery..." << std::endl;
    
    if (currentState != FSMState::InDocument) {
        setState(FSMState::InDocument);
    }
}

void FSM::handleAuthorCommand(const std::string& args) {
    try {
        if (args.empty()) {
            return;
        }

        parseAuthorBlock(args);
        
        std::string remaining = args;
        size_t pos = 0;
        
        while (pos < remaining.length()) {
            std::string authorChunk = extractNextAuthor(remaining, pos);
            if (authorChunk.empty()) break;
            
            Author author;
            processAuthorCommand(authorChunk, author);
            
            auto authorNode = createAuthorNode(author);
            if (authorNode) {
                processAffiliations(author, authorNode);
            }
            
            authors.push_back(author);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error in handleAuthorCommand: " << e.what() << std::endl;
        throw ParserError("Author command handling failed", 0);
    }
}

void FSM::parseAuthorBlock(const std::string& content) {
    setState(FSMState::InAuthor);
    insideAuthorBlock = true;
    
    std::stringstream ss;
    ss << "<author_block>\n";
    
    std::regex emailRegex(R"(\\email\{([^}]+)\})");
    std::regex thanksRegex(R"(\\thanks\{([^}]+)\})");
    std::regex instRegex(R"(\\inst\{([^}]+)\})");
    
    std::string remaining = content;
    std::smatch matches;
    
    while (std::regex_search(remaining, matches, emailRegex)) {
        ss << "  <email>" << matches[1].str() << "</email>\n";
        remaining = matches.suffix();
    }
    
    while (std::regex_search(remaining, matches, thanksRegex)) {
        ss << "  <thanks>" << matches[1].str() << "</thanks>\n";
        remaining = matches.suffix();
    }
    
    while (std::regex_search(remaining, matches, instRegex)) {
        ss << "  <institution>" << matches[1].str() << "</institution>\n";
        remaining = matches.suffix();
    }
    
    ss << "</author_block>\n";
    authorBlockContent = ss.str();
}

std::string FSM::extractNextAuthor(const std::string& content, size_t& pos) {
    static const std::vector<std::string> delimiters = {
        "\\and", ",", "\\\\", ";"
    };
    
    size_t nextPos = std::string::npos;
    std::string delimiter;
    
    for (const auto& delim : delimiters) {
        size_t foundPos = content.find(delim, pos);
        if (foundPos != std::string::npos && 
            (nextPos == std::string::npos || foundPos < nextPos)) {
            nextPos = foundPos;
            delimiter = delim;
        }
    }
    
    if (nextPos == std::string::npos) {
        std::string result = content.substr(pos);
        pos = content.length();
        return result;
    }
    
    std::string result = content.substr(pos, nextPos - pos);
    pos = nextPos + delimiter.length();
    return result;
}

std::shared_ptr<DAGNode> FSM::createAuthorNode(const Author& author) {
    std::string nodeId = "author_" + author.name;
    auto authorNode = createOrGetDAGNode(nodeId, ASTNode::NodeType::Author);
    
    if (!authorNode) {
        std::cerr << "Failed to create author node for: " << nodeId << std::endl;
        return nullptr;
    }
    
    std::string authorContent = author.name;
    if (!author.email.empty()) {
        authorContent += " <" + author.email + ">";
    }
    
    auto newNode = DAGNode::create(authorContent, NodeType::Author);
    dag.addNode(newNode);
    
    return authorNode;
}

void FSM::processAffiliations(const Author& author, std::shared_ptr<DAGNode> authorNode) {
    for (const auto& affIndex : author.affiliationIndices) {
        if (affIndex < static_cast<int>(unlabeledAffiliations.size())) {
            addAffiliationNode(authorNode, affIndex);
        }
    }
    
    for (const auto& label : author.affiliationLabels) {
        if (affiliationMap.find(label) != affiliationMap.end()) {
            addAffiliationNode(authorNode, label);
        }
    }
}

void FSM::addAffiliationNode(std::shared_ptr<DAGNode> authorNode, const std::string& label) {
    std::string affId = "aff_" + label;
    auto affNode = createOrGetDAGNode(affId, ASTNode::NodeType::Affiliation);
    if (affNode) {
        authorNode->addChild(affNode);
    }
}

void FSM::addAffiliationNode(std::shared_ptr<DAGNode> authorNode, int index) {
    std::string affId = "aff_" + std::to_string(index);
    auto affNode = createOrGetDAGNode(affId, ASTNode::NodeType::Affiliation);
    if (affNode) {
        authorNode->addChild(affNode);
    }
}

void FSM::handleAffiliationCommand(const std::string& args) {
    try {
        if (args.empty()) {
            return;
        }

        unlabeledAffiliations.push_back(args);
        
        auto affNode = createOrGetDAGNode(args, ASTNode::NodeType::Affiliation);
        
        std::regex labelRegex(R"(\{([^}]+)\})");
        std::smatch matches;
        std::string affContent = args;
        
        if (std::regex_search(affContent, matches, labelRegex)) {
            std::string label = matches[1].str();
            affiliationMap[label] = affContent;
        }
        
        if (!authors.empty()) {
            auto authorNode = createOrGetDAGNode(
                authors.back().name, 
                ASTNode::NodeType::Author
            );
            authorNode->addChild(affNode);
            
            authors.back().affiliations.push_back(args);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error in handleAffiliationCommand: " << e.what() << std::endl;
        throw ParserError("Affiliation command handling failed", 0);
    }
}

void FSM::handleFloatEnvironment(const std::string& type, 
                                const std::shared_ptr<ASTNode>& node,
                                std::string& currentChunk) {
    currentChunk += "<float type=\"" + type + "\">\n";
    
    for (const auto& child : node->getChildren()) {
        if (child->getType() == ASTNode::NodeType::Command) {
            std::string cmd = child->getContent();
            if (cmd.find("\\caption") != std::string::npos) {
                currentChunk += "  <caption>" + extractContentBetweenBraces(cmd, 8) + "</caption>\n";
            } else if (cmd.find("\\label") != std::string::npos) {
                currentChunk += "  <label>" + extractContentBetweenBraces(cmd, 6) + "</label>\n";
            }
        }
    }
}

void FSM::handleAbstractEnvironment(const std::shared_ptr<ASTNode>& node,
                                  std::string& currentChunk) {
    currentChunk += "<abstract>\n";
    
    for (const auto& child : node->getChildren()) {
        if (child->getType() == ASTNode::NodeType::Text) {
            currentChunk += "  <abstract_text>" + child->getContent() + "</abstract_text>\n";
        }
    }
    
    currentChunk += "</abstract>\n";
}

std::shared_ptr<DAGNode> FSM::createOrGetDAGNode(const std::string& content, ASTNode::NodeType astType) {
    auto dagTypeOpt = convertASTtoDAGNodeType(astType);
    if (!dagTypeOpt.has_value()) {
        std::cerr << "Warning: Could not convert AST type " << static_cast<int>(astType) 
                  << " to DAG type" << std::endl;
        return nullptr;
    }
    NodeType dagType = dagTypeOpt.value();

    std::string safeContent = content;
    safeContent.erase(std::remove_if(safeContent.begin(), safeContent.end(), 
                     [](char c) { return !std::isalnum(c) && c != '_'; }), 
                     safeContent.end());
    
    static int nodeCounter = 0;
    std::string nodeId = std::to_string(++nodeCounter) + "_" + 
                        std::to_string(static_cast<int>(dagType)) + "_" +
                        (safeContent.length() > 30 ? safeContent.substr(0, 30) : safeContent);

    auto existingNode = dag.getNode(nodeId);
    if (existingNode) {
        return existingNode;
    }

    try {
        auto newNode = DAGNode::create(nodeId, dagType);
        if (!newNode) {
            throw std::runtime_error("Failed to create DAG node");
        }
        dag.addNode(newNode);
        return newNode;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create DAG node: " << e.what() << std::endl;
        return nullptr;
    }
}


bool FSM::isDocumentCommand(const std::string& cmd) const {
    static const std::unordered_set<std::string> docCommands = {
        "documentclass", "usepackage", "title", "author", "date",
        "maketitle", "begin", "end", "institute", "abstract",
        "keywords", "thanks", "acknowledgements"
    };
    return docCommands.find(cmd) != docCommands.end();
}

bool FSM::isSectionCommand(const std::string& cmd) const {
    static const std::unordered_set<std::string> secCommands = {
        "section", "subsection", "subsubsection", "paragraph",
        "subparagraph", "part", "chapter"
    };
    return secCommands.find(cmd) != secCommands.end();
}

bool FSM::isMathCommand(const std::string& cmd) const {
    static const std::unordered_set<std::string> mathCommands = {
        // Greek letters
        "alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta",
        "iota", "kappa", "lambda", "mu", "nu", "xi", "pi", "rho", "sigma",
        "tau", "upsilon", "phi", "chi", "psi", "omega",
        // Math operators
        "sum", "int", "prod", "coprod", "bigcup", "bigcap", "frac", "sqrt",
        "partial", "nabla", "infty", "lim", "sup", "inf",
        // Math Relations
        "leq", "geq", "equiv", "sim", "simeq", "approx", "propto",
        // Math Environments
        "equation", "align", "gather", "multline", "matrix", "cases",
        // Math fonts
        "mathbf", "mathcal", "mathit", "mathrm", "mathsf", "mathtt"
    };
    return mathCommands.find(cmd) != mathCommands.end();
}

bool FSM::isTheoremCommand(const std::string& cmd) const {
    static const std::unordered_set<std::string> theoremCommands = {
        "theorem", "lemma", "proposition", "corollary", "definition",
        "remark", "proof", "example", "notation", "claim"
    };
    return theoremCommands.find(cmd) != theoremCommands.end();
}

bool FSM::isFloatCommand(const std::string& cmd) const {
    static const std::unordered_set<std::string> floatCommands = {
        "figure", "table", "algorithm", "listing",
        "includegraphics", "caption", "subfigure", "subfloat"
    };
    return floatCommands.find(cmd) != floatCommands.end();
}

bool FSM::isCitationCommand(const std::string& cmd) const {
    static const std::unordered_set<std::string> citeCommands = {
        "cite", "citep", "citet", "citeyear", "citeauthor",
        "bibliographystyle", "bibliography", "bibitem"
    };
    return citeCommands.find(cmd) != citeCommands.end();
}

bool FSM::isReferenceCommand(const std::string& cmd) const {
    static const std::unordered_set<std::string> refCommands = {
        "label", "ref", "eqref", "pageref", "autoref", "cref"
    };
    return refCommands.find(cmd) != refCommands.end();
}

FSM::FSMState FSM::getCurrentState() const {
    return currentState;
}

bool FSM::isInEnvironment(const std::string& env) const {
    return context.isInEnvironment(env);
}
