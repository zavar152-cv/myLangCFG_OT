#include "ot.h"
#include "../tokens.h"
#include <stdint.h>

OperationTreeNode *newOperationTreeNode(const char *label, uint32_t childCount, uint32_t line, uint32_t pos, bool isImaginary) {
  OperationTreeNode *node = (OperationTreeNode *)malloc(sizeof(OperationTreeNode));
  node->label = strdup(label);
  node->childCount = childCount;
  node->children = (OperationTreeNode **)malloc(childCount * sizeof(OperationTreeNode *));
  node->line = line;
  node->pos = pos;
  node->isImaginary = isImaginary;
  return node;
}

void destroyOperationTreeNodeTree(OperationTreeNode *root) {
  if (root == NULL) {
    return;
  }
  for (uint32_t i = 0; i < root->childCount; i++) {
    destroyOperationTreeNodeTree(root->children[i]);
  }
  free(root->children);
  free((void *)root->label);
  free(root);
}

bool isBinaryOp(const char *label) {
  return strcmp(label, PLUS) == 0 |
          strcmp(label, MINUS) == 0 |
          strcmp(label, MUL) == 0 |
          strcmp(label, DIV) == 0;
}

bool isUnaryOp(const char *label) {
  return strcmp(label, NEG) == 0 |
          strcmp(label, NOT) == 0;
}

bool isLiteral(const char *label) {
  return strcmp(label, BOOL) == 0 |
          strcmp(label, STR) == 0 |
          strcmp(label, SYMB) == 0 |
          strcmp(label, HEX) == 0 |
          strcmp(label, BITS) == 0 |
          strcmp(label, DEC) == 0;
}

OperationTreeNode *buildExprOperationTreeFromAstNode(MyAstNode* root, bool isLvalue, bool isFunctionName, OperationTreeErrorContainer *container, const char* filename) {
  if (strcmp(root->label, ASSIGN) == 0) {
    //left - EXPR
    //right - EXPR
    OperationTreeNode *writeOpNode = newOperationTreeNode(WRITE, 2, root->line, root->pos, root->isImaginary);
    OperationTreeNode *lValueExprNode = buildExprOperationTreeFromAstNode(root->children[0], true, false, container, filename);
    OperationTreeNode *rValueExprNode = buildExprOperationTreeFromAstNode(root->children[1], false, false, container, filename);
    writeOpNode->children[0] = lValueExprNode;
    writeOpNode->children[1] = rValueExprNode;
    return writeOpNode;
  } else if (strcmp(root->label, FUNC_CALL) == 0) {
    //if count == 2
    //left - EXPR_LIST
    //right - EXPR

    //if count == 1
    //child - EXPR
    if (root->childCount == 2) {
      OperationTreeNode *funcNameNode = buildExprOperationTreeFromAstNode(root->children[1], false, true, container, filename);
      OperationTreeNode *callNode = newOperationTreeNode(CALL, 1 + root->children[0]->childCount, funcNameNode->line, funcNameNode->pos, funcNameNode->isImaginary);
      callNode->children[0] = funcNameNode;
      for (uint32_t i = 0; i < root->children[0]->childCount; i++) {
        OperationTreeNode *argNode = buildExprOperationTreeFromAstNode(root->children[0]->children[i], false, false, container, filename);
        callNode->children[1 + i] = argNode;
      }
      if (isLvalue) {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer),
                "Assign error. Can't use function calling to assign at %s:%d:%d\n",
                filename, callNode->line,
                callNode->pos + 1);
        if (container->error == NULL) {
          container->error = createOperationTreeErrorInfo(buffer);
        } else {
          addOperationTreeError(container, buffer);
        }
      }
      return callNode;
    } else if (root->childCount == 1) {
      OperationTreeNode *funcNameNode = buildExprOperationTreeFromAstNode(root->children[0], false, true, container, filename);
      OperationTreeNode *callNode = newOperationTreeNode(CALL, 1, funcNameNode->line, funcNameNode->pos, funcNameNode->isImaginary);
      callNode->children[0] = funcNameNode;
      if (isLvalue) {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer),
                "Assign error. Can't use function calling to assign at %s:%d:%d\n",
                filename, callNode->line,
                callNode->pos + 1);
        if (container->error == NULL) {
          container->error = createOperationTreeErrorInfo(buffer);
        } else {
          addOperationTreeError(container, buffer);
        }
      }
      return callNode;
    }
  } else if (strcmp(root->label, INDEXING) == 0) {
    //left - EXPR_LISR
    //right - EXPR
    if (root->childCount == 1) {
      OperationTreeNode *indexNameNode = buildExprOperationTreeFromAstNode(root->children[0], false, true, container, filename);
      char buffer[1024];
      snprintf(buffer, sizeof(buffer),
               "Index error. Missing index value at %s:%d:%d\n",
               filename, indexNameNode->line,
               indexNameNode->pos + 1);
      if (container->error == NULL) {
        container->error = createOperationTreeErrorInfo(buffer);
      } else {
        addOperationTreeError(container, buffer);
      }
      return indexNameNode;
    } else {
      OperationTreeNode *indexNameNode = buildExprOperationTreeFromAstNode(root->children[1], false, true, container, filename);
      OperationTreeNode *indexNode = newOperationTreeNode(INDEX, 1 + root->children[0]->childCount, indexNameNode->line, indexNameNode->pos, indexNameNode->isImaginary);
      indexNode->children[0] = indexNameNode;
      for (uint32_t i = 0; i < root->children[0]->childCount; i++) {
        OperationTreeNode *iNode = buildExprOperationTreeFromAstNode(root->children[0]->children[i], false, false, container, filename);
        indexNode->children[1 + i] = iNode;
      }
      return indexNode;
    }
  } else if (isBinaryOp(root->label)) {
    //left - EXPR
    //right - EXPR 
    if (isLvalue) {
      char buffer[1024];
      snprintf(buffer, sizeof(buffer),
               "Assign error. Can't use binary operation result to assign at %s:%d:%d\n",
               filename, root->line,
               root->pos + 1);
      if (container->error == NULL) {
        container->error = createOperationTreeErrorInfo(buffer);
      } else {
        addOperationTreeError(container, buffer);
      }
    }
    if (isFunctionName) {
      char buffer[1024];
      snprintf(buffer, sizeof(buffer),
              "Call error. Can't use binary operation to call function at %s:%d:%d\n",
              filename, root->line,
              root->pos + 1);
      if (container->error == NULL) {
        container->error = createOperationTreeErrorInfo(buffer);
      } else {
        addOperationTreeError(container, buffer);
      }        
    }
    OperationTreeNode *binaryOpNode = newOperationTreeNode(root->label, 2, root->line, root->pos, root->isImaginary);
    OperationTreeNode *leftExprNode = buildExprOperationTreeFromAstNode(root->children[0], false, false, container, filename);
    OperationTreeNode *rightExprNode = buildExprOperationTreeFromAstNode(root->children[1], false, false, container, filename);
    binaryOpNode->children[0] = leftExprNode;
    binaryOpNode->children[1] = rightExprNode;
    return binaryOpNode;
  } else if (isUnaryOp(root->label)) {
    //child - EXPR 
    if (isLvalue) {
      char buffer[1024];
      snprintf(buffer, sizeof(buffer),
               "Assign error. Can't use unary operation result to assign at %s:%d:%d\n",
               filename, root->line,
               root->pos + 1);
      if (container->error == NULL) {
        container->error = createOperationTreeErrorInfo(buffer);
      } else {
        addOperationTreeError(container, buffer);
      }
    }
    if (isFunctionName) {
      char buffer[1024];
      snprintf(buffer, sizeof(buffer),
              "Call error. Can't use unary operation to call function at %s:%d:%d\n",
              filename, root->line,
              root->pos + 1);
      if (container->error == NULL) {
        container->error = createOperationTreeErrorInfo(buffer);
      } else {
        addOperationTreeError(container, buffer);
      }        
    }
    OperationTreeNode *unaryOpNode = newOperationTreeNode(root->label, 1, root->line, root->pos, root->isImaginary);
    OperationTreeNode *exprNode = buildExprOperationTreeFromAstNode(root->children[0], false, false, container, filename);
    unaryOpNode->children[0] = exprNode;
    return unaryOpNode;
  } else if (strcmp(root->label, IDENTIFIER) == 0) {
    //child - value, terminal
    OperationTreeNode *idValueNode = newOperationTreeNode(root->children[0]->label, 0, root->children[0]->line, root->children[0]->pos, root->children[0]->isImaginary);
    if (isLvalue | isFunctionName) {
      return idValueNode;
    } else {
      OperationTreeNode *readNode = newOperationTreeNode(READ, 1, 0, 0, true);
      readNode->children[0] = idValueNode;
      return readNode;
    }
  } else if (isLiteral(root->label)) {
    //child - value, terminal
    if (isLvalue) {
      char buffer[1024];
      snprintf(buffer, sizeof(buffer),
               "Assign error. Can't use literal to assign at %s:%d:%d\n",
               filename, root->children[0]->line,
               root->children[0]->pos + 1);
      addOperationTreeError(container, buffer);
    }
    if (isFunctionName) {
      char buffer[1024];
      snprintf(buffer, sizeof(buffer),
              "Call error. Can't use literal to call function at %s:%d:%d\n",
              filename, root->children[0]->line,
              root->children[0]->pos + 1);
      if (container->error == NULL) {
        container->error = createOperationTreeErrorInfo(buffer);
      } else {
        addOperationTreeError(container, buffer);
      }        
    }
    OperationTreeNode *literalValueNode = newOperationTreeNode(root->children[0]->label, 0, root->children[0]->line, root->children[0]->pos, root->children[0]->isImaginary);
    OperationTreeNode *literalTypeNode = newOperationTreeNode(root->label, 0, root->line, root->pos, root->isImaginary);
    OperationTreeNode *litReadNode = newOperationTreeNode(LIT_READ, 2, 0, 0, true);
    litReadNode->children[0] = literalTypeNode;
    litReadNode->children[1] = literalValueNode;
    return litReadNode;
  } else {
    return NULL;
  }      
}

void printOperationTreeHelper(OperationTreeNode* node, int level) {
    if (node == NULL) {
        return;
    }
    for (int i = 0; i < level; i++) {
        printf("    ");  // Adjust the number of spaces as needed
    }
    printf("%s (Line: %u, Pos: %u, Imaginary: %s)\n", node->label, node->line, node->pos,
           node->isImaginary ? "Yes" : "No");
    for (uint32_t i = 0; i < node->childCount; i++) {
        printOperationTreeHelper(node->children[i], level + 1);
    }
}

void printOperationTree(OperationTreeNode* root) {
    printOperationTreeHelper(root, 0);
}

OperationTreeErrorInfo* createOperationTreeErrorInfo(const char *message) {
    OperationTreeErrorInfo *errorInfo = (OperationTreeErrorInfo*)malloc(sizeof(OperationTreeErrorInfo));
    errorInfo->message = strdup(message);
    errorInfo->next = NULL;
    return errorInfo;
}

void addOperationTreeError(OperationTreeErrorContainer *container, const char *buffer) {
  if (container->error == NULL) {
    container->error = createOperationTreeErrorInfo(buffer);
  } else {
    OperationTreeErrorInfo *current = container->error;
    while (current->next != NULL) {
      OperationTreeErrorInfo *nextError = current->next;
      current = nextError;
    }
    current->next = createOperationTreeErrorInfo(buffer);
  }
}

void freeOperationTreeErrors(OperationTreeErrorInfo *error) {
  while (error != NULL) {
    OperationTreeErrorInfo *nextError = error->next;
    free(error->message);
    free(error);
    error = nextError;
  }
}