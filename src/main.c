#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <argp.h>

#include "grammar/myLang.h"
#include "dotUtils/dotUtils.h"
#include "cfg/cfg.h"
#include "cfg/cg/cg.h"

struct arguments {
    char **input_files;
    char *output_dir;
    int debug;
    int input_file_count;
};

static struct argp_option options[] = {
    { "debug",  'd', 0,       0, "Enable debug output" },
    { "output", 'o', "DIR",   0, "Output directory name" },
    { 0 }
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;

    switch (key) {
        case 'd':
            arguments->debug = 1;
            break;
        case 'o':
            arguments->output_dir = arg;
            break;
        case ARGP_KEY_ARG:
            arguments->input_files = realloc(arguments->input_files, sizeof(char*) * (arguments->input_file_count + 1));
            if (!arguments->input_files) {
                argp_failure(state, 1, ENOMEM, "Out of memory");
            }
            arguments->input_files[arguments->input_file_count] = arg;
            arguments->input_file_count++;
            break;
        case ARGP_KEY_END:
            if (arguments->input_file_count == 0) {
                argp_usage(state);
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static char args_doc[] = "INPUT_FILES...";

static char doc[] = "CFG and CG builder from AST";

static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc, char *argv[]) {

    struct arguments arguments;

    arguments.debug = 0;
    arguments.output_dir = NULL;
    arguments.input_files = NULL;
    arguments.input_file_count = 0;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if (arguments.debug) {
        printf("Debug output is enabled\n");
    }

    if (arguments.output_dir) {
        printf("Output directory: %s\n", arguments.output_dir);
    } else {
        printf("Output directory isn't provided\n");
        return 0;
    }

    printf("Input files:\n");
    for (int i = 0; i < arguments.input_file_count; i++) {
        printf("  %s\n", arguments.input_files[i]);
    }

    FilesToAnalyze files;
    files.filesCount = arguments.input_file_count;

    files.result = malloc(sizeof(MyLangResult*) * files.filesCount);
    files.fileName = arguments.input_files;

    for (int i = 0; i < arguments.input_file_count; i++) {
        MyLangResult *result = malloc(sizeof(MyLangResult));
        parseMyLangFromFile(result, arguments.input_files[i], arguments.debug);
        files.result[i] = result;
    }

    Program* prog = buildProgram(&files, arguments.debug);

    if (prog->errors != NULL) {
        printf("Errors:\n");
        ProgramErrorInfo *error = prog->errors;
        while (error != NULL) {
            printf("%s\n", error->message);
            error = error->next;
        }
    }

    freeProgram(prog);

    for (uint32_t i = 0; i < files.filesCount; i++) {
        destroyMyLangResult(files.result[i]);
        free(files.result[i]);
    }
    free(arguments.input_files);
    free(files.result);

    // CFG *cfg = createCFG();

    // BasicBlock *BB1 = createBasicBlock(1, CONDITIONAL, "BB1");
    // BasicBlock *BB2 = createBasicBlock(2, CONDITIONAL, "BB2");
    // BasicBlock *BB3 = createBasicBlock(3, CONDITIONAL, "BB3");
    // BasicBlock *BB4 = createBasicBlock(4, UNCONDITIONAL, "BB4");
    // BasicBlock *BB5 = createBasicBlock(5, CONDITIONAL, "BB5");
    // BasicBlock *BB6 = createBasicBlock(6, UNCONDITIONAL, "BB6");
    // BasicBlock *BB7 = createBasicBlock(7, UNCONDITIONAL, "BB7");
    // BasicBlock *BB8 = createBasicBlock(8, UNCONDITIONAL, "BB8");
    // BasicBlock *BB9 = createBasicBlock(9, CONDITIONAL, "BB9");
    // BasicBlock *BB10 = createBasicBlock(10, UNCONDITIONAL, "BB10");
    // BasicBlock *BB11 = createBasicBlock(11, UNCONDITIONAL, "BB11");
    // BasicBlock *BB12 = createBasicBlock(12, TERMINAL, "BB12");

    // addInstruction(BB1, "if (x > 0)");
    // addInstruction(BB2, "while (x > 0)");
    // addInstruction(BB3, "if (x % 2 == 0)");
    // addInstruction(BB4, "x = x / 2;");
    // addInstruction(BB5, "if (x % 3 == 0)");
    // addInstruction(BB6, "x = x / 3;");
    // addInstruction(BB7, "x = x - 1;");
    // addInstruction(BB8, "x += 2;");
    // addInstruction(BB9, "if (x < 0)");
    // addInstruction(BB10, "x = 100;");
    // addInstruction(BB11, "printf(\"Result: %d\\n\", x);");

    // addBasicBlock(cfg, BB12);
    // addBasicBlock(cfg, BB11);
    // addBasicBlock(cfg, BB10);
    // addBasicBlock(cfg, BB9);
    // addBasicBlock(cfg, BB8);
    // addBasicBlock(cfg, BB7);
    // addBasicBlock(cfg, BB6);
    // addBasicBlock(cfg, BB5);
    // addBasicBlock(cfg, BB4);
    // addBasicBlock(cfg, BB3);
    // addBasicBlock(cfg, BB2);
    // addBasicBlock(cfg, BB1);

    // cfg->entryBlock = BB1;

    // addEdge(BB1, BB2, TRUE_CONDITION, "x > 0");
    // addEdge(BB1, BB9, FALSE_CONDITION, "x > 0");
    // addEdge(BB2, BB3, TRUE_CONDITION, "x > 0");
    // addEdge(BB2, BB11, FALSE_CONDITION, "x > 0");
    // addEdge(BB3, BB4, TRUE_CONDITION, "x % 2 == 0");
    // addEdge(BB3, BB5, FALSE_CONDITION, "x % 2 == 0");
    // addEdge(BB4, BB2, UNCONDITIONAL_JUMP, NULL);
    // addEdge(BB5, BB6, TRUE_CONDITION, "x % 3 == 0");
    // addEdge(BB5, BB7, FALSE_CONDITION, "x % 3 == 0");
    // addEdge(BB6, BB2, UNCONDITIONAL_JUMP, NULL);
    // addEdge(BB7, BB2, UNCONDITIONAL_JUMP, NULL);
    // addEdge(BB9, BB8, TRUE_CONDITION, "x < 0");
    // addEdge(BB9, BB10, FALSE_CONDITION, "x < 0");
    // addEdge(BB8, BB9, UNCONDITIONAL_JUMP, NULL);
    // addEdge(BB10, BB11, UNCONDITIONAL_JUMP, NULL);
    // addEdge(BB11, BB12, UNCONDITIONAL_JUMP, NULL);

    // printCFG(cfg);

    // freeCFG(cfg);

    // CallGraph* graph = createCallGraph();

    // FunctionNode* f1 = createFunctionNode("functionA");
    // FunctionNode* f2 = createFunctionNode("functionB");
    // FunctionNode* f3 = createFunctionNode("functionC");

    // addFunction(graph, f1);
    // addFunction(graph, f2);
    // addFunction(graph, f3);

    // addCall(f1, f2);
    // addCall(f1, f3);
    // addCall(f2, f3);

    // printCallGraph(graph);

    // freeCallGraph(graph);

  return 0;
}
