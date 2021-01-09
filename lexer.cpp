#include "lexer.h"
#include <cstring>
#include <cassert>
#include <sstream>
#include <iostream>

using namespace Cmp;
using namespace std;


// --------------------------------------------------------------------

namespace Cmp {

class Match {
public:
    explicit Match(const char* str, LexToken ::Tokens type)
        : str(str)
        , len(strlen(str))
        , type(type)
    {}
    const char *str;
    const size_t len;
    const LexToken::Tokens type;
};


class Matches {
public:
    Matches(const Match *matches, size_t sz)
        : m(matches)
        , sz(sz)
        , minLen(10000)
        , maxLen(0)
    {
        // previously calculate min max str length
        for (size_t i = 0; i < sz; ++i) {
            size_t len = strnlen((matches+i)->str, 1024);
            if (len < minLen) minLen = len;
            if (len > maxLen) maxLen = len;
        }
    }

    // set cnt to how many matches there are for str, when we cnt == 1 match should be the correct one
    const Match* matchCnt(const char* str, size_t strlen, uint &cnt) const
    {
        const Match *match = nullptr;
        for (size_t i = 0; i < sz; ++i) {
            if (m[i].len >= strlen && (strncmp(str, m[i].str, strlen) == 0)) {
                match = &m[i];
                ++cnt;
            }
        }
        if (cnt > 1)
            return nullptr;
        return match;
    }
/*
    // match char by char, broken thought....
    const Match *match(const char c, size_t pos) const
    {
        if (pos > maxLen)
            return nullptr;
        for (size_t i = 0; i < sz; ++i) {
            if (m[i].len >= pos && m[i].str[pos] == c)
                return &m[i];
        }
        return nullptr;
    }
*/
    const Match *m;
    const size_t sz;
    size_t minLen, maxLen; // minimu len and maximum length of match
};

} // namespace Cmp



// static to this file
// keyword statements, must be longest str first descending order
static const Match _kws[] = {
    Match("return", LexToken::KwReturn),
    Match("int", LexToken::KwInt)
};
static const Match _delims[] = {
    Match("{", LexToken::OpenBrace), Match("}", LexToken::CloseBrace),
    Match("[", LexToken::OpenBracket), Match("]", LexToken::CloseBracket),
    Match("(", LexToken::OpenParen), Match(")", LexToken::CloseParen),
    Match(";", LexToken::SemiColon)
};

static Matches delims (&_delims[0], sizeof(_delims) / sizeof (_delims[0]));
static Matches kws (&_kws[0], sizeof (_kws) / sizeof (_kws[0]));


// -------------------------------------------------------------------------

LexToken::LexToken(LexToken::Tokens type, const char *pos, size_t len)
    : type(type)
    , pos(pos)
    , len(len)
{ }

LexToken::LexToken()
    : type(Undefined)
    , pos(nullptr)
    , len(0)
{ }

LexToken::LexToken(const LexToken &other)
    : type(other.type)
    , pos(other.pos)
    , len(other.len)
{ }

LexToken &LexToken::operator=(const LexToken &other)
{
    *const_cast<Tokens*>(&type) = other.type;
    *const_cast<char**>(&pos) = *const_cast<char**>(&other.pos);
    *const_cast<size_t*>(&len) = other.len;
    return *this;
}

const char* LexToken::type_to_cstr() const
{
    switch(type) {
    case Undefined: return "Undefined";
    case NewLine: return "NewLine";
    case Comment: return "Comment";
    case OpenBrace: return "OpenBrace";
    case CloseBrace: return "CloseBrace";
    case OpenParen: return "OpenParen";
    case CloseParen: return "CloseParen";
    case OpenBracket: return "OpenBracket";
    case CloseBracket: return "CloseBracket";
    case SemiColon: return "SemiColon";
    case KwInt: return "KwInt";
    case KwReturn: return "KwReturn";
    case Identifier: return "Identifier";
    case BinaryLitteral: return "BinaryLitteral";
    case OctalLitteral: return "OctalLittral";
    case IntLitteral: return "IntLitteral";
    case HexLitteral: return "HexLitteral";
    case FloatLitteral: return "FloatLitteral";
    case SglQteLitteral: return "SglQteLitteral";
    case DblQteLitteral: return "DblQuoteLitteral";
    }
    return nullptr;
}

// -----------------------------------------------------------------------------------------
Lexer::Lexer(bool breakOnSyntaxError)
    : _start(nullptr)
    , _curPos(nullptr)
    , _acceptedPos(nullptr)
    , _breakOnSyntaxError(breakOnSyntaxError)
{ }

Lexer::~Lexer()
{ }

bool Lexer::tokenize(const char *srcStr[], const char *filename)
{
    auto fileIt = files.find(filename);
    if (fileIt != files.end())
        files.erase(fileIt);

    tokens.clear();

    _start = _curPos = _acceptedPos = *srcStr;

    const char *lastIterPos = nullptr;
    bool res = true;

    for (;*_curPos != 0;) {
        lastIterPos = _acceptedPos;

        // first newline check must be done before whitespace
        LexToken tok = newLine();
        if (tryAccept(tok))
            continue;

        space(); // eat up all whitespace

        tok = comment();
        if (tryAccept(tok))
            continue; // might have newlines and spaces after it

        tok = delimiter();
        if (tryAccept(tok))
            continue;

        //tok = keyWord();
        //if (tryAccept(tok))
        //    continue;

        tok = stringLitteral();
        if (tryAccept(tok))
            continue;

        tok = intLitteral();
        if (tryAccept(tok))
            continue;

        tok = identifierOrKeword();
        if (tryAccept(tok))
            continue;

        if (lastIterPos >= _acceptedPos) {
            res = false;
            syntaxError(curPos()); // print err msg
            if (!_breakOnSyntaxError)
                for(char c = *nextPos(); c != 0 && c != '\n'; c = *nextPos())
                    ;
            else
                break; // we can't do this anymore
        }
    }

    // store this file in our filemap
    files[filename] = tokens;

    return res;
}

string Lexer::to_string(const char *filename)
{
    stringstream ret;
    if (files.find(filename) == files.end()) {
        ret << "Could not find " << filename << " among tokinized files" << endl;

    } else if(files[filename].size()) {
        T_Tokens toks = files[filename];
        const char *linestart = toks[0].pos;
        ret << 1 << ":";
        uint prevLinePos = 0;
        for (auto tok : toks) {
            if (tok.type == LexToken::NewLine) {
                ret << tok.type_to_cstr() << endl << lineAtPos(tok.pos) +1 << ":";
                linestart = tok.pos +1;
                prevLinePos = 0;
                continue;
            }

            for (const char *p = linestart + prevLinePos; p < tok.pos; ++p) {
                ret << "-";
                prevLinePos++;
            }

            ret << tok.type_to_cstr() << " ";

        }
    } else
        ret << "No tokens in "<<filename<<" for this\n";

    return ret.str();
}

uint Lexer::lineForToken(LexToken &tok)
{
    return lineAtPos(tok.pos);
}

void Lexer::syntaxError(const char* errPos)
{
    const char *start = errPos,
               *linePos = _start;
    // find out linenr and pos in line
    uint line = 0, pos = 0;
    const char *cp = _start;
    for (; *cp != 0 && cp < start; ++cp) {
        if (*cp == '\n') {
            ++line;
            linePos = cp+1;
        }
    }

    // pos in line
    pos = static_cast<uint>(cp - linePos);

    stringstream str;

    str << "Syntax Error on line: " << line
        << " at pos: " << pos << endl;
    // insert the line
    for (const char *lp = linePos; *lp != 0 && *lp != '\n'; ++lp)
        str << *lp;

    str << endl;

    // goto to point
    for (uint i = 0; i < pos; ++i)
        str << "-";

    str << "^" << endl;

    cerr << str.str();
}


LexToken Lexer::matchFunc(const Matches *matches)
{
    const char *start = curPos();

    size_t len = 1;
    for (char c = *start; c != 0; c = *nextPos(), ++len) {
        if (len >= matches->minLen) {
            if (len > matches->maxLen)
                break;
            uint cnt = 0;
            const Match *m = matches->matchCnt(start, len, cnt);
            if (cnt < 1)
                break;
            else if (cnt == 1)
                return LexToken(m->type, start, m->len);

        }
    }
    return LexToken(); // undef
}

LexToken Lexer::newLine()
{
    const char* start = curPos();
    if (*start == '\n' && *peek(-1) != '\\')
        return LexToken(LexToken::NewLine, start, 1);
    return LexToken();
}

void Lexer::space()
{
    const char *start = curPos();

    for (const char *cp = start; isspace(*cp);++cp)
        nextPos();

    if (start < curPos())
        accept();

    return;
}


LexToken Lexer::comment()
{
    const char *start = curPos();
    char c = *start;
    if (c != '/' || (*peek() != '*' && *peek() != '/')) {
        return LexToken();
    }
    const char close = *nextPos();

    // should be in a comment now
    while ((c = *nextPos()) != 0) {
        if (close == '/' && c == '\n')
            return LexToken(LexToken::Comment, start, static_cast<uint>(curPos() - start));
        if (c == '*' && close == '*' && '/' == *peek())
            return LexToken(LexToken::Comment, start, static_cast<uint>(curPos() - start +2));
    }

    return LexToken();
}

LexToken Lexer::keyWord(const char*start, const char *end)
{
    uint cnt = 0;
    auto m = kws.matchCnt(start, static_cast<size_t>(end - start), cnt);
    if (cnt == 1 && m->len)
        return LexToken(m->type, start, static_cast<size_t>(end - start));
    return LexToken();
}

LexToken Lexer::delimiter()
{
    return matchFunc(&delims);
}

LexToken Lexer::identifierOrKeword()
{
    // this is rather trcicky, return0 is not a kwyword, must check entire string before deciding
    const char* start = curPos();
    if (!isalpha(*start))
        return LexToken();

    for (char c = *nextPos(); c != 0; c = *nextPos()) {
        if (!isalpha(c) && !isdigit(c)) {
            auto kwTok = keyWord(start, curPos());
            if (kwTok.isValid())
                return kwTok;
            return LexToken(LexToken::Identifier, start, static_cast<uint>(curPos() - start));
        }
    }
    return LexToken();
}

LexToken Lexer::intLitteral()
{
    const char* start = curPos();
    char c = *start;
    LexToken::Tokens type = LexToken::IntLitteral;
    if (c == 'b')
        type = LexToken::BinaryLitteral;
    else if (c == '0') {
        c = *nextPos();
        if (c == 'x')
            type = LexToken::HexLitteral;
        else if (c < '8')
            type = LexToken::OctalLitteral;
    } else if (!isalnum(c))
        return LexToken();

    for(;c != 0 && !isspace(c); c = *nextPos()) {
        if (!isalnum(c) && c != '.' && c != '_')
            break;

        switch (type) {
        case LexToken::BinaryLitteral:
            if (c > '1') return LexToken();
            break;
        case LexToken::OctalLitteral:
            if (c == '.') type = LexToken::FloatLitteral;
            else if (c > '7') return LexToken();
            break;
        case LexToken::IntLitteral:
            if (c == '.') type = LexToken::FloatLitteral;
            else if (c > '9') return LexToken();
            break;
        case LexToken::HexLitteral:
            if (c > '9' && !((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')))
                return LexToken();
            break;
        case LexToken::FloatLitteral:
            if ((c != '.' && c != '_') && (c < '0' || c > '9'))
                return LexToken();
            break;
        default: ; // pass
        }
    }

    if (start < curPos())
        return LexToken(type, start, static_cast<uint>(curPos() - start));
    return LexToken();
}

LexToken Lexer::stringLitteral()
{
    const char *start = nextPos();
    char c = *start, close = *start;
    LexToken::Tokens type = LexToken::Undefined;
    if (c == '\'')
        type = LexToken::SglQteLitteral;
    else if (c == '"')
        type = LexToken::DblQteLitteral;
    else
        return LexToken();

    for (;c != 0 && c != '\n'; c = *nextPos()) {
        if (c == close && !('\\' == *peek(-1)))
            return LexToken(type, start, static_cast<uint>(curPos() - start));
    }

    return LexToken();
}

uint Lexer::lineAtPos(const char* pos) const
{
    uint line = 1;
    for (const char *cp = _start; *cp != 0 && cp != pos; ++cp)
        if (*cp == '\n')
            line++;
    return line;
}

bool Lexer::tryAccept(LexToken &tok)
{
    if (!tok.isValid()) {
        rewind();
        return false;
    }
    tokens.push_back(tok);

    _curPos = tok.pos + tok.len;
    accept();
    return true;
}

bool Lexer::accept()
{
    assert(_curPos > _acceptedPos && "Accept did not advance");
    _acceptedPos = _curPos;
    return true;
}

void Lexer::rewind()
{
   _curPos = _acceptedPos;
}
