/* File: ast_decl.h
 * ----------------
 * In our parse tree, Decl nodes are used to represent and
 * manage declarations. There are 4 subclasses of the base class,
 * specialized for declarations of variables, functions, classes,
 * and interfaces.
 *
 * pp3: You will need to extend the Decl classes to implement 
 * semantic processing including detection of declaration conflicts 
 * and managing scoping issues.
 */

#ifndef _H_ast_decl
#define _H_ast_decl

#include "ast.h"
#include "ast_stmt.h"
#include "ast_type.h"
#include "list.h"

class Type;
class NamedType;
class Identifier;
class Stmt;

class Decl : public Node 
{
  protected:
    Identifier *id;
    Scope *scope;
  
  public:
    Decl(Identifier *name);
    friend std::ostream& operator<<(std::ostream& out, Decl *d) { return out << d->id; }
    
    virtual bool AreEquivalent(Decl *other);

    const char* Name() { return id->Name(); }
    Scope* GetScope() { return scope; }

    virtual void ScopeMaker(Scope *parent);
    virtual void Check() = 0;
};

class VarDecl : public Decl 
{
  protected:
    Type *type;
    //bool type_declared;
    
  public:
    VarDecl(Identifier *name, Type *type);
    
    bool AreEquivalent(Decl *other);

    Type* TypeFinder() { return type; }
    void Check();
    //bool is_declared_type() { return type_declared; }
    
    
  private:
    void FindType();
};


class ClassDecl : public Decl 
{
  protected:
    List<Decl*> *members;
    NamedType *extends;
    List<NamedType*> *implements;

  public:
    ClassDecl(Identifier *name, NamedType *extends, 
              List<NamedType*> *implements, List<Decl*> *members);
    void ScopeMaker(Scope *parent);
    void Check();
    NamedType* TypeFinder() { return new NamedType(id); }
    NamedType* GetExtends() { return extends; }
    List<NamedType*>* GetImplements() { return implements; }

  private:
    void ExtendsFinder();
    void ImplementsFinder();
    void ExtendedMembersFinder(NamedType *extType);
    void ImplementedMembersFinder(NamedType *impType);
    void AgScopeFinder(Scope *other);
    void ImplementsFinderInterfaces();
};


class InterfaceDecl : public Decl 
{
  protected:
    List<Decl*> *members;
    
  public:
    InterfaceDecl(Identifier *name, List<Decl*> *members);
    void ScopeMaker(Scope *parent);
    void Check();
    Type* TypeFinder() { return new NamedType(id); }
    List<Decl*>* GetMembers() { return members; }
};



class FnDecl : public Decl 
{
  protected:
    List<VarDecl*> *formals;
    Type *returnType;
    Stmt *body;
    
  public:
    FnDecl(Identifier *name, Type *returnType, List<VarDecl*> *formals);
    void SetFunctionBody(Stmt *b);
    void Check();
    bool AreEquivalent(Decl *other);
    Type* GetReturnType() { return returnType; }
    List<VarDecl*>* GetFormals() { return formals; }
    void ScopeMaker(Scope *parent);
    
};

#endif
