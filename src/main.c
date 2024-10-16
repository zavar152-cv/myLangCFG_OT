#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <argp.h>

#include "grammar/myLang.h"
#include "dotUtils/dotUtils.h"
#include "cfg/cfg.h"
#include "cfg/cg/cg.h"

struct arguments {
  char *inputFile;
  char *outputFile;
  bool debug;
};

error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch (key) {
    case 'i':
      arguments->inputFile = arg;
      break;

    case 'o':
      arguments->outputFile = arg;
      break;

    case 'd':
      arguments->debug = true;
      break;

    case ARGP_KEY_END:
      if (arguments->inputFile == NULL || arguments->outputFile == NULL) {
        argp_usage(state);
      }
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

struct argp_option options[] = {
  {"input",  'i', "INPUTFILE",  0, "Input file"},
  {"output", 'o', "OUTPUTFILE", 0, "Output file"},
  {"debug",  'd', 0,            0, "Enable debug output"},
  {0}
};

struct argp argp = {options, parse_opt, 0, "MyLang Parser Program"};

int main(int argc, char *argv[]) {
  // struct arguments arguments;
  // arguments.debug = false;
  // arguments.inputFile = NULL;
  // arguments.outputFile = NULL;

  // argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // MyLangResult result;
  // parseMyLangFromFile(&result, arguments.inputFile, arguments.debug);

  // if (!result.isValid) {
  //   printErrors(&result.errorContext);
  // } 

  // int err = generateDotFileFromMyTree(result.tree, arguments.outputFile, arguments.debug);

  // if (err == FILE_ERROR) {
  //   fprintf(stderr, "Error opening file %s for writing.\n", arguments.outputFile);
  // }
  
  // destroyMyLangResult(&result);

    CFG *cfg = createCFG();

    BasicBlock *BB1 = createBasicBlock(1, CONDITIONAL, "BB1");
    BasicBlock *BB2 = createBasicBlock(2, CONDITIONAL, "BB2");
    BasicBlock *BB3 = createBasicBlock(3, CONDITIONAL, "BB3");
    BasicBlock *BB4 = createBasicBlock(4, UNCONDITIONAL, "BB4");
    BasicBlock *BB5 = createBasicBlock(5, CONDITIONAL, "BB5");
    BasicBlock *BB6 = createBasicBlock(6, UNCONDITIONAL, "BB6");
    BasicBlock *BB7 = createBasicBlock(7, UNCONDITIONAL, "BB7");
    BasicBlock *BB8 = createBasicBlock(8, UNCONDITIONAL, "BB8");
    BasicBlock *BB9 = createBasicBlock(9, CONDITIONAL, "BB9");
    BasicBlock *BB10 = createBasicBlock(10, UNCONDITIONAL, "BB10");
    BasicBlock *BB11 = createBasicBlock(11, UNCONDITIONAL, "BB11");
    BasicBlock *BB12 = createBasicBlock(12, TERMINAL, "BB12");

    addInstruction(BB1, "if (x > 0)");
    addInstruction(BB2, "while (x > 0)");
    addInstruction(BB3, "if (x % 2 == 0)");
    addInstruction(BB4, "x = x / 2;");
    addInstruction(BB5, "if (x % 3 == 0)");
    addInstruction(BB6, "x = x / 3;");
    addInstruction(BB7, "x = x - 1;");
    addInstruction(BB8, "x += 2;");
    addInstruction(BB9, "if (x < 0)");
    addInstruction(BB10, "x = 100;");
    addInstruction(BB11, "printf(\"Result: %d\\n\", x);");

    addBasicBlock(cfg, BB12);
    addBasicBlock(cfg, BB11);
    addBasicBlock(cfg, BB10);
    addBasicBlock(cfg, BB9);
    addBasicBlock(cfg, BB8);
    addBasicBlock(cfg, BB7);
    addBasicBlock(cfg, BB6);
    addBasicBlock(cfg, BB5);
    addBasicBlock(cfg, BB4);
    addBasicBlock(cfg, BB3);
    addBasicBlock(cfg, BB2);
    addBasicBlock(cfg, BB1);

    cfg->entryBlock = BB1;

    addEdge(BB1, BB2, TRUE_CONDITION, "x > 0");
    addEdge(BB1, BB9, FALSE_CONDITION, "x > 0");
    addEdge(BB2, BB3, TRUE_CONDITION, "x > 0");
    addEdge(BB2, BB11, FALSE_CONDITION, "x > 0");
    addEdge(BB3, BB4, TRUE_CONDITION, "x % 2 == 0");
    addEdge(BB3, BB5, FALSE_CONDITION, "x % 2 == 0");
    addEdge(BB4, BB2, UNCONDITIONAL_JUMP, NULL);
    addEdge(BB5, BB6, TRUE_CONDITION, "x % 3 == 0");
    addEdge(BB5, BB7, FALSE_CONDITION, "x % 3 == 0");
    addEdge(BB6, BB2, UNCONDITIONAL_JUMP, NULL);
    addEdge(BB7, BB2, UNCONDITIONAL_JUMP, NULL);
    addEdge(BB9, BB8, TRUE_CONDITION, "x < 0");
    addEdge(BB9, BB10, FALSE_CONDITION, "x < 0");
    addEdge(BB8, BB9, UNCONDITIONAL_JUMP, NULL);
    addEdge(BB10, BB11, UNCONDITIONAL_JUMP, NULL);
    addEdge(BB11, BB12, UNCONDITIONAL_JUMP, NULL);

    printCFG(cfg);

    freeCFG(cfg);

    CallGraph* graph = createCallGraph();

    FunctionNode* f1 = createFunctionNode("functionA");
    FunctionNode* f2 = createFunctionNode("functionB");
    FunctionNode* f3 = createFunctionNode("functionC");

    addFunction(graph, f1);
    addFunction(graph, f2);
    addFunction(graph, f3);

    addCall(f1, f2);
    addCall(f1, f3);
    addCall(f2, f3);

    printCallGraph(graph);

    freeCallGraph(graph);

  return 0;
}
