#include <antlr3.h>

#define MAX_ERRORS 50

typedef struct ErrorNode {
  char *errorText;
  unsigned int errorLine;
  int errPosInLine;
  char *errTokenText;
  struct ErrorNode *next;
} ErrorNode;

typedef struct __attribute__((packed)) ErrorContext {
  unsigned int errorCount;
  ErrorNode *head;
} ErrorContext;

void initErrorContext(ErrorContext *context);

void destroyErrorContext(ErrorContext *context);

void addError(ErrorContext *context, const char *errorMsg,
              unsigned int errorLine, int errPosInLine, const char *errTokenText);

void printErrors(ErrorContext *context);

void extractRecognitionError(pANTLR3_BASE_RECOGNIZER recognizer,
                         pANTLR3_UINT8 *tokenNames);

void reportLexerError(pANTLR3_BASE_RECOGNIZER recognizer);