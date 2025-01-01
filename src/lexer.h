#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <cstddef>
#include <cctype>
#include <stdexcept>


enum class TokenType {
	Command,            
	BeginEnvironment,   
	EndEnvironment,     
	Text,               
	OpenBrace,          
	CloseBrace,
	CloseBracket,
	MathShift,          
	Comment,           
	EOFToken            
};


struct Token {
	TokenType type;
	std::string value;
	std::size_t position;
};


class Lexer {
public:
	Lexer(const std::string& input);
	Token getNextToken();
	std::string parseBraceContent();
	std::string parseBracketContent();

private:
	std::string input;
	std::size_t pos;
	std::size_t length;
	
	char peek() const;
	char get();
	void skipWhitespace();
	void skipComment();

	Token lexCommand();
	
    Token lexText();
	void expect(char expectedChar);
};

#endif
