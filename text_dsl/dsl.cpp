#include "dsl.h"
#include "ast.h"

#include <stdio.h>
#include <cstdarg>
#include <stdexcept>
#include <cctype>
#include <assert.h>

// Should be called after any token is parsed so that fptr is moved to the start of the next token
void parseWhitespaces(const char** fptr) {
  while (std::isspace(**fptr)) {
    ++*fptr;
  }
}

int parseUint(const char** fptr) {
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
char parseCharInsideLiteral(const char** fptr) {
  assert(**fptr != '\'');
  char c = **fptr;
  if (c == '\0') {
    throw std::runtime_error("Reached end of string; expected char.");
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

char parseCharLiteral(const char** fptr) {  // TOKEN
  assert(**fptr == '\'');
  ++*fptr;
  if (**fptr == '\'') {
    throw std::runtime_error("Exected char literial; found '' instead.");
  }
  char c = parseCharInsideLiteral(&*fptr);
  if (**fptr != '\'') {
    throw std::runtime_error("Expected closing ' for char literal.");
  }
  ++*fptr;
  parseWhitespaces(fptr);
  return c;
}

FillerPtr parseStringLiteral(const char** fptr) { // TOKEN
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

LiteralLength parseLiteralLength(const char** fptr) {
  assert(std::isdigit(**fptr));
  LiteralLength ll(parseUint(fptr), false);
  if (**fptr == 's') {
    ll.shares = true;
    ++*fptr;
  }
  parseWhitespaces(fptr);
  return ll;
}

FunctionLength parseFunctionLength(const char** fptr, va_list* args) {
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
void parseFillers(const char** fptr, std::vector<FillerPtr>* fillers) {
  while (**fptr == '\'' || std::isdigit(**fptr)) {
    FillerPtr filler;
    if (**fptr == '\'') {
      filler = parseStringLiteral(fptr);
    } else {
      const char* f_at = *fptr;
      LiteralLength length = parseLiteralLength(fptr);
      if (**fptr == '\'') {
        char c = parseCharLiteral(fptr);
        filler.reset(new RepeatedChar(f_at, length, c));
      } else {
        throw std::runtime_error("Expected char literal after literal length.");
      }
    }
    fillers->push_back(std::move(filler));
  }
}

ASTPtr parseRepeatedCharFuncLength(const char** fptr, va_list* args) {
  assert(**fptr == '#');
  const char* f_at = *fptr;
  ASTPtr ast;
  FunctionLength length = parseFunctionLength(fptr, args);
  if (**fptr == '\'') {
    char c = parseCharLiteral(fptr);
    ast.reset(new RepeatedCharFuncLength(f_at, length, c));
  } else {
    throw std::runtime_error("Expected char literal after function length.");
  }
  return ast;
}

char parseSilhouetteCharLiteral(const char** fptr) {
  assert(**fptr == '-');
  ++*fptr;
  if (**fptr != '>') {
    throw std::runtime_error("Expected > immediately after -.");
  }
  ++*fptr;
  parseWhitespaces(fptr); // -> is a token
  if (**fptr != '\'') {
    throw std::runtime_error("Expected char literal after ->.");
  }
  return parseCharLiteral(fptr);
}

ASTPtr parseWords(const char** fptr, va_list* args) {
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
    throw std::runtime_error("Expected w after {.");
  }
  if (**fptr != '}') {
    throw std::runtime_error("Expected }, ->, or interword fillers.");
  }
  ++*fptr;
  parseWhitespaces(fptr); // } is a token
  return ast;
}

void parseTopOrBottomFiller(const char** fptr, std::vector<FillerPtr>* fillers, bool top) {
  char firstChar = top ? '^' : 'v';
  assert(**fptr == firstChar);
  ++*fptr;
  parseWhitespaces(fptr); // ^, v are tokens
  if (**fptr != '{') {
    throw std::runtime_error("Expected {.");
  }
  ++*fptr;
  parseWhitespaces(fptr); // { is a token
  parseFillers(fptr, fillers);
  if (**fptr != '}') {
    throw std::runtime_error("Expected }.");
  }
  ++*fptr;
  parseWhitespaces(fptr); // } is a token
}


ASTPtr parseSpecifiedLengthContent(const char** fptr, va_list* args) {
  assert(**fptr == '\'' || std::isdigit(**fptr) || **fptr == '#');
  ASTPtr slc;
  if (**fptr == '\'') {
    slc = parseStringLiteral(fptr);
  } else if (**fptr == '#') {
    slc = parseRepeatedCharFuncLength(fptr, args);
  } else {
    LiteralLength length = parseLiteralLength(fptr);
    if (**fptr == '\'') {
      slc.reset(new RepeatedChar(*fptr, length, parseCharLiteral(fptr)));
    } else if (**fptr == '[') {
      Block* block = new Block(*fptr, length);
      slc.reset(block);
      ++*fptr;
      parseWhitespaces(fptr); // [ is a token
      while (**fptr != ']') {
        if (**fptr == '\'' || std::isdigit(**fptr) || **fptr == '#') {
          block->addChild(parseSpecifiedLengthContent(fptr, args));
        } else if (**fptr == '{') {
          if (block->hasGreedyChild()) {
            throw std::runtime_error("Cannot have multiple greedy-content within a block.");
          }
          block->addGreedyChild(parseWords(fptr, args));
        } else {
          throw std::runtime_error("Expected ', digit, or # to begin specified-length content, "
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
      throw std::runtime_error("Expected ' or [ after length specifier.");
    }
  }
  return slc;
}

ASTPtr parseFormat(const char** fptr, va_list* args) {
  parseWhitespaces(fptr);
  ASTPtr root;
  // top-level node must be some specified-length content other than repeated char with function length.
  if (**fptr == '\'' || std::isdigit(**fptr)) {
    root = parseSpecifiedLengthContent(fptr, args);
  } else {
    throw std::runtime_error("Expected ' or digit.");
  }
  if (**fptr != '\0') {
    throw std::runtime_error("Unexpected character before end of format string.");
  }
  return root;
}

void dsl_printf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  const char* f_at = format;
  ASTPtr root;
  try {
    root = parseFormat(&f_at, &args);

    root->print();
    printf("\n");

  } catch (std::exception& e) {
    printf("%s\n", format);
    for (int i = 0; i < f_at - format; ++i) {
      putchar(' ');
    }
    printf("^\n");
    printf("Error at %d: %s\n", f_at - format, e.what());
  }
  va_end(args);


}

