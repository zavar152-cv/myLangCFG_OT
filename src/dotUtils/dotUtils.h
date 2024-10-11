#include <stdint.h>
#include <antlr3.h>
#include <stdbool.h>
#include "grammar/ast/myAst.h"

#define FILE_ERROR -1

typedef struct __attribute__((packed)) DotNode {
    struct DotNode** children;
    uint64_t id;
    uint32_t childCount;
    const char* label;
} DotNode;

DotNode* newDotNode(uint64_t id, const char* label, uint32_t childCount);

void destroyDotNodeTree(DotNode* root);

DotNode* createDotTreeFromAntlrTree(pANTLR3_BASE_TREE root, uint64_t layer, uint64_t *id, bool debug);

DotNode* createDotTreeFromMyTree(MyAstNode *root, uint64_t layer, uint64_t *id, bool debug);

void writeTreeToDot(FILE *file, DotNode* root);

int generateDotFile(DotNode* root, const char *filename);

int generateDotFileFromAntlrTree(pANTLR3_BASE_TREE tree, const char *filename, bool debug);

int generateDotFileFromMyTree(MyAstNode *tree, const char *filename, bool debug);