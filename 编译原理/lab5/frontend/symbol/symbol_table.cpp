#include <frontend/symbol/symbol_table.h>
#include <debug.h>

namespace FE::Sym
{
    void SymTable::reset_impl()
    {
        scopeStack.clear();
        scopeStack.emplace_back();
    }

    void SymTable::enterScope_impl()
    {
        if (scopeStack.empty()) reset_impl();
        scopeStack.emplace_back();
    }

    void SymTable::exitScope_impl()
    {
        if (scopeStack.empty()) return;
        scopeStack.pop_back();
        if (scopeStack.empty()) scopeStack.emplace_back();
    }

    void SymTable::addSymbol_impl(Entry* entry, FE::AST::VarAttr& attr)
    {
        if (scopeStack.empty()) reset_impl();
        attr.scopeLevel               = static_cast<int>(scopeStack.size() - 1);
        scopeStack.back()[entry] = attr;
    }

    FE::AST::VarAttr* SymTable::getSymbol_impl(Entry* entry)
    {
        if (scopeStack.empty()) return nullptr;
        for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it)
        {
            auto findIt = it->find(entry);
            if (findIt != it->end()) return &(findIt->second);
        }
        return nullptr;
    }

    bool SymTable::isGlobalScope_impl() { return scopeStack.size() <= 1; }

    int SymTable::getScopeDepth_impl()
    {
        if (scopeStack.empty()) return -1;
        return static_cast<int>(scopeStack.size() - 1);
    }
}  // namespace FE::Sym
