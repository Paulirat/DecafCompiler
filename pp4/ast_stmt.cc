/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"
#include "errors.h"
#include "ast_type.h"

int Scope::Add_Declaration(Decl *d) {
    Decl *lookup = table->Lookup(d->Name());

    if (lookup != NULL) {
        ReportError::DeclConflict(d, lookup); // Conflict Declarations with the same name.
        return 1;
    }

    table->Enter(d->Name(), d);
    return 0;
}

Scope *Program::G_Scope = new Scope();

Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
}

void Program::Check() {
    ScopeMaker();

    for (int i = 0, n = decls->NumElements(); i < n; ++i)
        decls->Nth(i)->Check();
}

void Program::ScopeMaker() {
    for (int i = 0, n = decls->NumElements(); i < n; ++i)
        G_Scope->Add_Declaration(decls->Nth(i));

    for (int i = 0, n = decls->NumElements(); i < n; ++i)
        decls->Nth(i)->ScopeMaker(G_Scope);
}

void Stmt::ScopeMaker(Scope *parent) {
    scope->SetParent(parent);
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}

void StmtBlock::ScopeMaker(Scope *parent) {
    scope->SetParent(parent);

    for (int i = 0, n = decls->NumElements(); i < n; ++i)
        scope->Add_Declaration(decls->Nth(i));

    for (int i = 0, n = decls->NumElements(); i < n; ++i)
        decls->Nth(i)->ScopeMaker(scope);

    for (int i = 0, n = stmts->NumElements(); i < n; ++i)
        stmts->Nth(i)->ScopeMaker(scope);
}

void StmtBlock::Check() {
    for (int i = 0, n = decls->NumElements(); i < n; ++i)
        decls->Nth(i)->Check();

    for (int i = 0, n = stmts->NumElements(); i < n; ++i)
        stmts->Nth(i)->Check();
}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) { 
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this); 
    (body=b)->SetParent(this);
}

void ConditionalStmt::ScopeMaker(Scope *parent) {
    scope->SetParent(parent);

    test->ScopeMaker(scope);
    body->ScopeMaker(scope);
}

void ConditionalStmt::Check() {
    test->Check();
    body->Check();

    if (!test->TypeFinder()->AreEquivalent(Type::boolType))
        ReportError::TestNotBoolean(test);
}

void LoopStatement::ScopeMaker(Scope *parent) {
    scope->SetParent(parent);
    scope->SetLoopStatement(this);

    test->ScopeMaker(scope);
    body->ScopeMaker(scope);
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStatement(t, b) { 
    Assert(i != NULL && t != NULL && s != NULL && b != NULL);
    (init=i)->SetParent(this);
    (step=s)->SetParent(this);
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) { 
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}

void IfStmt::ScopeMaker(Scope *parent) {
    scope->SetParent(parent);

    test->ScopeMaker(scope);
    body->ScopeMaker(scope);

    if (elseBody != NULL)
        elseBody->ScopeMaker(scope);
}

void IfStmt::Check() {
    test->Check();
    body->Check();

    if (!test->TypeFinder()->AreEquivalent(Type::boolType))
        ReportError::TestNotBoolean(test);

    if (elseBody != NULL)
        elseBody->Check();
}

void BreakStmt::Check() {
    Scope *s = scope;
    while (s != NULL) {
        if (s->GetLoopStatement() != NULL)
            return;
        if (s->GetSwitchStatement() != NULL)
            return;

        s = s->GetParent();
    }

    ReportError::BreakOutsideLoop(this);
}

ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) { 
    Assert(e != NULL);
    (expr=e)->SetParent(this);
}

void ReturnStmt::ScopeMaker(Scope *parent) {
    scope->SetParent(parent);

    expr->ScopeMaker(scope);
}

void ReturnStmt::Check() {
    expr->Check();

    FnDecl *d = NULL;
    Scope *s = scope;
    while (s != NULL) {
        if ((d = s->GetFnDecl()) != NULL)
            break;

        s = s->GetParent();
    }

    if (d == NULL) {
        ReportError::Formatted(location,
                               "return is only allowed inside a function");
        return;
    }

    Type *expected = d->GetReturnType();
    Type *given = expr->TypeFinder();

    if (!given->AreEquivalent(expected))
        ReportError::ReturnMismatch(this, given, expected);
    
    EmptyExpr *ee = dynamic_cast<EmptyExpr*>(expr);
    if (ee != NULL && expected != Type::voidType)
        ReportError::ReturnMismatch(this, Type::voidType, expected);
}
  
PrintStmt::PrintStmt(List<Expr*> *a) {    
    Assert(a != NULL);
    (args=a)->SetParentAll(this);
}

void PrintStmt::ScopeMaker(Scope *parent) {
    scope->SetParent(parent);

    for (int i = 0, n = args->NumElements(); i < n; ++i)
        args->Nth(i)->ScopeMaker(scope);
}

void PrintStmt::Check() {
    for (int i = 0, n = args->NumElements(); i < n; ++i) {
        Type *given = args->Nth(i)->TypeFinder();

        if (!(given->AreEquivalent(Type::intType) ||
              given->AreEquivalent(Type::boolType) ||
              given->AreEquivalent(Type::stringType)))
            ReportError::PrintArgMismatch(args->Nth(i), i+1, given);
    }

    for (int i = 0, n = args->NumElements(); i < n; ++i)
        args->Nth(i)->Check();
}

SwitchStatement::SwitchStatement(Expr *e, List<CaseStmt*> *s) {
    Assert(e != NULL && s != NULL); // DefaultStmt can be NULL
    (expr=e)->SetParent(this);
    (caseStmts=s)->SetParentAll(this);
}

void SwitchStatement::ScopeMaker(Scope *parent) {
    scope->SetParent(parent);
    scope->SetSwitchStatement(this);

    expr->ScopeMaker(scope);
    for (int i = 0, n = caseStmts->NumElements(); i < n; ++i)
        caseStmts->Nth(i)->ScopeMaker(scope);
}

void SwitchStatement::Check() {
    expr->Check();
    for (int i = 0, n = caseStmts->NumElements(); i < n; ++i)
        caseStmts->Nth(i)->Check();
}

SwitchStatement::CaseStmt::CaseStmt(Expr *e, List<Stmt*> *s) {
    Assert(s != NULL);
    
    intConst=e;
    if (intConst != NULL)
        intConst->SetParent(this);
        
    (caseBody=s)->SetParentAll(this);
}

void SwitchStatement::CaseStmt::ScopeMaker(Scope *parent) {
    scope->SetParent(parent);
    for (int i = 0, n = caseBody->NumElements(); i < n; ++i)
        caseBody->Nth(i)->ScopeMaker(scope);
}

void SwitchStatement::CaseStmt::Check() {
    for (int i = 0, n = caseBody->NumElements(); i < n; ++i)
        caseBody->Nth(i)->Check();
}
