#ifndef GENERATOR_H
#define GENERATOR_H

#include <string>
#include <sstream>

#include "parser.h"

namespace Cmp {


/// goal of this class is to generate asm code from the parsetree
class Generator
{
    Parser *_parser;
    Lexer  *_lexer;
    std::stringstream _res;
    ParseNode *_currentNode;
    bool _epilogCalled;
public:
    explicit Generator(Parser* parser, Lexer *lex);
    virtual ~Generator();

    std::string generate(ParseNode *root);

private:
    void visit();                                           // in post order
                                                            //  (3)op         * <- op last
                                                            //    /  \        *
                                                            //(1)id (2)vlu    *
    void programStart();
    void functionNode();
    void functionProlog();
    void functionEpilog();
    void statementNode();
    void returnNode();
    void constantInt();

    bool next();
};

} // namespace Cmp

#endif // GENERATOR_H
