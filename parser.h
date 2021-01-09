#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <sstream>

namespace Cmp {

// ---------------------------------------------------------------------

class ParseNode
{
public:
    enum Kind { Undefined, Program, Function, Statement, Return,
                Expression, Constant, DataType,
                EndMarker
              };
private:
    ParseNode *_parent,
            *_leftOperand,
            *_rightOperand,
            *_operator;
    LexToken *_tok;
    Kind _kind;
public:
   explicit ParseNode(ParseNode *parent, LexToken *tok, Kind type);
   ~ParseNode();

    Kind kind() const { return _kind; }
    LexToken *lexToken() const { return _tok; }
    ParseNode *parent() const { return _parent; }
    ParseNode *leftOperand() const { return _leftOperand; }
    ParseNode *rightOperand() const { return _rightOperand; }
    ParseNode *operat() const { return _operator; }
    void setLeftOper(ParseNode *left);
    void setRightOper(ParseNode * right);
    void setOperat(ParseNode *oper);
    void setParent(ParseNode *parent);
    void removeChild(ParseNode *child);

    const char* to_cstr() const;

};

// ---------------------------------------------------------------------

class Parser
{
    ParseNode *_root;
    Lexer *_lexer;
   // size_t _curTokIdx;
    const char* _currentfile;
    Lexer::T_Tokens *_tokFile;
    Lexer::T_Tokens::iterator _tokIt;
public:
    explicit Parser(Lexer* lexer, const char* currentfile);
    ~Parser();
    ParseNode *root() const { return _root; }

    bool parse(const char *srcStr = nullptr, const char* otherfile = nullptr);

    std::string to_string() const;

    std::string to_dot(ParseNode *root) const; // graphviz dot code

    bool isValid() const { return _root != nullptr; }

private:

    bool parseProgram();
    bool parseFunction(ParseNode *parent);
    bool parseStatement(ParseNode *parent);
    bool parseExpression(ParseNode *parent);
    bool parseReturn(ParseNode *parent);
    bool parseConstant(ParseNode *parent);

    LexToken *nextTok();
    LexToken *peek(int inc = 1);

    bool failCheck(LexToken *tok, LexToken::Tokens type, bool print = true);

    void recursePrint(ParseNode *node, std::stringstream &res,
                      size_t leftDepth, size_t longestname) const;
    void recurseDot(std::stringstream &dot, ParseNode *n, std::string parentName) const;
    void emptyDot(std::stringstream &dot, std::string parentName) const;
};

}// namespace Cmp

#endif // PARSER_H
