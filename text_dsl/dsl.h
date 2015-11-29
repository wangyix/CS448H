#ifndef DSL_H
#define DSL_H

#include <stdio.h>
#include <string>
#include <vector>

void dsl_fprintf(FILE* stream, const char* format, ...);
void dsl_sprintf(std::string* str, const char* format, ...);
void dsl_sprintf(std::vector<std::string>* lines, const char* format, ...);

#endif
