#pragma once

#include "grammar/myLang.h"

#define     EXPR "EXPR"
#define     TERM "TERM"
#define     BINARY_OP "BINARY_OP"
#define     UNARY_OP "UNARY_OP"
#define     BRACES "BRACES"
#define     PLACE "PLACE"
#define     LITERAL "LITERAL"
#define     PLUS "PLUS"
#define     MINUS "MINUS"
#define     MUL "MUL"
#define     ASSIGN "="
#define     NEG "NEG"
#define     NOT "NOT"
#define     BOOL "BOOL"
#define     STR "STR"
#define     SYMB "SYMB"
#define     HEX "HEX"
#define     BITS "BITS"
#define     DEC "DEC"
#define     IDENTIFIER "IDENTIFIER"
#define     BUILTIN_TYPE "BUILTIN_TYPE"
#define     CUSTOM_TYPE "CUSTOM_TYPE"
#define     ARRAY "ARRAY"
#define     SOURCE_ITEM "SOURCE_ITEM"
#define     VAR "VAR"
#define     SOURCE "SOURCE"
#define     TYPE "TYPE"
#define     LIST "LIST"
#define     INIT "INIT"
#define     ARRAY_SIZE "ARRAY_SIZE"
#define     CALL "CALL"
#define     INDEXER "INDEXER"
#define     PRIMARY "PRIMARY"
#define     TYPEREF "TYPEREF"
#define     BLOCK "BLOCK"
#define     FUNC_SIGNATURE "FUNC_SIGNATURE"
#define     FUNC_DEF "FUNC_DEF"
#define     ARGDEF_LIST "ARGDEF_LIST"
#define     ARGDEF "ARGDEF"
#define     NAME "NAME"
#define     BREAK "BREAK"
#define     FUNC_CALL "FUNC_CALL"
#define     EXPR_LIST "EXPR_LIST"
#define     INDEXING "INDEXING"
#define     WHILE "WHILE"
#define     DO_WHILE "DO_WHILE"
#define     IF "IF"
#define     ELSE "ELSE"

#define INITIAL_CAPACITY 4

typedef enum {
    CONDITIONAL,
    UNCONDITIONAL,
    TERMINAL 
} BlockType;

typedef enum {
    TRUE_CONDITION,
    FALSE_CONDITION,
    UNCONDITIONAL_JUMP,
} EdgeType;

typedef struct {
    char *text;
} Instruction;

struct BasicBlock;

typedef struct __attribute__((packed)) Edge {
    EdgeType type;
    char *condition; // NULL for unconditional
    struct BasicBlock *fromBlock;
    struct BasicBlock *targetBlock;
    struct Edge *nextOut;
    struct Edge *nextIn;
} Edge;

typedef struct BasicBlock {
    int id;
    BlockType type;
    Instruction *instructions;
    int instructionCount;
    int instructionCapacity;
    char *name;
    bool isEmpty;
    bool isBreak;
    Edge *outEdges;
    Edge *inEdges;
    struct BasicBlock *next;
} BasicBlock;

typedef struct {
    BasicBlock *entryBlock;
    BasicBlock *blocks;
} CFG;

typedef struct {
    char *typeName;
    bool custom;
    bool isArray;
    uint32_t arrayDim;
    uint32_t line;
    uint32_t pos;
} TypeInfo;

typedef struct ArgumentInfo {
    TypeInfo *type;
    char *name;
    struct ArgumentInfo *next;
    uint32_t line;
    uint32_t pos;
} ArgumentInfo;

typedef struct FunctionInfo {
    char *fileName;
    char *functionName;
    TypeInfo *returnType;
    ArgumentInfo *arguments;
    CFG *cfg;
    struct FunctionInfo *next;
    uint32_t line;
    uint32_t pos;
} FunctionInfo;

typedef struct FilesToAnalyze {
    uint32_t filesCount;
    char **fileName;
    MyLangResult **result;
} FilesToAnalyze;

typedef struct __attribute__((packed)) ProgramErrorInfo {
    char *message;
    struct ProgramErrorInfo *next;
} ProgramErrorInfo;

typedef struct __attribute__((packed)) ProgramWarningInfo {
    char *message;
    struct ProgramWarningInfo *next;
} ProgramWarningInfo;

typedef struct Program {
    FunctionInfo *functions;
    ProgramErrorInfo *errors;
    ProgramWarningInfo *warnings;
} Program;

BasicBlock* createBasicBlock(int id, BlockType type, const char *name);

//stub, should be changed
void addInstruction(BasicBlock *block, const char *text);

void addEdge(BasicBlock *from, BasicBlock *to, EdgeType type, const char *condition);

void addBasicBlock(CFG *cfg, BasicBlock *block);

CFG* createCFG();

void printCFG(CFG *cfg);

void freeInstructions(BasicBlock *block);

void freeOutEdges(Edge *edge);

void freeBasicBlocks(BasicBlock *block);

void freeCFG(CFG *cfg);

TypeInfo* createTypeInfo(const char *typeName, bool custom, bool isArray, uint32_t arrayDim, uint32_t line, uint32_t pos);

void freeTypeInfo(TypeInfo *typeInfo);

ArgumentInfo* createArgumentInfo(TypeInfo *type, const char *name, uint32_t line, uint32_t pos);

void addArgument(FunctionInfo *funcInfo, ArgumentInfo *argInfo);

void freeArguments(ArgumentInfo *arg);

void addFunctionToProgram(Program *program, FunctionInfo *funcInfo);

void freeProgram(Program *program);

FunctionInfo* createFunctionInfo(const char *fileName, const char *functionName, TypeInfo *returnType, uint32_t line, uint32_t pos);

void freeFunctionInfo(FunctionInfo *funcInfo);

void printFunctionInfo(FunctionInfo *funcInfo);

Program* buildProgram(FilesToAnalyze *files, bool debug);

ProgramErrorInfo* createProgramErrorInfo(const char *message);

void addProgramError(Program *program, ProgramErrorInfo *errorInfo);

void freeProgramErrors(ProgramErrorInfo *error);

ProgramWarningInfo* createProgramWarningInfo(const char *message);

void addProgramWarning(Program *program, ProgramWarningInfo *errorInfo);

void freeProgramWarnings(ProgramWarningInfo *error);

void writeCFGToDotFile(CFG *cfg, const char *filename);