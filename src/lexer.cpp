#include <cassert>
#include <cctype>
#include <cstring>
#include "lexer.h"
#include "logging.h"
#include "token.h"

static bool isempty(char c) {
    return c == '\t'
        || c == '\n'
        || c == ' '
        || c == '\r'
    ;
}

static bool isalpha_digit(char c) {
    return isalnum(c) || c == '_';
}

bool Lexer::isEnd() {
    return _cur == _end;
}

char Lexer::peek() {
    if (!_buf) {
        DEBUG("Lexer _buf is null");
        return -1;
    }
    if (_cur == _end) {
        return '\0';
    }
    return *_cur;
}

bool Lexer::peekExpect(size_t sz, const char* target) {
    if (_cur + sz > _end) return false;
    return memcmp(_cur, target, sz) == 0;
}

char Lexer::advance() {
    if (!_buf) {
        DEBUG("Lexer _buf is null");
        return -1;
    }
    if (_cur == _end) {
        return '\0';
    }
    char c = *_cur++;
    if (c == '\n') {
        ++_line;
        _col = 1;
    } else {
        ++_col;
    }
    return c;
}

int Lexer::scan(std::vector<Token>* tokseq) {
    Token tok;
    int e = 0;
    while ((e = getNextToken(&tok)) >= 0) {
        if (e > 0) {
            DEBUG("Fail to get next token");
        }
        tokseq->emplace_back(tok);
    }
    DEBUG("Consumed all tokens");
    return 0;
}

#define MAKETOKEN(Type) makeToken(tok, Type, {start_row, start_col}, {_line, _col})
#define MAKETOKENSTR(Type, lexeme, len) makeToken(tok, Type, {start_row, start_col}, {_line, _col}, lexeme, len)
#define MAKETOKENINT(Type, x) makeToken(tok, Type, {start_row, start_col}, {_line, _col}, x)
#define MAKETOKENFLT(Type, x) makeToken(tok, Type, {start_row, start_col}, {_line, _col}, x)
int Lexer::getNextToken(Token* tok) {
    if (!tok) {
        DEBUG("tok is null in getNextToken");
        return 1;
    }
    while (!isEnd() && isempty(peek())) {
        advance();
    }
    if (isEnd()) {
        tok->type = TokenType::ENDOFFILE;
        return -1;
    }
    char c = advance();
    int start_row = _line, start_col = _col-1;
    switch (c)
    {
    case '.':
        if (peek() == '.') {
            advance();
            if (peek() == '.') {
                /* ... */
                advance();
                MAKETOKEN(TokenType::TDOT);
                break;
            }
            /* .. */
            MAKETOKEN(TokenType::DDOT);
            break;
        }
        /* . */
        MAKETOKEN(TokenType::DOT);
        break;
    case '>':
        if (peek() == '>') {
            /* >> */
            advance();
            MAKETOKEN(TokenType::RSHIFT);
            break;
        } else if (peek() == '=') {
            /* >= */
            advance();
            MAKETOKEN(TokenType::GREATEREQ);
            break;
        }
        /* > */
        MAKETOKEN(TokenType::GREATER);
        break;
    case '<':
        if (peek() == '<') {
            /* << */
            advance();
            MAKETOKEN(TokenType::LSHIFT);
            break;
        } else if (peek() == '=') {
            /* <= */
            advance();
            MAKETOKEN(TokenType::LESSEQ);
            break;
        } else if (peek() == '-') {
            /* <- */
            advance();
            MAKETOKEN(TokenType::LARROW);
            break;
        }
        /* < */
        MAKETOKEN(TokenType::LESS);
        break;
    case ':':
        if (peek() == ':') {
            /* :: */
            advance();
            MAKETOKEN(TokenType::DCOLON);
            break;
        } else if (peek() == '=') {
            /* := */
            advance();
            MAKETOKEN(TokenType::COLONEQUAL);
            break;
        }
        /* : */
        MAKETOKEN(TokenType::COLON);
        break;
    case '!':
        if (peek() == '=') {
            /* != */
            advance();
            MAKETOKEN(TokenType::UNEQUAL);
            break;
        }
        /* ! */
        MAKETOKEN(TokenType::EXCLAMATION);
        break;
    case '=':
        if (peek() == '=') {
            /* == */
            advance();
            MAKETOKEN(TokenType::EQEQUAL);
            break;
        }
        /* = */
        MAKETOKEN(TokenType::EQUAL);
        break;
    case '/':
        if (peek() == '/') {
            advance();
            char* start = _cur;
            while (peek() != '\n' && peek() != '\0') {
                advance();
            }
            char* p = _ar->strdup(start, _cur - start);
            MAKETOKENSTR(TokenType::COMMENT, p, _cur - start);
            break;
        } else if (peek() == '*') {
            advance();
            char* start = _cur;
            while (!peekExpect(2, "*/") && peek() != '\0') {
                advance();
            }
            char* p = _ar->strdup(start, _cur - start);
            // consume the next two chars
            advance();
            advance();
            MAKETOKENSTR(TokenType::COMMENT, p, _cur - start);
            break;
        }
        MAKETOKEN(TokenType::SLASH);
    case '&':
        if (peek() == '&') {
            advance();
            MAKETOKEN(TokenType::DAMPER);
            break;
        }
        MAKETOKEN(TokenType::AMPER);
        break;
    case '|':
        if (peek() == '|') {
            advance();
            MAKETOKEN(TokenType::DVBAR);
            break;
        }
        MAKETOKEN(TokenType::VBAR);
        break;
    case '^':
        MAKETOKEN(TokenType::XOR);
    case '#':
        MAKETOKEN(TokenType::SHARP);
        break;
    case '%':
        MAKETOKEN(TokenType::PERCENT);
        break;
    case '\\':
        MAKETOKEN(TokenType::BACKSLASH);
        break;
    case '*':
        MAKETOKEN(TokenType::STAR);
        break;
    case '-':
        if (peek() == '>') {
            /* -> */
            advance();
            MAKETOKEN(TokenType::RARROW);
            break;
        }
        MAKETOKEN(TokenType::MINUS);
        break;
    case '+':
        MAKETOKEN(TokenType::PLUS);
        break;
    case ';':
        MAKETOKEN(TokenType::SEMI);
        break;
    case ',':
        MAKETOKEN(TokenType::COMMA);
        break;
    case '"': {
        size_t decoded_len = 0;
        char* sl = readStringLiteral(decoded_len);
        if (!sl) {
            DEBUG("string literal is not closed");
            return 1;
        }
        MAKETOKENSTR(TokenType::SLiteral, sl, decoded_len);
        break;
    }
    case '\'': {
        int64_t cl = readCharLiteral();
        MAKETOKENINT(TokenType::CLiteral, cl);
        break;
    }
    case '{':
        MAKETOKEN(TokenType::LBRACE);
        break;
    case '}':
        MAKETOKEN(TokenType::RBRACE);
        break;
    case '[':
        MAKETOKEN(TokenType::LSQB);
        break;
    case ']':
        MAKETOKEN(TokenType::RSQB);
        break;
    case '(':
        MAKETOKEN(TokenType::LPAR);
        break;
    case ')':
        MAKETOKEN(TokenType::RPAR);
        break;
    default:
        if (isdigit(c)) { /* Number */
            const char* start = _cur - 1;
            bool is_float = false;

            if (c == '0' && (peek() == 'x' || peek() == 'X')) {
                advance(); // consume x/X
                while (isxdigit(peek())) {
                    advance();
                }
                int64_t val = 0;
                size_t len = _cur - start;
                char* lex = (char*)_ar->alloc(len + 1);
                memcpy(lex, start, len);
                lex[len] = '\0';
                MAKETOKENINT(TokenType::ILiteral, strtoll(lex, nullptr, 16));
                break;
            }

            /* decimal */
            while (isdigit(peek())) {
                advance();
            }
            /* check if is float */
            if (peek() == '.') {
                /* there must be one or more digits following the dot to be a float number */
                if (_cur + 1 < _end && isdigit(*(_cur + 1))) {
                    is_float = true;
                    advance();
                    while (isdigit(peek())) {
                        advance();
                    }
                }
            }
            /* scientific number */
            if (peek() == 'e' || peek() == 'E') {
                is_float = true;
                advance();
                if (peek() == '+' || peek() == '-') {
                    advance();
                }
                if (!isdigit(peek())) {
                    // exponent missing
                    return -1;
                }
                while (isdigit(peek())) {
                    advance();
                }
            }

            size_t len = _cur - start;
            char* lex = _ar->strdup(start, len);

            if (is_float) {
                MAKETOKENFLT(TokenType::FLiteral, strtod(lex, nullptr));
            } else {
                MAKETOKENINT(TokenType::ILiteral, strtoll(lex, nullptr, 10));
            }
            break;
        } else if (isalpha(c) || c == '_') { /* Identifier */
            const char* start = _cur - 1;
            while (isalnum(peek()) || peek() == '_') {
                advance();
            }
            size_t len = _cur - start;

            char* name = _ar->strdup(start, len);

            // 关键字映射表（可用哈希或静态数组，此处简单用 if）
            TokenType tok_type;
            if (false) {}
            else if (strcmp(name, "if") == 0)        tok_type = TokenType::IF;
            else if (strcmp(name, "else") == 0)      tok_type = TokenType::ELSE;
            else if (strcmp(name, "while") == 0)     tok_type = TokenType::WHILE;
            else if (strcmp(name, "for") == 0)       tok_type = TokenType::FOR;
            else if (strcmp(name, "return") == 0)    tok_type = TokenType::RETURN;
            else if (strcmp(name, "struct") == 0)    tok_type = TokenType::STRUCT;
            else if (strcmp(name, "using") == 0)     tok_type = TokenType::USING;
            else if (strcmp(name, "import") == 0)    tok_type = TokenType::IMPORT;
            else if (strcmp(name, "true") == 0)      tok_type = TokenType::TRUE;
            else if (strcmp(name, "false") == 0)     tok_type = TokenType::FALSE;
            else if (strcmp(name, "break") == 0)     tok_type = TokenType::BREAK;
            else if (strcmp(name, "continue") == 0)  tok_type = TokenType::CONTINUE;
            else if (strcmp(name, "cast") == 0)      tok_type = TokenType::CAST;
            else if (strcmp(name, "New") == 0)       tok_type = TokenType::NEW;
            else if (strcmp(name, "null") == 0)      tok_type = TokenType::JNULL;
            else tok_type = TokenType::Identifier;

            MAKETOKENSTR(tok_type, name, len);
            break;
        } else {
            MAKETOKEN(TokenType::INVALID);
            break;
        }
    }
    return 0;
}

static int hexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int encodeUtf8(uint32_t codepoint, unsigned char* dest) {
    if (codepoint < 0x80) {
        dest[0] = (unsigned char)codepoint;
        return 1;
    } else if (codepoint < 0x800) {
        dest[0] = 0xC0 | (codepoint >> 6);
        dest[1] = 0x80 | (codepoint & 0x3F);
        return 2;
    } else if (codepoint < 0x10000) {
        dest[0] = 0xE0 | (codepoint >> 12);
        dest[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        dest[2] = 0x80 | (codepoint & 0x3F);
        return 3;
    } else if (codepoint < 0x110000) {
        dest[0] = 0xF0 | (codepoint >> 18);
        dest[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        dest[2] = 0x80 | ((codepoint >> 6) & 0x3F);
        dest[3] = 0x80 | (codepoint & 0x3F);
        return 4;
    }
    return 0;
}

static int decodeEscape(char*& p, unsigned char* dest) {
    char c = *p++;
    unsigned char buf[4];
    int len = 0;

    switch (c) {
        case 'a':  len = 1; if (dest) dest[0] = '\a'; break;
        case 'b':  len = 1; if (dest) dest[0] = '\b'; break;
        case 'f':  len = 1; if (dest) dest[0] = '\f'; break;
        case 'n':  len = 1; if (dest) dest[0] = '\n'; break;
        case 'r':  len = 1; if (dest) dest[0] = '\r'; break;
        case 't':  len = 1; if (dest) dest[0] = '\t'; break;
        case 'v':  len = 1; if (dest) dest[0] = '\v'; break;
        case '\\': len = 1; if (dest) dest[0] = '\\'; break;
        case '\'': len = 1; if (dest) dest[0] = '\''; break;
        case '\"': len = 1; if (dest) dest[0] = '\"'; break;
        case '?':  len = 1; if (dest) dest[0] = '\?'; break;
        case '0':  // \0 空字符，注意不要和八进制混淆，这里优先处理 \0 后非数字
            // 简单处理：\0 就是空字符，如果后面跟数字可能表示八进制，但标准中 \0 就是一个单独的转义
            len = 1; if (dest) dest[0] = '\0';
            break;

        case 'x': { // 十六进制
            int val = 0;
            int count = 0;
            while (count < 2) {  // 通常最多取2位
                int d = hexDigit(*p);
                if (d == -1) break;
                val = (val << 4) | d;
                p++; count++;
            }
            if (count == 0) {
                // 错误：\x 后没有十六进制数字，我们直接忽略 x
                len = 0;
            } else {
                len = 1;
                if (dest) dest[0] = (unsigned char)val;
            }
            break;
        }

        case 'u': { // Unicode \uHHHH
            int val = 0;
            for (int i = 0; i < 4; ++i) {
                if (*p == '\0') break;  // 意外 EOF
                int d = hexDigit(*p);
                if (d == -1) break;     // 非法字符
                val = (val << 4) | d;
                p++;
            }
            if (val > 0x10FFFF) val = 0xFFFD; // 替换非法码点
            len = encodeUtf8(val, buf);
            if (dest) memcpy(dest, buf, len);
            break;
        }

        case 'U': { // Unicode \UHHHHHHHH
            uint32_t val = 0;
            for (int i = 0; i < 8; ++i) {
                if (*p == '\0') break;
                int d = hexDigit(*p);
                if (d == -1) break;
                val = (val << 4) | d;
                p++;
            }
            if (val > 0x10FFFF) val = 0xFFFD;
            len = encodeUtf8(val, buf);
            if (dest) memcpy(dest, buf, len);
            break;
        }

        case '\n':  // 行继续符（反斜杠后紧跟换行）：忽略换行，产生0个字符
            len = 0;
            break;

        default:
            // 未知转义：保留反斜杠和该字符（或报错）
            len = 2;
            if (dest) {
                dest[0] = '\\';
                dest[1] = c;
            }
            break;
    }
    return len;
}

int Lexer::consumeEscape(char* dest) {
    char c = advance();
    unsigned char buf[4];
    int len = 0;

    switch (c) {
        case 'a':  len = 1; if (dest) *dest = '\a'; break;
        case 'b':  len = 1; if (dest) *dest = '\b'; break;
        case 'f':  len = 1; if (dest) *dest = '\f'; break;
        case 'n':  len = 1; if (dest) *dest = '\n'; break;
        case 'r':  len = 1; if (dest) *dest = '\r'; break;
        case 't':  len = 1; if (dest) *dest = '\t'; break;
        case 'v':  len = 1; if (dest) *dest = '\v'; break;
        case '\\': len = 1; if (dest) *dest = '\\'; break;
        case '\'': len = 1; if (dest) *dest = '\''; break;
        case '"':  len = 1; if (dest) *dest = '"';  break;
        case '?':  len = 1; if (dest) *dest = '\?'; break;
        case '0':  len = 1; if (dest) *dest = '\0'; break;

        case 'x': { // \xHH
            int val = 0;
            for (int i = 0; i < 2; ++i) {
                int d = hexDigit(peek());
                if (d == -1) break;
                val = (val << 4) | d;
                advance();
            }
            len = 1;
            if (dest) *dest = (unsigned char)val;
            break;
        }

        case 'u': { // \uHHHH
            uint32_t val = 0;
            for (int i = 0; i < 4; ++i) {
                int d = hexDigit(peek());
                if (d == -1) break;
                val = (val << 4) | d;
                advance();
            }
            len = encodeUtf8(val, buf);
            if (dest) memcpy(dest, buf, len);
            break;
        }

        case 'U': { // \UHHHHHHHH
            uint32_t val = 0;
            for (int i = 0; i < 8; ++i) {
                int d = hexDigit(peek());
                if (d == -1) break;
                val = (val << 4) | d;
                advance();
            }
            len = encodeUtf8(val, buf);
            if (dest) memcpy(dest, buf, len);
            break;
        }

        case '\n':      // 行继续：反斜杠后跟换行，不产生字符
            len = 0;
            break;
        case '\r':
            if (peek() == '\n') advance(); // 跳过 \r\n
            len = 0;
            break;

        default:
            // 未知转义：保留 \ 和该字符
            len = 2;
            if (dest) {
                dest[0] = '\\';
                dest[1] = c;
            }
            break;
    }
    return len;
}

char* Lexer::readStringLiteral(size_t& decoded_len) {
    char* start = _cur;

    decoded_len = 0;
    char* p = start;
    while (*p != '"' && *p != '\0') {
        if (*p == '\\') {
            ++p;
            decoded_len += decodeEscape(p, nullptr);
        } else {
            ++decoded_len;
            ++p;
        }
    }
    if (*p != '"') {
        DEBUG("No closing quote. " << (start-1));
        return nullptr;
    }

    char* result = (char*)_ar->alloc(decoded_len + 1);

    char* dest = result;
    while (peek() != '"' && peek() != '\0') {
        if (peek() == '\\') {
            advance();
            int len = consumeEscape(dest);
            dest += len;
        } else {
            *dest++ = advance();
        }
    }
    *dest = '\0';

    if (peek() == '"') {
        advance();
    } else {
        return nullptr;
    }

    return result;
}

static bool utf8Decode(const unsigned char* seq, int len, uint32_t& codepoint) {
    if (len < 1 || len > 4) return false;
    codepoint = 0;
    if (len == 1) {
        if (seq[0] > 0x7F) return false;
        codepoint = seq[0];
        return true;
    }
    // 多字节
    if (seq[0] < 0xC0) return false;
    for (int i = 1; i < len; ++i) {
        if ((seq[i] & 0xC0) != 0x80) return false;
    }
    switch (len) {
        case 2:
            codepoint = ((seq[0] & 0x1F) << 6) | (seq[1] & 0x3F);
            if (codepoint < 0x80) return false;
            break;
        case 3:
            codepoint = ((seq[0] & 0x0F) << 12) | ((seq[1] & 0x3F) << 6) | (seq[2] & 0x3F);
            if (codepoint < 0x800) return false;
            break;
        case 4:
            codepoint = ((seq[0] & 0x07) << 18) | ((seq[1] & 0x3F) << 12) | ((seq[2] & 0x3F) << 6) | (seq[3] & 0x3F);
            if (codepoint < 0x10000) return false;
            break;
    }
    return true;
}

int64_t Lexer::readCharLiteral() {
    if (peek() == '\'') return -1;

    unsigned char seq[4];
    int seq_len = 0;

    if (peek() == '\\') {
        advance();
        seq_len = consumeEscape((char*)seq);
        if (seq_len == 0) return -1;
    } else {
        char c = peek();
        int extraBytes = 0;
        if ((c & 0x80) == 0) {
            extraBytes = 0;
        } else if ((c & 0xE0) == 0xC0) {
            extraBytes = 1;
        } else if ((c & 0xF0) == 0xE0) {
            extraBytes = 2;
        } else if ((c & 0xF8) == 0xF0) {
            extraBytes = 3;
        } else {
            return -1;
        }
        seq_len = 1 + extraBytes;
        for (int i = 0; i < seq_len; ++i) {
            if (peek() == '\'' || peek() == '\0') return -1;
            seq[i] = (unsigned char)advance();
        }
    }

    uint32_t codepoint;
    if (!utf8Decode(seq, seq_len, codepoint)) {
        return -1;
    }
    // 码点合法范围检查：排除代理对 U+D800..U+DFFF 及超出 0x10FFFF
    if (codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
        return -1;
    }

    if (peek() != '\'') return -1;
    advance();

    return (int64_t)codepoint;
}