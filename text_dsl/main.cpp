#include <stdio.h>
#include <stdexcept>
#include "dsl.h"


using namespace std;

int linefunc(int line) {
  return line;
}
int main() {
  /*dsl_printf("100[ 1s[ #s' ' {w ->'_' 1s' ''hey'} ] '|' 1s[ 1s' ' {w' '} 1s' ' ]v{2'_''sup'}^{'h'2'*'} '\\o' 40[ 1s' ' {w} ] ]",
    &linefunc, "source1", "source2", "source3");*/

  dsl_printf("100[1s' ' 2s' ' 3s' ' 2s' ' 5s' ' 7s' ' 4s' ' 'heym' ]");

  getchar();
  return 0;
}
