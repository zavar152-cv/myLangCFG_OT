#include "cfg.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "grammar/myLang.h"

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

TypeInfo* parseTyperef(MyAstNode* typeRef) {
  assert(typeRef->childCount == 1 || typeRef->childCount == 2);

  if (typeRef->childCount == 1) {
    MyAstNode* type = typeRef->children[0];
    return createTypeInfo(type->children[0]->label, strcmp(type->label, CUSTOM_TYPE) == 0, false, 0, type->children[0]->line, type->children[0]->pos);
  } else if (typeRef->childCount == 2) {
    MyAstNode* type = typeRef->children[0];
    assert(strcmp(typeRef->children[1]->label, ARRAY) == 0);
    uint32_t dim = typeRef->children[1]->childCount == 1 ? typeRef->children[1]->children[0]->childCount : 1;
    return createTypeInfo(type->children[0]->label, strcmp(type->label, CUSTOM_TYPE) == 0, true, dim, type->children[0]->line, type->children[0]->pos);
  }
}

void parseArgdefList(MyAstNode* argdefList, FunctionInfo* info) {
  if (argdefList->childCount == 0) {
    return;
  } else {
    for (uint32_t i = 0; i < argdefList->childCount; i++) {
      assert(strcmp(argdefList->children[i]->children[0]->label, TYPEREF) == 0);
      assert(strcmp(argdefList->children[i]->children[1]->label, IDENTIFIER) == 0);
      TypeInfo* argType = parseTyperef(argdefList->children[i]->children[0]);
      ArgumentInfo* arg = createArgumentInfo(argType, argdefList->children[i]->children[1]->children[0]->label, 
      argdefList->children[i]->children[1]->children[0]->line, argdefList->children[i]->children[1]->children[0]->pos);
      addArgument(info, arg);
    }
  }
}

Program *buildProgram(FilesToAnalyze *files, bool debug) {
  Program *program = (Program *)malloc(sizeof(Program));
  program->functions = NULL;
  program->errors = NULL;

  for (uint32_t i = 0; i < files->filesCount; i++) {
    MyLangResult* result = files->result[i];
    MyAstNode** funcDefs = result->tree->children;
    uint32_t childCount = result->tree->childCount;
    for (uint32_t j = 0; j < childCount; j++) {
      MyAstNode* funcSignature = funcDefs[j]->children[0];
      assert(strcmp(funcSignature->label, FUNC_SIGNATURE) == 0);

      MyAstNode* typeRef = NULL;
      MyAstNode* name = NULL;
      MyAstNode* argdefList = NULL;

      assert(funcSignature->childCount == 3 || funcSignature->childCount == 2);

      if (funcSignature->childCount == 2) {
        name = funcSignature->children[0];
        argdefList = funcSignature->children[1];
        assert(strcmp(name->label, NAME) == 0);
        assert(strcmp(argdefList->label, ARGDEF_LIST) == 0);
      } else if (funcSignature->childCount == 3) {
        typeRef = funcSignature->children[0];
        name = funcSignature->children[1];
        argdefList = funcSignature->children[2];
        assert(strcmp(typeRef->label, TYPEREF) == 0);
        assert(strcmp(name->label, NAME) == 0);
        assert(strcmp(argdefList->label, ARGDEF_LIST) == 0);
      }

      TypeInfo* returnType;
      if (typeRef == NULL) {
        returnType = createTypeInfo("void", false, false, 0, name->line, name->pos);
      } else {
        returnType = parseTyperef(typeRef);
      }

      FunctionInfo* info = createFunctionInfo(files->fileName[i], name->children[0]->label, returnType, name->children[0]->line, name->children[0]->pos);
      parseArgdefList(argdefList, info);

      FunctionInfo *func = program->functions;
      while (func != NULL) {
        FunctionInfo *nextFunc = func->next;
        if (strcmp(func->functionName, info->functionName) == 0) {
          char buffer[1024];

          snprintf(buffer, sizeof(buffer),
            "Redeclaration error. Function %s at %d:%d in file %s was previously declared at %d:%d in file %s\n",
            info->functionName, info->line, info->pos, info->fileName,
            func->line, func->pos, func->fileName);
          ProgramErrorInfo* error = createProgramErrorInfo(buffer);
          addProgramError(program, error);
          break;
        }
        func = nextFunc;
      }

      addFunctionToProgram(program, info);
    }
  }

  if (debug) {
    FunctionInfo *func = program->functions;
    while (func != NULL) {
      printFunctionInfo(func);
      func = func->next;
    }
  }

  return program;
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

TypeInfo *createTypeInfo(const char *typeName, bool custom, bool isArray, uint32_t arrayDim, uint32_t line, uint32_t pos) {
  TypeInfo *typeInfo = (TypeInfo *)malloc(sizeof(TypeInfo));
  typeInfo->typeName = strdup(typeName);
  typeInfo->custom = custom;
  typeInfo->isArray = isArray;
  typeInfo->arrayDim = arrayDim;
  typeInfo->line = line;
  typeInfo->pos = pos;
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

ArgumentInfo *createArgumentInfo(TypeInfo *type, const char *name, uint32_t line, uint32_t pos) {
  ArgumentInfo *argInfo = (ArgumentInfo *)malloc(sizeof(ArgumentInfo));
  argInfo->type = type;
  argInfo->name = strdup(name);
  argInfo->next = NULL;
  argInfo->line = line;
  argInfo->pos = pos;
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

FunctionInfo *createFunctionInfo(const char *fileName, const char *functionName,
                                 TypeInfo *returnType, uint32_t line, uint32_t pos) {
  FunctionInfo *funcInfo = (FunctionInfo *)malloc(sizeof(FunctionInfo));
  funcInfo->fileName = strdup(fileName);
  funcInfo->functionName = strdup(functionName);
  funcInfo->returnType = returnType;
  funcInfo->arguments = NULL;
  funcInfo->cfg = NULL;
  funcInfo->next = NULL;
  funcInfo->line = line;
  funcInfo->pos = pos;
  return funcInfo;
}

void addFunctionToProgram(Program *program, FunctionInfo *funcInfo) {
  funcInfo->next = program->functions;
  program->functions = funcInfo;
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

void freeProgram(Program *program) {
  FunctionInfo *func = program->functions;
  while (func != NULL) {
    FunctionInfo *nextFunc = func->next;
    freeFunctionInfo(func);
    func = nextFunc;
  }
  freeProgramErrors(program->errors);
  free(program);
}

ProgramErrorInfo* createProgramErrorInfo(const char *message) {
    ProgramErrorInfo *errorInfo = (ProgramErrorInfo*)malloc(sizeof(ProgramErrorInfo));
    errorInfo->message = strdup(message);
    errorInfo->next = NULL;
    return errorInfo;
}

void addProgramError(Program *program, ProgramErrorInfo *errorInfo) {
    errorInfo->next = program->errors;
    program->errors = errorInfo;
}

void freeProgramErrors(ProgramErrorInfo *error) {
    while (error != NULL) {
        ProgramErrorInfo *nextError = error->next;
        free(error->message);
        free(error);
        error = nextError;
    }
}

void printFunctionInfo(FunctionInfo *funcInfo) {
  printf("File: %s\n", funcInfo->fileName);
  printf("Function: %s\n", funcInfo->functionName);
  printf("Return type: %s", funcInfo->returnType->typeName);
  if (funcInfo->returnType->custom)
    printf(", custom type");
  if (funcInfo->returnType->isArray)
    printf(", array with dim %d", funcInfo->returnType->arrayDim);
  printf("\n");
  printf("Arguments:\n");
  ArgumentInfo *arg = funcInfo->arguments;
  while (arg != NULL) {
    printf("  %s %s", arg->type->typeName, arg->name);
    if (arg->type->custom)
      printf(", custom type");
    if (arg->type->isArray)
      printf(", array with dim %d", arg->type->arrayDim);
    printf("\n");
    arg = arg->next;
  }
  printf("\n");

  if (funcInfo->cfg != NULL)
    printCFG(funcInfo->cfg);
}