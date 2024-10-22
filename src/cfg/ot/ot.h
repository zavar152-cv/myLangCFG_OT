#pragma once

#include "grammar/ast/myAst.h"
#include <stdbool.h>
#include <stdint.h>

#define LIT_READ "litRead"
#define READ "read"
#define WRITE "write"
#define CALL "call"
#define INDEX "index"

typedef struct OperationTreeNode {
  struct OperationTreeNode **children;
  uint32_t childCount;
  const char *label;
  uint32_t line;
  uint32_t pos;
  bool isImaginary;
} OperationTreeNode;

typedef struct __attribute__((packed)) OperationTreeErrorInfo {
    char *message;
    struct OperationTreeErrorInfo *next;
} OperationTreeErrorInfo;

typedef struct OperationTreeErrorContainer {
    struct OperationTreeErrorInfo *error;
} OperationTreeErrorContainer;

OperationTreeNode *newOperationTreeNode(const char *label, uint32_t childCount, uint32_t line, uint32_t pos, bool isImaginary);

void destroyOperationTreeNodeTree(OperationTreeNode *root);

OperationTreeNode *buildExprOperationTreeFromAstNode(MyAstNode* root, bool isLvalue, bool isFunctionName, OperationTreeErrorContainer *error, const char* filename);

void printOperationTree(OperationTreeNode *root);

OperationTreeErrorInfo* createOperationTreeErrorInfo(const char *message);

void addOperationTreeError(OperationTreeErrorContainer *container, const char *message);

void freeOperationTreeErrors(OperationTreeErrorInfo *error);