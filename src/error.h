class Error {
public:
    Error() = default;
    virtual void dump() = 0;
};

class LexerError: public Error {
    LexerError() = default;
    
};