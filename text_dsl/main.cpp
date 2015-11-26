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

  //dsl_printf("100[  10s[3[1s' ']  {w} 10[1s' ']] 4s' ' 'heym' ]", "source1");

  dsl_printf("100[  1s[5[1s' ']^{}  1s' '{w}3s' '  5[1s' ']v{}]^{}v{}  '|'  6s[ 1[2s' ']^{}v{} #s' ' 2s' ' ]^{}v{}  ]^{}v{}",
    "source1", &linefunc);

  //dsl_printf("100[ 1s[1s' ']^{'3'}v{'4'} 1s[1s' ']^{'5'}v{'6'}  ]^{'1'}v{'2'}");

  getchar();
  return 0;
}
