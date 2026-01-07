// LEDray_trace_demo.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "LEDray_trace_demo.h"
#include <chrono>
#include <thread>
#include <mutex>
#include <string>
#include <sstream>
#include <iomanip>

#define MAX_LOADSTRING 100

#define RAYTRACE_WIDTH 1200
#define RAYTRACE_HEIGHT 600

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

const auto START_TIME = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
POINT centerPoint;

struct AppState {
    UINT framecount = 0;
    bool mouseCaptured = false;
    POINT mousePos = {0, 0};
    std::wstring lastKeyDown = L"";
    std::wstring lastKeyUp = L"";
    long long fpsTime = START_TIME;
    HMENU hMainMenu = nullptr;
    HMENU hFramerateMenu = nullptr;
    std::mutex mut;
    HDC hdesktop = nullptr;
    HBITMAP output = nullptr;
    HDC outputDC = nullptr;
};

AppState state;


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
    wcex.hbrBackground  = NULL;
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_LEDRAYTRACEDEMO);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//  FUNCTION: CenterCursor(HWND)
//
//  PURPOSE: Centers the cursor for when mouse is being captured
//
void CenterCursor(HWND hWnd) {
    RECT rect;
    GetClientRect(hWnd, &rect);
    POINT pt;
    pt.x = (rect.right - rect.left) / 2;
    pt.y = (rect.bottom - rect.top) / 2;
    ClientToScreen(hWnd, &pt);
    centerPoint = pt;
    SetCursorPos(pt.x, pt.y);
}


//
//  FUNCTION: EnableMouseCapture(HWND)
//
//  PURPOSE: Captures the mouse
//
void EnableMouseCapture(HWND hwnd) {
    SetCapture(hwnd);         // Capture mouse input
    SetCursor(NULL);        // Hide cursor
    CenterCursor(hwnd);       // Center it
    state.mouseCaptured = true;
}

//
//  FUNCTION: DisableMouseCapture()
//
//  PURPOSE: Releases the mouse
//
void DisableMouseCapture() {
    ReleaseCapture();
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    state.mouseCaptured = false;
}

//
//   FUNCTION: UpdateToolbar(HWND)
//
//   PURPOSE: Creates the toolbar for the main window settings features
//
void CreateMenuBar(HWND hWnd) {
    state.hMainMenu = CreateMenu();

    //Start Menu Items here, remember to append to hMenu!

    state.hFramerateMenu = CreateMenu();
    AppendMenuW(state.hFramerateMenu, MF_DEFAULT, IDM_FRAMERATE_LOW, L"&5");
    AppendMenuW(state.hFramerateMenu, MF_DEFAULT, IDM_FRAMERATE_MEDIUM, L"&15");
    AppendMenuW(state.hFramerateMenu, MF_DEFAULT, IDM_FRAMERATE_HIGH, L"&30");
    AppendMenuW(state.hFramerateMenu, MF_DEFAULT, IDM_FRAMERATE_VHIGH, L"&60");
    AppendMenuW(state.hFramerateMenu, MF_DEFAULT, IDM_FRAMERATE_CUSTOM, L"&Custom ...");
    CheckMenuRadioItem(state.hFramerateMenu, IDM_FRAMERATE_RADIO_BEGIN, IDM_FRAMERATE_RADIO_END, IDM_FRAMERATE_LOW, MF_BYCOMMAND);
    AppendMenuW(state.hMainMenu, MF_POPUP, (UINT_PTR) state.hFramerateMenu, L"&Refresh Rate");

    AppendMenuW(state.hMainMenu, MF_DEFAULT, IDM_EXIT, L"&Exit");

    //End Menu Items here

    SetMenu(hWnd, state.hMainMenu);
    DrawMenuBar(hWnd);
}

//
//  FUNCTION: UpdateFramerateRadio(UINT)
//
//  PURPOSE: Updates which radio item is checked
//
void UpdateFramerateRadio(UINT id) {
    CheckMenuRadioItem(
        state.hFramerateMenu,
        IDM_FRAMERATE_RADIO_BEGIN,
        IDM_FRAMERATE_RADIO_END,
        id,
        MF_BYCOMMAND
    );
}


void resetFramerateCounter() {
    state.fpsTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    state.framecount = 0;
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

   state.hdesktop = GetDC(0);
   state.output = CreateCompatibleBitmap(state.hdesktop, RAYTRACE_WIDTH, RAYTRACE_HEIGHT);
   state.outputDC = CreateCompatibleDC(state.hdesktop);

   SelectObject(state.outputDC, state.output);

   ShowWindow(hWnd, nCmdShow);

   CreateMenuBar(hWnd);

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
                UpdateFramerateRadio(IDM_FRAMERATE_LOW);
                SetTimer(hWnd, 1, 200, NULL);
                resetFramerateCounter();
                break;
            case IDM_FRAMERATE_MEDIUM: 
                UpdateFramerateRadio(IDM_FRAMERATE_MEDIUM);
                SetTimer(hWnd, 1, 66, NULL);
                resetFramerateCounter();
                break;
            case IDM_FRAMERATE_HIGH: 
                UpdateFramerateRadio(IDM_FRAMERATE_HIGH);
                SetTimer(hWnd, 1, 33, NULL);
                resetFramerateCounter();
                break;
            case IDM_FRAMERATE_VHIGH: 
                UpdateFramerateRadio(IDM_FRAMERATE_VHIGH);
                SetTimer(hWnd, 1, 16, NULL);
                resetFramerateCounter();
                break;
            case IDM_FRAMERATE_CUSTOM: {
                int fps = DialogBox(hInst, MAKEINTRESOURCE(IDD_CUSTOM_FRAMERATE), hWnd, CustomFramerateProc);
                if (fps > 0) {
                    UpdateFramerateRadio(IDM_FRAMERATE_CUSTOM);
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
            state.framecount++;
            auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            auto elapsedTime = currentTime-state.fpsTime;
            if(elapsedTime < 1) {
                elapsedTime = 1;
            }
            //Dont mind all the overlay
            RECT fpsLocation = {10, 10, 200, 25};
            std::wstringstream wss;
            wss<<std::fixed<<std::setprecision(2)<<((1000.0*state.framecount)/elapsedTime);
            std::wstring fps = wss.str();
            DrawTextW(state.outputDC, (L"Refresh Rate: "+fps).c_str(), -1, &fpsLocation, DT_LEFT);
            RECT keydownLocation = {10, 35, 200, 50};
            DrawTextW(state.outputDC, (L"Last KeyDown: "+state.lastKeyDown).c_str(), -1, &keydownLocation, DT_LEFT);
            RECT keyupLocation = {10, 60, 200, 75};
            DrawTextW(state.outputDC, (L"Last KeyUp: "+state.lastKeyUp).c_str(), -1, &keyupLocation, DT_LEFT);
            RECT mouseCapturedLocation = {10, 85, 200, 100};
            std::wstring mouseCap = (state.mouseCaptured) ? L"True" : L"False";
            DrawTextW(state.outputDC, (L"Mouse Captured? "+mouseCap).c_str(), -1, &mouseCapturedLocation, DT_LEFT);
            RECT mousePosLocation = {10, 110, 300, 125};
            std::wstringstream wss2;
            wss2<<"Mouse Position: ("<<state.mousePos.x<<", "<<state.mousePos.y<<")";
            std::wstring mPos = wss2.str();
            DrawTextW(state.outputDC, mPos.c_str(), -1, &mousePosLocation, DT_LEFT);

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rect;
            GetClientRect(hWnd, &rect);
            {
                std::lock_guard<std::mutex> lock(state.mut);
                StretchBlt(hdc, 0, 0, rect.right-rect.left, rect.bottom-rect.top, state.outputDC, 0, 0, RAYTRACE_WIDTH, RAYTRACE_HEIGHT, SRCCOPY);
                //RECT backRect = {0, 0, RAYTRACE_WIDTH, RAYTRACE_HEIGHT};
                //HBRUSH brush = (HBRUSH)GetStockObject(BLACK_BRUSH);
                //FillRect(state.outputDC, &backRect, brush);

            }
            EndPaint(hWnd, &ps);
            if(elapsedTime > 1000) {
                resetFramerateCounter();
            }
        }
        break;
    case WM_KEYDOWN: 
        state.lastKeyDown = std::to_wstring(wParam);
        InvalidateRect(hWnd, NULL, TRUE);
        if(wParam == VK_ESCAPE) {
            DisableMouseCapture();
        }
        break;
    case WM_KEYUP: 
        state.lastKeyUp = std::to_wstring(wParam);
        InvalidateRect(hWnd, NULL, TRUE);
        break;
    case WM_MOUSEMOVE:
        if (state.mouseCaptured) {
            POINT current;
            GetCursorPos(&current);

            int dx = current.x - centerPoint.x;
            int dy = current.y - centerPoint.y;

            state.mousePos.x += dx;
            state.mousePos.y += dy;

            if (dx != 0 || dy != 0) {
                CenterCursor(hWnd); // Reset to center for next frame
            }
        }
        break;
    case WM_LBUTTONDOWN: 
        EnableMouseCapture(hWnd);
        break;
    case WM_DESTROY:
        DisableMouseCapture();
        DeleteObject(state.output);
        DeleteDC(state.outputDC);
        ReleaseDC(NULL, state.hdesktop);
        PostQuitMessage(0);
        break;
    case WM_TIMER: 
        InvalidateRect(hWnd, NULL, TRUE);
        break;
    case WM_KILLFOCUS: 
    case WM_CAPTURECHANGED: 
        DisableMouseCapture();
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
