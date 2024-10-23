#pragma once

typedef struct FunctionNode {
    char *functionName;
    struct FunctionNode *next;
    struct CallEdge *outEdges;
    struct CallEdge *inEdges;
} FunctionNode;

typedef struct CallEdge {
    FunctionNode *caller;
    FunctionNode *callee;
    struct CallEdge *nextOut;
    struct CallEdge *nextIn;
} CallEdge;

typedef struct CallGraph {
    FunctionNode *functions;
} CallGraph;

FunctionNode* createFunctionNode(const char *functionName);

FunctionNode* findFunction(CallGraph *cg, const char *functionName);

void addFunctionToCallGraph(CallGraph *cg, const char *functionName);

void addCallEdge(CallGraph *cg, const char *callerName, const char *calleeName);

void freeCallGraph(CallGraph *cg);

void writeCallGraphToDot(CallGraph *cg, const char *filename);