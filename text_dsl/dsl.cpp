#include "dsl.h"
#include "ast.h"

#include <stdio.h>
#include <cstdarg>
#include <stdexcept>
#include <cctype>
#include <assert.h>

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

// Used for parsing the next char within a char or string literal (denoted by single-quotes)
// Can escape (with backslash) an apostrophe or a backslash; all other characters parsed as-is.
static char parseCharInsideLiteral(const char** fptr) {
  assert(**fptr != '\'');
  char c = **fptr;
  if (c == '\0') {
    throw DSLException(*fptr, "Reached end of string; expected char.");
  }
  ++*fptr;
  if (c == '\\') {
    // If the backslash is followed by an apostrophe or backslash, then both are parsed and the
    // second character is returned. Otherwise, only the backslash is parsed and returned as-is.
    if (**fptr == '\'' || **fptr == '\\') {
      c = **fptr;
      ++*fptr;
    }
  }
  return c;
}

static char parseCharLiteral(const char** fptr) {  // TOKEN
  assert(**fptr == '\'');
  ++*fptr;
  if (**fptr == '\'') {
    throw DSLException(*fptr, "Exected char literial; found '' instead.");
  }
  char c = parseCharInsideLiteral(&*fptr);
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
  const char* strBegin = *fptr;
  std::string str;
  while (**fptr != '\'') {
    str += parseCharInsideLiteral(&*fptr);
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

static FunctionLength parseFunctionLength(const char** fptr, va_list* args) {
  assert(**fptr == '#');
  FunctionLength fl(va_arg(*args, LengthFunc), false);
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

static ASTPtr parseRepeatedCharFL(const char** fptr, va_list* args) {
  assert(**fptr == '#');
  const char* f_at = *fptr;
  ASTPtr ast;
  FunctionLength length = parseFunctionLength(fptr, args);
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

static ASTPtr parseWords(const char** fptr, va_list* args) {
  assert(**fptr == '{');
  Words* words = new Words(*fptr, va_arg(*args, char*));
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


static ASTPtr parseSpecifiedLengthContent(const char** fptr, va_list* args) {
  assert(**fptr == '\'' || std::isdigit(**fptr) || **fptr == '#');
  ASTPtr slc;
  if (**fptr == '\'') {
    slc = parseStringLiteral(fptr);
  } else if (**fptr == '#') {
    slc = parseRepeatedCharFL(fptr, args);
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
          block->addChild(parseSpecifiedLengthContent(fptr, args));
        } else if (**fptr == '{') {
          block->addWords(parseWords(fptr, args));
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

static ASTPtr parseFormat(const char** fptr, va_list* args) {
  parseWhitespaces(fptr);
  ASTPtr root;
  // top-level node must be some specified-length content other than repeated char with function length.
  if (**fptr == '\'' || std::isdigit(**fptr)) {
    root = parseSpecifiedLengthContent(fptr, args);
  } else {
    throw DSLException(*fptr, "Expected ' or digit.");
  }
  if (**fptr != '\0') {
    throw DSLException(*fptr, "Unexpected character before end of format string.");
  }
  /*if (root->type != BLOCK) {
    throw DSLException(root->f_at, "Root content must be a block.");
  }*/
  if (root->getFixedLength() == UNKNOWN_COL) {
    throw DSLException(root->f_at, "Root content must be fixed-length.");
  }
  return root;
}


static ASTPtr generateCCs(std::vector<ConsistentContent>* ccs, const char* format, va_list* args) {
  ccs->clear();
  const char* f_at = format;
  ASTPtr root;
  try {
    //printf("\n\n%s\n", format);

    root = parseFormat(&f_at, args);
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
    fprintf(stderr, "%s\n", format);
    for (int i = 0; i < e.f_at - format; ++i) {
      fputc(' ', stderr);
    }
    fprintf(stderr, "^\n");
    fprintf(stderr, "Error at %d: %s\n", e.f_at - format, e.what());
  }

  return root;
}

void dsl_fprintf(FILE* stream, const char* format, ...) {
  va_list args;
  va_start(args, format);
  std::vector<ConsistentContent> ccs;
  ASTPtr root = generateCCs(&ccs, format, &args);
  va_end(args);
    
  for (int i = 0; i < root->numTotalLines; ++i) {
    for (ConsistentContent& cc : ccs) {
      cc.printContentLine(stream, i, root->numTotalLines);
    }
    fprintf(stream, "\n");
  }
}

void dsl_sprintf(std::string* str, const char* format, ...) {
  va_list args;
  va_start(args, format);
  std::vector<ConsistentContent> ccs;
  ASTPtr root = generateCCs(&ccs, format, &args);
  va_end(args);

  int rootNumCols = root->endCol - root->startCol;
  str->resize((rootNumCols + 1) * root->numTotalLines);
  char* bufAt = &str->front();
  for (int i = 0; i < root->numTotalLines; ++i) {
    for (ConsistentContent& cc : ccs) {
      cc.printContentLine(&bufAt, i, root->numTotalLines);
    }
    *bufAt = '\n';
    ++bufAt;
  }  
}

void dsl_sprintf_lines(std::vector<std::string>* lines, const char* format, ...) {
  va_list args;
  va_start(args, format);
  std::vector<ConsistentContent> ccs;
  ASTPtr root = generateCCs(&ccs, format, &args);
  va_end(args);

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

void dsl_sprintf_lines_append(std::vector<std::string>* lines, const char* format, ...) {
  va_list args;
  va_start(args, format);
  std::vector<ConsistentContent> ccs;
  ASTPtr root = generateCCs(&ccs, format, &args);
  va_end(args);

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
