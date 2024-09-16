#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <unordered_map>
#include <memory>
#include <iostream>
#include "ast.h"

class SymbolTable {
public:
	void addSymbol(const std::string& label, const std::shared_ptr<ASTNode>& node);
	std::shared_ptr<ASTNode> getSymbol(const std::string& label) const;
	bool hasSymbol(const std::string& label) const;

private:	
	std::unordered_map<std::string, std::shared_ptr<ASTNode>> table;
};


#endif
