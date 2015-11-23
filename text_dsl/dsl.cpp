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

// Used to specify vertical spacing fillers and interword fillers.
RepeatedCharPtr parseRepeatedChar(const char** fptr, va_list* args) {
  assert(std::isdigit(**fptr) || **fptr == '#' || **fptr == '\'');
  SpecifiedLengthPtr sl(new LiteralLength(1, false));
  if (std::isdigit(**fptr) || **fptr == '#') {
    sl = parseSpecifiedLength(fptr, args);
  }
  if (**fptr != '\'') {
    throw std::runtime_error("Expected char literal.");
  }
  char c = parseCharLiteral(fptr);
  return RepeatedCharPtr(new RepeatedChar(std::move(sl), c));
}

RepeatedCharPtr parseTopOrBottomFiller(const char** fptr, va_list* args, bool top) {
  char firstChar = top ? '^' : 'v';
  assert(**fptr == firstChar);
  ++*fptr;
  parseWhitespaces(fptr); // ^, v are tokens
  if (!(std::isdigit(**fptr) || **fptr == '#')) {   //|| **fptr == '\'')) {
    throw std::runtime_error("Expected # or digit at start of vertical filler specifier.");
  }
  return parseRepeatedChar(fptr, args);
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


GreedyLengthContentPtr parseGreedyLengthContent(const char** fptr, va_list* args) {
  assert(**fptr == '{');
  ++*fptr;
  parseWhitespaces(fptr); // { is a token
  GreedyLengthContentPtr glc;
  if (**fptr == '\'') {
    char c = parseCharLiteral(fptr);
    glc = GreedyLengthContentPtr(new GreedyRepeatedChar(c));
  } else if (**fptr == 'w') {
    ++*fptr;
    parseWhitespaces(fptr); // w is a token
    char wordSilhouette = '\0';
    RepeatedCharPtr interword(new RepeatedChar(SpecifiedLengthPtr(new LiteralLength(1, false)), ' '));
    if (**fptr == '-') {
      wordSilhouette = parseSilhouetteCharLiteral(fptr);
    }
    if (std::isdigit(**fptr) || **fptr == '#' || **fptr == '\'') {
      interword = parseRepeatedChar(fptr, args);
    }
    char* wordsSource = va_arg(*args, char*);
    glc = GreedyLengthContentPtr(new Words(std::string(wordsSource), std::move(interword), wordSilhouette));
  } else {
    throw std::runtime_error("Expected ' or w after {.");
  }
  if (**fptr != '}') {
    throw std::runtime_error("Expected }.");
  }
  ++*fptr;
  parseWhitespaces(fptr); // } is a token
  return glc;
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
      slc = SpecifiedLengthContentPtr(new RepeatedChar(std::move(sl), c));
    } else if (**fptr == '[') {
      ++*fptr;
      parseWhitespaces(fptr); // [ is a token
      std::vector<SpecifiedLengthContentPtr> slcs;
      GreedyLengthContentPtr glc;
      int greedyChildIndex = -1;
      RepeatedCharPtr topFill(new RepeatedChar(SpecifiedLengthPtr(new LiteralLength(0, false)), ' '));
      RepeatedCharPtr bottomFill(new RepeatedChar(SpecifiedLengthPtr(new LiteralLength(0, false)), ' '));
      while (**fptr != ']') {
        if (**fptr == '\'' || std::isdigit(**fptr) || **fptr == '#') {
          slcs.push_back(parseSpecifiedLengthContent(fptr, args));
        } else if (**fptr == '{') {
          if (glc) {
            throw std::runtime_error("Cannot have multiple greedy-content within a block.");
          }
          glc = parseGreedyLengthContent(fptr, args);
          greedyChildIndex = (int)slcs.size();
        } else {
          throw std::runtime_error("Expected ', digit, or # to begin specified-length content, "
            "or { to begin greedy-length content.");
        }
      }
      ++*fptr;
      parseWhitespaces(fptr); // ] is a token
      if (**fptr == '^') {
        topFill = parseTopOrBottomFiller(fptr, args, true);
        if (**fptr == 'v') {
          bottomFill = parseTopOrBottomFiller(fptr, args, false);
        }
      } else if (**fptr == 'v') {
        bottomFill = parseTopOrBottomFiller(fptr, args, false);
        if (**fptr == '^') {
          topFill = parseTopOrBottomFiller(fptr, args, true);
        }
      }
      slc.reset(new Block(std::move(sl), std::move(slcs), std::move(glc), greedyChildIndex,
        std::move(topFill), std::move(bottomFill)));
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

