#include <stdio.h>
#include <stdexcept>
#include "dsl.h"


using namespace std;

char* s1 = "Candy had always prided herself upon having a vivid imagination. When, for instance, she privately compared her dreams with those her brothers described over the breakfast table, or her friends at school exchanged at break, she always discovered her own night visions were a lot wilder and weirder than anybody else's. But there was nothing she could remember dreaming -- by day or night -- that came close to the sight that greeted her in The Great Head of the Yebba Dim Day."
"It was a city, a city built from the litter of the sea.The street beneath her feet was made from timbers that had clearly been in the water for a long time, and the walls were lined with barnacle - encrusted stone.There were three columns supporting the roof, made of coral fragments cemented together.They were buzzing hives of life unto themselves; their elaborately constructed walls pierced with dozens of windows, from which light poured.";


int linefunc(int line) {
  return line;
}
int main() {
  /*dsl_printf("100[ 1s[ #s' ' {w ->'_' 1s' ''hey'} ] '|' 1s[ 1s' ' {w' '} 1s' ' ]v{2'_''sup'}^{'h'2'*'} '\\o' 40[ 1' ' {w} ] ]",
    &linefunc, "source1", "source2", "source3");

  dsl_printf("100[  10s[3[1s' ']  {w} 10[1s' ']] 4s' ' 'heym' ]", "source1");

  dsl_printf("100[  1s[5[1s' ']^{}  1s' '{w}3s' '  5[1s' ']v{}]^{}v{}  '|'  6s[ 1[2s' ']^{}v{} #s' ' 2s' ' ]^{}v{}  ]^{}v{}",
    "source1", &linefunc);

  dsl_printf("100[ 1s[1s' '] 1s[1s' ']  ]^{'1'}v{'2'}");*/

  dsl_printf("10[1s' ' {w ' ' 1s' '}]", s1);

  getchar();
  return 0;
}
