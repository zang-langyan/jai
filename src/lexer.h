#ifndef JAI_LEXER_H
#define JAI_LEXER_H
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "arena.h"
#include "token.h"
#include "logging.h"

class Lexer {
    /* these pointers are not owned by Lexer
     * except for _buf.
     * _buf copies all external data
     */
    FILE* _f;
    char* _buf;
    size_t _buf_size;
    char* _cur;
    char* _end;
    int _line, _col;
    Arena* _ar;
public:
    Lexer():
        _f(nullptr), _ar(nullptr),
        _buf(nullptr), _buf_size(0),
        _cur(nullptr), _end(nullptr),
        _line(1), _col(1)
    {}
    ~Lexer() { if (_buf) delete[] _buf; };

    void setfile(FILE* f) {
        _f = f;
        long lSize;
        char *buffer;

        fseek( _f , 0L , SEEK_END);
        lSize = ftell( _f );
        rewind( _f );

        /* allocate memory for entire content */
        _buf = (char*)calloc( 1, lSize+1 );
        size_t bytesRead = fread(_buf, 1, lSize, _f); // 用元素大小为1，返回实际字节数
        _end = _buf + bytesRead;
        _cur = _buf;

        _line = 1;
        _col = 1;
        fclose(_f);
    }

    void setsource(const char* src, size_t n) {
        if (_buf) {
            delete[] _buf;
            _buf = nullptr;
        }
        _buf = new char[n + 1];
        memcpy(_buf, src, n);
        _buf[n] = '\0';
        _cur = _buf;
        _end = _buf + n;
        _line = 1;
        _col = 1;
    }

    int setArena(Arena* a) {
        if (!a) {
            ERROR("provided a null arena\n");
            return -1;
        }
        _ar = a;
        return 0;
    }

    int scan(std::vector<Token>* tokseq);
    int getNextToken(Token* tok);
private:
    char peek();
    bool peekExpect(size_t, const char*);
    char advance();
    bool isEnd();

private:
    char* readStringLiteral(size_t& decoded_len);
    int64_t readCharLiteral();
    int consumeEscape(char* dest);
};



#endif