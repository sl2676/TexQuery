#include "symbol_table.h"

void SymbolTable::addSymbol(const std::string& label, const std::shared_ptr<ASTNode>& node) {
	if (table.find(label) != table.end()) {
		std::cerr << "Warning: Symbol \"" << label << "\" is being overwritten in the symbol table.\n";
	} 
	table[label] = node;
}

std::shared_ptr<ASTNode> SymbolTable::getSymbol(const std::string& label) const {
	auto it = table.find(label);
	if (it != table.end()) {
		return it->second;
	}
	std::cerr << "Warning: Symbol \"" << label << "\" not found in the symbol table.\n";
	return nullptr;
}

bool SymbolTable::hasSymbol(const std::string& label) const {
	return table.find(label) != table.end();	
}
