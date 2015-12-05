#include <stdio.h>
#include <stdexcept>
#include <vector>
#include <string>
#include <iostream>

#include "dsl.h"

using namespace std;

char* s1 = "Candy had always prided herself upon having a vivid imagination. When, for instance, she privately compared her dreams with those her brothers described over the breakfast table, or her friends at school exchanged at break, she always discovered her own night visions were a lot wilder and weirder than anybody else's. But there was nothing she could remember dreaming -- by day or night -- that came close to the sight that greeted her in The Great Head of the Yebba Dim Day."
"\nIt was a city, a city built from the litter of the sea. The street beneath her feet was made from timbers that had clearly been in the water for a long time, and the walls were lined with barnacle - encrusted stone.There were three columns supporting the roof, made of coral fragments cemented together.They were buzzing hives of life unto themselves; their elaborately constructed walls pierced with dozens of windows, from which light poured.";

char* s2 = "The lufwood was burning very well. Purple flames blazed all round the stubby logs as they bumped and tumbled around inside the stove."
"\nThe woodtrolls had many types of wood to choose from and each had its own special properties.Scentwood, for instance, burned with a fragrance that sent those who breathed it drifting into a dream - filled sleep, while wood from the silvery - turquoise lullabee tree sang as the flames lapped at its bark - strange mournful songs, they were, and not at all to everyone’s taste.And then there was the bloodoak, complete with its parasitic sidekick, a barbed creeper known as tarry vine.";

char* s3 = "\n";

int linefunc(int line) {
  return line % 4;
}
int linefunc2(int line) {
  return line % 8;
}

int main() {
  string format;
  while (true) {
    cout << "Enter format string:" << endl;
    getline(cin, format);


    const char* wordSources[8] = { s1, s2, s1, s2, s1, s2, s1, s2 };
    int(*lengthFuncs[8])(int) = { &linefunc, &linefunc2, &linefunc, &linefunc2, &linefunc, &linefunc2, &linefunc, &linefunc2 };

    cout << endl;
    dsl_fprintf(stdout, format.c_str(), wordSources, lengthFuncs, 80, "hello world");
    cout << endl << endl;
  }

  /*
  dsl_fprintf(stdout, "150[ 1s[1s'_']^{1s'@'}v{1s'@'}  ' [''?''] '   5s[ 1s[#' '{w' '1s' '}1s' ']^{1s'^'}v{1s'v'} ' | ' 1s[1s' '{w' '}1s' ' ]^{'='1s'^'}v{2s'v''-'} ' | ' 40[1s' '{w' '}]^{'WEW'1s'^'}v{1s'v''LAD'} ]^{1s'<'}v{1s'>'}     ]",
    &linefunc, s1, s2, s1);

  getchar();
  */
  return 0;
}




const int fontPointSize = 10;

int widthPixels, heightPixels;

//string formatNoLength = "[ 1s[1s'_']^{1s'@'}v{1s'@'}  ' [''?''] '   5s[ 1s[#' '{w' '1s' '}1s' ']^{1s'^'}v{1s'v'} ' | ' 1s[1s' '{w' '}1s' ' ]^{'='1s'^'}v{2s'v''-'} ' | ' 40[1s' '{w' '}]^{'WEW'1s'^'}v{1s'v''LAD'} ]^{1s'<'}v{1s'>'}     ]";
string textFormat = "%d[' ' 1s[{w' '}1s' ']^{}v{1s'.'} ' | ' 1s[1s' '{w' '1s' '}]^{1s' ''='}v{'='1s' '} ' @ ' 1s[1s' '{w'::'}]^{1s' '}v{1s' '} ' ']";
string borderFormat = "%d[' ' 1s'-' ' + ' 1s'-' ' @ ' 1s'-' ' ']";
vector<string> lines;

void updateLines(int numCols) {
  lines.clear();
  //dsl_sprintf(&lines, format.c_str(), &linefunc, s1, s2, s1);
  const char* wordSources[3] = { s1, s2, s1 };
  const char* wordSources2[3] = { s2, s1, s2 };
  dsl_sprintf_lines_append(&lines, textFormat.c_str(), wordSources, NULL, numCols);
  dsl_sprintf_lines_append(&lines, borderFormat.c_str(), NULL, NULL, numCols);
  dsl_sprintf_lines_append(&lines, textFormat.c_str(), wordSources2, NULL, numCols);
}


#include <windows.h>
#define ErrorMessageBox(a,b) MessageBox(a,b,"Error:",MB_ICONWARNING);

bool SetUpWindowClass(char*, int, int, int);
LRESULT CALLBACK WindowProcedure(HWND, unsigned int, WPARAM, LPARAM);

HFONT font = (HFONT)GetStockObject(OEM_FIXED_FONT);


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

HFONT createAndSetSizedFont(HDC hDC, int pointSize, HFONT* oldFont) {
  LOGFONT logFont;
  GetObject(font, sizeof(LOGFONT), &logFont);
  logFont.lfHeight = -MulDiv(pointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
  HFONT newFont = CreateFontIndirect(&logFont);
  *oldFont = (HFONT)SelectObject(hDC, newFont);
  return newFont;
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
  
  int iY = 5;
  for (int i = 0; i < lines.size(); i++, iY += 20) {
    TextOut(hDC, 0, iY, lines[i].c_str(), lines[i].length());
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
    
    HFONT oldFont;
    HFONT sizedFont = createAndSetSizedFont(hDC, fontPointSize, &oldFont);

    ABCFLOAT abcf;
    GetCharABCWidthsFloat(hDC, 'a', 'a', &abcf);
    int numCols = widthPixels / abcf.abcfB;

    updateLines(numCols);
    drawLines(hDC);

    SelectObject(hDC, oldFont);
    DeleteObject(sizedFont);

    ReleaseDC(hWnd, hDC);
  } break;
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(hWnd, &ps);

    HFONT oldFont;
    HFONT sizedFont = createAndSetSizedFont(hDC, fontPointSize, &oldFont);

    drawLines(hDC);

    SelectObject(hDC, oldFont);
    DeleteObject(sizedFont);

    EndPaint(hWnd, &ps);
  } break;
  }
  return DefWindowProc(hWnd, uiMsg, wParam, lParam);
}

