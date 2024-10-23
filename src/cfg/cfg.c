#include "cfg.h"
#include "ot/ot.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "grammar/myLang.h"

BasicBlock *parseBlock(MyAstNode* block, Program *program, const char* filename, bool isLoop, BasicBlock* prevBlock, BasicBlock* existingBlock, BasicBlock* loopBlock, CFG *cfg, uint32_t *uid);

BasicBlock *createBasicBlock(int id, BlockType type, const char *name) {
  BasicBlock *block = (BasicBlock *)malloc(sizeof(BasicBlock));
  block->id = id;
  block->type = type;
  block->instructionCount = 0;
  block->instructionCapacity = INITIAL_CAPACITY;
  block->instructions =
      (Instruction *)malloc(sizeof(Instruction) * block->instructionCapacity);
  block->outEdges = NULL;
  block->inEdges = NULL;
  block->next = NULL;
  block->name = strdup(name);
  block->isEmpty = false;
  block->isBreak = false;
  return block;
}

BasicBlock *createEmptyBasicBlock(int id, BlockType type, const char *name) {
  BasicBlock *block = createBasicBlock(id, type, name);
  block->isEmpty = true;
  return block;
}

void addInstruction(BasicBlock *block, const char *text, OperationTreeNode *otRoot) {
  if (block->instructionCount >= block->instructionCapacity) {
    block->instructionCapacity *= 2;
    block->instructions = (Instruction *)realloc(
        block->instructions, sizeof(Instruction) * block->instructionCapacity);
  }
  block->instructions[block->instructionCount].text = strdup(text);
  block->instructions[block->instructionCount].otRoot = otRoot;
  block->instructionCount++;

  if (block->isEmpty) {
    block->isEmpty = false;
  }

}

void addEdge(BasicBlock *from, BasicBlock *to, EdgeType type,
             const char *condition) {
  if (!from->isBreak) {
    Edge *edge = (Edge *)malloc(sizeof(Edge));
    edge->type = type;
    if (condition != NULL) {
      edge->condition = strdup(condition);
    } else {
      edge->condition = NULL;
    }
    edge->fromBlock = from;
    edge->targetBlock = to;

    edge->nextOut = from->outEdges;
    from->outEdges = edge;

    edge->nextIn = to->inEdges;
    to->inEdges = edge;
  }
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

void parseVar(MyAstNode* var, BasicBlock *currentBlock, Program *program, const char* filename) {
  OperationTreeErrorContainer *errorContainer = (OperationTreeErrorContainer*)malloc(sizeof(OperationTreeErrorContainer));
  errorContainer->error = NULL;
  TypeInfo *typeInfo = parseTyperef(var->children[0]);
  OperationTreeNode *otNode = buildVarOperationTreeFromAstNode(var, errorContainer, typeInfo, filename);
  addInstruction(currentBlock, var->label, otNode);
  freeTypeInfo(typeInfo);

  OperationTreeErrorInfo *errorInfo = errorContainer->error;
  while (errorInfo != NULL) {
    ProgramErrorInfo* error = createProgramErrorInfo(errorInfo->message);
    addProgramError(program, error);
    errorInfo = errorInfo->next;
  }
  freeOperationTreeErrors(errorContainer->error);
  free(errorContainer);
}

void parseExpr(MyAstNode* expr, BasicBlock *currentBlock, Program *program, const char* filename) {
  assert(strcmp(expr->label, EXPR) == 0);
  OperationTreeErrorContainer *errorContainer = (OperationTreeErrorContainer*)malloc(sizeof(OperationTreeErrorContainer));
  errorContainer->error = NULL;
  OperationTreeNode *otNode = buildExprOperationTreeFromAstNode(expr->children[0], false, false, errorContainer, filename);
  addInstruction(currentBlock, expr->children[0]->label, otNode);

  OperationTreeErrorInfo *errorInfo = errorContainer->error;
  while (errorInfo != NULL) {
    ProgramErrorInfo* error = createProgramErrorInfo(errorInfo->message);
    addProgramError(program, error);
    errorInfo = errorInfo->next;
  }
  freeOperationTreeErrors(errorContainer->error);
  free(errorContainer);
}

BasicBlock* parseDoWhile(MyAstNode* doWhileBlock, Program *program, const char* filename, BasicBlock* prevBlock, BasicBlock* existingBlock, CFG *cfg, uint32_t *uid) {
  assert(strcmp(doWhileBlock->label, DO_WHILE) == 0);
  BasicBlock *bodyBlock;
  if (existingBlock == NULL) {
    bodyBlock = createBasicBlock(++(*uid), CONDITIONAL, "Do While body");
    addBasicBlock(cfg, bodyBlock);
    addEdge(prevBlock, bodyBlock, UNCONDITIONAL_JUMP, NULL);
  } else {
    bodyBlock = existingBlock;
    free(bodyBlock->name);
    bodyBlock->name = strdup("Do While body");
  }

  BasicBlock *emptyBlock = createEmptyBasicBlock(++(*uid), UNCONDITIONAL, "Empty block");
  addBasicBlock(cfg, emptyBlock);

  BasicBlock *conditionBlock = createBasicBlock(++(*uid), CONDITIONAL, "Do While Condition");
  addBasicBlock(cfg, conditionBlock);

  OperationTreeErrorContainer *errorContainer = (OperationTreeErrorContainer*)malloc(sizeof(OperationTreeErrorContainer));
  errorContainer->error = NULL;
  OperationTreeNode *otNode = buildExprOperationTreeFromAstNode(doWhileBlock->children[1]->children[0], false, false, errorContainer, filename);
  addInstruction(conditionBlock, doWhileBlock->children[1]->label, otNode);

  OperationTreeErrorInfo *errorInfo = errorContainer->error;
  while (errorInfo != NULL) {
    ProgramErrorInfo* error = createProgramErrorInfo(errorInfo->message);
    addProgramError(program, error);
    errorInfo = errorInfo->next;
  }
  freeOperationTreeErrors(errorContainer->error);
  free(errorContainer);

  addEdge(conditionBlock, bodyBlock, TRUE_CONDITION, NULL);   // TODO
  addEdge(conditionBlock, emptyBlock, FALSE_CONDITION, NULL); // TODO

  BasicBlock *bodyExitBlock = parseBlock(doWhileBlock->children[0], program, filename, true, conditionBlock, bodyBlock, emptyBlock, cfg, uid);

  addEdge(bodyExitBlock, conditionBlock, UNCONDITIONAL_JUMP, NULL);
  return emptyBlock;
}

BasicBlock* parseWhile(MyAstNode* whileBlock, Program *program, const char* filename, BasicBlock* prevBlock, BasicBlock* existingBlock, CFG *cfg, uint32_t *uid) {
    assert(strcmp(whileBlock->label, WHILE) == 0);

    BasicBlock *conditionBlock;            
    if (existingBlock == NULL) {
      conditionBlock = createBasicBlock(++(*uid), CONDITIONAL, "While Condition");
      addBasicBlock(cfg, conditionBlock);
      addEdge(prevBlock, conditionBlock, UNCONDITIONAL_JUMP, NULL);
    } else {
      conditionBlock = existingBlock;
      free(conditionBlock->name);
      conditionBlock->name = strdup("While Condition");
    }


    BasicBlock *emptyBlock = createEmptyBasicBlock(++(*uid), UNCONDITIONAL, "Empty block");
    addBasicBlock(cfg, emptyBlock);

    OperationTreeErrorContainer *errorContainer = (OperationTreeErrorContainer*)malloc(sizeof(OperationTreeErrorContainer));
    errorContainer->error = NULL;
    OperationTreeNode *otNode = buildExprOperationTreeFromAstNode(whileBlock->children[0]->children[0], false, false, errorContainer, filename);
    addInstruction(conditionBlock, whileBlock->children[0]->label, otNode);

    OperationTreeErrorInfo *errorInfo = errorContainer->error;
    while (errorInfo != NULL) {
      ProgramErrorInfo* error = createProgramErrorInfo(errorInfo->message);
      addProgramError(program, error);
      errorInfo = errorInfo->next;
    }
    freeOperationTreeErrors(errorContainer->error);
    free(errorContainer);

    BasicBlock *bodyBlock = createBasicBlock(++(*uid), UNCONDITIONAL, "While Body");
    addBasicBlock(cfg, bodyBlock);

    addEdge(conditionBlock, bodyBlock, TRUE_CONDITION, NULL); //TODO
    addEdge(conditionBlock, emptyBlock, FALSE_CONDITION, NULL); //TODO

    BasicBlock *bodyExitBlock = parseBlock(whileBlock->children[1], program, filename, true, conditionBlock, bodyBlock, emptyBlock, cfg, uid);

    addEdge(bodyExitBlock, conditionBlock, UNCONDITIONAL_JUMP, NULL);
    return emptyBlock;
}

BasicBlock *parseIf(MyAstNode* ifBlock, Program *program, const char* filename, bool isLoop, BasicBlock* prevBlock, BasicBlock* existingBlock, BasicBlock* loopExitBlock, CFG *cfg, uint32_t *uid) {
    assert(strcmp(ifBlock->label, IF) == 0);

    BasicBlock *conditionBlock;
    if (existingBlock == NULL) {
      conditionBlock = createBasicBlock(++(*uid), CONDITIONAL, "If Condition");
      addBasicBlock(cfg, conditionBlock);
      addEdge(prevBlock, conditionBlock, UNCONDITIONAL_JUMP, NULL);
    } else {
      conditionBlock = existingBlock;
      free(conditionBlock->name);
      conditionBlock->name = strdup("If Condition");
    }

    BasicBlock *emptyBlock = createEmptyBasicBlock(++(*uid), UNCONDITIONAL, "Empty block");
    addBasicBlock(cfg, emptyBlock);

    OperationTreeErrorContainer *errorContainer = (OperationTreeErrorContainer*)malloc(sizeof(OperationTreeErrorContainer));
    errorContainer->error = NULL;
    OperationTreeNode *otNode = buildExprOperationTreeFromAstNode(ifBlock->children[0]->children[0], false, false, errorContainer, filename);
    addInstruction(conditionBlock, ifBlock->children[0]->label, otNode);

    OperationTreeErrorInfo *errorInfo = errorContainer->error;
    while (errorInfo != NULL) {
      ProgramErrorInfo* error = createProgramErrorInfo(errorInfo->message);
      addProgramError(program, error);
      errorInfo = errorInfo->next;
    }
    freeOperationTreeErrors(errorContainer->error);
    free(errorContainer);

    BasicBlock *thenBlock = createBasicBlock(++(*uid), UNCONDITIONAL, "Then Block");
    addBasicBlock(cfg, thenBlock);

    BasicBlock *elseBlock = NULL;
    if (ifBlock->childCount == 3) {
        assert(strcmp(ifBlock->children[2]->label, ELSE) == 0);
        elseBlock = createBasicBlock(++(*uid), UNCONDITIONAL, "Else Block");
        addBasicBlock(cfg, elseBlock);
    }

    addEdge(conditionBlock, thenBlock, TRUE_CONDITION, NULL); //TODO
    if (elseBlock != NULL) {
        addEdge(conditionBlock, elseBlock, FALSE_CONDITION, NULL); //TODO
    } else {
        addEdge(conditionBlock, emptyBlock, FALSE_CONDITION, NULL); //TODO
    }

    BasicBlock *thenExitBlock = parseBlock(ifBlock->children[1], program, filename, isLoop, conditionBlock, thenBlock, loopExitBlock, cfg, uid);

    addEdge(thenExitBlock, emptyBlock, UNCONDITIONAL_JUMP, NULL);

    if (elseBlock != NULL) {
        BasicBlock *elseExitBlock = parseBlock(ifBlock->children[2]->children[0], program, filename, isLoop, conditionBlock, elseBlock, loopExitBlock, cfg, uid);
        addEdge(elseExitBlock, emptyBlock, UNCONDITIONAL_JUMP, NULL);
    }
    return emptyBlock;
}

BasicBlock *parseBlock(MyAstNode* block, Program *program, const char* filename, bool isLoop, BasicBlock* prevBlock, BasicBlock* existingBlock, BasicBlock* loopExitBlock, CFG *cfg, uint32_t *uid) {
  assert(strcmp(block->label, BLOCK) == 0);
  BasicBlock *currentBlock;
  if (existingBlock == NULL) {
    currentBlock = createBasicBlock(++(*uid), UNCONDITIONAL, "Base block");
    addBasicBlock(cfg, currentBlock);
    addEdge(prevBlock, currentBlock, UNCONDITIONAL_JUMP, NULL);
  } else {
    currentBlock = existingBlock;
  }

  for (uint32_t i = 0; i < block->childCount; i++) {
    if (currentBlock->isEmpty) {
      free(currentBlock->name);
      currentBlock->name = strdup("Base block");
    }
    if (strcmp(block->children[i]->label, VAR) == 0) {
      parseVar(block->children[i], currentBlock, program, filename);
    } else if (strcmp(block->children[i]->label, BLOCK) == 0) {
      BasicBlock *toExistingBlock = currentBlock->isEmpty ? currentBlock : NULL;
      BasicBlock *nestedExitBlock = parseBlock(block->children[i], program, filename, isLoop, currentBlock, toExistingBlock, loopExitBlock, cfg, uid);
      currentBlock = nestedExitBlock;
    } else if (strcmp(block->children[i]->label, IF) == 0) {
      BasicBlock *toExistingBlock = currentBlock->isEmpty ? currentBlock : NULL;
      BasicBlock *nestedExitBlock = parseIf(block->children[i], program, filename, isLoop, currentBlock, toExistingBlock, loopExitBlock, cfg, uid);
      currentBlock = nestedExitBlock;
    } else if (strcmp(block->children[i]->label, WHILE) == 0) {
      BasicBlock *toExistingBlock = currentBlock->isEmpty ? currentBlock : NULL;
      BasicBlock *nestedExitBlock = parseWhile(block->children[i], program, filename, currentBlock, toExistingBlock, cfg, uid);
      currentBlock = nestedExitBlock;
    } else if (strcmp(block->children[i]->label, DO_WHILE) == 0) {
      BasicBlock *toExistingBlock = currentBlock->isEmpty ? currentBlock : NULL;
      BasicBlock *nestedExitBlock = parseDoWhile(block->children[i], program, filename, currentBlock, toExistingBlock, cfg, uid);
      currentBlock = nestedExitBlock;      
    } else if (strcmp(block->children[i]->label, BREAK) == 0) {
      OperationTreeNode *breakOtNode = newOperationTreeNode(OT_BREAK, 0, block->children[i]->children[0]->line, block->children[i]->children[0]->pos, false);
      addInstruction(currentBlock, block->children[i]->children[0]->label, breakOtNode);
      if (isLoop) {
        addEdge(currentBlock, loopExitBlock, UNCONDITIONAL_JUMP, NULL);
        currentBlock->isBreak = true;
        if (i < block->childCount - 1) {
          char buffer[1024];

          snprintf(buffer, sizeof(buffer),
            "Control error. Unreachable code after break at %s:%d:%d\n",
            filename, block->children[i]->children[0]->line, block->children[i]->children[0]->pos + 1);
          ProgramErrorInfo* error = createProgramErrorInfo(buffer);
          addProgramError(program, error);
          break;
        }
      } else {
        char buffer[1024];

        snprintf(buffer, sizeof(buffer),
          "Control error. Break at %s:%d:%d is out of loop\n",
          filename, block->children[i]->children[0]->line, block->children[i]->children[0]->pos + 1);
        ProgramErrorInfo* error = createProgramErrorInfo(buffer);
        addProgramError(program, error);
      }
    } else if (strcmp(block->children[i]->label, EXPR) == 0) {
      parseExpr(block->children[i], currentBlock, program, filename);
    }
  }

  return currentBlock;
}

Program *buildProgram(FilesToAnalyze *files, bool debug) {
  Program *program = (Program *)malloc(sizeof(Program));
  program->functions = NULL;
  program->errors = NULL;
  program->warnings = NULL;

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
            "Redeclaration error. Function %s at %s:%d:%d was previously declared at %s:%d:%d\n",
            info->functionName, info->fileName, info->line, info->pos + 1,
            func->fileName, func->line, func->pos);
          ProgramErrorInfo* error = createProgramErrorInfo(buffer);
          addProgramError(program, error);
          break;
        }
        func = nextFunc;
      }

      addFunctionToProgram(program, info);
    }
  }

  for (uint32_t i = 0; i < files->filesCount; i++) {
    MyLangResult *result = files->result[i];
    MyAstNode **funcDefs = result->tree->children;
    uint32_t childCount = result->tree->childCount;
    for (uint32_t j = 0; j < childCount; j++) {
      MyAstNode *block = funcDefs[j]->children[1];
      assert(strcmp(block->label, BLOCK) == 0);
      MyAstNode* funcSignature = funcDefs[j]->children[0];
      assert(strcmp(funcSignature->label, FUNC_SIGNATURE) == 0);
      MyAstNode* name;
      if (funcSignature->childCount == 2) {
        name = funcSignature->children[0];
        assert(strcmp(name->label, NAME) == 0);
      } else if (funcSignature->childCount == 3) {
        name = funcSignature->children[1];
        assert(strcmp(name->label, NAME) == 0);
      }
      CFG *cfg = createCFG();
      uint32_t uid = 0;
      BasicBlock *startBlock = createBasicBlock(uid, UNCONDITIONAL, "START");
      cfg->entryBlock = startBlock;
      addBasicBlock(cfg, startBlock);

      BasicBlock *lastBlock = parseBlock(block, program, files->fileName[i], false, startBlock, NULL, NULL, cfg, &uid);
      BasicBlock *retCheckBlock;
      if (lastBlock->isEmpty) {
        lastBlock->type = TERMINAL;
        free(lastBlock->name);
        lastBlock->name = strdup("END");
        retCheckBlock = lastBlock;
      } else {
        BasicBlock *endBlock = createBasicBlock(++uid, TERMINAL, "END");
        addBasicBlock(cfg, endBlock);
        addEdge(lastBlock, endBlock, UNCONDITIONAL_JUMP, NULL);
        retCheckBlock = endBlock;
      }

      Edge *inEdge = retCheckBlock->inEdges;
      while (inEdge != NULL) {
          BasicBlock *incomingBlock = inEdge->fromBlock;
          if (incomingBlock->instructionCount > 0) {
            OperationTreeNode *lastOperation = incomingBlock->instructions[incomingBlock->instructionCount - 1].otRoot;
            if (incomingBlock->type == UNCONDITIONAL && ( isBinaryOp(lastOperation->label) ||
                isUnaryOp(lastOperation->label) ||
                strcmp(lastOperation->label, LIT_READ) == 0 ||
                strcmp(lastOperation->label, READ) == 0 ||
                strcmp(lastOperation->label, OT_CALL) == 0 ||
                strcmp(lastOperation->label, INDEX) == 0)) {
                OperationTreeNode *returnNode = newOperationTreeNode(RETURN, 1, lastOperation->line, lastOperation->pos, false);
                returnNode->children[0] = lastOperation;
                incomingBlock->instructions[incomingBlock->instructionCount - 1].otRoot = returnNode;
            } else {
              char buffer[1024];
              
              snprintf(buffer, sizeof(buffer), 
              "No return warning. Can't use instruction at %s:%d:%d as a return value",
                      files->fileName[i], lastOperation->line, lastOperation->pos + 1);
              ProgramWarningInfo* warning = createProgramWarningInfo(buffer);
              addProgramWarning(program, warning);
            }
          } else {
            char buffer[1024];

            snprintf(buffer, sizeof(buffer), 
            "No return warning. There is no instructions to use as a return value at %s in function %s",
                    files->fileName[i], name->children[0]->label);
            ProgramWarningInfo* warning = createProgramWarningInfo(buffer);
            addProgramWarning(program, warning);
          }
          inEdge = inEdge->nextIn;
      }

      FunctionInfo *func = program->functions;
      while (func != NULL) {
        if (strcmp(func->functionName, name->children[0]->label) == 0) {
          func->cfg = cfg;
        }
        func = func->next;
      }
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
    printf("Base block %d, %s (%s):\n", block->id, block->name,
           (block->type == CONDITIONAL)     ? "conditional"
           : (block->type == UNCONDITIONAL) ? "unconditional"
                                            : "terminal");
    for (int i = 0; i < block->instructionCount; i++) {
      printf("  Instruction %d: %s\nOperation tree:\n", i, block->instructions[i].text);
      if (block->instructions[i].otRoot != NULL) {
        printOperationTree(block->instructions[i].otRoot);
      }
      printf("\n");
    }
    Edge *edge = block->outEdges;
    while (edge != NULL) {
      printf("  Jump to block %d", edge->targetBlock->id);
      switch (edge->type) {
      case TRUE_CONDITION:
        printf(" if true");
        break;
      case FALSE_CONDITION:
        printf(" if false");
        break;
      case UNCONDITIONAL_JUMP:
        printf(" unconditional");
        break;
      }
      printf("\n");
      edge = edge->nextOut;
    }
    printf("\n");
    block = block->next;
  }
}

void freeInstructions(BasicBlock *block) {
  for (int i = 0; i < block->instructionCount; i++) {
    free(block->instructions[i].text);
    if (block->instructions[i].otRoot != NULL)
      destroyOperationTreeNodeTree(block->instructions[i].otRoot);
  }
  free(block->instructions);
}

void freeOutEdges(Edge *edge) {
  while (edge != NULL) {
    Edge *nextEdge = edge->nextOut;
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
    freeOutEdges(block->outEdges);
    block->inEdges = NULL;
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
  freeProgramWarnings(program->warnings);
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

ProgramWarningInfo* createProgramWarningInfo(const char *message) {
    ProgramWarningInfo *warningInfo = (ProgramWarningInfo*)malloc(sizeof(ProgramWarningInfo));
    warningInfo->message = strdup(message);
    warningInfo->next = NULL;
    return warningInfo;
}

void addProgramWarning(Program *program, ProgramWarningInfo *warningInfo) {
    warningInfo->next = program->warnings;
    program->warnings = warningInfo;
}

void freeProgramWarnings(ProgramWarningInfo *warning) {
    while (warning != NULL) {
        ProgramWarningInfo *nextWarning = warning->next;
        free(warning->message);
        free(warning);
        warning = nextWarning;
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

void writeOperationTreeToDot(FILE *file, OperationTreeNode *node, int *nodeCounter) {
    if (node == NULL) {
        return;
    }

    int currentNodeId = (*nodeCounter)++;
    
    char escapedLabel[256];
    const char *src = node->label;
    char *dst = escapedLabel;
    while (*src && (dst - escapedLabel) < 255) {
        if (*src == '<') {
            *dst++ = '&';
            *dst++ = 'l';
            *dst++ = 't';
            *dst++ = ';';
        } else if (*src == '>') {
            *dst++ = '&';
            *dst++ = 'g';
            *dst++ = 't';
            *dst++ = ';';
        } else if (*src == '&') {
            *dst++ = '&';
            *dst++ = 'a';
            *dst++ = 'm';
            *dst++ = 'p';
            *dst++ = ';';
        } else if (*src == '"') {
            *dst++ = '\\';
            *dst++ = '"';
        } else {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';

    fprintf(file, "        node%d [label=\"%s\", color=blue];\n", currentNodeId, escapedLabel);

    for (uint32_t i = 0; i < node->childCount; i++) {
        int childNodeId = *nodeCounter;
        writeOperationTreeToDot(file, node->children[i], nodeCounter);
        fprintf(file, "        node%d -> node%d[color=blue];\n", currentNodeId, childNodeId);
    }
}

void writeCFGToDotFile(CFG *cfg, const char *filename, bool drawOt) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Can't open file %s to write\n", filename);
        return;
    }

    fprintf(file, "digraph CFG {\n");
    fprintf(file, "    compound=true;\n");
    fprintf(file, "    graph [splines=true];\n");
    fprintf(file, "    node [shape=rectangle];\n\n");

    BasicBlock *block = cfg->blocks;
    int nodeCounter = 0;
    int clusterCounter = 0;

    while (block != NULL) {
        fprintf(file, "    BB%d [label=<", block->id);
        fprintf(file, "<B>BB%d: %s</B><BR ALIGN=\"CENTER\"/>", block->id, block->name);

        for (int i = 0; i < block->instructionCount; i++) {
            char instruction[256];
            char *src = block->instructions[i].text;
            char *dst = instruction;
            while (*src && (dst - instruction) < 255) {
                if (*src == '<') {
                    *dst++ = '&';
                    *dst++ = 'l';
                    *dst++ = 't';
                    *dst++ = ';';
                } else if (*src == '>') {
                    *dst++ = '&';
                    *dst++ = 'g';
                    *dst++ = 't';
                    *dst++ = ';';
                } else if (*src == '&') {
                    *dst++ = '&';
                    *dst++ = 'a';
                    *dst++ = 'm';
                    *dst++ = 'p';
                    *dst++ = ';';
                } else if (*src == '"') {
                    *dst++ = '\\';
                    *dst++ = '"';
                } else {
                    *dst++ = *src;
                }
                src++;
            }
            *dst = '\0';
            fprintf(file, "%s<BR ALIGN=\"CENTER\"/>", instruction);
        }

        fprintf(file, ">];\n");

        if (drawOt) {
          for (int i = 0; i < block->instructionCount; i++) {
              if (block->instructions[i].otRoot != NULL) {
                  fprintf(file, "    subgraph cluster_instruction%d {\n", clusterCounter);
                  fprintf(file, "        label = \"OT of BB%d:%d\";\n", block->id, i);
                  fprintf(file, "        style=rounded;\n");
                  fprintf(file, "        color=blue;\n");

                  char entryNodeName[64];
                  snprintf(entryNodeName, sizeof(entryNodeName), "entry%d", clusterCounter);
                  fprintf(file, "        %s [shape=point, style=invis];\n", entryNodeName);

                  writeOperationTreeToDot(file, block->instructions[i].otRoot, &nodeCounter);

                  fprintf(file, "    }\n");

                  fprintf(file, "    BB%d -> %s [lhead=cluster_instruction%d, color=blue];\n", block->id, entryNodeName, clusterCounter);

                  clusterCounter++;
              }
          }
        }

        block = block->next;
    }

    fprintf(file, "\n");

    block = cfg->blocks;
    while (block != NULL) {
        Edge *edge = block->outEdges;
        while (edge != NULL) {
            const char *edgeLabel = "";
            switch (edge->type) {
                case TRUE_CONDITION:
                    edgeLabel = " [label=\"True\", color=green]";
                    break;
                case FALSE_CONDITION:
                    edgeLabel = " [label=\"False\", color=red]";
                    break;
                case UNCONDITIONAL_JUMP:
                    edgeLabel = "";
                    break;
            }
            fprintf(file, "    BB%d -> BB%d%s;\n", block->id, edge->targetBlock->id, edgeLabel);
            edge = edge->nextOut;
        }
        block = block->next;
    }

    fprintf(file, "}\n");

    fclose(file);
}