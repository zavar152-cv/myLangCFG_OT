#pragma once

#include <antlr3.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct __attribute__((packed)) MyAstNode {
  struct MyAstNode **children;
  uint32_t childCount;
  const char *label;
  uint32_t line;
  uint32_t pos;
  bool isImaginary;
} MyAstNode;

MyAstNode *newMyAstNode(const char *label, uint32_t childCount, uint32_t line, uint32_t pos, bool isImaginary);

void destroyMyAstNodeTree(MyAstNode *root);

MyAstNode *createMyTreeFromAntlrTree(pANTLR3_BASE_TREE root, uint64_t layer,
                                      bool debug);

const char *postProcessingNodeToken(const char *tokenText);