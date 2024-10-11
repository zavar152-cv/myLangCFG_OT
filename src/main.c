#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <argp.h>

#include "grammar/myLang.h"
#include "dotUtils/dotUtils.h"

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
  struct arguments arguments;
  arguments.debug = false;
  arguments.inputFile = NULL;
  arguments.outputFile = NULL;

  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  MyLangResult result;
  parseMyLangFromFile(&result, arguments.inputFile, arguments.debug);

  if (!result.isValid) {
    printErrors(&result.errorContext);
  } 

  int err = generateDotFileFromMyTree(result.tree, arguments.outputFile, arguments.debug);

  if (err == FILE_ERROR) {
    fprintf(stderr, "Error opening file %s for writing.\n", arguments.outputFile);
  }
  
  destroyMyLangResult(&result);

  return 0;
}
