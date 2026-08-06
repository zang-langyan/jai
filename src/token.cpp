#include <unordered_set>
#include "token.h"
#include "logging.h"
#include "tabulate.h"


int makeToken(Token* tok, TokenType typ, std::pair<int, int> start, std::pair<int, int> end) {
    if (!tok) {
        DEBUG("tok is nullptr");
        return 1;
    }
    tok->type = typ;
    tok->ignore = (typ == TokenType::COMMENT) || (typ == TokenType::NEWLINE);
    tok->pos.sr = start.first;
    tok->pos.sc = start.second;
    tok->pos.er = end.first;
    tok->pos.ec = end.second;
    return 0;
}

int makeToken(Token* tok, TokenType typ, std::pair<int, int> start, std::pair<int, int> end, char* p, size_t len) {
    if (!tok) {
        DEBUG("tok is nullptr");
        return 1;
    }
    tok->type = typ;
    tok->ignore = (typ == TokenType::COMMENT) || (typ == TokenType::NEWLINE);
    tok->data.lexeme = p;
    tok->length = len;
    tok->pos.sr = start.first;
    tok->pos.sc = start.second;
    tok->pos.er = end.first;
    tok->pos.ec = end.second;
    return 0;
}

int makeToken(Token* tok, TokenType typ, std::pair<int, int> start, std::pair<int, int> end, int64_t il) {
    if (!tok) {
        DEBUG("tok is nullptr");
        return 1;
    }
    tok->type = typ;
    tok->ignore = (typ == TokenType::COMMENT) || (typ == TokenType::NEWLINE);
    tok->data.intValue = il;
    tok->pos.sr = start.first;
    tok->pos.sc = start.second;
    tok->pos.er = end.first;
    tok->pos.ec = end.second;
    return 0;
}

int makeToken(Token* tok, TokenType typ, std::pair<int, int> start, std::pair<int, int> end, double d) {
    if (!tok) {
        DEBUG("tok is nullptr");
        return 1;
    }
    tok->type = typ;
    tok->ignore = (typ == TokenType::COMMENT) || (typ == TokenType::NEWLINE);
    tok->data.floatValue = d;
    tok->pos.sr = start.first;
    tok->pos.sc = start.second;
    tok->pos.er = end.first;
    tok->pos.ec = end.second;
    return 0;
}

static std::unordered_set<TokenType> lexeme_set {
    COMMENT, SLiteral, Identifier
};

static std::unordered_set<TokenType> int_set {
    ILiteral, CLiteral
};

static std::unordered_map<TokenType, std::string> distok = {
    {Identifier,  "Identifier"  },
    {ILiteral,    "ILiteral"    },
    {FLiteral,    "FLiteral"    },
    {CLiteral,    "CLiteral"    },
    {SLiteral,    "SLiteral"    },
    {IF,          "IF"          },
    {ELSE,        "ELSE"        },
    {WHILE,       "WHILE"       },
    {FOR,         "FOR"         },
    {RETURN,      "RETURN"      },
    {STRUCT,      "STRUCT"      },
    {USING,       "USING"       },
    {IMPORT,      "IMPORT"      },
    {TRUE,        "TRUE"        },
    {FALSE,       "FALSE"       },
    {LPAR,        "LPAR"        },
    {RPAR,        "RPAR"        },
    {LSQB,        "LSQB"        },
    {RSQB,        "RSQB"        },
    {LBRACE,      "LBRACE"      },
    {RBRACE,      "RBRACE"      },
    {SQUO,        "SQUO"        },
    {DQUO,        "DQUO"        },
    {EXCLAMATION, "EXCLAMATION" },
    {COLON,       "COLON"       },
    {COMMA,       "COMMA"       },
    {SEMI,        "SEMI"        },
    {EQUAL,       "EQUAL"       },
    {PLUS,        "PLUS"        },
    {MINUS,       "MINUS"       },
    {STAR,        "STAR"        },
    {SLASH,       "SLASH"       },
    {BACKSLASH,   "BACKSLASH"   },
    {LESS,        "LESS"        },
    {GREATER,     "GREATER"     },
    {DOT,         "DOT"         },
    {PERCENT,     "PERCENT"     },
    {SHARP,       "SHARP"       },
    {AMPER,       "AMPER"       },
    {VBAR,        "VBAR"        },
    {XOR,         "XOR"         },
    {DAMPER,      "DAMPER"      },
    {DVBAR,       "DVBAR"       },
    {DCOLON,      "DCOLON"      },
    {RARROW,      "RARROW"      },
    {LARROW,      "LARROW"      },
    {EQEQUAL,     "EQEQUAL"     },
    {UNEQUAL,     "UNEQUAL"     },
    {COLONEQUAL,  "COLONEQUAL"  },
    {LESSEQ,      "LESSEQ"      },
    {GREATEREQ,   "GREATEREQ"   },
    {LSHIFT,      "LSHIFT"      },
    {RSHIFT,      "RSHIFT"      },
    {DDOT,        "DDOT"        },
    {TDOT,        "TDOT"        },
    {COMMENT,     "COMMENT"     },
    {NEWLINE,     "NEWLINE"     },
    {ENDOFFILE,   "ENDOFFILE"   },
    {BREAK,       "BREAK"       },
    {CONTINUE,    "CONTINUE"    },
    {CAST,        "CAST"        },
    {NEW,         "NEW"         },
    {INVALID,     "INVALID"     }
};

void dumpTokens(const std::vector<Token>& tokseq) {
    Tabulate table;
    table.setHeaders(
        {"ID", "Token Type", "data", "ignore", "start", "end"}, 
        {Align::Left, Align::Left, Align::Left, Align::Left, Align::Left, Align::Left}
    );

#define POS(r,c) ("(" + std::to_string(r) + "," + std::to_string(c) + ")")
    for (int i = 0; i < tokseq.size(); ++i) {
        auto& t = tokseq[i];
        if (lexeme_set.find(t.type) != lexeme_set.end()) {
            table.addRow(
                i,
                distok[t.type],
                escapeString(t.data.lexeme, t.length),
                t.ignore ? "True" : "False",
                POS(t.pos.sr, t.pos.sc),
                POS(t.pos.er, t.pos.ec)
            );
        } else if (int_set.find(t.type) != int_set.end()) {
            table.addRow(
                i,
                distok[t.type],
                t.data.intValue,
                t.ignore ? "True" : "False",
                POS(t.pos.sr, t.pos.sc),
                POS(t.pos.er, t.pos.ec)
            );
        } else if (t.type == FLiteral) {
            table.addRow(
                i,
                distok[t.type],
                t.data.floatValue,
                t.ignore ? "True" : "False",
                POS(t.pos.sr, t.pos.sc),
                POS(t.pos.er, t.pos.ec)
            );
        } else {
            table.addRow(
                i,
                distok[t.type],
                "",
                t.ignore ? "True" : "False",
                POS(t.pos.sr, t.pos.sc),
                POS(t.pos.er, t.pos.ec)
            );
        }
    }
    std::cout << table.to_string();
}

void dumpTokens(const std::vector<Token*>& tokseq) {
    Tabulate table;
    table.setHeaders(
        {"ID", "Token Type", "data", "ignore", "start", "end"}, 
        {Align::Left, Align::Left, Align::Left, Align::Left, Align::Left, Align::Left}
    );

#define POS(r,c) ("(" + std::to_string(r) + "," + std::to_string(c) + ")")
    for (int i = 0; i < tokseq.size(); ++i) {
        auto& t = tokseq[i];
        if (lexeme_set.find(t->type) != lexeme_set.end()) {
            table.addRow(
                i,
                distok[t->type],
                escapeString(t->data.lexeme, t->length),
                t->ignore ? "True" : "False",
                POS(t->pos.sr, t->pos.sc),
                POS(t->pos.er, t->pos.ec)
            );
        } else if (int_set.find(t->type) != int_set.end()) {
            table.addRow(
                i,
                distok[t->type],
                t->data.intValue,
                t->ignore ? "True" : "False",
                POS(t->pos.sr, t->pos.sc),
                POS(t->pos.er, t->pos.ec)
            );
        } else if (t->type == FLiteral) {
            table.addRow(
                i,
                distok[t->type],
                t->data.floatValue,
                t->ignore ? "True" : "False",
                POS(t->pos.sr, t->pos.sc),
                POS(t->pos.er, t->pos.ec)
            );
        } else {
            table.addRow(
                i,
                distok[t->type],
                "",
                t->ignore ? "True" : "False",
                POS(t->pos.sr, t->pos.sc),
                POS(t->pos.er, t->pos.ec)
            );
        }
    }
    std::cout << table.to_string();
}

std::string escapeString(const char* s, size_t len) {
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = s[i];
        switch (c) {
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            default:
                if (c < 0x20 || c == 0x7F) {
                    char buf[5];
                    snprintf(buf, sizeof(buf), "\\x%02X", c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}