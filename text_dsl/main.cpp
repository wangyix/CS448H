#include <stdio.h>
#include <stdexcept>
#include "dsl.h"


using namespace std;

int linefunc(int line) {
  return line;
}
int main() {
  dsl_printf("100[ 1s[ 1s' ' {  w } ]  '|' #s[   1s' '{w #' '   }1s' ' ]v1'_'^1s'\\*'     '\\o'    40[ 1s' '{w}   ]]",
    "source1", &linefunc, "source2", &linefunc, "source3");
  getchar();
  return 0;
}
