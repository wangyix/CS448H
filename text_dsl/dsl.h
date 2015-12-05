#ifndef DSL_H
#define DSL_H

#include "ast.h"

#include <stdio.h>
#include <string>
#include <vector>

void dsl_fprintf(FILE* stream, const char* format, const char** wordSources, const LengthFunc* lengthFuncs, ...);
void dsl_sprintf(std::string* str, const char* format, const char** wordSources, const LengthFunc* lengthFuncs, ...);
void dsl_sprintf_lines(std::vector<std::string>* lines, const char* format, const char** wordSources, const LengthFunc* lengthFuncs, ...);
void dsl_sprintf_lines_append(std::vector<std::string>* lines, const char* format, const char** wordSources, const LengthFunc* lengthFuncs, ...);

#endif
