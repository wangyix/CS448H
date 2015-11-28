#include <stdio.h>
#include <stdexcept>
#include "dsl.h"


using namespace std;

char* s1 = "Candy had always prided herself upon having a vivid imagination. When, for instance, she privately compared her dreams with those her brothers described over the breakfast table, or her friends at school exchanged at break, she always discovered her own night visions were a lot wilder and weirder than anybody else's. But there was nothing she could remember dreaming -- by day or night -- that came close to the sight that greeted her in The Great Head of the Yebba Dim Day."
"\nIt was a city, a city built from the litter of the sea.The street beneath her feet was made from timbers that had clearly been in the water for a long time, and the walls were lined with barnacle - encrusted stone.There were three columns supporting the roof, made of coral fragments cemented together.They were buzzing hives of life unto themselves; their elaborately constructed walls pierced with dozens of windows, from which light poured.";

char* s2 = "The lufwood was burning very well. Purple flames blazed all round the stubby logs as they bumped and tumbled around inside the stove."
"\nThe woodtrolls had many types of wood to choose from and each had its own special properties.Scentwood, for instance, burned with a fragrance that sent those who breathed it drifting into a dream - filled sleep, while wood from the silvery - turquoise lullabee tree sang as the flames lapped at its bark - strange mournful songs, they were, and not at all to everyone’s taste.And then there was the bloodoak, complete with its parasitic sidekick, a barbed creeper known as tarry vine.";

int linefunc(int line) {
  return line % 8;
}
int main() {
  dsl_printf("120[ 1s[ #' ' {w 1s' '} ]^{'-'1s'^'}v{1s'v''='} '|' 1s[ 1s' ' {w' '} 1s' ' ]^{'+'1s'^'}v{1s'v''-'} '|' 40[ 1s' ' {w' '} ]^{'WEW'1s'^'}v{1s'v''LAD'} ]",
    &linefunc, s1, s2, s1);

  
  /*
  dsl_printf("100[  10s[3[1s' ']  {w} 10[1s' ']] 4s' ' 'heym' ]", "source1");

  
  dsl_printf("100[  1s[5[1s' ']^{}  1s' '{w}3s' '  5[1s' ']v{}]^{}v{}  '|'  6s[ 1[2s' ']^{}v{} #s' ' 2s' ' ]^{}v{}  ]^{}v{}",
    "source1", &linefunc);

  dsl_printf("100[ 1s[1s' '] 1s[1s' ']  ]^{'1'}v{'2'}");

  dsl_printf("100[ 1s[1s' ' {w ' ' 1s' '}]  2s[ #'|' 2s' ' {w'__'} 1s' ' #'|' ]  ]", s1, &linefunc, s1, &linefunc);
  */

  //dsl_printf("60[ #'|' {w 1s' '' '} #'|' 1s' ']", &linefunc, s1, &linefunc);

  //dsl_printf("60'-'");

  //dsl_printf("100[  1s[{w' '}1s' ']^{1s'*'}v{'=='} '|' 2s[{w' '}1s' ']^{2s'+' 'hi'}v{'--' 1s'o'} ]", s1, s2);

  getchar();
  return 0;
}
