#include <stdio.h>
#include <stdexcept>
#include <vector>
#include <string>

#include "dsl.h"

using namespace std;

char* s1 = "Candy had always prided herself upon having a vivid imagination. When, for instance, she privately compared her dreams with those her brothers described over the breakfast table, or her friends at school exchanged at break, she always discovered her own night visions were a lot wilder and weirder than anybody else's. But there was nothing she could remember dreaming -- by day or night -- that came close to the sight that greeted her in The Great Head of the Yebba Dim Day."
"\nIt was a city, a city built from the litter of the sea.The street beneath her feet was made from timbers that had clearly been in the water for a long time, and the walls were lined with barnacle - encrusted stone.There were three columns supporting the roof, made of coral fragments cemented together.They were buzzing hives of life unto themselves; their elaborately constructed walls pierced with dozens of windows, from which light poured.";

char* s2 = "The lufwood was burning very well. Purple flames blazed all round the stubby logs as they bumped and tumbled around inside the stove."
"\nThe woodtrolls had many types of wood to choose from and each had its own special properties.Scentwood, for instance, burned with a fragrance that sent those who breathed it drifting into a dream - filled sleep, while wood from the silvery - turquoise lullabee tree sang as the flames lapped at its bark - strange mournful songs, they were, and not at all to everyone’s taste.And then there was the bloodoak, complete with its parasitic sidekick, a barbed creeper known as tarry vine.";

char* s3 = "\n";

int linefunc(int line) {
  return line % 8;
}

#if 0
int main() {
  //FILE* file = fopen("out.txt", "w");

  std::string out;
  dsl_sprintf(&out, "150[ 1s[1s'_']^{1s'@'}v{1s'@'}  ' [''?''] '   5s[ 1s[#' '{w' '1s' '}1s' ']^{1s'^'}v{1s'v'} ' | ' 1s[1s' '{w' '}1s' ' ]^{'='1s'^'}v{2s'v''-'} ' | ' 40[1s' '{w' '}]^{'WEW'1s'^'}v{1s'v''LAD'} ]^{1s'<'}v{1s'>'}     ]",
    &linefunc, s1, s2, s1);

  printf("%s", out.c_str());


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

  //fclose(file);

  getchar();
  return 0;
}
#endif


int widthPixels, heightPixels;

//string formatNoLength = "[ 1s[1s'_']^{1s'@'}v{1s'@'}  ' [''?''] '   5s[ 1s[#' '{w' '1s' '}1s' ']^{1s'^'}v{1s'v'} ' | ' 1s[1s' '{w' '}1s' ' ]^{'='1s'^'}v{2s'v''-'} ' | ' 40[1s' '{w' '}]^{'WEW'1s'^'}v{1s'v''LAD'} ]^{1s'<'}v{1s'>'}     ]";
string formatNoLength = "[ '+ ' 1s' ' {w' '} ]";
vector<string> lines;

void updateLines(int numCols) {
  string format = to_string(numCols) + formatNoLength;
  //dsl_sprintf(&lines, format.c_str(), &linefunc, s1, s2, s1);
  dsl_sprintf(&lines, format.c_str(), s1);
}


#include <windows.h>
#define ErrorMessageBox(a,b) MessageBox(a,b,"Error:",MB_ICONWARNING);

bool SetUpWindowClass(char*, int, int, int);
LRESULT CALLBACK WindowProcedure(HWND, unsigned int, WPARAM, LPARAM);

HFONT font = (HFONT)GetStockObject(ANSI_FIXED_FONT);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpsCmdLine, int iCmdShow) {
  if (!SetUpWindowClass("1", 255, 255, 255)) {
    ErrorMessageBox(NULL, "Window class \"1\" failed");
    return 0;
  }
  HWND hWnd = CreateWindow("1", "Hello World - Win32 API", WS_OVERLAPPEDWINDOW, 315, 115, 700, 480, NULL, NULL, hInstance, NULL);
  if (!hWnd) {
    ErrorMessageBox(NULL, "Window handle = NUL");
    return 0;
  }
  ShowWindow(hWnd, SW_SHOW);
  MSG uMsg;
  while (GetMessage(&uMsg, NULL, 0, 0) > 0) {
    TranslateMessage(&uMsg);
    DispatchMessage(&uMsg);
  }
  return 0;
}

bool SetUpWindowClass(char* cpTitle, int iR, int iG, int iB) {
  WNDCLASSEX WindowClass;
  WindowClass.cbClsExtra = 0;
  WindowClass.cbWndExtra = 0;
  WindowClass.cbSize = sizeof(WNDCLASSEX);
  WindowClass.style = 0;
  WindowClass.lpszClassName = cpTitle;
  WindowClass.lpszMenuName = NULL;
  WindowClass.lpfnWndProc = WindowProcedure;
  WindowClass.hInstance = GetModuleHandle(NULL);
  WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  WindowClass.hbrBackground = CreateSolidBrush(RGB(iR, iG, iB));
  WindowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  WindowClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
  if (RegisterClassEx(&WindowClass)) return true;
  else return false;
}

void drawLines(HDC hDC) {
  RECT rect;
  rect.left = 0;
  rect.right = widthPixels;
  rect.top = 0;
  rect.bottom = heightPixels;
  HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));
  FillRect(hDC, &rect, brush);
  DeleteObject(brush);

  SelectObject(hDC, font);
  int iY = 5;
  for (int i = 0; i < lines.size(); i++, iY += 20) {
    TextOut(hDC, 5, iY, lines[i].c_str(), lines[i].length());
  }
}

LRESULT CALLBACK WindowProcedure(HWND hWnd, unsigned int uiMsg, WPARAM wParam, LPARAM lParam) {
  switch (uiMsg) {
  case WM_CLOSE:
    DestroyWindow(hWnd);
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  case WM_SIZE: {
    widthPixels = LOWORD(lParam);
    heightPixels = HIWORD(lParam);

    HDC hDC = GetDC(hWnd);
    SelectObject(hDC, font);
    ABCFLOAT abcf;
    GetCharABCWidthsFloat(hDC, 'a', 'a', &abcf);
    int numCols = widthPixels / abcf.abcfB - 1;
    updateLines(numCols);    
    drawLines(hDC);
    ReleaseDC(hWnd, hDC);
  } break;
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(hWnd, &ps);
    drawLines(hDC);
    EndPaint(hWnd, &ps);
  } break;
  }
  return DefWindowProc(hWnd, uiMsg, wParam, lParam);
}

