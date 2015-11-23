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

char parseChar(const char** fptr) {
  if (**fptr == '\\') {
    ++*fptr;
  }
  char c = **fptr;
  if (c == '\0') {
    throw std::runtime_error("Reached end of string; expected char.");
  }
  ++*fptr;
  return c;
}

char parseCharLiteral(const char** fptr) {
  assert(**fptr == '\'');
  ++*fptr;
  char c = parseChar(&*fptr);
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
    str += parseChar(&*fptr);
  }
  ++*fptr;
  return SpecifiedLengthContentPtr(new StringLiteral(str));
}

SpecifiedLengthPtr parseSpecifiedLength(const char** fptr, va_list* args) {
  assert(isdigit(**fptr) || **fptr == '#');
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

/*SpecifiedLengthContent* parseSpecifiedLengthContent(const char** fptr, va_list* args);

Block* parseBlock(const char** fptr, va_list* args) {
  assert(**fptr == '[');
  while 
}*/

SpecifiedLengthContentPtr parseSpecifiedLengthContent(const char** fptr, va_list* args) {
  assert(**fptr == '\'' || isdigit(**fptr));
  SpecifiedLengthContentPtr slc;
  if (**fptr == '\'') {
    slc = parseStringLiteral(fptr);
  } else {
    SpecifiedLength* sl = parseSpecifiedLength(fptr, args);
    if (**fptr == '\'') {
      char c = parseCharLiteral(fptr);
      slc = new RepeatedChar(sl, c);
    } else if (**fptr == '[') {
      parseWhitespaces(fptr);
      while (**fptr != ']') {
        if (!(**fptr == '\'' || isdigit(**fptr))) {
          delete sl;
          throw std::runtime_error("Expected ' or digit at start of specified-length content.");
        }

      }
    } else {
      delete sl;
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
