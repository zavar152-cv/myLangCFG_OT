#pragma once

#define INITIAL_CAPACITY 4

typedef enum {
    CONDITIONAL,
    UNCONDITIONAL,
    TERMINAL 
} BlockType;

typedef enum {
    TRUE_CONDITION,
    FALSE_CONDITION,
    UNCONDITIONAL_JUMP
} EdgeType;

typedef struct {
    char *text;
} Instruction;

struct BasicBlock;

typedef struct Edge {
    EdgeType type;
    char *condition; // NULL for unconditional
    struct BasicBlock *targetBlock;
    struct Edge *next;
} Edge;

typedef struct BasicBlock {
    int id;
    BlockType type;
    Instruction *instructions;
    int instructionCount;
    int instructionCapacity;
    char *name;
    Edge *outEdges;
    struct BasicBlock *next;
} BasicBlock;

typedef struct {
    BasicBlock *entryBlock;
    BasicBlock *blocks;
} CFG;

//TODO custom
typedef struct {
    char *typeName;
} TypeInfo;

typedef struct ArgumentInfo {
    TypeInfo *type;
    char *name;
    struct ArgumentInfo *next;
} ArgumentInfo;

typedef struct FunctionInfo {
    char *fileName;
    char *functionName;
    TypeInfo *returnType;
    ArgumentInfo *arguments;
    CFG *cfg;
    struct FunctionInfo *next;
} FunctionInfo;

BasicBlock* createBasicBlock(int id, BlockType type, const char *name);

//stub, should be changed
void addInstruction(BasicBlock *block, const char *text);

void addEdge(BasicBlock *from, BasicBlock *to, EdgeType type, const char *condition);

void addBasicBlock(CFG *cfg, BasicBlock *block);

CFG* createCFG();

void printCFG(CFG *cfg);

void freeInstructions(BasicBlock *block);

void freeEdges(Edge *edge);

void freeBasicBlocks(BasicBlock *block);

void freeCFG(CFG *cfg);

TypeInfo* createTypeInfo(const char *typeName);

void freeTypeInfo(TypeInfo *typeInfo);

ArgumentInfo* createArgumentInfo(TypeInfo *type, const char *name);

void addArgument(FunctionInfo *funcInfo, ArgumentInfo *argInfo);

void freeArguments(ArgumentInfo *arg);

FunctionInfo* createFunctionInfo(const char *fileName, const char *functionName, TypeInfo *returnType);

void freeFunctionInfo(FunctionInfo *funcInfo);

void printFunctionInfo(FunctionInfo *funcInfo);