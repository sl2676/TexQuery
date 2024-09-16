#include "lexer.h"

Lexer::Lexer(const std::string& input) : input(input), pos(0), length(input.length()) {}

char Lexer::peek() const {
    return pos < length ? input[pos] : '\0';
}

char Lexer::get() {
    return pos < length ? input[pos++] : '\0';
}

void Lexer::skipWhitespace() {
    while (std::isspace(peek())) {
        get();
    }
}

void Lexer::skipComment() {
    while (peek() != '\n' && peek() != '\0') {
        get();
    }
}

Token Lexer::lexCommand() {
    size_t start = pos - 1;

    while (std::isalpha(peek())) {
        get();
    }
    std::string commandName = input.substr(start, pos - start);

    std::string optionalArg;
    std::string requiredArg;

    if (peek() == '[') {
        optionalArg = parseBracketContent();
    }
    if (peek() == '{') {
        requiredArg = parseBraceContent();
    }
    return {TokenType::Command, commandName + " [" + optionalArg + "] {" + requiredArg + "}", start};
}

Token Lexer::getNextToken() {
    skipWhitespace();

    if (pos >= length) {
        return {TokenType::EOFToken, "", pos};
    }
    char currentChar = get();

    if (currentChar == '$') {
        if (peek() == '$') {
            get();
            return {TokenType::MathShift, "$$", pos - 2};
        } else {
            return {TokenType::MathShift, "$", pos - 1};
        }
    } else if (currentChar == '\\') {
        if (std::isalpha(peek())) {
            return lexCommand();
        } else {
            return {TokenType::Command, std::string(1, currentChar), pos - 1};
        }
    } else if (currentChar == '{') {
        return {TokenType::OpenBrace, "{", pos - 1};
    } else if (currentChar == '}') {
        return {TokenType::CloseBrace, "}", pos - 1};
    } else if (currentChar == '%') {
        skipComment();
        return getNextToken();
    } else {
        pos--;
        return lexText();
    }
}

Token Lexer::lexText() {
    size_t start = pos;
    std::string text;
    while (true) {
        char c = peek();
        if (c == '\\' || c == '{' || c == '}' || c == '%' || c == '\0') {
            break;
        }
        text += get();
    }
    return {TokenType::Text, text, start};
}

void Lexer::expect(char expectedChar) {
    if (peek() != expectedChar) {
        throw std::runtime_error(std::string("Expected '") + expectedChar + "' but found '" + peek() + "'");
    }
    get();
}

std::string Lexer::parseBracketContent() {
    expect('[');
    std::string content;
    int bracketCount = 1;
    while (bracketCount > 0 && peek() != '\0') {
        char c = get();
        if (c == '[') {
            bracketCount++;
        } else if (c == ']') {
            bracketCount--;
            if (bracketCount == 0) {
                break;
            }
        }
        content += c;
    }
    return content;
}

std::string Lexer::parseBraceContent() {
    expect('{');
    std::string content;
    int braceCount = 1;
    while (braceCount > 0 && peek() != '\0') {
        char c = get();
        if (c == '{') {
            braceCount++;
        } else if (c == '}') {
            braceCount--;
            if (braceCount == 0) {
                break;
            }
        }
        content += c;
    }
    return content;
}

