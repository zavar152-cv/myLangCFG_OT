#include "cfg.h"
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

void addInstruction(BasicBlock *block, const char *text) {
  if (block->instructionCount >= block->instructionCapacity) {
    block->instructionCapacity *= 2;
    block->instructions = (Instruction *)realloc(
        block->instructions, sizeof(Instruction) * block->instructionCapacity);
  }
  block->instructions[block->instructionCount].text = strdup(text);
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
    edge->targetBlock = to;
    edge->next = from->outEdges;
    from->outEdges = edge;
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

//TODO operation tree
void parseVar(MyAstNode* var, BasicBlock *currentBlock, Program *program, const char* filename) {
  assert(strcmp(var->children[0]->label, TYPEREF) == 0);

  uint32_t varCount = (var->childCount - 1) / 2;

  if (varCount == 1) {
    //use DECLARE node
  } else {
    //use SEQ_DECLARE with childern type DECLARE
    for (uint32_t i = 0; i < varCount; i++) {
      var->children[i + 1]->label;     // ids
      var->children[i + 1 + varCount]; // init
    }
  }

  addInstruction(currentBlock, var->label);
}

//TODO operation tree
void parseExpr(MyAstNode* expr, BasicBlock *currentBlock, Program *program, const char* filename) {
  assert(strcmp(expr->label, EXPR) == 0);

  if (strcmp(expr->children[0]->label, ASSIGN) == 0) {
    //parse ASSIGN
  } else if (strcmp(expr->children[0]->label, FUNC_CALL) == 0) {
    //parse FUNC_CALL
  } else if (strcmp(expr->children[0]->label, INDEXING) == 0) {
    //parse INDEXING
  } else {
    char buffer[1024];
    MyAstNode* temp = expr;
    while (temp->childCount != 0) {
      temp = temp->children[0];
    }

    snprintf(buffer, sizeof(buffer),
             "Useless expression warning. Expression at %d:%d in file %s is useless\n",
             temp->line, temp->pos + 1, filename);
    ProgramWarningInfo *warning = createProgramWarningInfo(buffer);
    addProgramWarning(program, warning);
  }

  addInstruction(currentBlock, expr->children[0]->label);
}

BasicBlock* parseWhile(MyAstNode* whileBlock, Program *program, const char* filename,
                       BasicBlock* prevBlock, BasicBlock* existingBlock, CFG *cfg, uint32_t *uid) {
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

    //TODO operation tree for condition
    addInstruction(conditionBlock, whileBlock->children[0]->label);

    BasicBlock *bodyBlock = createBasicBlock(++(*uid), UNCONDITIONAL, "While Body");
    addBasicBlock(cfg, bodyBlock);

    addEdge(conditionBlock, bodyBlock, TRUE_CONDITION, NULL); //TODO
    addEdge(conditionBlock, emptyBlock, FALSE_CONDITION, NULL); //TODO

    BasicBlock *bodyExitBlock = parseBlock(whileBlock->children[1], program, filename, true, conditionBlock, bodyBlock, conditionBlock, cfg, uid);

    addEdge(bodyExitBlock, conditionBlock, UNCONDITIONAL_JUMP, NULL);
    return emptyBlock;
}

BasicBlock *parseIf(MyAstNode* ifBlock, Program *program, const char* filename, bool isLoop, BasicBlock* prevBlock, BasicBlock* existingBlock, BasicBlock* loopBlock, CFG *cfg, uint32_t *uid) {
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

    //TODO operation tree for condition
    addInstruction(conditionBlock, ifBlock->children[0]->label);

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

    BasicBlock *thenExitBlock = parseBlock(ifBlock->children[1], program, filename, isLoop, conditionBlock, thenBlock, loopBlock, cfg, uid);

    addEdge(thenExitBlock, emptyBlock, UNCONDITIONAL_JUMP, NULL);

    if (elseBlock != NULL) {
        BasicBlock *elseExitBlock = parseBlock(ifBlock->children[2]->children[0], program, filename, isLoop, conditionBlock, elseBlock, loopBlock, cfg, uid);
        addEdge(elseExitBlock, emptyBlock, UNCONDITIONAL_JUMP, NULL);
    }
    return emptyBlock;
}

BasicBlock *parseBlock(MyAstNode* block, Program *program, const char* filename, bool isLoop, BasicBlock* prevBlock, BasicBlock* existingBlock, BasicBlock* loopBlock, CFG *cfg, uint32_t *uid) {
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
      BasicBlock *nestedExitBlock = parseBlock(block->children[i], program, filename, isLoop, currentBlock, toExistingBlock, loopBlock, cfg, uid);
      currentBlock = nestedExitBlock;
    } else if (strcmp(block->children[i]->label, IF) == 0) {
      BasicBlock *toExistingBlock = currentBlock->isEmpty ? currentBlock : NULL;
      BasicBlock *nestedExitBlock = parseIf(block->children[i], program, filename, isLoop, currentBlock, toExistingBlock, loopBlock, cfg, uid);
      currentBlock = nestedExitBlock;
    } else if (strcmp(block->children[i]->label, WHILE) == 0) {
      BasicBlock *toExistingBlock = currentBlock->isEmpty ? currentBlock : NULL;
      BasicBlock *nestedExitBlock = parseWhile(block->children[i], program, filename, currentBlock, toExistingBlock, cfg, uid);
      currentBlock = nestedExitBlock;
    } else if (strcmp(block->children[i]->label, DO_WHILE) == 0) {

    } else if (strcmp(block->children[i]->label, BREAK) == 0) {
      addInstruction(currentBlock, block->children[i]->children[0]->label);
      if (isLoop) {
        addEdge(currentBlock, loopBlock, UNCONDITIONAL_JUMP, NULL);
        currentBlock->isBreak = true;
        if (i < block->childCount - 1) {
          char buffer[1024];

          snprintf(buffer, sizeof(buffer),
            "Control error. Unreachable code after break in %s at %d:%d\n",
            filename, block->children[i]->children[0]->line, block->children[i]->children[0]->pos + 1);
          ProgramErrorInfo* error = createProgramErrorInfo(buffer);
          addProgramError(program, error);
          break;
        }
      } else {
        char buffer[1024];

        snprintf(buffer, sizeof(buffer),
          "Control error. Break in %s at %d:%d is out of loop\n",
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
            "Redeclaration error. Function %s at %d:%d in file %s was previously declared at %d:%d in file %s\n",
            info->functionName, info->line, info->pos + 1, info->fileName,
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

  for (uint32_t i = 0; i < files->filesCount; i++) {
    MyLangResult *result = files->result[i];
    MyAstNode **funcDefs = result->tree->children;
    uint32_t childCount = result->tree->childCount;
    for (uint32_t j = 0; j < childCount; j++) {
      MyAstNode *block = funcDefs[j]->children[1];
      assert(strcmp(block->label, BLOCK) == 0);
      CFG *cfg = createCFG();
      uint32_t uid = 0;
      BasicBlock *startBlock = createBasicBlock(uid, UNCONDITIONAL, "START");
      cfg->entryBlock = startBlock;
      addBasicBlock(cfg, startBlock);

      BasicBlock *lastBlock = parseBlock(block, program, files->fileName[j], false, startBlock, NULL, NULL, cfg, &uid);
      if (lastBlock->isEmpty) {
        lastBlock->type = TERMINAL;
        free(lastBlock->name);
        lastBlock->name = strdup("END");
      } else {
        BasicBlock *endBlock = createBasicBlock(++uid, TERMINAL, "END");
        addBasicBlock(cfg, endBlock);
        addEdge(lastBlock, endBlock, UNCONDITIONAL_JUMP, NULL);
      }
      program->functions[j].cfg = cfg;
    }
  }

  if (debug) {
    FunctionInfo *func = program->functions;
    while (func != NULL) {
      printFunctionInfo(func);
      char *newString = NULL;
      size_t originalLength = strlen(func->functionName);
      size_t suffixLength = strlen(".dot");
      size_t totalLength = originalLength + suffixLength + 1;

      newString = malloc(totalLength);
      strcpy(newString, func->functionName);

      strcat(newString, ".dot");
      writeCFGToDotFile(func->cfg, newString);
      func = func->next;
      free(newString);
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

void writeCFGToDotFile(CFG *cfg, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Не удалось открыть файл %s для записи\n", filename);
        return;
    }

    fprintf(file, "digraph CFG {\n");
    fprintf(file, "    graph [splines=ortho];");
    fprintf(file, "    node [shape=rectangle];\n\n");

    BasicBlock *block = cfg->blocks;
    while (block != NULL) {
        char label[1024];
        snprintf(label, sizeof(label), "BB%d, %s\\n", block->id, block->name);
        for (int i = 0; i < block->instructionCount; i++) {
            char instruction[256];
            char *src = block->instructions[i].text;
            char *dst = instruction;
            while (*src && (dst - instruction) < 255) {
                if (*src == '"' || *src == '\\') {
                    *dst++ = '\\';
                }
                *dst++ = *src++;
            }
            *dst = '\0';
            strncat(label, instruction, sizeof(label) - strlen(label) - 1);
            strncat(label, "\\n", sizeof(label) - strlen(label) - 1);
        }

        fprintf(file, "    BB%d [label=\"%s\"];\n", block->id, label);

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
            edge = edge->next;
        }
        block = block->next;
    }

    fprintf(file, "}\n");

    fclose(file);
}


