#include <stdio.h>
#include <stdexcept>
#include "dsl.h"


using namespace std;

int linefunc(int line) {
  return line;
}
int main() {
  dsl_printf("    100 [ {w}1s [ 1s ' ' { w -> '_' 1s ' ' } ] '|' #s [ 1s ' ' { w' ' } 1s ' ' ] v 2  '_' ^ 2 '*' '\\o' 40 [ 1s' ' { w } ] ] ",
    "source0", "source1", &linefunc, "source2", "source3");
  getchar();
  return 0;
}
