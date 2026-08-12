classes = [
    'Module',
    'Interactive',
    'Name',
    'Op',
    'Literal',
    'VariableDecl',
    'ConstantDecl',
    'FuncDecl',
    'StructDecl',
    'UsingDecl',
    'CompoundStmts',
    'SingleStmt',
    'BlockStmt',
    'ImportStmt',
    'IfStmt',
    'WhileStmt',
    'ForStmt',
    'ReturnStmt',
    'DeferStmt',
    'ExprStmt',
    'BreakStmt',
    'ContinueStmt',
    'CompileStmt',
    'BinaryExpr',
    'UnaryExpr',
    'CallExpr',
    'IndexExpr',
    'MemberAccessExpr',
    'IdentifierExpr',
    'LiteralExpr',
    'CastExpr',
    'ArrayLiteralExpr',
    'NewExpr',
    'LambdaExpr',
    'IfExpr',
    'RunExpr',
    'NamedType',
    'PointerType',
    'ArrayType',
    'FunctionType',
]

return_type = 'int'
method = 'visit_impl'
params = 'SymTable& symtable'
with open('src/ast-impl.cpp', 'w') as f:
    f.write('''#include "ast.h"

''')
    for name in classes:
        f.write(
f'''{return_type} {name}::{method}({params}) ''' +
r'''{
    return 0;
}''' + '\n\n'
        )
    