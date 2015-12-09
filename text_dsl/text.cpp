#include "text.h"
#include "ast.h"

#include <stdio.h>
#include <cstdarg>
#include <stdexcept>
#include <cctype>
#include <assert.h>


int sprintf(std::string* str, const char* format, ...) {
  va_list args;
  va_start(args, format);
  int ret = vsprintf(str, format, args);
  va_end(args);
  return ret;
}

int vsprintf(std::string* str, const char* format, va_list args) {
  str->resize(1024);
  int sizeNeeded = vsnprintf(&str->front(), str->size(), format, args);
  if (sizeNeeded < 0) {       // on Windows, vsnprintf will return -1 if not enough size
    do {
      str->resize(2 * str->size());
      sizeNeeded = vsnprintf(&str->front(), str->size(), format, args);
    } while (sizeNeeded < 0);
  } else if (sizeNeeded + 1 > str->size()) { // on other platforms, vsnprintf will return the size required not including '\0'
    str->resize(sizeNeeded + 1);
    int n = vsnprintf(&str->front(), sizeNeeded + 1, format, args);
    assert(n == sizeNeeded);
  }
  return sizeNeeded;
}


static bool isSpace(char c) {
  return (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v');
}

// Should be called after any token is parsed so that fptr is moved to the start of the next token
static void parseWhitespaces(const char** fptr) {
  while (isSpace(**fptr)) {
    ++*fptr;
  }
}

static int parseUint(const char** fptr) {
  assert(std::isdigit(**fptr));
  int value = 0;
  do {
    int digit = **fptr - '0';
    value = value * 10 + digit;
    ++*fptr;
  } while (std::isdigit(**fptr));
  return value;
}

// Used for parsing the next char within a quote, surrounded by the specified quote character.
// Any character can be escaped by a preceding backslash; only the escaped character is returned.
static char parseCharInsideQuotes(const char** fptr, char quote) {
  assert(**fptr != quote);
  char c = **fptr;
  if (c == '\0') {
    throw DSLException(*fptr, "Reached end of string; expected char.");
  }
  ++*fptr;
  if (c == '\\') {
    // parse and return the character directly after the backslash.
    c = **fptr;
    ++*fptr;
  }
  return c;
}

static char parseCharLiteral(const char** fptr) {  // TOKEN
  assert(**fptr == '\'');
  ++*fptr;
  if (**fptr == '\'') {
    throw DSLException(*fptr, "Exected char literial; found '' instead.");
  }
  char c = parseCharInsideQuotes(&*fptr, '\'');
  if (**fptr != '\'') {
    throw DSLException(*fptr, "Expected closing ' for char literal.");
  }
  ++*fptr;
  parseWhitespaces(fptr);
  return c;
}

static FillerPtr parseStringLiteral(const char** fptr) { // TOKEN
  assert(**fptr == '\'');
  const char* f_at = *fptr;
  ++*fptr;
  std::string str;
  while (**fptr != '\'') {
    str += parseCharInsideQuotes(&*fptr, '\'');
  }
  ++*fptr;
  parseWhitespaces(fptr);
  return FillerPtr(new StringLiteral(f_at, str));
}

static LiteralLength parseLiteralLength(const char** fptr) {
  assert(std::isdigit(**fptr));
  LiteralLength ll(parseUint(fptr), false);
  if (**fptr == 's') {
    ll.shares = true;
    ++*fptr;
  }
  parseWhitespaces(fptr);
  return ll;
}

static FunctionLength parseFunctionLength(const char** fptr, const LengthFunc** lengthFuncsPtr) {
  assert(**fptr == '#');
  FunctionLength fl(**lengthFuncsPtr, false);
  ++*lengthFuncsPtr;
  ++*fptr;
  if (**fptr == 's') {
    fl.shares = true;
    ++*fptr;
  }
  parseWhitespaces(fptr);
  return fl;
}

// Parses 0 or more fillers
static void parseFillers(const char** fptr, std::vector<FillerPtr>* fillers) {
  while (**fptr == '\'' || std::isdigit(**fptr)) {
    FillerPtr filler;
    if (**fptr == '\'') {
      filler = parseStringLiteral(fptr);
    } else {
      const char* f_at = *fptr;
      LiteralLength length = parseLiteralLength(fptr);
      if (**fptr == '\'') {
        char c = parseCharLiteral(fptr);
        filler.reset(new RepeatedCharLL(f_at, length, c));
      } else {
        throw DSLException(*fptr, "Expected char literal after literal length.");
      }
    }
    fillers->push_back(std::move(filler));
  }
}

static ASTPtr parseRepeatedCharFL(const char** fptr, const LengthFunc** lengthFuncsPtr) {
  assert(**fptr == '#');
  const char* f_at = *fptr;
  ASTPtr ast;
  FunctionLength length = parseFunctionLength(fptr, lengthFuncsPtr);
  if (**fptr == '\'') {
    char c = parseCharLiteral(fptr);
    ast.reset(new RepeatedCharFL(f_at, length, c));
  } else {
    throw DSLException(*fptr, "Expected char literal after function length.");
  }
  return ast;
}

static char parseSilhouetteCharLiteral(const char** fptr) {
  assert(**fptr == '-');
  ++*fptr;
  if (**fptr != '>') {
    throw DSLException(*fptr, "Expected > immediately after -.");
  }
  ++*fptr;
  parseWhitespaces(fptr); // -> is a token
  if (**fptr != '\'') {
    throw DSLException(*fptr, "Expected char literal after ->.");
  }
  return parseCharLiteral(fptr);
}

static ASTPtr parseWords(const char** fptr, const char*** wordSourcesPtr) {
  assert(**fptr == '{');
  Words* words = new Words(*fptr, **wordSourcesPtr);
  ++*wordSourcesPtr;
  ASTPtr ast(words);
  ++*fptr;
  parseWhitespaces(fptr); // { is a token
  if (**fptr == 'w') {
    ++*fptr;
    parseWhitespaces(fptr); // w is a token
    if (**fptr == '-') {
      words->wordSilhouette = parseSilhouetteCharLiteral(fptr);
    }
    parseFillers(fptr, &words->interwordFillers);
  } else {
    throw DSLException(*fptr, "Expected w after {.");
  }
  if (**fptr != '}') {
    throw DSLException(*fptr, "Expected }, ->, or interword fillers.");
  }
  ++*fptr;
  parseWhitespaces(fptr); // } is a token
  return ast;
}

static void parseTopOrBottomFiller(const char** fptr, std::vector<FillerPtr>* fillers, bool top) {
  char firstChar = top ? '^' : 'v';
  assert(**fptr == firstChar);
  ++*fptr;
  parseWhitespaces(fptr); // ^, v are tokens
  if (**fptr != '{') {
    throw DSLException(*fptr, "Expected {.");
  }
  ++*fptr;
  parseWhitespaces(fptr); // { is a token
  parseFillers(fptr, fillers);
  if (**fptr != '}') {
    throw DSLException(*fptr, "Expected }.");
  }
  ++*fptr;
  parseWhitespaces(fptr); // } is a token
}


static ASTPtr parseSpecifiedLengthContent(const char** fptr, const char*** wordSourcesPtr, const LengthFunc** lengthFuncsPtr) {
  assert(**fptr == '\'' || std::isdigit(**fptr) || **fptr == '#');
  ASTPtr slc;
  if (**fptr == '\'') {
    slc = parseStringLiteral(fptr);
  } else if (**fptr == '#') {
    slc = parseRepeatedCharFL(fptr, lengthFuncsPtr);
  } else {
    const char* f_at = *fptr;
    LiteralLength length = parseLiteralLength(fptr);
    if (**fptr == '\'') {
      slc.reset(new RepeatedCharLL(f_at, length, parseCharLiteral(fptr)));
    } else if (**fptr == '[') {
      Block* block = new Block(f_at, length);
      slc.reset(block);
      ++*fptr;
      parseWhitespaces(fptr); // [ is a token
      while (**fptr != ']') {
        if (**fptr == '\'' || std::isdigit(**fptr) || **fptr == '#') {
          block->addChild(parseSpecifiedLengthContent(fptr, wordSourcesPtr, lengthFuncsPtr));
        } else if (**fptr == '{') {
          block->addWords(parseWords(fptr, wordSourcesPtr));
        } else {
          throw DSLException(*fptr, "Expected ', digit, or # to begin specified-length content, "
            "or { to begin greedy-length content.");
        }
      }
      ++*fptr;
      parseWhitespaces(fptr); // ] is a token
      if (**fptr == '^') {
        parseTopOrBottomFiller(fptr, &block->topFillers, true);
        if (**fptr == 'v') {
          parseTopOrBottomFiller(fptr, &block->bottomFillers, false);
        }
      } else if (**fptr == 'v') {
        parseTopOrBottomFiller(fptr, &block->bottomFillers, false);
        if (**fptr == '^') {
          parseTopOrBottomFiller(fptr, &block->topFillers, true);
        }
      }
    } else {
      throw DSLException(*fptr, "Expected ' or [ after length specifier.");
    }
  }
  return slc;
}

static ASTPtr parseFormat(const char** fptr, const char*** wordSourcesPtr, const LengthFunc** lengthFuncsPtr) {
  parseWhitespaces(fptr);
  // Will insert all root content as children into a super-root Block.
  Block* rootsParentBlock = new Block(*fptr, LiteralLength(0, false));
  ASTPtr rootsParent(rootsParentBlock);
  while (**fptr != '\0') {
    if (**fptr == '\'' || std::isdigit(**fptr)) {
      ASTPtr root = parseSpecifiedLengthContent(fptr, wordSourcesPtr, lengthFuncsPtr);
      int rootLength = root->getFixedLength();
      if (rootLength == UNKNOWN_COL) {
        throw DSLException(root->f_at, "Root content must be fixed-length.");
      }
      rootsParentBlock->addChild(std::move(root));
      rootsParentBlock->length.value += rootLength;
    } else {
      throw DSLException(*fptr, "Expected ' or digit.");
    }
  }
  return rootsParent;
}


static ASTPtr generateCCs(std::vector<ConsistentContent>* ccs, const char* format, const char*** wordSourcesPtr, const LengthFunc** lengthFuncsPtr, va_list args) {
  ccs->clear();
  std::string evaluatedFormat;
  vsprintf(&evaluatedFormat, format, args);
  const char* f_begin = evaluatedFormat.c_str();
  const char* f_at = f_begin;
  ASTPtr root;
  try {
    //printf("\n\n%s\n", format);

    root = parseFormat(&f_at, wordSourcesPtr, lengthFuncsPtr);
    root->convertLLSharesToLength();
    root->computeStartEndCols(0, root->getFixedLength());

    std::vector<FillerPtr> topFillersStack, bottomFillersStack;
    root->flatten(root, root, ccs, true, &topFillersStack, &bottomFillersStack);
    //root->print();
    //printf("\n");

    for (ConsistentContent& cc : *ccs) {
      cc.generateCCLines();
    }

    root->computeNumContentLines();
    root->computeNumTotalLines(true);
    root->computeBlockVerticalFillersShares();

    //printf("\n");
    for (ConsistentContent& cc : *ccs) {
      /*printf("\n");
      cc.print();
      printf("\n");
      printf("content: %d  fixed: %d  total: %d\n", cc.srcAst->numContentLines, cc.srcAst->numFixedLines, cc.srcAst->numTotalLines);*/
      cc.generateLinesChars(root->numTotalLines);
    }
    //printf("\n\n");

  } catch (DSLException& e) {
    fprintf(stderr, "%s\n", f_begin);
    for (int i = 0; i < e.f_at - f_begin; ++i) {
      fputc(' ', stderr);
    }
    fprintf(stderr, "^\n");
    fprintf(stderr, "Error at %d: %s\n", e.f_at - f_begin, e.what());
    
    return ASTPtr();
  }

  return root;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------

void text_printf(const char* format, const char** wordSources, const LengthFunc* lengthFuncs, ...) {
  va_list args;
  va_start(args, lengthFuncs);
  std::vector<ConsistentContent> ccs;
  ASTPtr root = generateCCs(&ccs, format, &wordSources, &lengthFuncs, args);
  va_end(args);
  if (!root) {
    return;
  }

  for (ConsistentContent& cc : ccs) {
    cc.printContentLine(stdout, 0, root->numTotalLines);
  }
  for (int i = 1; i < root->numTotalLines; ++i) {
    printf("\n");
    for (ConsistentContent& cc : ccs) {
      cc.printContentLine(stdout, i, root->numTotalLines);
    }
  }
}

void text_fprintf(FILE* stream, const char* format, const char** wordSources, const LengthFunc* lengthFuncs, ...) {
  va_list args;
  va_start(args, lengthFuncs);  
  std::vector<ConsistentContent> ccs;
  ASTPtr root = generateCCs(&ccs, format, &wordSources, &lengthFuncs, args);
  va_end(args);
  if (!root) {
    return;
  }

  for (ConsistentContent& cc : ccs) {
    cc.printContentLine(stream, 0, root->numTotalLines);
  }
  for (int i = 1; i < root->numTotalLines; ++i) {
    fprintf(stream, "\n");
    for (ConsistentContent& cc : ccs) {
      cc.printContentLine(stream, i, root->numTotalLines);
    }
  }
}

void text_sprintf(std::string* str, const char* format, const char** wordSources, const LengthFunc* lengthFuncs, ...) {
  va_list args;
  va_start(args, lengthFuncs);
  std::vector<ConsistentContent> ccs;
  ASTPtr root = generateCCs(&ccs, format, &wordSources, &lengthFuncs, args);
  va_end(args);
  if (!root) {
    return;
  }

  int rootNumCols = root->endCol - root->startCol;
  str->resize((rootNumCols + 1) * root->numTotalLines - 1);
  char* bufAt = &str->front();
  for (ConsistentContent& cc : ccs) {
    cc.printContentLine(&bufAt, 0, root->numTotalLines);
  }
  for (int i = 1; i < root->numTotalLines; ++i) {
    *bufAt = '\n';
    ++bufAt;
    for (ConsistentContent& cc : ccs) {
      cc.printContentLine(&bufAt, i, root->numTotalLines);
    }
  }  
}

void text_sprintf_lines(std::vector<std::string>* lines, const char* format, const char** wordSources, const LengthFunc* lengthFuncs, ...) {
  va_list args;
  va_start(args, lengthFuncs);
  std::vector<ConsistentContent> ccs;
  ASTPtr root = generateCCs(&ccs, format, &wordSources, &lengthFuncs, args);
  va_end(args);
  if (!root) {
    return;
  }

  int rootNumCols = root->endCol - root->startCol;
  lines->resize(root->numTotalLines);
  for (int lineNum = 0; lineNum < root->numTotalLines; ++lineNum) {
    std::string& line = lines->at(lineNum);
    line.resize(rootNumCols);
    char* bufAt = &line.front();
    for (int i = 0; i < ccs.size(); ++i) {
      ccs[i].printContentLine(&bufAt, lineNum, root->numTotalLines);
    }
  }
}

void text_sprintf_lines_append(std::vector<std::string>* lines, const char* format, const char** wordSources, const LengthFunc* lengthFuncs, ...) {
  va_list args;
  va_start(args, lengthFuncs);
  std::vector<ConsistentContent> ccs;
  ASTPtr root = generateCCs(&ccs, format, &wordSources, &lengthFuncs, args);
  va_end(args);
  if (!root) {
    return;
  }

  int rootNumCols = root->endCol - root->startCol;
  lines->reserve(lines->size() + root->numTotalLines);
  for (int lineNum = 0; lineNum < root->numTotalLines; ++lineNum) {
    lines->push_back(std::string());
    std::string& line = lines->back();
    line.resize(rootNumCols);
    char* bufAt = &line.front();
    for (int i = 0; i < ccs.size(); ++i) {
      ccs[i].printContentLine(&bufAt, lineNum, root->numTotalLines);
    }
  }
}
