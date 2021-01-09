#ifndef LEXER_H
#define LEXER_H
#include <inttypes.h>
#include <vector>
#include <string>
#include <map>

namespace Cmp {
class Matches;

// create one for each token
class LexToken
{
public:
    enum Tokens { Undefined, NewLine, //Indent, Dedent,  // wait with these
                  Comment, OpenBrace, CloseBrace, OpenParen, CloseParen,
                  OpenBracket, CloseBracket,
                  SemiColon, KwInt, KwReturn, Identifier,
                  // these must be in order 1base, 8base, 10base, 16 base etc
                  BinaryLitteral, OctalLitteral, IntLitteral, HexLitteral, FloatLitteral,
                  SglQteLitteral, DblQteLitteral
                };
    explicit LexToken(Tokens type, const char* pos, size_t len);
    explicit LexToken(); // undefined token
    LexToken(const LexToken &other);
    LexToken &operator=(const LexToken &other);
    bool isValid() const { return type != Undefined; }
    const char *type_to_cstr() const;
    std::string srcStr() const { return std::string(pos, len); }
    const Tokens type;
    const char* pos;
    const size_t len;
};



// ---------------------------------------------------------------------


// creates a token for each
class Lexer
{
    const char *_start, *_curPos, *_acceptedPos;
    bool _breakOnSyntaxError;
public:
    explicit Lexer(bool breakOnSyntaxError = true);
    ~Lexer();

    bool tokenize(const char *srcStr[], const char* filename);

    std::string to_string(const char* filename);

    uint lineForToken(LexToken &tok);

    // filename, tokens
    typedef std::vector<LexToken> T_Tokens;
    std::map<const char*, T_Tokens> files;

    void syntaxError(const char* errPos);
private:
    LexToken matchFunc(const Matches *matches);

    void space();
    LexToken newLine();
    LexToken comment();
    LexToken keyWord(const char *start, const char *end);
    LexToken delimiter();
    LexToken identifierOrKeword();
    LexToken intLitteral();
    LexToken stringLitteral();

    inline const char *curPos() const { return _curPos; };
    inline const char *nextPos() { return ++_curPos;};
    inline const char *peek(int inc = 1) const { return _curPos + inc; }
    bool tryAccept(LexToken &tok);
    bool accept();
    void rewind();
    uint lineAtPos(const char *pos) const;
    T_Tokens tokens;

};


} //namespace Cmp

#endif // LEXER_H
