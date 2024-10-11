#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errorsUtils/errorUtils.h"

void initErrorContext(ErrorContext *context) {
  context->errorCount = 0;
  context->head = NULL;
}

void destroyErrorContext(ErrorContext *context) {
  ErrorNode *current = context->head;
  ErrorNode *next;
  while (current != NULL) {
    next = current->next;
    free(current->errorText);
    free(current->errTokenText);
    free(current);
    current = next;
  }
  context->head = NULL;
  context->errorCount = 0;
}

void addError(ErrorContext *context, const char *errorMsg,
              unsigned int errorLine, int errPosInLine,
              const char *errTokenText) {
  ErrorNode *newNode = (ErrorNode *)malloc(sizeof(ErrorNode));
  newNode->errorText = strdup(errorMsg);
  newNode->errorLine = errorLine;
  newNode->errPosInLine = errPosInLine;
  newNode->errTokenText = strdup(errTokenText);
  newNode->next = NULL;

  if (context->head == NULL) {
    context->head = newNode;
  } else {
    ErrorNode *current = context->head;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = newNode;
  }

  context->errorCount++;
}

void printErrors(ErrorContext *context) {
  ErrorNode *current = context->head;
  int i = 1;
  while (current != NULL) {
    fprintf(stderr, "Error %d in line %u at %d in token '%s': %s\n", i,
           current->errorLine, current->errPosInLine,
           current->errTokenText, current->errorText);
    current = current->next;
    i++;
  }
}

void extractRecognitionError(pANTLR3_BASE_RECOGNIZER recognizer,
                             pANTLR3_UINT8 *tokenNames) {
  pANTLR3_EXCEPTION exception = recognizer->state->exception;

  pANTLR3_UINT8 errMsg = (pANTLR3_UINT8)exception->message;
  ANTLR3_UINT32 errLine = exception->line;
  ANTLR3_INT32 errPosInLine = exception->charPositionInLine;
  pANTLR3_COMMON_TOKEN errToken = (pANTLR3_COMMON_TOKEN)(exception->token);
  pANTLR3_STRING errTokenText = errToken->toString(errToken);

  ErrorContext *context = (ErrorContext *)(recognizer->state->userp);

  addError(context, (const char *)errMsg, (unsigned int)errLine,
           (int)errPosInLine, (const char *)errTokenText->chars);
}

void reportLexerError(pANTLR3_BASE_RECOGNIZER recognizer) {
  // IGNORE
}