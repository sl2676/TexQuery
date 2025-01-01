#include "parser.h"

Parser::Parser(Lexer& lexer) : lexer(lexer) {
    stateStack.push(ParserState::DefaultState);
    advance();
}


void Parser::advance() {
    currentToken = lexer.getNextToken();

    switch (currentToken.type) {
        case TokenType::MathShift:
            if (currentState() != ParserState::MathModeState) {
                pushState(ParserState::MathModeState);
            } else {
                popState();
            }
            break;
        case TokenType::BeginEnvironment:
            pushState(ParserState::EnvironmentState);
            break;
        case TokenType::EndEnvironment:
            popState();
            break;
        default:
            break;
    }
/*
    std::cout << "Advanced to token: Type=" << static_cast<int>(currentToken.type)
              << ", Value=\"" << currentToken.value << "\""
              << ", State=" << static_cast<int>(currentState()) << "\n";    
*/
}

ParserState Parser::currentState() const {
    if (stateStack.empty()) {
        throw std::runtime_error("State stack is empty. Cannot retrieve current state.");
    }
    return stateStack.top();
}

void Parser::pushState(ParserState state) {
    stateStack.push(state);
}

void Parser::popState() {
    if (stateStack.size() > 1) {
        stateStack.pop();
    }
}

void Parser::expect(TokenType type) {
    if (currentToken.type != type) {
        throw std::runtime_error(
            "Unexpected token at position " + std::to_string(currentToken.position) +
            ". Expected: " + std::to_string(static_cast<int>(type)) +
            ", Got: " + std::to_string(static_cast<int>(currentToken.type)) +
            ", Value: \"" + currentToken.value + "\""
        );
    }
    advance();
}

std::shared_ptr<AST> Parser::parseDocument() {
    auto ast = std::make_shared<AST>();
    while (currentToken.type != TokenType::EOFToken) {
        auto element = parseElement();
        if (element) {
            ast->root->addChild(element);
        }
    }
    return ast;
}

std::shared_ptr<ASTNode> Parser::parseElement() {
    switch (currentState()) {
        case ParserState::DefaultState:
            if (currentToken.type == TokenType::Command) {
                return parseCommand();
            } else if (currentToken.type == TokenType::BeginEnvironment) {
                pushState(ParserState::EnvironmentState);
                return parseEnvironment();
            } else if (currentToken.type == TokenType::Text) {
                return parseText();
            } else if (currentToken.type == TokenType::MathShift) {
                pushState(ParserState::MathModeState);
                return parseMathMode();
            } else if (currentToken.type == TokenType::EOFToken) {
                return nullptr;
            } else {
                advance(); 
                return nullptr;
            }

        case ParserState::MathModeState:
            return parseMathMode();

        case ParserState::EnvironmentState:
            return parseEnvironmentContent();

        default:
            advance();
            return nullptr;
    }
}

std::shared_ptr<ASTNode> Parser::parseCommand() {
    std::string commandName = currentToken.value;
    size_t position = currentToken.position;
    advance();

    if (commandName == "\\section") {
        expect(TokenType::OpenBrace);
        std::string content = parseBraceContent();
        auto node = std::make_shared<ASTNode>(ASTNode::NodeType::Section, content, position, currentState());
        return node;
    }
    auto node = std::make_shared<ASTNode>(ASTNode::NodeType::Command, commandName, position, currentState());
    return node;
}

std::shared_ptr<ASTNode> Parser::parseEnvironment() {
    std::string envName = currentToken.value;
    size_t position = currentToken.position;
    advance();

    auto node = std::make_shared<ASTNode>(ASTNode::NodeType::Environment, envName, position, currentState());
    while (currentToken.type != TokenType::EOFToken) {
        if (currentToken.type == TokenType::EndEnvironment && currentToken.value == envName) {
            advance();
            break;
        }
        auto child = parseElement();
        if (child) {
            node->addChild(child);
        }
    }
    popState();
    return node;
}

std::shared_ptr<ASTNode> Parser::parseEnvironmentContent() {
    auto node = std::make_shared<ASTNode>(ASTNode::NodeType::EnvironmentContent, "", currentToken.position, currentState());
    while (currentToken.type != TokenType::EndEnvironment && currentToken.type != TokenType::EOFToken) {
        auto child = parseElement();
        if (child) {
            node->addChild(child);
        }
    }
    return node;
}

std::shared_ptr<ASTNode> Parser::parseMathMode() {
    std::string mathDelimiter = currentToken.value;
    size_t position = currentToken.position;
    std::string mathContent;

    advance();
    while (currentToken.type != TokenType::EOFToken) {
        if (currentToken.type == TokenType::MathShift && currentToken.value == mathDelimiter) {
            advance();
            popState();
            break;
        }
        mathContent += currentToken.value;
        advance();
    }
    auto mathNode = std::make_shared<ASTNode>(ASTNode::NodeType::Math, mathContent, position, currentState());
    return mathNode;
}

std::shared_ptr<ASTNode> Parser::parseText() {
    auto node = std::make_shared<ASTNode>(ASTNode::NodeType::Text, currentToken.value, currentToken.position, currentState());
    advance();
    return node;
}

std::string Parser::parseBraceContent() {
    expect(TokenType::OpenBrace);
    std::string content;
    int braceCount = 1;

    while (braceCount > 0 && currentToken.type != TokenType::EOFToken) {
        if (currentToken.type == TokenType::OpenBrace) {
            braceCount++;
        } else if (currentToken.type == TokenType::CloseBrace) {
            braceCount--;
            if (braceCount == 0) {
                advance();
                break;
            }
        }
        content += currentToken.value;
        advance();    
    }
    return content;
}

void Parser::handleLabel(const std::string& label, const std::shared_ptr<ASTNode>& node) {
    symbolTable.addSymbol(label, node);
}

void Parser::handleReference(const std::string& label, const std::shared_ptr<ASTNode>& node) {
    auto referencedNode = symbolTable.getSymbol(label);
    if (referencedNode) {
        node->addReference(referencedNode);
    }
}

std::vector<std::string> Parser::chunkDocument() {
    FSM fsm;
    auto ast = parseDocument(); 
    return fsm.chunkDocument(ast->root);
}
