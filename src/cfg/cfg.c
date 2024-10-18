#include "cfg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

BasicBlock *createBasicBlock(int id, BlockType type, const char *name) {
  BasicBlock *block = (BasicBlock *)malloc(sizeof(BasicBlock));
  block->id = id;
  block->type = type;
  block->instructionCount = 0;
  block->instructionCapacity = INITIAL_CAPACITY;
  block->instructions =
      (Instruction *)malloc(sizeof(Instruction) * block->instructionCapacity);
  block->outEdges = NULL;
  block->next = NULL;
  block->name = strdup(name);
  return block;
}

void addInstruction(BasicBlock *block, const char *text) {
  if (block->instructionCount >= block->instructionCapacity) {
    block->instructionCapacity *= 2;
    block->instructions = (Instruction *)realloc(
        block->instructions, sizeof(Instruction) * block->instructionCapacity);
  }
  block->instructions[block->instructionCount].text = strdup(text);
  block->instructionCount++;
}

void addEdge(BasicBlock *from, BasicBlock *to, EdgeType type,
             const char *condition) {
  Edge *edge = (Edge *)malloc(sizeof(Edge));
  edge->type = type;
  if (condition != NULL) {
    edge->condition = strdup(condition);
  } else {
    edge->condition = NULL;
  }
  edge->targetBlock = to;
  edge->next = from->outEdges;
  from->outEdges = edge;
}

void addBasicBlock(CFG *cfg, BasicBlock *block) {
  block->next = cfg->blocks;
  cfg->blocks = block;
}

CFG *createCFG() {
  CFG *cfg = (CFG *)malloc(sizeof(CFG));
  cfg->entryBlock = NULL;
  cfg->blocks = NULL;
  return cfg;
}

void printCFG(CFG *cfg) {
  BasicBlock *block = cfg->blocks;
  while (block != NULL) {
    printf("Базовый блок %d, %s (%s):\n", block->id, block->name,
           (block->type == CONDITIONAL)     ? "условный"
           : (block->type == UNCONDITIONAL) ? "безусловный"
                                            : "терминальный");
    for (int i = 0; i < block->instructionCount; i++) {
      printf("  Инструкция %d: %s\n", i, block->instructions[i].text);
    }
    Edge *edge = block->outEdges;
    while (edge != NULL) {
      printf("  Переход к блоку %d", edge->targetBlock->id);
      switch (edge->type) {
      case TRUE_CONDITION:
        printf(" если (%s) истинно", edge->condition);
        break;
      case FALSE_CONDITION:
        printf(" если (%s) ложно", edge->condition);
        break;
      case UNCONDITIONAL_JUMP:
        printf(" безусловный переход");
        break;
      }
      printf("\n");
      edge = edge->next;
    }
    printf("\n");
    block = block->next;
  }
}

void freeInstructions(BasicBlock *block) {
  for (int i = 0; i < block->instructionCount; i++) {
    free(block->instructions[i].text);
  }
  free(block->instructions);
}

void freeEdges(Edge *edge) {
  while (edge != NULL) {
    Edge *nextEdge = edge->next;
    if (edge->condition != NULL) {
      free(edge->condition);
    }
    free(edge);
    edge = nextEdge;
  }
}

void freeBasicBlocks(BasicBlock *block) {
  while (block != NULL) {
    BasicBlock *nextBlock = block->next;
    freeInstructions(block);
    freeEdges(block->outEdges);
    free(block->name);
    free(block);
    block = nextBlock;
  }
}

void freeCFG(CFG *cfg) {
  freeBasicBlocks(cfg->blocks);
  free(cfg);
}

TypeInfo* createTypeInfo(const char *typeName) {
    TypeInfo *typeInfo = (TypeInfo*)malloc(sizeof(TypeInfo));
    typeInfo->typeName = strdup(typeName);
    return typeInfo;
}

void freeTypeInfo(TypeInfo *typeInfo) {
    if (typeInfo != NULL) {
        if (typeInfo->typeName != NULL) {
            free(typeInfo->typeName);
        }
        free(typeInfo);
    }
}

ArgumentInfo* createArgumentInfo(TypeInfo *type, const char *name) {
    ArgumentInfo *argInfo = (ArgumentInfo*)malloc(sizeof(ArgumentInfo));
    argInfo->type = type;
    argInfo->name = strdup(name);
    argInfo->next = NULL;
    return argInfo;
}

void addArgument(FunctionInfo *funcInfo, ArgumentInfo *argInfo) {
    argInfo->next = funcInfo->arguments;
    funcInfo->arguments = argInfo;
}

void freeArguments(ArgumentInfo *arg) {
    while (arg != NULL) {
        ArgumentInfo *nextArg = arg->next;
        if (arg->type != NULL) {
            freeTypeInfo(arg->type);
        }
        if (arg->name != NULL) {
            free(arg->name);
        }
        free(arg);
        arg = nextArg;
    }
}

FunctionInfo* createFunctionInfo(const char *fileName, const char *functionName, TypeInfo *returnType) {
    FunctionInfo *funcInfo = (FunctionInfo*)malloc(sizeof(FunctionInfo));
    funcInfo->fileName = strdup(fileName);
    funcInfo->functionName = strdup(functionName);
    funcInfo->returnType = returnType;
    funcInfo->arguments = NULL;
    funcInfo->cfg = NULL;
    funcInfo->next = NULL;
    return funcInfo;
}

void freeFunctionInfo(FunctionInfo *funcInfo) {
    if (funcInfo != NULL) {
        if (funcInfo->fileName != NULL) {
            free(funcInfo->fileName);
        }
        if (funcInfo->functionName != NULL) {
            free(funcInfo->functionName);
        }
        if (funcInfo->returnType != NULL) {
            freeTypeInfo(funcInfo->returnType);
        }
        if (funcInfo->arguments != NULL) {
            freeArguments(funcInfo->arguments);
        }
        if (funcInfo->cfg != NULL) {
            freeCFG(funcInfo->cfg);
        }
        free(funcInfo);
    }
}

void printFunctionInfo(FunctionInfo *funcInfo) {
    printf("Файл: %s\n", funcInfo->fileName);
    printf("Функция: %s\n", funcInfo->functionName);
    printf("Тип возвращаемого значения: %s\n", funcInfo->returnType->typeName);
    printf("Аргументы:\n");
    ArgumentInfo *arg = funcInfo->arguments;
    while (arg != NULL) {
        printf("  %s %s\n", arg->type->typeName, arg->name);
        arg = arg->next;
    }
    printf("\n");
    
    printCFG(funcInfo->cfg);
}