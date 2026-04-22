// LEDray_trace_demo.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include <commdlg.h>
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
#include <fstream>
#include <iostream>
#include "Point3D.h"
#include "Vector.h"
#include "Quaternion.h"
#include "Material.h"
#include "Plane.h"
#include "Triangle.h"
#include "Camera.h"
#include <shared_mutex>
#include "MaterialReflective.h"

#define MAX_LOADSTRING 100

#define RAYTRACE_WIDTH 400
#define RAYTRACE_HEIGHT 200

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
    const int scale = 3;
    UINT framecount = 0;
    bool mouseCaptured = false;
    POINT mousePos = {0, 0};
    std::wstring lastKeyDown = L"";
    std::wstring lastKeyUp = L"";
    long long fpsTime = START_TIME;
	long long lastFrameTime = START_TIME;
	std::vector<long long> frameTimes;
    long frameDelay = 1000 * (1 / 5.0);// In ms
    HMENU hMainMenu = nullptr;
    HMENU hFramerateMenu = nullptr;
	HMENU hRenderModeMenu = nullptr;
	HMENU hFOVMenu = nullptr;
    HMENU hThreadsMenu = nullptr;
    std::mutex mut;
    HDC hdesktop = nullptr;
    HBITMAP output = nullptr;
    HDC outputDC = nullptr;
    int numThreads = 1;
    bool stopping = false;
    std::thread gameLoop = std::thread(loop);
    Camera cam = Camera(-2, 1.5, -2, RAYTRACE_WIDTH, RAYTRACE_HEIGHT, Quaternion());
    bool debug = false;
	int tilesX = 4;
	int tilesY = 4;
    bool useColor = false;
    int red = 0;
    int green = 255;
    int blue = 255;
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
std::atomic<int> runningThreads = 0;

//
//  FUNCTION: work(int)
//
//  PURPOSE: The worker function for rendering threads, waits for jobs to be added to the renderJobs vector and executes them
//
void work(int id) {
    RenderingJob w{};
    while (!renderThreads.at(id).terminate) {
        {
            std::unique_lock<std::mutex> lock(jobMut);
            if(!renderJobs.empty()) {
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
    for(int i = 0; i < renderThreads.size(); i++) {
        RenderingThread& rt = renderThreads.at(i);
        rt.terminate = true;
    }
    cv.notify_all();
    for(int i = 0; i < renderThreads.size(); i++) {
        RenderingThread& rt = renderThreads.at(i);
        if(rt.worker.joinable()) {
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
    std::vector<BGRPixel> data;

    data = state.cam.render(bounds.left, bounds.top, bounds.right, bounds.bottom);
    pixels = data.data();

    {
        std::unique_lock<std::mutex> lock(state.mut);
		SetDIBitsToDevice(*hdc, bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top, 0, 0, 0, bounds.bottom - bounds.top, data.data(), &bmi, DIB_RGB_COLORS);
    }
}

//
//  FUNCTION: loop()
//
//  PURPOSE: Runs game loop, automatically refreshes. Designed to be run on separate thread
//
void loop() {
    auto lastTime = std::chrono::steady_clock::now();
    const std::chrono::nanoseconds frameLength(state.frameDelay * 1000000);
    while(!state.stopping) {
        auto currentTime = std::chrono::steady_clock::now();
        auto deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        {
            std::unique_lock<std::mutex> lock(jobMut);
            if(state.mousePos.x != 0 || state.mousePos.y != 0) {
                state.cam.eulerRotate(state.mousePos.x / 200.0, -state.mousePos.y / 200.0);
                state.mousePos.x = 0;
                state.mousePos.y = 0;
            }
            if(renderJobs.size() == 0) {//Only actually render when the previous frame is done
                long long currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                if (state.frameTimes.size() < 501) {
                    state.frameTimes.push_back(currentTime - state.lastFrameTime);
                }
                if(state.frameTimes.size() == 500) {
                    //Stats calculation for average frame time and SE
                    long long sum = 0;
                    long long max = 0;
					long long min = LLONG_MAX;
                    for (long long time : state.frameTimes) {
                        sum += time;
                        if(time > max) {
                            max = time;
						}
                        if(time < min) {
                            min = time;
						}
                    }
					double average = sum / (state.frameTimes.size() * 1.0);
                    double diffsqsum = 0;
                    for (long long time : state.frameTimes) {
						double diff = time - average;
                        double diffsq = diff * diff;
                        diffsqsum += diffsq;
                    }
					double stdev = sqrtf(diffsqsum / (state.frameTimes.size() * 1.0));
					double sterr = stdev / sqrtf(state.frameTimes.size());
					double sterr2 = sterr * 2;
				}
                state.lastFrameTime = currentTime;
                for(int i = 0, x = 0; i < state.tilesX; x += (int)(RAYTRACE_WIDTH / state.tilesX), i++) {
                    for(int j = 0, y = 0; j < state.tilesY; y += (int)(RAYTRACE_HEIGHT / state.tilesY), j++) {
                        std::function<void(HDC*, RECT, HDC)> render = renderWork;
                        RenderingJob job(render, &state.outputDC, RECT{x, y, x + RAYTRACE_WIDTH / state.tilesX, y + RAYTRACE_HEIGHT / state.tilesY});
                        renderJobs.push_back(job);
                        cv.notify_all();
                    }
                }
            }
        }
        auto timeTaken = std::chrono::steady_clock::now() - currentTime;
        if(timeTaken < frameLength) {
            std::this_thread::sleep_for(frameLength - timeTaken);
        }
    }
}

//
//  FUNCTION: getFaceData(std::istringstream, int, int, int)
//
//  PURPOSE: Gets the blender .obj face data from the istringstream and places the result in the passed in ints by reference, accounts for the different formats that blender can export faces in (v, v/vt, v//vn, v/vt/vn)
//
void getFaceData(std::istringstream& iss, int& v, int& vt, int& vn) {
    char slash;
    iss >> v;
    if(iss.peek() == '/') {
        iss >> slash;
        if(iss.peek() == '/') {
            iss >> slash >> vn;
        }
        else {
            iss >> vt;
            if(iss.peek() == '/') {
                iss >> slash >> vn;
            }
        }
    }
}

std::vector<Point3D> vertices;
std::vector<std::unique_ptr<Plane>> planes;
void createSceneFromFile(std::ifstream file);//Forward declaration

//
//  FUNCTION: setupScene()
//
//  PURPOSE: Defines the scene to be raytraced
//
void setupScene() {
	std::ifstream file("scene.obj");
	createSceneFromFile(std::move(file));
}

//
//  FUNCTION: createSceneFromFile(std::ifstream)
//
//  PURPOSE: Reads a blender .obj file from the passed in ifstream and creates planes and vertices according to the data, adds the planes to the camera's scene. Accounts for different formats that blender can export faces in (v, v/vt, v//vn, v/vt/vn)
//
void createSceneFromFile(std::ifstream file) {
    std::unique_lock<std::shared_mutex> lock(state.cam.invalidateMut);
	int baseIndexV = vertices.size();
    std::string line;
    if(!file.is_open()) {
        MessageBox(NULL, L"Failed to open scene.obj", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    while (std::getline(file, line)) {
        if(line.empty()) continue;
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        if(command == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            vertices.emplace_back(Point3D(x, y, z));
        }
        else if(command == "f") {
            int v1, v2, v3, vt1, vt2, vt3, vn1, vn2, vn3;
            getFaceData(iss, v1, vt1, vn1);
            getFaceData(iss, v2, vt2, vn2);
            getFaceData(iss, v3, vt3, vn3);
            uint8_t b, g, r;
            if(!state.useColor) {
                uint32_t col = rand() % (256 * 256 * 256);
                b = col % 256;
                g = (col / 256) % 256;
                r = ((col / 256) / 256) % 256;
            }
            else {
                b = state.blue % 256;
                g = state.green % 256;
                r = state.red % 256;
            }
            Material mat = Material(BGRPixel{ b, g, r });
            planes.emplace_back(std::make_unique<Triangle>(mat, vertices.at(baseIndexV + v1 - 1), vertices.at(baseIndexV + v2 - 1), vertices.at(baseIndexV + v3 - 1)));
        }
    }
    for (std::unique_ptr<Plane>& plane : planes) {
        state.cam.scene.push_back(plane.get());
    }
    state.cam.invalidate();
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
    if(!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LEDRAYTRACEDEMO));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if(!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
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

	state.hRenderModeMenu = CreateMenu();
    AppendMenuW(state.hRenderModeMenu, MF_DEFAULT, IDM_RENDERMODE_FLAT, L"&Flat");
    AppendMenuW(state.hRenderModeMenu, MF_DEFAULT, IDM_RENDERMODE_CURVED, L"&Curved");
    CheckMenuRadioItem(state.hRenderModeMenu, IDM_RENDERMODE_RADIO_BEGIN, IDM_RENDERMODE_RADIO_END, IDM_RENDERMODE_FLAT, MF_BYCOMMAND);
	AppendMenuW(state.hMainMenu, MF_POPUP, (UINT_PTR)state.hRenderModeMenu, L"&Render Mode");

	state.hFOVMenu = CreateMenu();
	AppendMenuW(state.hFOVMenu, MF_DEFAULT, IDM_FOV_LOW, L"&Narrow");
	AppendMenuW(state.hFOVMenu, MF_DEFAULT, IDM_FOV_MEDIUM, L"&Medium");
	AppendMenuW(state.hFOVMenu, MF_DEFAULT, IDM_FOV_HIGH, L"&Wide");
	AppendMenuW(state.hFOVMenu, MF_DEFAULT, IDM_FOV_CUSTOM, L"&Custom ...");
	CheckMenuRadioItem(state.hFOVMenu, IDM_FOV_RADIO_BEGIN, IDM_FOV_RADIO_END, IDM_FOV_MEDIUM, MF_BYCOMMAND);
	AppendMenuW(state.hMainMenu, MF_POPUP, (UINT_PTR)state.hFOVMenu, L"&Field of View");

    AppendMenuW(state.hMainMenu, MF_DEFAULT, IDM_IMPORT_SCENE, L"&Import scene ...");

    AppendMenuW(state.hMainMenu, MF_DEFAULT, IDM_CLEAR_SCENE, L"&Clear scene");

    state.hThreadsMenu = CreateMenu();
    AppendMenuW(state.hThreadsMenu, MF_DEFAULT, IDM_THREADS_AUTO, L"&Auto");
    AppendMenuW(state.hThreadsMenu, MF_DEFAULT, IDM_THREADS_ONE, L"&1");
    AppendMenuW(state.hThreadsMenu, MF_DEFAULT, IDM_THREADS_TWO, L"&2");
    AppendMenuW(state.hThreadsMenu, MF_DEFAULT, IDM_THREADS_FOUR, L"&4");
    AppendMenuW(state.hThreadsMenu, MF_DEFAULT, IDM_THREADS_EIGHT, L"&8");
    AppendMenuW(state.hThreadsMenu, MF_DEFAULT, IDM_THREADS_CUSTOM, L"&Custom ...");
    CheckMenuRadioItem(state.hThreadsMenu, IDM_THREADS_RADIO_BEGIN, IDM_THREADS_RADIO_END, IDM_THREADS_AUTO, MF_BYCOMMAND);
    AppendMenuW(state.hMainMenu, MF_POPUP, (UINT_PTR)state.hThreadsMenu, L"&# Threads");

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

//
//  FUNCTION: UpdateRenderModeRadio(UINT)
//
//  PURPOSE: Updates which render mode radio item is checked
//
void UpdateRenderModeRadio(UINT id) {
    CheckMenuRadioItem(
        state.hRenderModeMenu,
        IDM_RENDERMODE_RADIO_BEGIN,
        IDM_RENDERMODE_RADIO_END,
        id,
        MF_BYCOMMAND
    );
}

//
//  FUNCTION: UpdateFOVRadio(UINT)
//
//  PURPOSE: Updates which fov radio item is checked
//
void UpdateFOVRadio(UINT id) {
    CheckMenuRadioItem(
        state.hFOVMenu,
        IDM_FOV_RADIO_BEGIN,
        IDM_FOV_RADIO_END,
        id,
        MF_BYCOMMAND
    );
}

//
//  FUNCTION: UpdateThreadsRadio(UINT)
//
//  PURPOSE: Updates which threads radio item is checked
//
void UpdateThreadsRadio(UINT id) {
    CheckMenuRadioItem(
        state.hThreadsMenu, 
        IDM_THREADS_RADIO_BEGIN, 
        IDM_THREADS_RADIO_END, 
        id, 
        MF_BYCOMMAND
    );
}

//
//  FUNCTION: resetFramerateCounter()
//
//  PURPOSE: Resets the display rate counter for accurate fps measurement when changing display rate settings
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
//        create, display the main program window, and initialize various one-time inits.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance; // Store instance handle in our global variable

    RECT windowRect = {0, 0, RAYTRACE_WIDTH * state.scale, RAYTRACE_HEIGHT * state.scale};
    DWORD dwStyle = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&windowRect, dwStyle, FALSE);

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, dwStyle,
       CW_USEDEFAULT, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, nullptr, nullptr, hInstance, nullptr);

    if(!hWnd) {
       return FALSE;
    }

    int threadsAvailable = std::thread::hardware_concurrency();

    state.numThreads = threadsAvailable - 2;
   
    if (state.numThreads < 1) {
        state.numThreads = 1;
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

    Camera::type = Camera::FLAT;

    renderJobs.reserve(state.tilesX * state.tilesY);

    setupScene();

    return TRUE;
}

//
//  FUNCTION: CustomFramerateProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Provides custom functionality for a framerate input dialog
//
INT_PTR CALLBACK CustomFramerateProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
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

            if(fps > 0)
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
//  FUNCTION: CustomFOVProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Provides custom functionality for a fov input dialog
//
INT_PTR CALLBACK CustomFOVProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
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
            GetDlgItemText(hDlg, IDC_FOV_EDIT, buffer, 16);
            int fov = _wtoi(buffer);
            EndDialog(hDlg, fov);
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
//  FUNCTION: CustomSceneProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Provides custom functionality for a scene import dialog, allows the user to select whether to use a custom color or random colors for the imported scene, and if custom is selected allows the user to input RGB values for the color
//
INT_PTR CALLBACK CustomSceneProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message)
    {
    case WM_INITDIALOG:
		CheckRadioButton(hDlg, IDC_SCENE_COLOR_RANDOM, IDC_SCENE_COLOR_CUSTOM, IDC_SCENE_COLOR_RANDOM);
        SetDlgItemInt(hDlg, IDC_SCENE_COLOR_RED, 0, FALSE);
        SetDlgItemInt(hDlg, IDC_SCENE_COLOR_GREEN, 0, FALSE);
        SetDlgItemInt(hDlg, IDC_SCENE_COLOR_BLUE, 0, FALSE);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_SCENE_COLOR_CUSTOM: 
            if(HIWORD(wParam) == BN_CLICKED) {
                EnableWindow(GetDlgItem(hDlg, IDC_SCENE_COLOR_RED), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_SCENE_COLOR_GREEN), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_SCENE_COLOR_BLUE), TRUE);
            }
            break;
        case IDC_SCENE_COLOR_RANDOM: 
            if(HIWORD(wParam) == BN_CLICKED) {
                EnableWindow(GetDlgItem(hDlg, IDC_SCENE_COLOR_RED), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_SCENE_COLOR_GREEN), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_SCENE_COLOR_BLUE), FALSE);
            }
            break;
        case IDC_SCENE_COLOR_RED: 
            if(HIWORD(wParam) == EN_UPDATE) {
                wchar_t buffer[16];
                GetDlgItemText(hDlg, IDC_SCENE_COLOR_RED, buffer, 16);
                UINT red = _wtoi(buffer);
                if(red > 255) {
                    SetDlgItemInt(hDlg, IDC_SCENE_COLOR_RED, 255, FALSE);
                }
            }
            break;
        case IDC_SCENE_COLOR_GREEN:
            if(HIWORD(wParam) == EN_UPDATE) {
                wchar_t buffer[16];
                GetDlgItemText(hDlg, IDC_SCENE_COLOR_GREEN, buffer, 16);
                UINT green = _wtoi(buffer);
                if(green > 255) {
                    SetDlgItemInt(hDlg, IDC_SCENE_COLOR_GREEN, 255, FALSE);
                }
            }
            break;
        case IDC_SCENE_COLOR_BLUE:
            if(HIWORD(wParam) == EN_UPDATE) {
                wchar_t buffer[16];
                GetDlgItemText(hDlg, IDC_SCENE_COLOR_BLUE, buffer, 16);
                UINT blue = _wtoi(buffer);
                if(blue > 255) {
                    SetDlgItemInt(hDlg, IDC_SCENE_COLOR_BLUE, 255, FALSE);
                }
            }
            break;
        case IDOK: {
            if(IsDlgButtonChecked(hDlg, IDC_SCENE_COLOR_CUSTOM)) {
                wchar_t buffer[16];
                GetDlgItemText(hDlg, IDC_SCENE_COLOR_RED, buffer, 16);
                state.red = _wtoi(buffer);
                GetDlgItemText(hDlg, IDC_SCENE_COLOR_GREEN, buffer, 16);
                state.green = _wtoi(buffer);
                GetDlgItemText(hDlg, IDC_SCENE_COLOR_BLUE, buffer, 16);
                state.blue = _wtoi(buffer);
                state.useColor = true;
            }
            if(IsDlgButtonChecked(hDlg, IDC_SCENE_COLOR_RANDOM)) {
                state.useColor = false;
            }
            EndDialog(hDlg, 1);
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
//  FUNCTION: CustomThreadsProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Provides custom functionality for a threads controller dialog
//
INT_PTR CALLBACK CustomThreadsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        {
            wchar_t buffer[16];
            GetDlgItemText(hDlg, IDC_THREADS_EDIT, buffer, 16);
            int threads = _wtoi(buffer);

            if(threads > 0) {
                EndDialog(hDlg, threads);
            }
            else {
                MessageBox(hDlg, L"Please enter a valid value.", L"Error", MB_OK | MB_ICONERROR);
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
//  FUNCTION: OpenFileSelectionDialog(HWND)
//
//  PURPOSE: Opens a file selection dialog for importing .obj files, if a file is successfully selected it is loaded into the scene using createSceneFromFile()
void OpenFileSelectionDialog(HWND hwnd)
{
    OPENFILENAME ofn;        // Common dialog box structure
    TCHAR szFile[MAX_PATH] = { 0 }; // Buffer for file name
    // wide character string for filters (double null terminated)
    const TCHAR szFilter[] = _T("Object Files (*.obj)\0*.obj\0All Files (*.*)\0*.*\0");

    // Initialize OPENFILENAME
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    // Set lpstrFile[0] to null, so that GetOpenFileName does not use the contents of szFile to initialize itself
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;//Default to the first filter ("*.obj")
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;//Ensure the selected path and file exist

    if(GetOpenFileName(&ofn) == TRUE) {
		std::ifstream file(ofn.lpstrFile);
        createSceneFromFile(std::move(file));
		state.cam.invalidate();
    }
    else {
        MessageBox(NULL, L"Failed to open scene", L"Error", MB_OK | MB_ICONERROR);
    }
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//  WM_TIMER    - Triggered on game loop timer refresh, causes the window to repaint and the next frame to be rendered
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
                if(fps > 0) {
                    UpdateFramerateRadio(IDM_FRAMERATE_CUSTOM);
                    UINT interval = 1000 / fps;
                    SetTimer(hWnd, 1, interval, NULL);
                    state.frameDelay = interval;
                }
                resetFramerateCounter();
            }
                break;
			case IDM_RENDERMODE_FLAT:
				UpdateRenderModeRadio(IDM_RENDERMODE_FLAT);
				Camera::type = Camera::FLAT;
                state.cam.invalidate();
				break;
			case IDM_RENDERMODE_CURVED:
				UpdateRenderModeRadio(IDM_RENDERMODE_CURVED);
				Camera::type = Camera::CURVED;
                state.cam.invalidate();
				break;
			case IDM_FOV_LOW:
				UpdateFOVRadio(IDM_FOV_LOW);
				state.cam.setFOV(31/180.0, 31/180.0);
				break;
			case IDM_FOV_MEDIUM:
				UpdateFOVRadio(IDM_FOV_MEDIUM);
				state.cam.setFOV((3.1415) / 2, (3.1415) / 2);
				break;
			case IDM_FOV_HIGH:
				UpdateFOVRadio(IDM_FOV_HIGH);
				state.cam.setFOV((3.1415) * 5 / 6, (3.1415) * 5 / 6);
				break;
            case IDM_FOV_CUSTOM: {
				int fov = DialogBox(hInst, MAKEINTRESOURCE(IDD_CUSTOM_FOV), hWnd, CustomFOVProc);
				if(fov > 0) {
                    state.cam.setFOV(3.1415 * fov / 180.0, 3.1415 * fov / 180.0);
                }
                UpdateFOVRadio(IDM_FOV_CUSTOM);
            }
				break;
            case IDM_IMPORT_SCENE: {
                if(DialogBox(hInst, MAKEINTRESOURCE(IDD_CUSTOM_SCENE), hWnd, CustomSceneProc)) {
                    OpenFileSelectionDialog(hWnd);
                }
                break;
			}
            case IDM_CLEAR_SCENE: {
                std::unique_lock<std::shared_mutex> lock(state.cam.invalidateMut);
                state.cam.scene.clear();
                vertices.clear();
                planes.clear();
                state.cam.invalidate();
            }
                break;
            case IDM_THREADS_AUTO: 
                state.numThreads = std::thread::hardware_concurrency() - 2;
                if(state.numThreads < 1) state.numThreads = 1;
                createThreads();
                UpdateThreadsRadio(IDM_THREADS_AUTO);
                break;
            case IDM_THREADS_ONE: 
                state.numThreads = 1;
                createThreads();
                UpdateThreadsRadio(IDM_THREADS_ONE);
                break;
            case IDM_THREADS_TWO: 
                state.numThreads = 2;
                createThreads();
                UpdateThreadsRadio(IDM_THREADS_TWO);
                break;
            case IDM_THREADS_FOUR: 
                state.numThreads = 4;
                createThreads();
                UpdateThreadsRadio(IDM_THREADS_FOUR);
                break;
            case IDM_THREADS_EIGHT: 
                state.numThreads = 8;
                createThreads();
                UpdateThreadsRadio(IDM_THREADS_EIGHT);
                break;
            case IDM_THREADS_CUSTOM: {
                int threads = DialogBox(hInst, MAKEINTRESOURCE(IDD_CUSTOM_THREADS), hWnd, CustomThreadsProc);
                if (threads > 0) {
                    state.numThreads = threads;
                }
                createThreads();
                UpdateThreadsRadio(IDM_THREADS_CUSTOM);
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
            {
                std::unique_lock<std::mutex> lock(state.mut);

                if(state.debug) {
                    RECT fpsLocation = { 10, 10, 200, 25 };
                    std::wstringstream wss;
                    wss << std::fixed << std::setprecision(2) << ((1000.0 * state.framecount) / elapsedTime);
                    std::wstring fps = wss.str();
                    DrawTextW(state.outputDC, (L"Refresh Rate: " + fps).c_str(), -1, &fpsLocation, DT_LEFT);
                    RECT keydownLocation = { 10, 35, 200, 50 };
                    DrawTextW(state.outputDC, (L"Last KeyDown: " + state.lastKeyDown).c_str(), -1, &keydownLocation, DT_LEFT);
                    RECT keyupLocation = { 10, 60, 200, 75 };
                    DrawTextW(state.outputDC, (L"Last KeyUp: " + state.lastKeyUp).c_str(), -1, &keyupLocation, DT_LEFT);
                    RECT mouseCapturedLocation = { 10, 85, 200, 100 };
                    std::wstring mouseCap = (state.mouseCaptured) ? L"True" : L"False";
                    DrawTextW(state.outputDC, (L"Mouse Captured? " + mouseCap).c_str(), -1, &mouseCapturedLocation, DT_LEFT);
                    RECT mousePosLocation = { 10, 110, 300, 125 };
                    std::wstringstream wss2;
                    wss2 << "Mouse Position: (" << state.mousePos.x << ", " << state.mousePos.y << ")";
                    std::wstring mPos = wss2.str();
                    DrawTextW(state.outputDC, mPos.c_str(), -1, &mousePosLocation, DT_LEFT);
                    RECT numJobsLocation = { 10, 135, 200, 150 };
                    std::wstring jobsString = L"# Jobs: " + std::to_wstring(renderJobs.size());
                    DrawTextW(state.outputDC, jobsString.c_str(), -1, &numJobsLocation, DT_LEFT);
                }

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
        if(wParam == VK_ESCAPE) {
            DisableMouseCapture();
        }
        if(wParam == 118) {//f7
			state.frameTimes.clear();
		}
        if(wParam == 117) {//f6
            state.cam.setRot(Quaternion());
            state.cam.setFOV((3.1415) / 2, 5 * (3.1415) / 12);
		}
		if(wParam == 116) {//f5
            if(Camera::type == Camera::FLAT) {
                Camera::type = Camera::CURVED;
            }
            else {
                Camera::type = Camera::FLAT;
            }
        }
        if(wParam == 114) {//f3
            state.debug = !state.debug;
        }
        if(wParam == 113) {//f2
            state.cam.setFOV(10, 10);
        }
		if(wParam == 87) {//w
            state.cam.move(0, 0, 0.1);
        }
        if(wParam == 65) {//a
            state.cam.move(-0.1, 0, 0);
        }
        if(wParam == 83) {//s
            state.cam.move(0, 0, -0.1);
        }
        if(wParam == 68) {//d
            state.cam.move(0.1, 0, 0);
        }
        if(wParam == 32) {//space
            state.cam.move(0, -0.1, 0);
        }
        if(wParam == 16) {//shift
            state.cam.move(0, 0.1, 0);
        }
        break;
    case WM_KEYUP: 
        state.lastKeyUp = std::to_wstring(wParam);
        break;
    case WM_MOUSEMOVE:
        if(state.mouseCaptured) {
            POINT current;
            GetCursorPos(&current);

            int dx = current.x - centerPoint.x;
            int dy = current.y - centerPoint.y;

            state.mousePos.x += dx;
            state.mousePos.y += dy;

            if(dx != 0 || dy != 0) {
                CenterCursor(hWnd);//Reset to center for next frame
            }
        }
        break;
    case WM_MOUSEWHEEL:
	    state.cam.eulerRotate(0, 0, GET_WHEEL_DELTA_WPARAM(wParam) / 1200.0);
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
