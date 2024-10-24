#pragma once

#include "grammar/ast/myAst.h"
#include <stdbool.h>
#include <stdint.h>

#define LIT_READ "litRead"
#define READ "read"
#define WRITE "write"
#define OT_CALL "call"
#define INDEX "index"
#define DECLARE "declare"
#define SEQ_DECLARE "seqDeclare"
#define WITH_TYPE "withType"
#define CUSTOM "custom"
#define BUILTIN "builtin"
#define OT_ARRAY "array"
#define RETURN "return"
#define OT_BREAK "break"

typedef struct OperationTreeNode {
  struct OperationTreeNode **children;
  uint32_t childCount;
  const char *label;
  uint32_t line;
  uint32_t pos;
  bool isImaginary;
} OperationTreeNode;

typedef struct TypeInfo TypeInfo;

typedef struct TypeInfo {
    char *typeName;
    bool custom;
    bool isArray;
    uint32_t arrayDim;
    uint32_t line;
    uint32_t pos;
    TypeInfo *next;
} TypeInfo;

typedef struct __attribute__((packed)) OperationTreeErrorInfo {
    char *message;
    struct OperationTreeErrorInfo *next;
} OperationTreeErrorInfo;

typedef struct OperationTreeErrorContainer {
    struct OperationTreeErrorInfo *error;
} OperationTreeErrorContainer;

OperationTreeNode *newOperationTreeNode(const char *label, uint32_t childCount, uint32_t line, uint32_t pos, bool isImaginary);

OperationTreeNode *buildVarOperationTreeFromAstNode(MyAstNode* root, OperationTreeErrorContainer *container, TypeInfo* varType, const char* filename);

TypeInfo* parseTyperef(MyAstNode* typeRef);

void destroyOperationTreeNodeTree(OperationTreeNode *root);

OperationTreeNode *buildExprOperationTreeFromAstNode(MyAstNode* root, bool isLvalue, bool isFunctionName, OperationTreeErrorContainer *error, const char* filename);

void printOperationTree(OperationTreeNode *root);

OperationTreeErrorInfo* createOperationTreeErrorInfo(const char *message);

void addOperationTreeError(OperationTreeErrorContainer *container, const char *message);

void freeOperationTreeErrors(OperationTreeErrorInfo *error);

bool isBinaryOp(const char *label);

bool isUnaryOp(const char *label);

bool isLiteral(const char *label);