#include "dsl.h"
#include "ast.h"

#include <stdio.h>
#include <cstdarg>
#include <stdexcept>
#include <cctype>
#include <assert.h>


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
// Can escape an apostrophe; all other characters parsed as-is.
char parseCharInsideLiteral(const char** fptr) {
  assert(**fptr != '\'');
  char c = **fptr;
  if (c == '\0') {
    throw std::runtime_error("Reached end of string; expected char.");
  }
  ++*fptr;
  if (c == '\\') {
    // If the escape is followed by an apostrophe, then they're both parsed to return an apostrophe.
    // Otherwise, only the backslash character is parsed and returned.
    if (**fptr == '\'') {
      c = '\'';
      ++*fptr;
    }
  }
  return c;
}

char parseCharLiteral(const char** fptr) {
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
  return c;
}

SpecifiedLengthContentPtr parseStringLiteral(const char** fptr) {
  assert(**fptr == '\'');
  ++*fptr;
  const char* strBegin = *fptr;
  std::string str;
  while (**fptr != '\'') {
    str += parseCharInsideLiteral(&*fptr);
  }
  ++*fptr;
  return SpecifiedLengthContentPtr(new StringLiteral(str));
}


SpecifiedLengthPtr parseSpecifiedLength(const char** fptr, va_list* args) {
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
  return sl;
}

// Used to specify vertical spacing fillers and interword fillers.
RepeatedChar parseRepeatedChar(const char** fptr, va_list* args) {
  assert(std::isdigit(**fptr) || **fptr == '#');
  SpecifiedLengthPtr sl = parseSpecifiedLength(fptr, args);
  if (**fptr != '\'') {
    throw std::runtime_error("Expected char literal after length specifier.");
  }
  char c = parseCharLiteral(fptr);
  return RepeatedChar(std::move(sl), c);
}


char parseSilhouetteCharLiteral(const char** fptr) {
  assert(**fptr == '-');
  ++*fptr;
  if (**fptr != '>') {
    throw std::runtime_error("Expected > after -.");
  }
  ++*fptr;
  if (**fptr != '\'') {
    throw std::runtime_error("Expected ' after >");
  }
  return parseCharLiteral(fptr);
}


GreedyLengthContentPtr parseGreedyLengthContent(const char** fptr, va_list* args) {
  assert(**fptr == '{');
  ++*fptr;
  GreedyLengthContentPtr glc;
  parseWhitespaces(fptr);
  if (**fptr == '\'') {
    char c = parseCharLiteral(fptr);
    glc = GreedyLengthContentPtr(new GreedyRepeatedChar(c));
    parseWhitespaces(fptr);
  } else if (**fptr == 'w') {
    ++*fptr;
    char wordSilhouette = '\0';
    RepeatedChar interword(SpecifiedLengthPtr(new LiteralLength(1, false)), ' ');
    if (**fptr == '-') {
      wordSilhouette = parseSilhouetteCharLiteral(fptr);
      parseWhitespaces(fptr);
    }
    if (std::isdigit(**fptr) || **fptr == '#') {
      interword = parseRepeatedChar(fptr, args);
      parseWhitespaces(fptr);
    }
    char* wordsSource = va_arg(*args, char*);
    glc = GreedyLengthContentPtr(new Words(std::string(wordsSource), interword, wordSilhouette));
  } else {
    throw std::runtime_error("Expected ' or w after {.");
  }

  if (**fptr != '}') {
    throw std::runtime_error("Expected }.");
  }
  return glc;
}

SpecifiedLengthContentPtr parseSpecifiedLengthContent(const char** fptr, va_list* args) {
  assert(**fptr == '\'' || std::isdigit(**fptr));
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
      std::vector<SpecifiedLengthContentPtr> slcs;
      GreedyLengthContentPtr glc;
      RepeatedChar topFill(SpecifiedLengthPtr(new LiteralLength(0, false)), ' ');
      RepeatedChar bottomFill(SpecifiedLengthPtr(new LiteralLength(0, false)), ' ');
      parseWhitespaces(fptr);
      while (**fptr != ']') {
        if (**fptr == '\'' || std::isdigit(**fptr)) {
          slcs.push_back(parseSpecifiedLengthContent(fptr, args));
        } else if (**fptr == '{') {
          if (glc) {
            throw std::runtime_error("Cannot have multiple greedy-content within a block.");
          }
          glc = parseGreedyLengthContent(fptr, args);
        } else {
          throw std::runtime_error("Expected ' or digit at start of specified-length content.");
        }
        parseWhitespaces(fptr);
      }
      if (**fptr == '^' || **fptr == 'v') {

      } else {
        ++*fptr;
      }
      slc.reset(new Block(std::move(sl), slcs, std::move(glc), topFill, bottomFill));
    } else {
      throw std::runtime_error("Expected ' or [ after length specifier.");
    }
  }
  return slc;
}



void dsl_printf(const char* format, ...) {
  va_list args;
  va_start(args, format);

  const char* f_at = format;
  while (*f_at != '\0') {
    int l = parseLength(&f_at, &args, 13);
    printf("%d\n", l);
  }

  va_end(args);
}




using namespace std;

int main() {
  char* format = "'fugg'3745ggggg";
  const char* f_at = format;
  string str;
  int num;
  try {
     str = parseStringLiteral(&f_at);
     num = parseUint(&f_at);
  } catch (exception& e) {
    printf("Error at %d: %s\n", f_at - format, e.what());
  }
  printf("str = %s\n", str.c_str());
  printf("num = %d\n", num);

  getchar();
  return 0;
}
