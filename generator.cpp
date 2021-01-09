#include "generator.h"
#include <string>
#include <sstream>
#include <iostream>
#include <cstdlib>

using namespace Cmp;
using namespace std;


Generator::Generator(Parser *parser, Lexer *lex)
    : _parser(parser)
    , _lexer(lex)
    , _epilogCalled(false)
{ }

Generator::~Generator()
{ }

std::string Generator::generate(ParseNode *root)
{
    _res.clear();
    _currentNode = root;
    if (root)
        visit();

    return _res.str();
}

void Generator::visit()
{
    do {

        switch(_currentNode->kind()) {
        case ParseNode::Program:
            programStart();
            break;
        case ParseNode::Function:
            functionNode();  // calls this visit function recursively
            break;
        case ParseNode::Statement:
            statementNode();
            break;
        case ParseNode::Return:
            returnNode();
            return ; //
        case ParseNode::Expression:
            continue;
        case ParseNode::Constant:
            constantInt();
            break;
        default:
            cerr << "Unhandled ParseNode kind, sould never end up here. its a bug" << endl;
            abort();
        }
    } while (next());
}

void Generator::programStart()
{
    _res << "# assembler created by c-compiler by fredrikjohansson,"
        << "example from norasandler let's build a c-compiler\n\n"
        //<< "    .section\n " //__TEXT,__text_startup,regular,pure_instructions\n"
        << "    .align 4\n"
        << "    .text\n"
        << "    .globl  main\n"
        << "    .type   main, @function\n";
}

void Generator::functionNode()
{
    functionProlog();
    next();
    visit(); // recurse
    if (!_epilogCalled)
        functionEpilog();
}

void Generator::functionProlog()
{
    _epilogCalled = false;
    _res << _currentNode->lexToken()->srcStr() << ":\n"
        << "    # preamble\n"
        << "    push %ebp\n"
        << "    movl %esp, %ebp\n"
        << "    # end preamble\n";
}

void Generator::functionEpilog()
{
    _epilogCalled = true;
    _res << "    #epilog\n"
         << "    movl %ebp, %esp\n"
         << "    popl %ebp\n"
         << "    ret\n";
}

void Generator::statementNode()
{
}

void Generator::returnNode()
{
    next();
    visit();
    _res << "    # return\n"
         << "    popl %eax\n";
    functionEpilog();
}

void Generator::constantInt()
{
    _res << "    # expressionNode\n";
    switch (_currentNode->lexToken()->type) {
    case LexToken::IntLitteral:
        _res << "    pushl $" << std::stoi(_currentNode->lexToken()->srcStr(), nullptr, 10) << "\n";
        break;
    case LexToken::OctalLitteral:
        _res << "    push $" << std::stoi(_currentNode->lexToken()->srcStr(), nullptr, 8) << "\n";
        break;
    case LexToken::BinaryLitteral:
        _res << "    push $" << std::stoi(_currentNode->lexToken()->srcStr(), nullptr, 2) << "\n";
        break;
    case LexToken::HexLitteral:
        _res << "    push $" << std::stoi(_currentNode->lexToken()->srcStr(), nullptr, 16) << "\n";
        break;
    default:
        cout << "Error ParseNode LexToken->type not handled\n";
    }
}

bool Generator::next()
{
    return _currentNode && (_currentNode = _currentNode->operat());
}
