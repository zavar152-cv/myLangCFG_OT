#include "dotUtils/dotUtils.h"
#include <antlr3.h>
#include <antlr3defs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DotNode *newDotNode(uint64_t id, const char *label, uint32_t childCount) {
  DotNode *node = (DotNode *)malloc(sizeof(DotNode));
  node->id = id;
  node->label = strdup(label);
  node->childCount = childCount;
  node->children = (DotNode **)malloc(childCount * sizeof(DotNode *));
  return node;
}

void destroyDotNodeTree(DotNode *root) {
  if (root == NULL) {
    return;
  }
  for (uint32_t i = 0; i < root->childCount; i++) {
    destroyDotNodeTree(root->children[i]);
  }
  free(root->children);
  free((void *)root->label);
  free(root);
}

char *removeQuotes(const char *str) {
  int length = strlen(str);
  if (length > 1 && str[0] == '"' && str[length - 1] == '"') {
    str++;
    length -= 2;
  }
  return strndup(str, length);
}

DotNode *createDotTreeFromAntlrTree(pANTLR3_BASE_TREE root, uint64_t layer,
                                    uint64_t *id, bool debug) {
  if (root == NULL) {
    return NULL;
  }

  uint64_t currentId = (*id)++;

  if (debug) {
    for (uint32_t i = 0; i < layer; i++) {
      printf("  ");
    }
    pANTLR3_BASE_TREE parent = root->getParent(root);
    char *tokenText = removeQuotes((const char *)root->getToken(root)
                                       ->getText(root->getToken(root))
                                       ->chars);
    const char *nodeName = postProcessingNodeToken(tokenText);
    printf("node %s_%lu [label=\"%s\"]", nodeName, currentId, tokenText);
    printf("\n");
    free(tokenText);
  }

  pANTLR3_UINT8 tokenText =
      root->getToken(root)->getText(root->getToken(root))->chars;

  char *label = removeQuotes((const char *)tokenText);

  DotNode *newNode =
      newDotNode(currentId, (const char *)label, root->getChildCount(root));

  free(label);

  for (uint32_t i = 0; i < root->getChildCount(root); i++) {
    newNode->children[i] = createDotTreeFromAntlrTree(
        (pANTLR3_BASE_TREE)root->getChild(root, i), layer + 1, id, debug);
  }

  return newNode;
}

DotNode *createDotTreeFromMyTree(MyAstNode *root, uint64_t layer, uint64_t *id,
                                 bool debug) {
  if (root == NULL) {
    return NULL;
  }

  uint64_t currentId = (*id)++;

  if (debug) {
    for (uint32_t i = 0; i < layer; i++) {
      printf("  ");
    }
    char *tokenText = removeQuotes(root->label);
    const char *nodeName = postProcessingNodeToken(tokenText);
    printf("node %s_%lu [label=\"%s\"]", nodeName, currentId, tokenText);
    printf("\n");
    free(tokenText);
  }

  char *label = removeQuotes(root->label);

  DotNode *newNode =
      newDotNode(currentId, (const char *)label, root->childCount);

  free(label);

  for (uint32_t i = 0; i < root->childCount; i++) {
    newNode->children[i] = createDotTreeFromMyTree(root->children[i], layer + 1, id, debug);
  }

  return newNode;
}

void writeTreeToDot(FILE *file, DotNode *root) {
  if (root == NULL) {
    return;
  }

  fprintf(file, "    node_%lu [label=\"%s\"]\n", root->id, root->label);

  for (uint32_t i = 0; i < root->childCount; i++) {
    DotNode *child = root->children[i];
    fprintf(file, "    node_%lu -> node_%lu;\n", root->id, child->id);
    writeTreeToDot(file, child);
  }
}

int generateDotFile(DotNode *root, const char *filename) {
  FILE *file = fopen(filename, "w");
  if (file == NULL) {
    return FILE_ERROR;
  }

  fprintf(file, "digraph Tree {\n");
  fprintf(file, "    node [shape=hexagon];\n");

  writeTreeToDot(file, root);

  fprintf(file, "}\n");
  fclose(file);
  return 0;
}

int generateDotFileFromAntlrTree(pANTLR3_BASE_TREE tree, const char *filename,
                                 bool debug) {
  DotNode *dotTree = NULL;
  uint64_t id = 0;

  if (debug) {
    printf("%s\n", tree->toStringTree(tree)->chars);
    printf("%u\n", tree->getChildCount(tree));
  }

  dotTree = createDotTreeFromAntlrTree(tree, 0, &id, debug);
  int err = generateDotFile(dotTree, filename);

  destroyDotNodeTree(dotTree);
  return err;
}

int generateDotFileFromMyTree(MyAstNode *tree, const char *filename,
                              bool debug) {
  DotNode *dotTree = NULL;
  uint64_t id = 0;

  dotTree = createDotTreeFromMyTree(tree, 0, &id, debug);
  int err = generateDotFile(dotTree, filename);

  destroyDotNodeTree(dotTree);
  return err;
}
