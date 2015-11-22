#include "dsl.h"
#include "ast.h"

#include <stdio.h>
#include <cstdarg>
#include <stdexcept>
#include <assert.h>


bool isDigit(char c) {
  return ('0' <= c && c <= '9');
}

int parseUint(const char** fptr) {
  assert(isDigit(**fptr));
  int value = 0;
  do {
    int digit = **fptr - '0';
    value = value * 10 + digit;
    ++*fptr;
  } while (isDigit(**fptr));
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
    throw std::runtime_error("Expected closing \"'\" for char literal.");
  }
  ++*fptr;
  return c;
}

std::string parseStringLiteral(const char** fptr) {
  assert(**fptr == '\'');
  ++*fptr;
  const char* strBegin = *fptr;
  std::string str;
  while (**fptr != '\'') {
    str += parseChar(&*fptr);
  }
  ++*fptr;
  return str;
}

typedef int(*LengthFunc)(int);

int parseLength(const char** fptr, va_list* args, int line) {
  assert(isDigit(**fptr) || **fptr == '#');
  int length;
  if (**fptr == '#') {
    LengthFunc lengthFunc = va_arg(*args, LengthFunc);
    length = (*lengthFunc)(line);
    ++*fptr;
  } else {
    length = parseUint(&*fptr);
  }
  return length;
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
