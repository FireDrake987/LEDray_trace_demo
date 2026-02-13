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
#include <vector>
#include <functional>
#include <random>
#include "Point3D.h"
#include "Vector.h"
#include "Quaternion.h"
#include "Material.h"
#include "Plane.h"
#include "Triangle.h"
#include "Camera.h"

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
void                loop();

const auto START_TIME = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
POINT centerPoint;

struct AppState {
    UINT framecount = 0;
    bool mouseCaptured = false;
    POINT mousePos = {0, 0};
    std::wstring lastKeyDown = L"";
    std::wstring lastKeyUp = L"";
    long long fpsTime = START_TIME;
    long frameDelay = 1000 * (1 / 5.0);// In ms
    HMENU hMainMenu = nullptr;
    HMENU hFramerateMenu = nullptr;
    std::mutex mut;
    HDC hdesktop = nullptr;
    HBITMAP output = nullptr;
    HDC outputDC = nullptr;
    int numThreads = 1;
    bool stopping = false;
    std::thread gameLoop = std::thread(loop);
    Camera cam = Camera(0, 0, 0, RAYTRACE_WIDTH, RAYTRACE_HEIGHT, Quaternion());
};

AppState state;

//Work jobs
std::mutex jobMut;
std::condition_variable cv;

struct RenderingJob {
    HDC *hdc;
    RECT bounds;
    std::function<void(HDC*, RECT, HDC)> work;
    RenderingJob(std::function<void(HDC*, RECT, HDC)> work, HDC* outputDC, RECT bounds) {
        this->work = work;
        this->hdc = outputDC;
        this->bounds = bounds;
    }
    RenderingJob() {
        bounds = RECT{0, 0, 0, 0};
        hdc = nullptr;
    }
};

std::vector<RenderingJob> renderJobs = std::vector<RenderingJob>();

struct RenderingThread {
    std::thread worker;
    bool terminate = false;
    HDC buffer = CreateCompatibleDC(0);
};

std::vector<RenderingThread> renderThreads = std::vector<RenderingThread>();
int runningThreads = 0;

void work(int id) {
    RenderingJob w{};
    while (!renderThreads.at(id).terminate) {
        {
            std::unique_lock<std::mutex> lock(jobMut);
            if (!renderJobs.empty()) {
                w = renderJobs.back();
                renderJobs.pop_back();
                runningThreads++;
            }
            else {
                cv.wait(lock);
                continue;
            }
        }
        w.work(w.hdc, w.bounds, renderThreads.at(id).buffer);
        {
            std::unique_lock<std::mutex> lock(jobMut);
            runningThreads --;
        }
    }
}

//
//  FUNCTION: clearThreads()
//
//  PURPOSE: Clear out the threads for deinitialization
//
void clearThreads() {
    for (int i = 0; i < renderThreads.size(); i++) {
        RenderingThread &rt = renderThreads.at(i);
        rt.terminate = true;
    }
    cv.notify_all();
    for (int i = 0; i < renderThreads.size(); i++) {
        RenderingThread& rt = renderThreads.at(i);
        if (rt.worker.joinable()) {
            rt.worker.join();
        }
    }
}

//
//  FUNCTION: createThreads()
//
//  PURPOSE: Clear out and (re)initialize threads according to state.numThreads
//
void createThreads() {
    clearThreads();
    renderThreads.clear();
    renderThreads.reserve(state.numThreads);
    for (int i = 0; i < state.numThreads; i++) {
        renderThreads.emplace_back();
        renderThreads.back().worker = std::thread(work, i);
    }
}

//
//  FUNCTION: renderWork()
//
//  PURPOSE: The worker function for rendering calls of the game loop
//
void renderWork(HDC *hdc, RECT bounds, HDC buffer) {

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bounds.right - bounds.left;
    bmi.bmiHeader.biHeight = -(bounds.bottom - bounds.top);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;
    void* pixels;
    HBITMAP bufferBitmap = CreateDIBSection(buffer, &bmi, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if(!bufferBitmap) return;
    SelectObject(buffer, bufferBitmap);

    std::vector<std::vector<BGRPixel>> data = state.cam.render(bounds.left, bounds.top, bounds.right, bounds.bottom);
    uint8_t* row = static_cast<uint8_t*>(pixels);
    int width = bounds.right - bounds.left;
    int height = bounds.bottom - bounds.top;
    int stride = ((width * 3 + 3) & ~3);
    for (int y = 0; y < height; y ++) {
        uint8_t* px = row + y * stride;
        for (int x = 0; x < width; x ++) {
            BGRPixel* ref = &data.at(y).at(x);
            px[x*3 + 0] = ref->b; // blue
            px[x*3 + 1] = ref->g; // green
            px[x*3 + 2] = ref->r; // red
        }
    }

    /*std::vector<std::vector<BGRPixel>> data = state.cam.render(bounds.left, bounds.top, bounds.right, bounds.bottom);
    std::vector<BGRPixel> dataAsOne = std::vector<BGRPixel>();
    for (std::vector<BGRPixel> arr : data) {
        dataAsOne.insert(dataAsOne.end(), arr.begin(), arr.end());
    }
    pixels = dataAsOne.data();*/

    {
        std::unique_lock<std::mutex> lock(state.mut);
        BitBlt(*hdc, bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top, buffer, 0, 0, SRCCOPY);
    }
    DeleteObject(bufferBitmap);
}

//
//  FUNCTION: loop()
//
//  PURPOSE: Runs game loop, automatically refreshes. Designed to be run on separate thread
//
void loop() {
    auto lastTime = std::chrono::steady_clock::now();
    const std::chrono::nanoseconds frameLength(state.frameDelay * 1000000);
    while (!state.stopping) {
        auto currentTime = std::chrono::steady_clock::now();
        auto deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        {
            std::unique_lock<std::mutex> lock(jobMut);
            for(int i = 0, x = 0; i < 4; x += (int)(RAYTRACE_WIDTH / 4), i ++) {
                for(int j = 0, y = 0; j < 4; y += (int)(RAYTRACE_HEIGHT / 4), j ++) {
                    std::function<void(HDC*, RECT, HDC)> render = renderWork;
                    RenderingJob job(render, &state.outputDC, RECT{ x, y, x + RAYTRACE_WIDTH/4, y + RAYTRACE_HEIGHT/4});
                    renderJobs.push_back(job);
                    cv.notify_one();
                }
            }
        }
        auto timeTaken = std::chrono::steady_clock::now() - currentTime;
        if (timeTaken < frameLength) {
            std::this_thread::sleep_for(frameLength - timeTaken);
        }
    }
}

//
//  FUNCTION: setupScene()
//
//  PURPOSE: Defines the scene to be raytraced
//
void setupScene() {
    state.cam.scene = std::vector<Plane>();
    state.cam.scene.push_back(Triangle(Material(BGRPixel{ 255, 0, 0 }), Point3D(1, 1, 1), Point3D(1, 0, 1), Point3D(0, 0, 1)));
    state.cam.scene.push_back(Triangle(Material(BGRPixel{ 0, 255, 0 }), Point3D(1, 1, -1), Point3D(1, 0, -1), Point3D(0, 0, -1)));
}

//
//  FUNCTION: wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
//
//  PURPOSE: Entry point for project
//
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    

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
    WNDCLASSEXW wcex{};

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
    POINT pt{};
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

   createThreads();

   state.hdesktop = GetDC(0);
   state.output = CreateCompatibleBitmap(state.hdesktop, RAYTRACE_WIDTH, RAYTRACE_HEIGHT);
   state.outputDC = CreateCompatibleDC(state.hdesktop);

   SelectObject(state.outputDC, state.output);

   ShowWindow(hWnd, nCmdShow);

   CreateMenuBar(hWnd);

   SetTimer(hWnd, 1, 200, NULL);

   state.frameDelay = 200;

   setupScene();

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
                state.frameDelay = 200;
                resetFramerateCounter();
                break;
            case IDM_FRAMERATE_MEDIUM: 
                UpdateFramerateRadio(IDM_FRAMERATE_MEDIUM);
                SetTimer(hWnd, 1, 66, NULL);
                state.frameDelay = 66;
                resetFramerateCounter();
                break;
            case IDM_FRAMERATE_HIGH: 
                UpdateFramerateRadio(IDM_FRAMERATE_HIGH);
                SetTimer(hWnd, 1, 33, NULL);
                state.frameDelay = 33;
                resetFramerateCounter();
                break;
            case IDM_FRAMERATE_VHIGH: 
                UpdateFramerateRadio(IDM_FRAMERATE_VHIGH);
                SetTimer(hWnd, 1, 16, NULL);
                state.frameDelay = 16;
                resetFramerateCounter();
                break;
            case IDM_FRAMERATE_CUSTOM: {
                int fps = DialogBox(hInst, MAKEINTRESOURCE(IDD_CUSTOM_FRAMERATE), hWnd, CustomFramerateProc);
                if (fps > 0) {
                    UpdateFramerateRadio(IDM_FRAMERATE_CUSTOM);
                    UINT interval = 1000 / fps;
                    SetTimer(hWnd, 1, interval, NULL);
                    state.frameDelay = interval;
                }
                resetFramerateCounter();
            }
                break;
            case IDM_EXIT:
                state.stopping = true;
                clearThreads();
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
            {
                std::unique_lock<std::mutex> lock(state.mut);
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
                RECT numJobsLocation = {10, 135, 200, 150};
                std::wstring jobsString = L"# Jobs: " + std::to_wstring(renderJobs.size());
                DrawTextW(state.outputDC, jobsString.c_str(), -1, &numJobsLocation, DT_LEFT);

                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);

                RECT rect;
                GetClientRect(hWnd, &rect);

                StretchBlt(hdc, 0, 0, rect.right-rect.left, rect.bottom-rect.top, state.outputDC, 0, 0, RAYTRACE_WIDTH, RAYTRACE_HEIGHT, SRCCOPY);

                EndPaint(hWnd, &ps);
            }
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
        clearThreads();
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
