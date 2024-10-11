#include "grammar/myLang.h"
#include "MyLangLexer.h"
#include "MyLangParser.h"
#include <antlr3.h>
#include <stdbool.h>

void parseMyLangFromFile(MyLangResult *result, char *filename, bool debug) {
  pMyLangLexer lex;
  pANTLR3_COMMON_TOKEN_STREAM tokens;
  pMyLangParser parser;

  pANTLR3_INPUT_STREAM input =
      antlr3FileStreamNew((pANTLR3_UINT8)filename, ANTLR3_ENC_8BIT);

  lex = MyLangLexerNew(input);
  lex->pLexer->rec->reportError = reportLexerError;

  tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lex));

  initErrorContext(&result->errorContext);
  result->isValid = false;
  result->tree = NULL;

  parser = MyLangParserNew(tokens);
  parser->pParser->rec->state->userp = &result->errorContext;
  parser->pParser->rec->displayRecognitionError = extractRecognitionError;

  MyLangParser_source_return r = parser->source(parser);
  MyAstNode* myTree = createMyTreeFromAntlrTree(r.tree, 0, debug);
  result->tree = myTree;
  result->isValid = parser->pParser->rec->state->errorCount == 0;

  parser->free(parser);
  tokens->free(tokens);
  lex->free(lex);
  input->close(input);
}

void parseMyLangFromText(MyLangResult *result, const char *text, bool debug) {
  pMyLangLexer lex;
  pANTLR3_COMMON_TOKEN_STREAM tokens;
  pMyLangParser parser;

  pANTLR3_INPUT_STREAM input =
      antlr3StringStreamNew((pANTLR3_UINT8)text, ANTLR3_ENC_8BIT, strlen(text), (pANTLR3_UINT8)"QueryString");

  lex = MyLangLexerNew(input);
  lex->pLexer->rec->reportError = reportLexerError;

  tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lex));

  initErrorContext(&result->errorContext);
  result->isValid = false;
  result->tree = NULL;

  parser = MyLangParserNew(tokens);
  parser->pParser->rec->state->userp = &result->errorContext;
  parser->pParser->rec->displayRecognitionError = extractRecognitionError;

  MyLangParser_source_return r = parser->source(parser);
  MyAstNode* myTree = createMyTreeFromAntlrTree(r.tree, 0, debug);
  result->tree = myTree;
  result->isValid = parser->pParser->rec->state->errorCount == 0;

  parser->free(parser);
  tokens->free(tokens);
  lex->free(lex);
  input->close(input);  
}

void destroyMyLangResult(MyLangResult *result) {
  destroyMyAstNodeTree(result->tree);
  destroyErrorContext(&result->errorContext);
}