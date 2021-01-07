#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <sstream>

namespace Cmp {

// ---------------------------------------------------------------------

class AstNode
{
public:
    enum Type { Undefined, Program, Function, Statement, Return,
                Expression, Constant, DataType,
                EndMarker
              };
private:
    AstNode *_parent,
            *_leftOperand,
            *_rightOperand,
            *_operator;
    LexToken *_tok;
    Type _type;
public:
   explicit AstNode(AstNode *parent, LexToken *tok, Type type);
   ~AstNode();

    LexToken *lexToken() const { return _tok; }
    AstNode *parent() const { return _parent; }
    AstNode *leftOperand() const { return _leftOperand; }
    AstNode *rightOperand() const { return _rightOperand; }
    AstNode *operat() const { return _operator; }
    void setLeftOper(AstNode *left);
    void setRightOper(AstNode * right);
    void setOperat(AstNode *oper);
    void setParent(AstNode *parent);
    void removeChild(AstNode *child);

    const char* to_cstr() const;

};

// ---------------------------------------------------------------------

class Parser
{
    AstNode *_root;
    Lexer *_lexer;
   // size_t _curTokIdx;
    const char* _currentfile;
    Lexer::T_Tokens *_tokFile;
    Lexer::T_Tokens::iterator _tokIt;
public:
    explicit Parser(Lexer* lexer, const char* currentfile);
    ~Parser();
    AstNode *root() const { return _root; }

    bool parse(const char *srcStr = nullptr, const char* otherfile = nullptr);

    std::string to_string() const;

    std::string to_dot(AstNode *root) const; // graphviz dot code

    bool isValid() const { return _root != nullptr; }

private:

    bool parseProgram();
    bool parseFunction(AstNode *parent);
    bool parseStatement(AstNode *parent);
    bool parseExpression(AstNode *parent);
    bool parseConstant(AstNode *parent);

    LexToken *nextTok();
    LexToken *peek(int inc = 1);

    bool failCheck(LexToken *tok, LexToken::Tokens type, bool print = true);

    void recursePrint(AstNode *node, std::stringstream &res,
                      size_t leftDepth, size_t longestname) const;
    void recurseDot(std::stringstream &dot, AstNode *n, std::string parentName) const;
    void emptyDot(std::stringstream &dot, std::string parentName) const;
};

}// namespace Cmp

#endif // PARSER_H
