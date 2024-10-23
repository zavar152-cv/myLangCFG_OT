#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <argp.h>
#include <libgen.h>
#include <string.h>

#include "grammar/myLang.h"
#include "dotUtils/dotUtils.h"
#include "cfg/cfg.h"
#include "cfg/cg/cg.h"

struct arguments {
    char **input_files;
    char *output_dir;
    int debug;
    int ot;
    int input_file_count;
};

static struct argp_option options[] = {
    { "debug",  'd', 0,       0, "Enable debug output" },
    { "output", 'o', "DIR",   0, "Output directory name" },
    { "operation tree", 't', 0,   0, "Draw operation tree in dot with CFG" },
    { 0 }
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;

    switch (key) {
        case 'd':
            arguments->debug = 1;
            break;
        case 't':
            arguments->ot = 1;
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

char *concat(char *original, char *suffix) {
  char *newString = NULL;
  size_t originalLength = strlen(original);
  size_t suffixLength = strlen(suffix);
  size_t totalLength = originalLength + suffixLength + 1;

  newString = malloc(totalLength);
  strcpy(newString, original);

  strcat(newString, suffix);
  return newString;
}

char* getFileNameWithoutExtension(const char* filePath) {
    char *pathCopy = strdup(filePath);
    char *baseName = basename(pathCopy);
    char *dot = strrchr(baseName, '.');
    if (dot != NULL) {
        *dot = '\0';
    }
    char *result = strdup(baseName);
    free(pathCopy);
    return result;
}

char* getOutputFileName(const char* sourceFilePath, const char* functionName, const char* ext, const char* outputDir) {
    char *sourceName = getFileNameWithoutExtension(sourceFilePath);
    size_t totalLength = strlen(sourceName) + 1 + strlen(functionName) + 1 + strlen(ext) + 1;
    char *fileName = (char*)malloc(totalLength);
    snprintf(fileName, totalLength, "%s.%s.%s", sourceName, functionName, ext);

    char *outputFilePath;
    if (outputDir != NULL) {
        size_t dirLength = strlen(outputDir);
        if (outputDir[dirLength - 1] == '/') {
            dirLength--;
        }
        size_t pathLength = dirLength + 1 + strlen(fileName) + 1;
        outputFilePath = (char*)malloc(pathLength);
        snprintf(outputFilePath, pathLength, "%.*s/%s", (int)dirLength, outputDir, fileName);
    } else {
        char *pathCopy = strdup(sourceFilePath);
        char *dirName = dirname(pathCopy);
        size_t pathLength = strlen(dirName) + 1 + strlen(fileName) + 1;
        outputFilePath = (char*)malloc(pathLength);
        snprintf(outputFilePath, pathLength, "%s/%s", dirName, fileName);
        free(pathCopy);
    }

    free(sourceName);
    free(fileName);

    return outputFilePath;
}

char* getDirectory(const char* path) {
    if (path == NULL) {
        return NULL;
    }

    char* path_copy = strdup(path);
    if (path_copy == NULL) {
        return NULL;
    }

    char* last_slash = strrchr(path_copy, '/');
#ifdef _WIN32
    char* last_backslash = strrchr(path_copy, '\\');
    if (last_backslash != NULL && (last_slash == NULL || last_backslash > last_slash)) {
        last_slash = last_backslash;
    }
#endif

    if (last_slash != NULL) {
        *last_slash = '\0';
        char* directory = strdup(path_copy);
        free(path_copy);
        return directory;
    } else {
        free(path_copy);
        return NULL;
    }
}

int main(int argc, char *argv[]) {

    struct arguments arguments;

    arguments.debug = 0;
    arguments.ot = 0;
    arguments.output_dir = NULL;
    arguments.input_files = NULL;
    arguments.input_file_count = 0;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if (arguments.debug) {
        printf("Debug output is enabled\n");
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
        if (!result->isValid && arguments.debug) {
            printErrors(&result->errorContext);
        } 
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

    if (prog->warnings != NULL) {
      printf("Warnings:\n");
      ProgramWarningInfo *warning = prog->warnings;
      while (warning != NULL) {
        printf("%s\n", warning->message);
        warning = warning->next;
      }
    }

    FunctionInfo *func = prog->functions;
    const char *mainFileName = NULL;
    while (func != NULL) {
      if (strcmp(func->functionName, "main") == 0) {
        mainFileName = func->fileName;
      }
      char *outputFilePath = getOutputFileName(func->fileName, func->functionName, "dot", arguments.output_dir);
      writeCFGToDotFile(func->cfg, outputFilePath, arguments.ot);
      func = func->next;
      free(outputFilePath);
    }

    if (mainFileName == NULL) {
        fprintf(stderr, "Error: main function is not defined\n");
    }

    if (prog->errors == NULL && (mainFileName != NULL || arguments.output_dir != NULL)) {
        CallGraph *graph = (CallGraph *)malloc(sizeof(CallGraph));
        graph->functions = NULL;

        traverseProgramAndBuildCallGraph(prog, graph, arguments.debug);
        char* dir;
        char* path;
        if (arguments.output_dir != NULL) {
            dir = arguments.output_dir;
            path = concat(dir, "/cg.dot");
            writeCallGraphToDot(graph, path);
            free(path);
        } else if (mainFileName != NULL) {
            dir = getDirectory(mainFileName);
            path = concat(dir, "/cg.dot");
            writeCallGraphToDot(graph, path);
            free(path);
            free(dir);
        } else {
            fprintf(stderr, "Error: can't save CG to dot file because main function and output directory are not defined\n");
        }

        freeCallGraph(graph);
    }

    freeProgram(prog);

    for (uint32_t i = 0; i < files.filesCount; i++) {
        destroyMyLangResult(files.result[i]);
        free(files.result[i]);
    }
    free(arguments.input_files);
    free(files.result);
    return 0;
}
