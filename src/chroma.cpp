#include "chroma.h"

void Evaluator::visit(const Command &n)
{
    n.children[0]->accept(*this);
}

void Evaluator::visit(const Statement &n)
{
    n.children[0]->accept(*this);
}

void Evaluator::visit(const Expression &n)
{
    n.children[0]->accept(*this);
}

void Evaluator::visit(const FuncDeclaration &n)
{
    
}
