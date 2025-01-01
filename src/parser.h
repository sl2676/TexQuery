#ifndef PARSER_H
#define PARSER_H
#include <stack>
#include "lexer.h"
#include "ast.h"
#include "symbol_table.h"
#include "parser_state.h"
#include "fsm.h"
#include <stdexcept>
#include <iostream>

class Parser {
public:
    Parser(Lexer& lexer);

    std::shared_ptr<AST> parseDocument();

    std::vector<std::string> chunkDocument();
private:
    Lexer& lexer;
    Token currentToken;
    std::stack<ParserState> stateStack;
    SymbolTable symbolTable;

    void advance();

    ParserState currentState() const;
    void pushState(ParserState state);
    void popState();
    void expect(TokenType type);

    std::shared_ptr<ASTNode> parseElement();
    std::shared_ptr<ASTNode> parseMathMode();
    std::shared_ptr<ASTNode> parseEnvironment();
    std::shared_ptr<ASTNode> parseEnvironmentContent();
    std::shared_ptr<ASTNode> parseCommand();
    std::shared_ptr<ASTNode> parseText();


    std::string parseBraceContent();
    void handleLabel(const std::string& label, const std::shared_ptr<ASTNode>& node);
    void handleReference(const std::string& label, const std::shared_ptr<ASTNode>& node);
};
#endif
