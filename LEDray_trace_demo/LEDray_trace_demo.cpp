// LEDray_trace_demo.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "LEDray_trace_demo.h"
#include <chrono>
#include <thread>
#include <string>
#include <sstream>
#include <iomanip>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

const auto STARTTIME = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
UINT framecount = 0;
auto fpsTime = STARTTIME;
std::wstring lastKeyDown = L"";
std::wstring lastKeyUp = L"";

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_LEDRAYTRACEDEMO, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LEDRAYTRACEDEMO));

    MSG msg;

    // Main message loop:
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

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LEDRAYTRACEDEMO));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_LEDRAYTRACEDEMO);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: UpdateToolbar(HWND)
//
//   PURPOSE: Creates the toolbar for the main window settings features
//
void UpdateToolbar(HWND hWnd, UINT RadioId) {
    HMENU hMenu = CreateMenu();

    //Start Menu Items here, remember to append to hMenu!

    HMENU hFramerates = CreateMenu();
    AppendMenuW(hFramerates, MF_DEFAULT, IDM_FRAMERATE_LOW, L"&5");
    AppendMenuW(hFramerates, MF_DEFAULT, IDM_FRAMERATE_MEDIUM, L"&15");
    AppendMenuW(hFramerates, MF_DEFAULT, IDM_FRAMERATE_HIGH, L"&30");
    AppendMenuW(hFramerates, MF_DEFAULT, IDM_FRAMERATE_UNLIMITED, L"&Unlimited (VSync)");
    AppendMenuW(hFramerates, MF_DEFAULT, IDM_FRAMERATE_CUSTOM, L"&Custom ...");
    CheckMenuRadioItem(hFramerates, IDM_FRAMERATE_RADIO_BEGIN, IDM_FRAMERATE_RADIO_END, RadioId, MF_BYCOMMAND);
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR) hFramerates, L"&Refresh Rate");

    AppendMenuW(hMenu, MF_DEFAULT, IDM_EXIT, L"&Exit");

    //End Menu Items here

    SetMenu(hWnd, hMenu);
    DrawMenuBar(hWnd);
}

void resetFramerateCounter() {
    fpsTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    framecount = 0;
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);

   UpdateToolbar(hWnd, IDM_FRAMERATE_LOW);

   SetTimer(hWnd, 1, 200, NULL);

   return TRUE;
}

//
//  FUNCTION: CustomFramerateProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Provides custom functionality for a framerate input dialog
//
INT_PTR CALLBACK CustomFramerateProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            wchar_t buffer[16];
            GetDlgItemText(hDlg, IDC_FRAMERATE_EDIT, buffer, 16);
            int fps = _wtoi(buffer);

            if (fps > 0)
            {
                EndDialog(hDlg, fps);
            }
            else
            {
                MessageBox(hDlg, L"Please enter a valid FPS value.", L"Error", MB_OK | MB_ICONERROR);
            }
        }
        return TRUE;

        case IDCANCEL:
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_FRAMERATE_LOW: 
                UpdateToolbar(hWnd, IDM_FRAMERATE_LOW);
                SetTimer(hWnd, 1, 200, NULL);
                resetFramerateCounter();
                break;
            case IDM_FRAMERATE_MEDIUM: 
                UpdateToolbar(hWnd, IDM_FRAMERATE_MEDIUM);
                SetTimer(hWnd, 1, 66, NULL);
                resetFramerateCounter();
                break;
            case IDM_FRAMERATE_HIGH: 
                UpdateToolbar(hWnd, IDM_FRAMERATE_HIGH);
                SetTimer(hWnd, 1, 33, NULL);
                resetFramerateCounter();
                break;
            case IDM_FRAMERATE_UNLIMITED: 
                UpdateToolbar(hWnd, IDM_FRAMERATE_UNLIMITED);
                SetTimer(hWnd, 1, 0, NULL);
                resetFramerateCounter();
                break;
            case IDM_FRAMERATE_CUSTOM:
            {
                int fps = DialogBox(hInst, MAKEINTRESOURCE(IDD_CUSTOM_FRAMERATE), hWnd, CustomFramerateProc);

                if (fps > 0)
                {
                    // Update menu radio selection
                    UpdateToolbar(hWnd, IDM_FRAMERATE_CUSTOM);

                    // Convert FPS to timer interval
                    UINT interval = 1000 / fps;
                    SetTimer(hWnd, 1, interval, NULL);
                }
            }
            break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            framecount++;
            auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            auto elapsedTime = currentTime-fpsTime;
            if(elapsedTime < 1) {
                elapsedTime = 1;
            }
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT fpsLocation = {10, 10, 200, 25};
            std::wstringstream wss;
            wss<<std::fixed<<std::setprecision(2)<<((1000.0*framecount)/elapsedTime);
            std::wstring fps = wss.str();
            DrawTextW(hdc, (L"Refresh Rate: "+fps).c_str(), -1, &fpsLocation, DT_LEFT);
            RECT keydownLocation = {10, 35, 200, 50};
            DrawTextW(hdc, (L"Last KeyDown: "+lastKeyDown).c_str(), -1, &keydownLocation, DT_LEFT);
            RECT keyupLocation = {10, 60, 200, 75};
            DrawTextW(hdc, (L"Last KeyUp: "+lastKeyUp).c_str(), -1, &keyupLocation, DT_LEFT);
            //TODO: Draw stuff here
            EndPaint(hWnd, &ps);
            if(elapsedTime > 1000) {
                resetFramerateCounter();
            }
        }
        break;
    case WM_KEYDOWN: 
        lastKeyDown = std::to_wstring(wParam);
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    case WM_KEYUP: 
        lastKeyUp = std::to_wstring(wParam);
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_TIMER: 
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
