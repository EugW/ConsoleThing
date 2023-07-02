#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <hidsdi.h>
#include <d2d1_3.h>
#include <d3d11_1.h>
#include <iostream>
#include <timeapi.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI DrawThread(LPVOID param);
void Init();
void InitDX();
void InitEffects();
void GetSwapChain(HWND hwnd);
void AnimateRect();
void AnimateFade();
void PrecSleep(int dur);
void ParseRawInput(PRAWINPUT pRawInput);

HWND hwnd01;
HRESULT hr;
ID3D11Device* device3D;
ID2D1DeviceContext* context2D;
ID2D1SolidColorBrush* white;
DXGI_SAMPLE_DESC sampleDesc;
DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
IDXGIFactory2* factoryDXGI;
D2D1_PIXEL_FORMAT format;
D2D1_BITMAP_PROPERTIES1 bitmapProperties;
ID2D1Effect* scaleEffect[5];
IDXGISwapChain1* swapChain;
DXGI_PRESENT_PARAMETERS parameters;
ID2D1Effect* eff; 
ID2D1Image* img1;
ID2D1Image* img2;

HANDLE mutex;
BOOL launched = FALSE;
BOOL drawn = FALSE;

int X;
int Y;
int selected = 0;
int values[9];
float onefourth;
float thickness;
float halfthickness;
float rad;
char path[4][4096];
char args[4][4096];
float current1 = 0.0f;
float current2 = 0.0f;
float opacity = 1.0f;

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow) {
    mutex = CreateMutexA(NULL, TRUE, "ConsoleThingMutex");
    if (mutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }
    Init();
    InitDX();
    InitEffects();
    LPCSTR CLASS_NAME = "Platforms";
    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(WNDCLASSA));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = NULL;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowExA(0, CLASS_NAME, CLASS_NAME, WS_BORDER,
        0, 0, X, Y, NULL, NULL, NULL, NULL);
    hwnd01 = hwnd;
    SetWindowLongA(hwnd, GWL_STYLE, 0);
    SetCursor(NULL);
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));
    PostMessage(hwnd, WM_PAINT, nCmdShow, 0);
    CreateThread(NULL, 0, &DrawThread, hwnd, 0, NULL);
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_INPUT: {
        PRAWINPUT pRawInput;
        UINT bufferSize = 0;;
        HANDLE hHeap;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &bufferSize, sizeof(RAWINPUTHEADER));
        hHeap = GetProcessHeap();
        pRawInput = (PRAWINPUT)HeapAlloc(hHeap, 0, bufferSize);
        if (pRawInput == NULL) {
            return 0;
        }
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, pRawInput, &bufferSize, sizeof(RAWINPUTHEADER));
        ParseRawInput(pRawInput);
        HeapFree(hHeap, 0, pRawInput);
        break;
    }
    case WM_PAINT: {
        if (!drawn) {
            ShowWindow(hwnd, (int)wParam);
            drawn = TRUE;
        }
        break;
    }
    return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

DWORD WINAPI DrawThread(LPVOID param) {
    MMRESULT r = timeBeginPeriod(1);
    LARGE_INTEGER StartingTime, EndingTime;
    LARGE_INTEGER Frequency;
    QueryPerformanceFrequency(&Frequency);
    HWND hwnd = (HWND)param;
    if (swapChain == NULL) {
        GetSwapChain(hwnd);
    }
    current1 = selected * onefourth + halfthickness;
    current2 = selected * onefourth + onefourth - halfthickness;
    while (true) {
        QueryPerformanceCounter(&StartingTime);
        context2D->BeginDraw();
        context2D->Clear(NULL);
        if (launched) {
            eff->SetValue(D2D1_CROSSFADE_PROP_WEIGHT, opacity);
            context2D->DrawImage(eff);
        }
        else {
            context2D->DrawImage(scaleEffect[0]);
            context2D->DrawRoundedRectangle(D2D1::RoundedRect(
                D2D1::RectF(
                    current1,
                    halfthickness,
                    current2,
                    Y - halfthickness
                ),
                rad, rad
            ), white, thickness);
        }
        hr = context2D->EndDraw();
        swapChain->Present1(1, 0, &parameters);
        while (true) {
            QueryPerformanceCounter(&EndingTime);
            if ((float)(EndingTime.QuadPart - StartingTime.QuadPart) / (float)Frequency.QuadPart * 1000.0f - 1000.0f / ((float)values[2]) >= 0)
                break;
        }
    }
}

void Init() {
    CreateDirectoryA("Platforms", NULL);
    FILE* f0 = fopen("Platforms\\path.txt", "r");
    if (f0 != NULL) {
        char buffInt[256];
        for (int i = 0; i < 9; i++) {
            fgets(buffInt, 255, f0);
            values[i] = atoi(buffInt);
        }
        char buff[4096];
        for (int i = 0; i < 4; i++) {
            fgets(buff, 4095, f0);
            buff[strcspn(buff, "\r\n")] = 0;
            char* p = strstr(buff, ".exe") + 4;
            if (p != NULL) {
                *p = '\0';
                p++;
                strcpy(path[i], buff);
                strcpy(args[i], p);
            }
        }
        fclose(f0);
    }
    else {
        f0 = fopen("Platforms\\path.txt", "w");
        fclose(f0);
    }
    RAWINPUTDEVICE rid;
    ZeroMemory(&rid, sizeof(RAWINPUTDEVICE));
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x05;
    rid.dwFlags = 0;
    rid.hwndTarget = 0;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));
    X = GetSystemMetrics(SM_CXSCREEN);
    Y = GetSystemMetrics(SM_CYSCREEN);
    onefourth = X / 4.0f;
    thickness = (float)values[0];
    halfthickness = thickness / 2.0f;
    rad = (float)values[1];
    sampleDesc = { 1, 0 };
    swapChainDesc = { 0, 0, DXGI_FORMAT_B8G8R8A8_UNORM, FALSE, sampleDesc, DXGI_USAGE_RENDER_TARGET_OUTPUT,
    2, DXGI_SCALING_NONE, DXGI_SWAP_EFFECT_FLIP_DISCARD, DXGI_ALPHA_MODE_IGNORE, 0 };
    format = { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE };
    bitmapProperties = { format, 0, 0, D2D1_BITMAP_OPTIONS_NONE };
    parameters = { 0, NULL, NULL, NULL };
}

void InitDX() {
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };
    ID3D11DeviceContext* context3D;
    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT, featureLevels,
        ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &device3D, NULL, &context3D);
    if (device3D == NULL) {
        return;
    }
    IDXGIDevice1* deviceDXGI;
    hr = device3D->QueryInterface(__uuidof(IDXGIDevice1), (void**) & deviceDXGI);
    if (deviceDXGI == NULL) {
        return;
    }
    ID2D1Factory1* factory2D = {};
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1Factory1), (void**) & factory2D);
    ID2D1Device* device2D;
    hr = factory2D->CreateDevice(deviceDXGI, &device2D);
    hr = device2D->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, &context2D);
    hr = context2D->CreateSolidColorBrush(D2D1::ColorF(255, 255, 255), &white);
    IDXGIAdapter* adapterDXGI;
    hr = deviceDXGI->GetAdapter(&adapterDXGI);
    hr = adapterDXGI->GetParent(IID_PPV_ARGS(&factoryDXGI));
}

void InitEffects() {
    LPCSTR files[5] = {
        "Platforms\\Platforms.bmp",
        "Platforms\\steam.bmp",
        "Platforms\\xbox.bmp",
        "Platforms\\ps.bmp",
        "Platforms\\nintendo.bmp"
    };
    BITMAP bt = { 0 };
    BITMAPINFO inf = { 0 };
    HDC DC = CreateCompatibleDC(NULL);
    ID2D1Bitmap1* tmp;
    for (int i = 0; i < 5; i++) {
        HBITMAP hBitmap = (HBITMAP)LoadImageA(NULL, files[i], IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        GetObject(hBitmap, sizeof(bt), &bt);
        inf.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        inf.bmiHeader.biWidth = bt.bmWidth;
        inf.bmiHeader.biHeight = -bt.bmHeight;
        inf.bmiHeader.biPlanes = 1;
        inf.bmiHeader.biBitCount = bt.bmBitsPixel;
        inf.bmiHeader.biCompression = BI_RGB;
        inf.bmiHeader.biSizeImage = ((bt.bmWidth * bt.bmBitsPixel + 31) / 32) * 4 * bt.bmHeight;
        void* pixels = malloc(inf.bmiHeader.biSizeImage);
        GetDIBits(DC, hBitmap, 0, bt.bmHeight, pixels, &inf, DIB_RGB_COLORS);
        hr = context2D->CreateBitmap(D2D1::SizeU(bt.bmWidth, bt.bmHeight), pixels, bt.bmWidthBytes, bitmapProperties, &tmp);
        hr = context2D->CreateEffect(CLSID_D2D1Scale, &scaleEffect[i]);
        scaleEffect[i]->SetInput(0, tmp);
        scaleEffect[i]->SetValue(D2D1_SCALE_PROP_SCALE, D2D1::Vector2F((float)X / bt.bmWidth, (float)Y / bt.bmHeight));
        scaleEffect[i]->SetValue(D2D1_SCALE_PROP_SHARPNESS, 1.0f);
        scaleEffect[i]->SetValue(D2D1_SCALE_PROP_INTERPOLATION_MODE, D2D1_SCALE_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC);
        free(pixels);
    }
}

void GetSwapChain(HWND hwnd) {
    hr = factoryDXGI->CreateSwapChainForHwnd(device3D, hwnd, &swapChainDesc, NULL, NULL, &swapChain);
    bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
    IDXGISurface* backBufferDXGI;
    hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&backBufferDXGI));
    if (backBufferDXGI == NULL) {
        return;
    }
    ID2D1Bitmap1* bitmap2D;
    hr = context2D->CreateBitmapFromDxgiSurface(backBufferDXGI, &bitmapProperties, &bitmap2D);
    context2D->SetTarget(bitmap2D);
}

void AnimateFade() {
    context2D->CreateEffect(CLSID_D2D1CrossFade, &eff);
    if (eff == NULL) {
        return;
    }
    scaleEffect[0]->GetOutput(&img1);
    scaleEffect[selected + 1]->GetOutput(&img2);
    eff->SetInput(0, img1);
    eff->SetInput(1, img2);
    while (opacity >= 0.0f) {
        opacity -= 0.001f;
        PrecSleep(values[4]);
    }
}

void AnimateRect() {
    float target1 = selected * onefourth + halfthickness;
    float target2 = selected * onefourth + onefourth - halfthickness;
    while (current1 != target1) {
        if (current1 < target1) {
            current1++;
            current2++;
        }
        if (current1 > target1) {
            current1--;
            current2--;
        }
        PrecSleep(values[3]);
    }
}

void PrecSleep(int durMcs) {
    LARGE_INTEGER StartingTime, EndingTime;
    LARGE_INTEGER Frequency;
    QueryPerformanceFrequency(&Frequency);
    QueryPerformanceCounter(&StartingTime);
    while (true) {
        QueryPerformanceCounter(&EndingTime);
        if ((float)(EndingTime.QuadPart - StartingTime.QuadPart) / (float)Frequency.QuadPart * 1000000.0f >= (float)durMcs)
            break;
    }
}

void ParseRawInput(PRAWINPUT pRawInput) {
    PHIDP_PREPARSED_DATA pPreparsedData;
    UINT bufferSize = 0;
    HANDLE hHeap;
    HRESULT rt;

    pPreparsedData = NULL;
    hHeap = GetProcessHeap();

    GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_PREPARSEDDATA, NULL, &bufferSize);
    pPreparsedData = (PHIDP_PREPARSED_DATA)HeapAlloc(hHeap, 0, bufferSize);
    GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_PREPARSEDDATA, pPreparsedData, &bufferSize);
    if (pPreparsedData == NULL) {
        return;
    }

    //process nav
    ULONG val = 0;
    rt = HidP_GetUsageValue(HidP_Input, 1, 0, 0x39, &val, pPreparsedData,
        (PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid);
    if (rt != HIDP_STATUS_SUCCESS) {
        return;
    }
    switch (val) {
    case 3: {
        if (selected < 3) {
            selected++;
            AnimateRect();
        }
        return;
    }
    case 7: {
        if (selected > 0) {
            selected--;
            AnimateRect();
        }
        return;
    }
    }

    //process btn
    ULONG usageLength = 1;
    USAGE usage = 0;
    rt = HidP_GetUsages(HidP_Input, 9, 0, &usage, &usageLength, pPreparsedData,
        (PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid);
    if (rt != HIDP_STATUS_SUCCESS) {
        return;
    }
    switch (usage) {
    case 1: {
        SetWindowPos(hwnd01, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        STARTUPINFOA info = { sizeof(info) };
        PROCESS_INFORMATION processInfo;
        CreateProcessA(path[selected], args[selected], NULL, NULL,
            TRUE, 0, NULL, NULL, &info, &processInfo);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
        launched = TRUE;
        AnimateFade();
        Sleep(values[5 + selected]);
        exit(0);
        return;
    }
    case 2: {
        exit(0);
        break;
    }
    }
}
