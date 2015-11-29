#include <stdio.h>
#include <stdexcept>
#include <windows.h>

#include "dsl.h"

using namespace std;

#if 0
char* s1 = "Candy had always prided herself upon having a vivid imagination. When, for instance, she privately compared her dreams with those her brothers described over the breakfast table, or her friends at school exchanged at break, she always discovered her own night visions were a lot wilder and weirder than anybody else's. But there was nothing she could remember dreaming -- by day or night -- that came close to the sight that greeted her in The Great Head of the Yebba Dim Day."
"\nIt was a city, a city built from the litter of the sea.The street beneath her feet was made from timbers that had clearly been in the water for a long time, and the walls were lined with barnacle - encrusted stone.There were three columns supporting the roof, made of coral fragments cemented together.They were buzzing hives of life unto themselves; their elaborately constructed walls pierced with dozens of windows, from which light poured.";

char* s2 = "The lufwood was burning very well. Purple flames blazed all round the stubby logs as they bumped and tumbled around inside the stove."
"\nThe woodtrolls had many types of wood to choose from and each had its own special properties.Scentwood, for instance, burned with a fragrance that sent those who breathed it drifting into a dream - filled sleep, while wood from the silvery - turquoise lullabee tree sang as the flames lapped at its bark - strange mournful songs, they were, and not at all to everyone’s taste.And then there was the bloodoak, complete with its parasitic sidekick, a barbed creeper known as tarry vine.";

char* s3 = "\n";

int linefunc(int line) {
  return line % 8;
}
int main() {
  FILE* file = fopen("out.txt", "w");

  dsl_fprintf(file, "150[ 1s[1s'_']^{1s'@'}v{1s'@'}  ' [''?''] '   5s[ 1s[#' '{w' '1s' '}1s' ']^{1s'^'}v{1s'v'} ' | ' 1s[1s' '{w' '}1s' ' ]^{'='1s'^'}v{2s'v''-'} ' | ' 40[1s' '{w' '}]^{'WEW'1s'^'}v{1s'v''LAD'} ]^{1s'<'}v{1s'>'}     ]",
             &linefunc, s1, s2, s1);
  

  //dsl_fprintf(file, "100[ 4[1s' ']^{30'1'}v{30'1'}  1s[  4[1s' ']^{20'2'}v{20'2'}  1s[  4[1s' ']^{10'3'}v{10'3'}   1s[{w' '}1s' ']^{1s'a'}v{1s'z'}   ]^{1s'b'}v{1s'y'}  ]^{1s'c'}v{1s'x'}  ]", s2);
  
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

  fclose(file);

  getchar();
  return 0;
}
#endif



HANDLE consoleInputHandle;
HANDLE consoleOutputHandle;
DWORD fdwSaveOldMode;

int width = 0, height = 0;


void updateWindowSizeThread(PINPUT_RECORD irInbuf) {
  while (true) {
    //WaitForSingleObject(consoleInputHandle, INFINITE);
    DWORD cNumRead;
    if (!ReadConsoleInput(consoleInputHandle, irInbuf, 128, &cNumRead)) {
      exit(1);
    }
    for (int i = 0; i < cNumRead; ++i) {
      switch (irInbuf[i].EventType) {
      case WINDOW_BUFFER_SIZE_EVENT: {
        //updateWindowSize();
        WINDOW_BUFFER_SIZE_RECORD wbsr = irInbuf[i].Event.WindowBufferSizeEvent;
        width = wbsr.dwSize.X;
        height = wbsr.dwSize.Y;
        printf("%d x %d\n", height, width);
      } break;
      default:
        break;
      }
    }
  }
}

int main() {
  /*INPUT_RECORD irInBuf[128];

  consoleInputHandle = GetStdHandle(STD_INPUT_HANDLE);
  if (consoleInputHandle == INVALID_HANDLE_VALUE) {
    exit(1);
  }
  consoleOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
  if (consoleOutputHandle == INVALID_HANDLE_VALUE) {
    exit(1);
  }
  
  // get initial console dimensions
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (GetConsoleScreenBufferInfo(consoleOutputHandle, &csbi)) {
    height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    printf("%d x %d\n", height, width);
  } else {
    exit(1);
  }

  // save console mode, set console mode to enable window input
  if (!GetConsoleMode(consoleInputHandle, &fdwSaveOldMode)) {
    exit(1);
  }
  if (!SetConsoleMode(consoleInputHandle, ENABLE_WINDOW_INPUT)) {
    exit(1);
  }
  
  updateWindowSizeThread(irInBuf);
  
  getchar();
  
  // restore input mode on exit
  SetConsoleMode(consoleInputHandle, fdwSaveOldMode);
  */

  HWND consoleWindow = GetConsoleWindow();
  
  RECT consoleRect;
  if (!GetClientRect(consoleWindow, &consoleRect)) {
    exit(1);
  }

  int widthPixels = consoleRect.right - consoleRect.left;
  int heightPixels = consoleRect.bottom - consoleRect.top;
  printf("%d x %d", widthPixels, heightPixels);

  getchar();

  return 0;
}
