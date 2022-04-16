// Plotter.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "Plotter.h"
#include "fparser.hh"
#include "Utility.h"
#include <strsafe.h>
#include <commctrl.h>
#include <uxtheme.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Глобальные константы
#define MAX_LOADSTRING  100
#define MAX_PLOTS       20
#define DEFAULT_WIDTH   720
#define DEFAULT_HEIGHT  480
#define MAX_EXPR_LENGTH 512
#define CANVAS_OFFSET_L 160
#define CANVAS_OFFSET_T 30 
#define MIN_STEP        1
#define MAX_STEP        20

// Индексы элементов управления
#define ID_LST_EXPRESSIONS       10000
#define ID_СMB_COLORS            10001
#define ID_BTN_GRID              10002
#define ID_BTN_REDRAW            10003
#define ID_BTN_SAVE              10004
#define ID_TXT_INPUT             10005
#define ID_BTN_CLEAR             10006
#define ID_BTN_AXES              10007
#define ID_SCR_V                 10008
#define ID_SCR_H                 10009

// Глабаольные переменные окна
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING]; 

// Глобальные переменные
FunctionParser expressions[MAX_PLOTS];
int plotsColors[MAX_PLOTS] = { 4 };
int lastSelection = -1;
int quality = (MAX_STEP - MIN_STEP) / 4;
bool drawGrid = true;
bool drawAxes = true;
bool exprChanged = false;
ViewPort viewPort;

// Дескрипторы элементов управления
HWND hLBL_cursor, hTXT_input, hCMB_colors, hBTN_save, hLBL_label, hBTN_clear;
HWND hBTN_redraw, hLST_expressions;
HWND hTXT_leftPos, hTXT_topPos, hTXT_botPos, hTXT_rightPos;
HWND hLBL_leftPos, hLBL_topPos, hLBL_botPos, hLBL_rightPos, hLBL_status;
HWND hSCR_scrollH, hSCR_scrollV;
HPEN pGrid, pAxes;

// Дескрипторы элементов управления настроек
HWND hSLD_quality, hBTN_grid, hBTN_axes, hLBL_stepMin, hLBL_stepMax, hIMG_logo;

// Иконки
HICON hICO_yes, hICO_no, hICO_graph;
HBITMAP hPNG_logo;

// Объявления функций
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Settings(HWND, UINT, WPARAM, LPARAM);
void                InitItems();
void                Redraw(HWND);
void                ResetView(HWND);
void                DrawGrid(HDC&);
void                DrawAxes(HDC&);
void                DrawGraphs(HDC& hdc);
void                DrawGraph(HDC&, int);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PLOTTER, szWindowClass, MAX_LOADSTRING);
    
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PLOTTER));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_PLOTTER);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    viewPort.wScreen = DEFAULT_WIDTH - CANVAS_OFFSET_L - 30;
    viewPort.hScreen = DEFAULT_HEIGHT - CANVAS_OFFSET_T - 30;
    viewPort.xScreen = CANVAS_OFFSET_L;
    viewPort.yScreen = CANVAS_OFFSET_T;
    viewPort.xReal = -1;
    viewPort.yReal = -1;
    viewPort.wReal = 2;
    viewPort.hReal = 2;

    RegisterClassExW(&wcex);

    pGrid = CreatePen(PS_SOLID, 1, CL_GRID);
    pAxes = CreatePen(PS_SOLID, 2, CL_AXES);

    if (!InitInstance (hInstance, nCmdShow))
        return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PLOTTER));

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
   hInst = hInstance;

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, DEFAULT_WIDTH, DEFAULT_HEIGHT, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
      return FALSE;

   hLBL_label = CreateWindow(L"STATIC", L"f1(x) =", WS_CHILD | WS_VISIBLE,
       5, 5, 40, 23, hWnd, 0, hInst, 0);
   hTXT_input = CreateWindow(L"EDIT", 0, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_LOWERCASE,
       45, 5, 300, 23, hWnd, (HMENU)ID_TXT_INPUT, hInst, 0);
   hCMB_colors = CreateWindow(L"COMBOBOX", NULL, CBS_DROPDOWN | CBS_HASSTRINGS | WS_VISIBLE | WS_CHILD | CBS_SIMPLE,
       344, 5, 115, 280, hWnd, (HMENU)ID_СMB_COLORS, hInst, 0);
   hBTN_save = CreateWindow(L"BUTTON", L"Добавить", WS_CHILD | WS_VISIBLE | BS_TEXT,
       459, 5, 100, 23, hWnd, (HMENU)ID_BTN_SAVE, hInst, 0);
   hBTN_clear = CreateWindow(L"BUTTON", L"Очистить", WS_CHILD | WS_VISIBLE | BS_TEXT,
       559, 5, 100, 23, hWnd, (HMENU)ID_BTN_CLEAR, hInst, 0);

   CreateWindow(L"STATIC", L"Выражения:", WS_CHILD | WS_VISIBLE,
       5, 33, CANVAS_OFFSET_L - 10, 20, hWnd, 0, hInst, 0);
   hLST_expressions = CreateWindow(L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY,
       5, 53, CANVAS_OFFSET_L - 10, 200, hWnd, (HMENU)ID_LST_EXPRESSIONS, hInst, 0);

   hLBL_leftPos = CreateWindow(L"STATIC", L"x1:", WS_CHILD | WS_VISIBLE,
       5, 258, 25, 23, hWnd, 0, hInst, 0);
   hTXT_leftPos = CreateWindow(L"EDIT", L"-1", WS_CHILD | WS_VISIBLE | WS_BORDER,
       30, 258, 50, 23, hWnd, 0, hInst, 0);
   
   hLBL_rightPos= CreateWindow(L"STATIC", L"x2:", WS_CHILD | WS_VISIBLE,
       80, 258, 25, 23, hWnd, 0, hInst, 0);
   hTXT_rightPos = CreateWindow(L"EDIT", L"1", WS_CHILD | WS_VISIBLE | WS_BORDER,
       105, 258, 50, 23, hWnd, 0, hInst, 0);;

   hLBL_topPos = CreateWindow(L"STATIC", L"y1:", WS_CHILD | WS_VISIBLE,
       5, 286, 25, 23, hWnd, 0, hInst, 0);
   hTXT_topPos = CreateWindow(L"EDIT", L"-1", WS_CHILD | WS_VISIBLE | WS_BORDER,
       30, 286, 50, 23, hWnd, 0, hInst, 0);

   hLBL_botPos = CreateWindow(L"STATIC", L"y2:", WS_CHILD | WS_VISIBLE,
       80, 286, 25, 23, hWnd, 0, hInst, 0);
   hTXT_botPos = CreateWindow(L"EDIT", L"1", WS_CHILD | WS_VISIBLE | WS_BORDER,
       105, 286, 50, 23, hWnd, 0, hInst, 0);

   hBTN_redraw = CreateWindow(L"BUTTON", L"Перерисовать", WS_CHILD | WS_VISIBLE | BS_TEXT,
       5, 314, CANVAS_OFFSET_L - 10, 23, hWnd, (HMENU)ID_BTN_REDRAW, hInst, 0);
   hLBL_status = CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
       5, 342, CANVAS_OFFSET_L - 10, 70, hWnd, 0, hInst, 0);

   hSCR_scrollV = CreateWindowEx(0, L"SCROLLBAR", 0, WS_CHILD | WS_VISIBLE | SBS_VERT,
       DEFAULT_WIDTH - 40, CANVAS_OFFSET_T, 15, DEFAULT_HEIGHT - CANVAS_OFFSET_T - 88, hWnd, (HMENU)ID_SCR_V, hInst, 0);
   hSCR_scrollH = CreateWindowEx(0, L"SCROLLBAR", 0, WS_CHILD | WS_VISIBLE | SBS_HORZ,
       CANVAS_OFFSET_L, DEFAULT_HEIGHT - 84, DEFAULT_WIDTH - CANVAS_OFFSET_L - 45, 15, hWnd, (HMENU)ID_SCR_H, hInst, 0);

   InitItems();
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}



LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_SETTINGS:
            if (DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTINGS), hWnd, Settings) == IDOK)
                Redraw(hWnd);
            break;
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;  
        case ID_BTN_SAVE: // Добавление выражения
        {
            TCHAR textW[MAX_EXPR_LENGTH] = { '\0' };
            char textA[MAX_EXPR_LENGTH] = { '\0' };
            GetWindowText(hTXT_input, textW, MAX_EXPR_LENGTH);
            wcstombs_s(0, textA, textW, MAX_EXPR_LENGTH);

            int selectedId = SendMessage(hLST_expressions, LB_GETCURSEL, 0, 0);
            if (selectedId == LB_ERR) selectedId = ++lastSelection;
            bool isModified = false;

            if (expressions[selectedId].Parse(textA, "x") != -1 && textW[0] != '\0')
                SetWindowText(hLBL_status, L"Выражение не распознано");
            else if (textW[0] != '\0') {
                expressions[selectedId].Optimize();
                SendMessage(hLST_expressions, LB_DELETESTRING, selectedId, 0);
                SendMessage(hLST_expressions, LB_INSERTSTRING, selectedId, (LPARAM)textW);
                SendMessage(hLST_expressions, LB_SETCURSEL, selectedId, 0);
                int colorId = SendMessage(hCMB_colors, CB_GETCURSEL, 0, 0);
                if (colorId != CB_ERR) 
                    plotsColors[selectedId] = colorId;
                Redraw(hWnd);
            }
        } break;
        case ID_BTN_CLEAR: // Очистка выражения
        {
            TCHAR text[MAX_EXPR_LENGTH] = { '\0' };          
            GetWindowText(hTXT_input, text, MAX_EXPR_LENGTH);
            if (text[0] != '\0') {
                SetWindowText(hTXT_input, L"");
                int selectedId = SendMessage(hLST_expressions, LB_GETCURSEL, 0, 0);
                if (selectedId != LB_ERR) {
                    SendMessage(hLST_expressions, LB_DELETESTRING, selectedId, 0);
                    SendMessage(hLST_expressions, LB_INSERTSTRING, selectedId, (LPARAM)L"");
                    SendMessage(hLST_expressions, LB_SETCURSEL, selectedId, 0);
                }
                expressions[selectedId].Parse("\0", "x");
                Redraw(hWnd);
            }
           
        } break;
        case ID_LST_EXPRESSIONS: // Выбор и начало редактирования выражения
            if (HIWORD(wParam) == LBN_DBLCLK) {
                int selectedId = SendMessage(hLST_expressions, LB_GETCURSEL, 0, 0);
                if (selectedId != LB_ERR) {
                    TCHAR text[MAX_EXPR_LENGTH] = { '\0' };
                    if (SendMessage(hLST_expressions, LB_GETTEXT, selectedId, (LPARAM)text) != LB_ERR) {
                        SetWindowText(hTXT_input, text);
                        SendMessage(hCMB_colors, CB_SETCURSEL, plotsColors[selectedId], 0);
                        StringCbPrintfW(text, 20, L"f%d(x) =", selectedId + 1);
                        SetWindowText(hLBL_label, text);
                    }
                }
            }
            break;
        case ID_BTN_REDRAW: // Ручная перерисовка, изменение масштаба
        {
            TCHAR text[10] = { '\0' };

            GetWindowText(hTXT_leftPos, text, 10);
            double x1 = _wtof(text);
            GetWindowText(hTXT_rightPos, text, 10);
            double x2 = _wtof(text);
            GetWindowText(hTXT_topPos, text, 10);
            double y1 = _wtof(text);
            GetWindowText(hTXT_botPos, text, 10);
            double y2 = _wtof(text);

            if (x1 < x2 && y1 < y2) {
                viewPort.xReal = x1;
                viewPort.yReal = y1;
                viewPort.wReal = abs(x2 - x1);
                viewPort.hReal = abs(y2 - y1);
                Redraw(hWnd);
                SetWindowText(hLBL_status, L"");
            }
            else SetWindowText(hLBL_status, L"Неверные границы!");

        } break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    } break;
    case WM_VSCROLL:
        if (LOWORD(wParam) == SB_LINEDOWN) {
            viewPort.yReal -= viewPort.hReal / 10.0;
            ResetView(hWnd);
        }
        else if (LOWORD(wParam) == SB_LINEUP) {
            viewPort.yReal += viewPort.hReal / 10.0;
            ResetView(hWnd);
        }
        break;
    case WM_HSCROLL:
        if (LOWORD(wParam) == SB_LINEDOWN) {
            viewPort.xReal -= viewPort.wReal / 10.0;
            ResetView(hWnd);
        }
        else if (LOWORD(wParam) == SB_LINEUP) {
            viewPort.xReal += viewPort.wReal / 10.0;
            ResetView(hWnd);
        }
        break;
    case WM_MOUSEWHEEL: {
        int wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (wheelDelta > 0) {
            viewPort.xReal -= viewPort.wReal / 2.0;
            viewPort.wReal *= 2.0;
            viewPort.yReal -= viewPort.hReal / 2.0;
            viewPort.hReal *= 2.0;
        }
        else {
            viewPort.xReal /= 2.0;
            viewPort.wReal /= 2.0;
            viewPort.yReal /= 2.0;
            viewPort.hReal /= 2.0;
        }
        ResetView(hWnd);
    }  break;
    case WM_PAINT: 
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        
        SelectObject(hdc, pGrid);
        Rectangle(hdc, viewPort.xScreen, viewPort.yScreen, viewPort.xScreen + viewPort.wScreen, viewPort.yScreen + viewPort.hScreen);
        
        if (drawAxes)
            DrawAxes(hdc);
        if (drawGrid)
            DrawGrid(hdc);

        DrawGraphs(hdc);
        EndPaint(hWnd, &ps);
    } break;
    case WM_GETMINMAXINFO: // Установка минимального размера окна
    {
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = DEFAULT_WIDTH;
        lpMMI->ptMinTrackSize.y = DEFAULT_HEIGHT;
    } break;
    case WM_SIZE:
        viewPort.wScreen = LOWORD(lParam) - CANVAS_OFFSET_L - 30;
        viewPort.hScreen = HIWORD(lParam) - CANVAS_OFFSET_T - 30;
        SetWindowPos(hSCR_scrollH, 0, viewPort.xScreen, viewPort.yScreen + viewPort.hScreen + 5, viewPort.wScreen, 15, SWP_SHOWWINDOW);
        SetWindowPos(hSCR_scrollV, 0, viewPort.xScreen + viewPort.wScreen + 5, viewPort.yScreen, 15, viewPort.hScreen, SWP_SHOWWINDOW);
        DefWindowProcW(hWnd, message, wParam, lParam);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}



INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG: {
    case WM_CREATE:
        hPNG_logo = LoadBitmapA(hInst, MAKEINTRESOURCEA(IDB_PNG1));
        hIMG_logo = GetDlgItem(hDlg, IDC_LOGO);
        SendMessage(hIMG_logo, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hPNG_logo);
    } return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        } break;
    }
    return (INT_PTR)FALSE;
}


INT_PTR CALLBACK Settings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG: {
        hBTN_axes = GetDlgItem(hDlg, IDC_CHK_AXES);
        hBTN_grid = GetDlgItem(hDlg, IDC_CHK_GRID);
        hSLD_quality = GetDlgItem(hDlg, IDC_SLDR_DETAILS);
        hLBL_stepMin = GetDlgItem(hDlg, IDC_LBL_STEP_MIN);
        hLBL_stepMax = GetDlgItem(hDlg, IDC_LBL_STEP_MAX);
        SendMessage(hSLD_quality, TBM_SETRANGEMAX, false, MAX_STEP);
        SendMessage(hSLD_quality, TBM_SETRANGEMIN, false, MIN_STEP);
        SendMessage(hSLD_quality, TBM_SETPOS, true, quality);
        SendMessage(hBTN_axes, BM_SETCHECK, drawAxes ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessage(hBTN_grid, BM_SETCHECK, drawGrid ? BST_CHECKED : BST_UNCHECKED, 0);

        TCHAR text[15] = { '\0' };
        StringCbPrintfW(text, 15, L"%d px", MAX_STEP);
        SetWindowText(hLBL_stepMax, text);
        StringCbPrintfW(text, 15, L"%d px", MIN_STEP);
        SetWindowText(hLBL_stepMin, text);

    } return (INT_PTR)TRUE;
    case WM_COMMAND: 
    {
        int wmId = LOWORD(wParam);
        switch (wmId) 
        {
        case IDOK:
            drawGrid = SendMessage(hBTN_grid, BM_GETCHECK, 0, 0);
            drawAxes = SendMessage(hBTN_axes, BM_GETCHECK, 0, 0);
            quality = SendMessage(hSLD_quality, TBM_GETPOS, 0, 0);
            EndDialog(hDlg, wmId);
            return (INT_PTR)TRUE;
        case IDCANCEL:
            EndDialog(hDlg, wmId);
            return (INT_PTR)FALSE;
        }
    }
    default: 
        return (INT_PTR)FALSE;
    }
}



void InitItems() {
    // hLST_expressions
    for (int i = 1; i <= MAX_PLOTS; i++)
        SendMessage(hLST_expressions, LB_ADDSTRING, 0, (LPARAM)L"");
    SendMessage(hLST_expressions, LB_SETCURSEL, 0, 0);

    // plotsColors
    for (int i = 0; i < MAX_COLORS; i++)
        SendMessageW(hCMB_colors, CB_ADDSTRING, 0, (LPARAM)getColorName(getColorByIndex(i)));
    for (int i = 0; i < MAX_PLOTS; i++)
        plotsColors[i] = i % MAX_COLORS;
    SendMessage(hCMB_colors, CB_SETCURSEL, 2, 0);

    // icons
    hICO_yes = LoadIconW(hInst, MAKEINTRESOURCE(IDI_YES));
    SendMessage(hBTN_save, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hICO_yes);
    hICO_no = LoadIconW(hInst, MAKEINTRESOURCE(IDI_NO));
    SendMessage(hBTN_clear, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hICO_no);
    hICO_graph = LoadIconW(hInst, MAKEINTRESOURCE(IDI_GRAPH));
    SendMessage(hBTN_redraw, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hICO_graph);
}


void ResetView(HWND hWnd) {
    TCHAR text[20] = { '\0' };
    StringCbPrintfW(text, 20, L"%.9f", viewPort.xReal);
    SetWindowText(hTXT_leftPos, text);
    StringCbPrintfW(text, 20, L"%.9f", viewPort.yReal);
    SetWindowText(hTXT_topPos, text);
    StringCbPrintfW(text, 20, L"%.9f", viewPort.xReal + viewPort.wReal);
    SetWindowText(hTXT_rightPos, text);
    StringCbPrintfW(text, 20, L"%.9f", viewPort.yReal + viewPort.hReal);
    SetWindowText(hTXT_botPos, text);
    Redraw(hWnd);
}


void Redraw(HWND hWnd) {
    RECT rect = viewPort.toRect();
    InvalidateRect(hWnd, &rect, true);
    UpdateWindow(hWnd);
}


void DrawGrid(HDC& hdc) {
    double stepX = viewPort.wScreen / 20.0;
    double stepY = viewPort.hScreen / 20.0;
    SelectObject(hdc, pGrid);
    for (int i = 0; i < 20; i++) {
        MoveToEx(hdc, viewPort.xScreen, viewPort.yScreen + i * stepY, 0);
        LineTo(hdc, viewPort.xScreen + viewPort.wScreen, viewPort.yScreen + i * stepY);
        MoveToEx(hdc, viewPort.xScreen + i * stepX, viewPort.yScreen, 0);
        LineTo(hdc, viewPort.xScreen + i * stepX, viewPort.yScreen + viewPort.hScreen);
    }
}


void DrawAxes(HDC& hdc) {
    double stepX = viewPort.wScreen / 20.0;
    double stepY = viewPort.hScreen / 20.0;

    SelectObject(hdc, pAxes);
    MoveToEx(hdc, viewPort.xScreen, viewPort.yScreen + viewPort.hScreen, 0);
    LineTo(hdc, viewPort.xScreen + viewPort.wScreen - 1, viewPort.yScreen + viewPort.hScreen);
    MoveToEx(hdc, viewPort.xScreen, viewPort.yScreen, 0);
    LineTo(hdc, viewPort.xScreen, viewPort.yScreen + viewPort.hScreen - 1);

    TCHAR label[8] = { '\0' };
    double dx = viewPort.wReal / 20.0;
    double dy = viewPort.hReal / 20.0;
    double x = viewPort.xReal + dx;
    double y = viewPort.yReal + viewPort.hReal - dy;
    bool useXscientific = abs(log10(viewPort.wReal)) > 4;
    bool useYscientific = abs(log10(viewPort.hReal)) > 4;

    // Метки и засечки на осях
    SetBkMode(hdc, TRANSPARENT);
    for (int i = 1; i < 20; i++) {
        if (i % 2 == 1) {
            if (useXscientific) StringCbPrintfW(label, 8, L"%.3e", x);
            else StringCbPrintfW(label, 10, L"%3.3f", x);
            TextOutW(hdc, viewPort.xScreen + i * stepX - 2, viewPort.yScreen + viewPort.hScreen - 20, label, 8);

            if (useYscientific) StringCbPrintfW(label, 8, L"%.3e", y);
            else StringCbPrintfW(label, 8, L"%3.3f", y);
            TextOutW(hdc, viewPort.xScreen + 6, viewPort.yScreen + i * stepY - 8, label, 8);

            MoveToEx(hdc, viewPort.xScreen + i * stepX, viewPort.yScreen + viewPort.hScreen, 0);
            LineTo(hdc, viewPort.xScreen + i * stepX, viewPort.yScreen + viewPort.hScreen - 4);
            MoveToEx(hdc, viewPort.xScreen, viewPort.yScreen + i * stepY, 0);
            LineTo(hdc, viewPort.xScreen + 4, viewPort.yScreen + i * stepY);
        }
        else {
            MoveToEx(hdc, viewPort.xScreen + i * stepX, viewPort.yScreen + viewPort.hScreen, 0);
            LineTo(hdc, viewPort.xScreen + i * stepX, viewPort.yScreen + viewPort.hScreen - 2);
            MoveToEx(hdc, viewPort.xScreen, viewPort.yScreen + i * stepY, 0);
            LineTo(hdc, viewPort.xScreen + 2, viewPort.yScreen + i * stepY);
        }

        y -= dy;
        x += dx;
    }
}


void DrawGraphs(HDC& hdc) {
    for (int i = 0; i < MAX_PLOTS; i++)
        DrawGraph(hdc, i);
}


void DrawGraph(HDC& hdc, int id) {
    if (id < 0 || id >= MAX_PLOTS) return;

    FunctionParser fp = expressions[id];
    if (fp.GetParseErrorType() != FunctionParser::FP_NO_ERROR) return;

    int rgb = getColorByIndex(plotsColors[id]);
    HPEN pen = CreatePen(PS_SOLID, 2, getColorByIndex(plotsColors[id]));
    SelectObject(hdc, pen);

    double dx = viewPort.wReal / ((double)viewPort.wScreen / (double)quality);

    double x[1] = { viewPort.xReal };
    double y = fp.Eval(x);
    double k = 1 / viewPort.hReal * viewPort.hScreen;
    int yOffset = viewPort.yScreen + viewPort.hScreen;
    MoveToEx(hdc, viewPort.xScreen, yOffset - (y - viewPort.yReal) * k, 0);

    for (int i = viewPort.xScreen + quality; i <= viewPort.xScreen + viewPort.wScreen; i += quality) {
        x[0] += dx;
        y = fp.Eval(x);
        LineTo(hdc, i, yOffset - (y - viewPort.yReal) * k);
    }
    DeleteObject(pen);
}