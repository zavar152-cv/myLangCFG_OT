#include "cg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FunctionNode *createFunctionNode(const char *name) {
  FunctionNode *newNode = (FunctionNode *)malloc(sizeof(FunctionNode));

  newNode->name = strdup(name);

  newNode->numCalls = 0;
  newNode->maxCalls = 2;
  newNode->calls =
      (FunctionNode **)malloc(newNode->maxCalls * sizeof(FunctionNode *));

  return newNode;
}

CallGraph *createCallGraph() {
  CallGraph *graph = (CallGraph *)malloc(sizeof(CallGraph));
  graph->numFunctions = 0;
  graph->maxFunctions = 10;
  graph->functions =
      (FunctionNode **)malloc(graph->maxFunctions * sizeof(FunctionNode *));
  return graph;
}

void addFunctionToCG(CallGraph *graph, FunctionNode *function) {
  if (graph->numFunctions >= graph->maxFunctions) {
    graph->maxFunctions *= 2;
    graph->functions = (FunctionNode **)realloc(
        graph->functions, graph->maxFunctions * sizeof(FunctionNode *));
  }
  graph->functions[graph->numFunctions++] = function;
}

void addCall(FunctionNode *caller, FunctionNode *callee) {
  if (caller->numCalls >= caller->maxCalls) {
    caller->maxCalls *= 2;
    caller->calls = (FunctionNode **)realloc(
        caller->calls, caller->maxCalls * sizeof(FunctionNode *));
  }
  caller->calls[caller->numCalls++] = callee;
}

void printCallGraph(const CallGraph *graph) {
  for (int i = 0; i < graph->numFunctions; i++) {
    FunctionNode *function = graph->functions[i];
    if (function->numCalls > 0) {
      printf("Function: %s calls -> ", function->name);
      for (int j = 0; j < function->numCalls; j++) {
        printf("%s ", function->calls[j]->name);
      }
      printf("\n");
    }
  }
}

void freeFunctionNode(FunctionNode *function) {
  free(function->name);
  free(function->calls);
  free(function);
}

void freeCallGraph(CallGraph *graph) {
  for (int i = 0; i < graph->numFunctions; i++) {
    freeFunctionNode(graph->functions[i]);
  }
  free(graph->functions);
  free(graph);
}