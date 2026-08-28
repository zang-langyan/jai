#ifndef JAI_TOKEN_H
#define JAI_TOKEN_H
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

enum TokenType {
    Identifier,
    ILiteral,              /* integer constant */
    FLiteral,              /* float constant */
    CLiteral,              /* char constant (it is stored as an int value) */
    SLiteral,              /* string constant */

    /* Keywords */
    /* Reserved */
    IF,                  /* if */
    ELSE,                /* else*/
    WHILE,               /* while */
    FOR,                 /* for */
    RETURN,              /* return */
    STRUCT,              /* struct */
    IMPORT,              /* import */
    USING,               /* using */
    TRUE,                /* true */
    FALSE,               /* false */
    BREAK,               /* break */
    CONTINUE,            /* continue */
    CAST,                /* cast */
    NEW,                 /* New */

    // ...

    /* One char */
    LPAR,                  /* ( */
    RPAR,                  /* ) */
    LSQB,                  /* [ */
    RSQB,                  /* ] */
    LBRACE,                /* { */
    RBRACE,                /* } */
    SQUO,                  /* ' */
    DQUO,                  /* " */
    EXCLAMATION,           /* ! */
    COLON,                 /* : */
    COMMA,                 /* , */
    SEMI,                  /* ; */
    EQUAL,                 /* = */
    PLUS,                  /* + */
    MINUS,                 /* - */
    STAR,                  /* * */
    SLASH,                 /* / */
    BACKSLASH,             /* \ */
    LESS,                  /* < */
    GREATER,               /* > */
    DOT,                   /* . */
    PERCENT,               /* % */
    SHARP,                 /* # */
    AMPER,                 /* & */
    VBAR,                  /* | */
    XOR,                   /* ^ */
    
    /* Two char */
    DCOLON,                /* :: */
    EQEQUAL,               /* == */
    UNEQUAL,               /* != */
    COLONEQUAL,            /* := */
    LESSEQ,                /* <= */
    GREATEREQ,             /* >= */
    LSHIFT,                /* << */
    RSHIFT,                /* >> */
    DDOT,                  /* .. */
    DAMPER,                /* && */
    DVBAR,                 /* || */
    RARROW,                /* -> */
    LARROW,                /* <- */

    
    /* Three char */
    TDOT,                  /* ... */
    
    /* Others */
    JNULL,                 /* null */
    COMMENT,
    NEWLINE,               /* New Line*/
    ENDOFFILE,             /* End of File */
    INVALID
};

struct Token {
    TokenType type;
    bool ignore;
    union {
        char* lexeme;
        int64_t intValue;        // IntegerLiteral
        double floatValue;       // FloatLiteral
    } data;
    size_t length;
    struct {
        int sr, sc; // start row & column
        int er, ec; // end row & column
    } pos; /* [start, end) */
};

int makeToken(Token* tok, TokenType typ, std::pair<int, int> start, std::pair<int, int> end);
int makeToken(Token* tok, TokenType typ, std::pair<int, int> start, std::pair<int, int> end, char* p, size_t len);
int makeToken(Token* tok, TokenType typ, std::pair<int, int> start, std::pair<int, int> end, int64_t il);
int makeToken(Token* tok, TokenType typ, std::pair<int, int> start, std::pair<int, int> end, double d);

std::string escapeString(const char* s, size_t len);
void dumpTokens(const std::vector<Token>& tokseq);
void dumpTokens(const std::vector<Token*>& tokseq);
#endif