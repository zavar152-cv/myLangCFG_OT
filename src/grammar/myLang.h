#include "ast/myAst.h"
#include "errorsUtils/errorUtils.h"
#include <stdbool.h>

typedef struct MyLangResult {
    MyAstNode *tree;
    ErrorContext errorContext;
    bool isValid;
} MyLangResult;

void parseMyLangFromFile(MyLangResult *result, char *filename, bool debug);

void parseMyLangFromText(MyLangResult *result, const char *text, bool debug);

void destroyMyLangResult(MyLangResult *result);