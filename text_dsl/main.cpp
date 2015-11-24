#include <stdio.h>
#include <stdexcept>
#include "dsl.h"


using namespace std;

int linefunc(int line) {
  return line;
}
int main() {
  dsl_printf("100s[ {w ' '1s' '} 1s[ #s' ' {w ->'_' 1s' ''hey'} ] '|' 1s[ 1s' ' {w' '} 1s' ' ]v{2'_''sup'}^{'h'2'*'} '\\o' 40[ 1s' ' {w} ] ]",
    "source0", &linefunc, "source1", "source2", "source3");
  getchar();
  return 0;
}
