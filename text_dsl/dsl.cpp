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

SpecifiedLengthContentPtr parseStringLiteral(const char** fptr) { // TOKEN
  assert(**fptr == '\'');
  ++*fptr;
  const char* strBegin = *fptr;
  std::string str;
  while (**fptr != '\'') {
    str += parseCharInsideLiteral(&*fptr);
  }
  ++*fptr;
  parseWhitespaces(fptr);
  return SpecifiedLengthContentPtr(new StringLiteral(str));
}


SpecifiedLengthPtr parseSpecifiedLength(const char** fptr, va_list* args) { // TOKEN
  assert(std::isdigit(**fptr) || **fptr == '#');
  SpecifiedLengthPtr sl;
  if (**fptr == '#') {
    sl.reset(new FunctionLength(va_arg(*args, LengthFunc)));
    ++*fptr;
  } else {
    sl.reset(new LiteralLength(parseUint(&*fptr)));
  }
  if (**fptr == 's') {
    sl->shares = true;
    ++*fptr;
  }
  parseWhitespaces(fptr);
  return sl;
}

// A RepeatedChar with literal length. Can also just be a char literal, which has implied length 1
Filler parseFiller(const char** fptr, va_list* args) {
  assert(std::isdigit(**fptr) || **fptr == '\'');
  int length = 1;
  bool shares = false;
  if (std::isdigit(**fptr)) {
    length = parseUint(fptr);
    if (**fptr == 's') {
      ++*fptr;
      shares = true;
    }
    parseWhitespaces(fptr); // literal length is a token
  }
  if (**fptr != '\'') {
    throw std::runtime_error("Expected char literal.");
  }
  char c = parseCharLiteral(fptr);
  return Filler(length, shares, c);
}

Filler parseTopOrBottomFiller(const char** fptr, va_list* args, bool top) {
  char firstChar = top ? '^' : 'v';
  assert(**fptr == firstChar);
  ++*fptr;
  parseWhitespaces(fptr); // ^, v are tokens
  // The shorthand version of the filler where the length is dropped is not allowed.
  if (!(std::isdigit(**fptr))) {  // || **fptr == '\'')) {
    throw std::runtime_error("Expected digit at start of vertical filler specifier.");
  }
  return parseFiller(fptr, args);
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


Words parseGreedyLengthContent(const char** fptr, va_list* args) {
  assert(**fptr == '{');
  ++*fptr;
  parseWhitespaces(fptr); // { is a token
  Words words;
  if (**fptr == 'w') {
    ++*fptr;
    parseWhitespaces(fptr); // w is a token
    if (**fptr == '-') {
      words.wordSilhouette = parseSilhouetteCharLiteral(fptr);
    }
    if (std::isdigit(**fptr) || **fptr == '\'') {
      words.interword = parseFiller(fptr, args);
    }
    words.source = std::string(va_arg(*args, char*));
  } else {
    throw std::runtime_error("Expected w after {.");
  }
  if (**fptr != '}') {
    throw std::runtime_error("Expected }.");
  }
  ++*fptr;
  parseWhitespaces(fptr); // } is a token
  return words;
}

SpecifiedLengthContentPtr parseSpecifiedLengthContent(const char** fptr, va_list* args) {
  assert(**fptr == '\'' || std::isdigit(**fptr) || **fptr == '#');
  SpecifiedLengthContentPtr slc;
  if (**fptr == '\'') {
    slc = parseStringLiteral(fptr);
  } else {
    SpecifiedLengthPtr sl = parseSpecifiedLength(fptr, args);
    if (**fptr == '\'') {
      char c = parseCharLiteral(fptr);
      slc.reset(new RepeatedChar(std::move(sl), c));
    } else if (**fptr == '[') {
      ++*fptr;
      parseWhitespaces(fptr); // [ is a token
      Block* block = new Block(std::move(sl));
      slc.reset(block);
      while (**fptr != ']') {
        if (**fptr == '\'' || std::isdigit(**fptr) || **fptr == '#') {
          block->addChild(parseSpecifiedLengthContent(fptr, args));
        } else if (**fptr == '{') {
          if (block->hasGreedyChild()) {
            throw std::runtime_error("Cannot have multiple greedy-content within a block.");
          }
          block->addGreedyChild(parseGreedyLengthContent(fptr, args));
        } else {
          throw std::runtime_error("Expected ', digit, or # to begin specified-length content, "
            "or { to begin greedy-length content.");
        }
      }
      ++*fptr;
      parseWhitespaces(fptr); // ] is a token
      if (**fptr == '^') {
        block->topFiller = parseTopOrBottomFiller(fptr, args, true);
        if (**fptr == 'v') {
          block->bottomFiller = parseTopOrBottomFiller(fptr, args, false);
        }
      } else if (**fptr == 'v') {
        block->bottomFiller = parseTopOrBottomFiller(fptr, args, false);
        if (**fptr == '^') {
          block->topFiller = parseTopOrBottomFiller(fptr, args, true);
        }
      }
    } else {
      throw std::runtime_error("Expected ' or [ after length specifier.");
    }
  }
  return slc;
}

SpecifiedLengthContentPtr parseFormat(const char** fptr, va_list* args) {
  parseWhitespaces(fptr);
  SpecifiedLengthContentPtr root = parseSpecifiedLengthContent(fptr, args);
  if (**fptr != '\0') {
    throw std::runtime_error("Unexpected character before end of format string.");
  }
  if (root->length->shares) {
    throw std::runtime_error("Outermost block must not have shares for length.");
  }
  return root;
}

void dsl_printf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  const char* f_at = format;
  SpecifiedLengthContentPtr root;
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

