/* File: ast_type.cc
 * -----------------
 * Implementation of type node classes.
 */
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h>

 
/* Class constants
 * ---------------
 * These are public constants for the built-in base types (int, double, etc.)
 * They can be accessed with the syntax Type::intType. This allows you to
 * directly access them and share the built-in types where needed rather that
 * creates lots of copies.
 */

Type *Type::intType    = new Type("int");
Type *Type::doubleType = new Type("double");
Type *Type::voidType   = new Type("void");
Type *Type::boolType   = new Type("bool");
Type *Type::nullType   = new Type("null");
Type *Type::stringType = new Type("string");
Type *Type::errorType  = new Type("error"); 

Type::Type(const char *n) {
    Assert(n);
    typeName = strdup(n);
    TD = true;
// No sabemos si el tipo fue declarado
// hasta ver la declaracion de la variable 
    //true por default
}
	
NamedType::NamedType(Identifier *i) : Type(*i->GetLocation()) {
    Assert(i != NULL);
    (id=i)->SetParent(this);
    TD = true;
} 

bool Type::AreEquivalent(Type *other) {
    if (IsEquivalentTo(Type::errorType))
        return true;

    if (IsEquivalentTo(Type::nullType) && dynamic_cast<NamedType*>(other))
        return true;

    return IsEquivalentTo(other);
}


void NamedType::UndeclaredId(reasonT reason) {
    ReportError::IdentifierNotDeclared(id, reason);
}

bool NamedType::IsEquivalentTo(Type *other) {
    NamedType *namedOther = dynamic_cast<NamedType*>(other);

    if (namedOther == NULL)
        return false;

    return *id == *(namedOther->id);
}

bool NamedType::AreEquivalent(Type *other) {
    if (IsEquivalentTo(other))
        return true;

    NamedType *nType = this;
    Decl *lookup;
    while ((lookup = Program::G_Scope->table->Lookup(nType->Name())) != NULL) {
        ClassDecl *c = dynamic_cast<ClassDecl*>(lookup);
        if (c == NULL)
            return false;

        List<NamedType*> *imps = c->GetImplements();
        for (int i = 0, n = imps->NumElements(); i < n; ++i) {
            if (imps->Nth(i)->IsEquivalentTo(other))
                return true;
        }

        nType = c->GetExtends();
        if (nType == NULL)
            break;

        if (nType->IsEquivalentTo(other))
            return true;
    }

    return false;
}

ArrayType::ArrayType(yyltype loc, Type *et) : Type(loc) {
    Assert(et != NULL);
    (elemType=et)->SetParent(this);
    TD = true;
}

ArrayType::ArrayType(Type *et) : Type() {
    Assert(et != NULL);
    (elemType=et)->SetParent(this);
}

void ArrayType::UndeclaredId(reasonT reason) {
    elemType->UndeclaredId(reason);
}

bool ArrayType::IsEquivalentTo(Type *other) {
    ArrayType *arrayOther = dynamic_cast<ArrayType*>(other);

    if (arrayOther == NULL)
        return false;

    return elemType->IsEquivalentTo(arrayOther->elemType);
}

bool ArrayType::AreEquivalent(Type *other) {
    ArrayType *arrayOther = dynamic_cast<ArrayType*>(other);

    if (arrayOther == NULL)
        return false;

    return elemType->AreEquivalent(arrayOther->elemType);
}
