#include "lexer.h"
#include "parser.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        fputs("provide a input file\n",stderr);
        exit(1);
    }

    FILE *fp;
    fp = fopen ( argv[1] , "rb" );
    
    Parser p;
    Arena lexer_ar;
    Arena token_ar;
    Arena ast_ar;
    if (0 != p.initialize(fp, CompilerType::File, &lexer_ar, &token_ar, &ast_ar)) {
        ERROR("Fail to initialize Parse. Exit Jai Compiler.");
        return -1;
    }

    ASTNode* ast = p.parse();
    if (ast) {
        INFO("DUMPING AST");
        TRACE('\n' << ast->dump());
    } else {
        ERROR("ast is null");
    }
    return 0;
}