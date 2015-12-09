#ifndef DSL_H
#define DSL_H

#include "ast.h"

#include <stdio.h>
#include <string>
#include <vector>

int sprintf(std::string* str, const char* format, ...);
int vsprintf(std::string* str, const char* format, va_list args);

void text_printf(const char* format, const char** wordSources=NULL, const LengthFunc* lengthFuncs=NULL, ...);
void text_fprintf(FILE* stream, const char* format, const char** wordSources=NULL, const LengthFunc* lengthFuncs=NULL, ...);
void text_sprintf(std::string* str, const char* format, const char** wordSources=NULL, const LengthFunc* lengthFuncs=NULL, ...);
void text_sprintf_lines(std::vector<std::string>* lines, const char* format, const char** wordSources=NULL, const LengthFunc* lengthFuncs=NULL, ...);
void text_sprintf_lines_append(std::vector<std::string>* lines, const char* format, const char** wordSources=NULL, const LengthFunc* lengthFuncs=NULL, ...);

#endif
