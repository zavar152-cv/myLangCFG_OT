#include "cg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FunctionNode* createFunctionNode(const char *functionName) {
    FunctionNode *node = (FunctionNode *)malloc(sizeof(FunctionNode));
    node->functionName = strdup(functionName);
    node->next = NULL;
    node->outEdges = NULL;
    node->inEdges = NULL;
    return node;
}

FunctionNode* findFunction(CallGraph *cg, const char *functionName) {
    FunctionNode *current = cg->functions;
    while (current != NULL) {
        if (strcmp(current->functionName, functionName) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void addFunctionToCallGraph(CallGraph *cg, const char *functionName) {
    if (findFunction(cg, functionName) == NULL) {
        FunctionNode *newNode = createFunctionNode(functionName);
        newNode->next = cg->functions;
        cg->functions = newNode;
    }
}

void addCallEdge(CallGraph *cg, const char *callerName, const char *calleeName) {
    FunctionNode *caller = findFunction(cg, callerName);
    if (caller == NULL) {
        caller = createFunctionNode(callerName);
        caller->next = cg->functions;
        cg->functions = caller;
    }
    
    FunctionNode *callee = findFunction(cg, calleeName);
    if (callee == NULL) {
        callee = createFunctionNode(calleeName);
        callee->next = cg->functions;
        cg->functions = callee;
    }
    
    CallEdge *existingEdge = caller->outEdges;
    while (existingEdge != NULL) {
        if (strcmp(existingEdge->callee->functionName, calleeName) == 0) {
            return;
        }
        existingEdge = existingEdge->nextOut;
    }
    
    CallEdge *outEdge = (CallEdge *)malloc(sizeof(CallEdge));
    outEdge->caller = caller;
    outEdge->callee = callee;
    outEdge->nextOut = caller->outEdges;
    caller->outEdges = outEdge;
    
    CallEdge *inEdge = (CallEdge *)malloc(sizeof(CallEdge));
    inEdge->caller = caller;
    inEdge->callee = callee;
    inEdge->nextIn = callee->inEdges;
    callee->inEdges = inEdge;
}

void freeCallGraph(CallGraph *cg) {
    FunctionNode *fn = cg->functions;
    while (fn != NULL) {
        FunctionNode *nextFn = fn->next;
        
        CallEdge *outEdge = fn->outEdges;
        while (outEdge != NULL) {
            CallEdge *nextOut = outEdge->nextOut;
            free(outEdge);
            outEdge = nextOut;
        }
        
        CallEdge *inEdge = fn->inEdges;
        while (inEdge != NULL) {
            CallEdge *nextIn = inEdge->nextIn;
            free(inEdge);
            inEdge = nextIn;
        }
        
        free(fn->functionName);
        free(fn);
        fn = nextFn;
    }
    free(cg);
}

void writeCallGraphToDot(CallGraph *cg, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Can't open file %s to write\n", filename);
        return;
    }
    
    fprintf(file, "digraph CallGraph {\n");
    fprintf(file, "    node [shape=ellipse, style=filled, color=lightblue];\n\n");
    
    FunctionNode *fn = cg->functions;
    while (fn != NULL) {
        fprintf(file, "    \"%s\";\n", fn->functionName);
        fn = fn->next;
    }
    
    fprintf(file, "\n");
    
    fn = cg->functions;
    while (fn != NULL) {
        CallEdge *edge = fn->outEdges;
        while (edge != NULL) {
            fprintf(file, "    \"%s\" -> \"%s\" [color=blue];\n", edge->caller->functionName, edge->callee->functionName);
            edge = edge->nextOut;
        }
        fn = fn->next;
    }
    
    fprintf(file, "}\n");
    fclose(file);
}