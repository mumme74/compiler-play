#include "parser.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <cassert>
#include "lexer.h"


using namespace Cmp;
using namespace std;



// -----------------------------------------------------------


ParseNode::ParseNode(ParseNode *parent, LexToken *tok, ParseNode::Kind type)
    : _parent(parent)
    , _leftOperand(nullptr)
    , _rightOperand(nullptr)
    , _operator(nullptr)
    , _tok(tok)
    , _kind(type)
{ }

ParseNode::~ParseNode()
{
    if (_parent)
        _parent->removeChild(this);

    delete _leftOperand;
    delete _rightOperand;
    delete _operator;

    auto oldRight = _rightOperand;
    auto oldLeft = _leftOperand;
    auto oldOper = _operator;

    _leftOperand = _rightOperand = _operator = nullptr;

    if (oldLeft)
        oldLeft->setParent(nullptr);
    if (oldRight)
        oldRight->setParent(nullptr);
    if (oldOper)
        oldOper->setParent(nullptr);

    delete oldRight;
    delete oldLeft;
    delete oldOper;
}

void ParseNode::setLeftOper(ParseNode *left)
{
    assert(left != _parent);
    assert(left != this);
    assert(_leftOperand != left);
    delete _leftOperand;
    _leftOperand = left;
}

void ParseNode::setRightOper(ParseNode *right)
{
    assert(right != _parent || !right);
    assert(right != this);
    assert(_rightOperand != right || !right);
    delete _rightOperand;
    _rightOperand = right;
}

void ParseNode::setOperat(ParseNode *oper)
{
    assert(oper != _parent || !oper);
    assert(oper != this);
    assert(oper != _operator || !oper);
    delete _operator;
    _operator = oper;
}

void ParseNode::setParent(ParseNode *parent)
{
    assert(parent != _leftOperand || !parent);
    assert(parent != _rightOperand || !parent);
    assert(parent != _operator || !parent);
    assert(parent != this);
    if (_parent != parent)
        removeChild(this);
    _parent = parent;
}

void ParseNode::removeChild(ParseNode *child)
{
    if (child == _leftOperand)
        _leftOperand = nullptr;
    else if (child == _rightOperand)
        _rightOperand = nullptr;
    else if (child == _operator)
        _operator = nullptr;
}

const char *ParseNode::to_cstr() const
{
    switch (_kind) {
    case Undefined: return "Undefined";
    case Program:   return "Program";
    case Function:  return "Function";
    case Statement: return "Statement";
    case Return:    return "Return";
    case Expression:return "Expression";
    case Constant:  return "Constant";
    case DataType:  return "DataType";
    case EndMarker: return "EndMarker";
    }
    assert(0 && "No name Type in Paser, should not happen");
    return nullptr;
}

// -----------------------------------------------------------

Parser::Parser(Lexer *lexer, const char *currentfile)
    : _root(nullptr)
    , _lexer(lexer)
    //, _curTokIdx(0)
    , _currentfile(currentfile)
    , _tokFile(nullptr)
{
    if (_lexer->files.find(_currentfile) != _lexer->files.end()) {
        _tokFile = &_lexer->files.at(_currentfile);
        parse();
    }
}

Parser::~Parser()
{
    delete _root;
}



bool Parser::parse(const char *srcStr, const char* otherfile)
{
    if (otherfile && strncmp(otherfile, _currentfile, 2048) != 0) {
        _currentfile = otherfile;
        if (_lexer->files.find(_currentfile) == _lexer->files.end()) {
            _lexer->tokenize(&srcStr, _currentfile);
        }
    }

    if (_lexer->files.find(_currentfile) == _lexer->files.end() ||
        _lexer->files.at(_currentfile).size() < 1)
    {
        cerr << "file " << _currentfile << " is not tokenized properly"<< endl;
        return false;
    }


    if (_root)
        delete _root;

    _root = nullptr;
    //_curTokIdx = 0;
    _tokFile = &_lexer->files.at(_currentfile);
    _tokIt = _tokFile->begin();

    return parseProgram();
}

string Parser::to_string() const
{
    // go max left first to determine how far out we should be
    size_t leftDepth = 0;
    for(auto n = _root; n != nullptr;) {
        if (n->leftOperand()) {
            n = n->leftOperand();
            ++leftDepth;
        } else
            n = n->operat();
    }

    size_t longestName = 0;
    for (int i = 0; i < ParseNode::EndMarker; ++i) {
        ParseNode n(nullptr, nullptr, static_cast<ParseNode::Kind>(i));
        auto len = strlen(n.to_cstr());
        if (len > longestName)
            longestName = len;
    }


    stringstream res;
    recursePrint(_root, res, leftDepth, longestName);
    return res.str();
}

string Parser::to_dot(ParseNode *root) const
{
    stringstream dot;
    dot << "digraph g{" << endl;
    recurseDot(dot, root, string());
    dot << "}" << endl;
    return dot.str();
}

void Parser::recursePrint(ParseNode *node, stringstream &res,
                          size_t leftDepth, size_t longestname) const
{

    string fill(longestname, ' ');

    for (auto n = node; n != nullptr; n = n->operat()) {
        // fill space left
        for (size_t i = 0; i < leftDepth; ++i)
            res << fill;

        res << n->to_cstr();

        // new line for my children
        res << endl;

        if (n->leftOperand())
            recursePrint(n->leftOperand(), res, leftDepth - 1, longestname);
        if (n->rightOperand())
            recursePrint(n->rightOperand(), res, leftDepth +1, longestname);
    }
}

void Parser::recurseDot(stringstream &dot, ParseNode *n, string parentName) const
{
    if (!n) return;
    //        Program[shape = box];
    //        ProgramLeft[style = invis ];
    //        Program -> ProgramLeft[style = invis ];
    //        Program -> Function;
    //        ProgramRight[style = invis];
    //        Program -> ProgramRight[style = invis];
    //        Function -> DataType;
    //        Function -> Statement;
    //        FunctionRight [style = invis ];
    //        Function -> FunctionRight[style = invis];
    //        Statement -> Expression;
    static map<string, int> nodeNrs;

    string name = n->to_cstr();
    string label = name ;
    if (n->lexToken()) {
        label += string("\\n(") + n->lexToken()->type_to_cstr() + ")"
               + "\\n[" + string(n->lexToken()->pos, n->lexToken()->len) + "]";
    }
    label = string("\"") + label + "\"";

    if (nodeNrs.find(name) == nodeNrs.end())
        nodeNrs[name] = 0;
    else
        ++nodeNrs[name];
    string nameAndNr = name + std::to_string(nodeNrs[name]);
    dot << "    " << nameAndNr << "[label=" << label << "];" << endl;
    if (!parentName.empty())
        dot << "    " << parentName << "->" << nameAndNr << endl;

    if (n->leftOperand())
        recurseDot(dot, n->leftOperand(), nameAndNr);
    else
        emptyDot(dot, nameAndNr);

    if (n->operat())
        recurseDot(dot, n->operat(), nameAndNr);

    if (n->rightOperand())
        recurseDot(dot, n->rightOperand(), nameAndNr);
    else
        emptyDot(dot, nameAndNr);
}

void Parser::emptyDot(stringstream &dot, string parentName) const
{
    static uint emptyNr = 0;
    string emptyName = parentName + "_empty" + std::to_string(emptyNr++);
    dot << "    " << emptyName << "[label=\"\", shape=plain, style=invis];" << endl
        << "    " << parentName << "->" << emptyName << "[style=invis];" << endl;
}

bool Parser::parseProgram()
{
    _root = new ParseNode(nullptr, nullptr, ParseNode::Program);
    bool res = parseFunction(_root);
    if (!res) {
        delete _root;
        _root = nullptr;
    }
    return res;
}

bool Parser::parseFunction(ParseNode *parent)
{
    bool res = true;
    ParseNode *node = nullptr;

    // <int> <ident> '(' ')' '{' <statement> '}'
    do {
        auto retTypetok = nextTok();
        res = failCheck(retTypetok, LexToken::KwInt);
        if (!res) break;

        auto tok = nextTok();
        res = failCheck(tok, LexToken::Identifier);
        if (!res) break;

        node = new ParseNode(parent, tok, ParseNode::Function);
        parent->setOperat(node);

        auto retval = new ParseNode(node, retTypetok, ParseNode::DataType);
        node->setLeftOper(retval);

        tok = nextTok();
        res = failCheck(tok, LexToken::OpenParen);
        if (!res) break;

        tok = nextTok();
        res = failCheck(tok, LexToken::CloseParen);
        if (!res) break;

        tok = nextTok();
        res = failCheck(tok, LexToken::OpenBrace);
        if (!res) break;

        res = parseStatement(node);

        tok = nextTok();
        res = failCheck(tok, LexToken::CloseBrace);
        if (!res) break;
    } while(0);

    if (!res && node) {
        parent->removeChild(node);
        delete node;
    }

    return res;
}

bool Parser::parseStatement(ParseNode *parent)
{
    bool res = true;
    ParseNode *node = nullptr;

    // <return> <exp> ';'
    do {
        auto tok = peek(0);
        res = failCheck(tok, LexToken::KwReturn);
        if (!res) break;

        node = new ParseNode(parent, tok, ParseNode::Statement);
        parent->setOperat(node);

        res = parseExpression(node);

        tok = nextTok();
        res = failCheck(tok, LexToken::SemiColon);
        if (!res) break;

    } while(0);

    if (!res && node) {
        parent->removeChild(node);
        delete node;
    }

    return res;
}

bool Parser::parseExpression(ParseNode *parent)
{
    bool res = true;
    ParseNode *node = nullptr;

    // <Return> -> <Expression>
    do {
        auto tok = peek(0);
        if (!tok) { res = false; break; }

        node = new ParseNode(parent, tok, ParseNode::Expression);
        parent->setOperat(node);

        res = parseReturn(node);
        if (!res) break;


    } while(0);

    if (!res && node) {
        parent->removeChild(node);
        delete node;
    }

    return res;
}

bool Parser::parseReturn(ParseNode *parent)
{
    bool res = true;
    ParseNode *node = nullptr;

    // < Return >
    do {
        auto tok = nextTok();
        res = failCheck(tok, LexToken::KwReturn);
        if (!res) break;

        node = new ParseNode(parent, tok, ParseNode::Return);
        parent->setOperat(node);

        res = parseConstant(node);

    } while(0);

    if (!res && node) {
        parent->removeChild(node);
        delete node;
    }

    return res;
}

bool Parser::parseConstant(ParseNode *parent)
{
     bool res = true;
     ParseNode *node = nullptr;

     // < IntLitteral | OctalLitteral | BinaryLitteral | HexLitteral | FloatLitteral >
     do {
         auto tok = nextTok();
         res = failCheck(tok, LexToken::IntLitteral, false);
         if (!res)
             res = failCheck(tok, LexToken::OctalLitteral, false);
         if (!res)
             res = failCheck(tok, LexToken::BinaryLitteral, false);
         if (!res)
             res = failCheck(tok, LexToken::HexLitteral, false);
         if (!res)
             res = failCheck(tok, LexToken::FloatLitteral);
         if (!res)
             break;

         node = new ParseNode(parent, tok, ParseNode::Constant);
         parent->setOperat(node);

     } while(0);

     if (!res && node) {
         parent->removeChild(node);
         delete node;
     }

     return res;
}

LexToken *Parser::nextTok()
{
    while (_tokIt != _tokFile->end()) {
        auto tok = &(*(_tokIt++));
        if (tok->type != LexToken::Comment && tok->type != LexToken::NewLine)
            return tok;
    }

    return nullptr;
}

LexToken *Parser::peek(int inc)
{
    auto it = _tokIt;
    while (it + inc != _tokFile->end()) {
        auto tok = &(*(it + inc));
        if (tok->type != LexToken::Comment && tok->type != LexToken::NewLine)
            return tok;
        else
            ++it;
    }

    return nullptr;
}


bool Parser::failCheck(LexToken *tok, LexToken::Tokens type, bool print)
{

    if (tok &&  tok->type == type)
        return true;

    if (print) {
        if (!tok) {
            cerr << "Failed parsing tok was not set from file " << _currentfile <<endl;
            if (_tokIt != _tokFile->end())
                tok = &_tokFile->back();
        }
        if (tok) {
            cerr << "Failed parsing at " << tok->type_to_cstr()
                 << " at line " << _lexer->lineForToken(*tok)
                 << endl;
            _lexer->syntaxError(tok->pos);
        }
    }
    return false;
}

