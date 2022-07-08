#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <hidsdi.h>
#include <d2d1_3.h>
#include <d3d11_1.h>


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Init();
void InitDX();
void InitEffects();
void GetSwapChain(HWND hwnd);
void AnimateFade();
void ParseRawInput(PRAWINPUT pRawInput);

HRESULT hr;
ID3D11Device* device3D;
ID2D1DeviceContext* context2D;
ID2D1SolidColorBrush* white;
DXGI_SAMPLE_DESC sampleDesc;
DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
IDXGIFactory2* factoryDXGI;
D2D1_PIXEL_FORMAT format;
D2D1_BITMAP_PROPERTIES1 bitmapProperties;
ID2D1Effect* scaleEffect[6];
IDXGISwapChain1* swapChain;
DXGI_PRESENT_PARAMETERS parameters = { 0, NULL, NULL, NULL };

int X;
int Y;
int selected = 0;
BOOL launched = FALSE;
BOOL drawn = FALSE;
int values[3];
char path[4][4096];
char args[4][4096];

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow) {
    Init();
    InitDX();
    InitEffects();
    LPCSTR CLASS_NAME = "ConsoleThing";
    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(WNDCLASSA));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = NULL;
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowExA(0, CLASS_NAME, CLASS_NAME, WS_BORDER,
        0, 0, X, Y, NULL, NULL, hInstance, NULL);
    SetWindowLongA(hwnd, GWL_STYLE, 0);
    SetCursor(NULL);
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));
    PostMessage(hwnd, WM_PAINT, nCmdShow, 0);
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
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        break;
    }
    case WM_PAINT: {
        if (swapChain == NULL) {
            GetSwapChain(hwnd);
        }
        if (launched) {
            AnimateFade();
            Sleep(values[2]);
            exit(0);
        }
        context2D->BeginDraw();
        context2D->Clear(NULL);
        context2D->DrawImage(scaleEffect[0]);
        float onefourth = X / 4.0f;
        context2D->DrawRectangle(D2D1::RectF(selected * onefourth + 5.0f, 5.0f,
            selected * onefourth + onefourth - 5.0f, Y - 5.0f), white, 10.0f, NULL);
        hr = context2D->EndDraw();
        swapChain->Present1(1, 0, &parameters);
        if (!drawn) {
            ShowWindow(hwnd, (int)wParam);
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            drawn = TRUE;
        }
        break;
    }
    return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

void Init() {
    CreateDirectoryA("ConsoleThing", NULL);
    FILE* f0 = fopen("ConsoleThing\\path.txt", "r");
    if (f0 != NULL) {
        char buffInt[256];
        for (int i = 0; i < 3; i++) {
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
        f0 = fopen("ConsoleThing\\path.txt", "w");
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
    sampleDesc = { 1, 0 };
    swapChainDesc = { 0, 0, DXGI_FORMAT_B8G8R8A8_UNORM, FALSE, sampleDesc, DXGI_USAGE_RENDER_TARGET_OUTPUT,
    2, DXGI_SCALING_NONE, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, DXGI_ALPHA_MODE_IGNORE, 0 };
    format = { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE };
    bitmapProperties = { format, 0, 0, D2D1_BITMAP_OPTIONS_NONE };
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
    hr = device3D->QueryInterface(__uuidof(IDXGIDevice1), reinterpret_cast<void**>(&deviceDXGI));
    if (deviceDXGI == NULL) {
        return;
    }
    ID2D1Factory1* factory2D = {};
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1Factory1), reinterpret_cast<void**>(&factory2D));
    ID2D1Device* device2D;
    hr = factory2D->CreateDevice(deviceDXGI, &device2D);
    hr = device2D->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, &context2D);
    hr = context2D->CreateSolidColorBrush(D2D1::ColorF(255, 255, 255), &white);
    IDXGIAdapter* adapterDXGI;
    hr = deviceDXGI->GetAdapter(&adapterDXGI);
    hr = adapterDXGI->GetParent(IID_PPV_ARGS(&factoryDXGI));
}

void InitEffects() {
    LPCSTR files[6] = {
        "ConsoleThing\\ConsoleThing.bmp",
        "ConsoleThing\\black.bmp",
        "ConsoleThing\\steam.bmp",
        "ConsoleThing\\ps.bmp",
        "ConsoleThing\\xbox.bmp",
        "ConsoleThing\\nintendo.bmp"
    };
    BITMAP bt = { 0 };
    BITMAPINFO inf = { 0 };
    HDC DC = CreateCompatibleDC(NULL);
    ID2D1Bitmap1* tmp;
    for (int i = 0; i < 6; i++) {
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
    ID2D1Effect* eff;
    context2D->CreateEffect(CLSID_D2D1CrossFade, &eff);
    if (eff == NULL) {
        return;
    }
    ID2D1Image* img1;
    scaleEffect[0]->GetOutput(&img1);
    ID2D1Image* mid;
    scaleEffect[1]->GetOutput(&mid);
    ID2D1Image* img2;
    scaleEffect;
    scaleEffect[selected + 2]->GetOutput(&img2);
    eff->SetInput(0, img1);
    eff->SetInput(1, mid);
    for (int frame = values[0]; frame >= 0; frame -= 1) {
        context2D->BeginDraw();
        context2D->Clear(NULL);
        eff->SetValue(D2D1_CROSSFADE_PROP_WEIGHT, (float)frame / values[0]);
        context2D->DrawImage(eff);
        hr = context2D->EndDraw();
        swapChain->Present1(1, 0, &parameters);
    }
    eff->SetInput(0, mid);
    eff->SetInput(1, img2);
    for (int frame = values[1]; frame >= 0; frame -= 1) {
        context2D->BeginDraw();
        context2D->Clear(NULL);
        eff->SetValue(D2D1_CROSSFADE_PROP_WEIGHT, (float)frame / values[1]);
        context2D->DrawImage(eff);
        hr = context2D->EndDraw();
        swapChain->Present1(1, 0, &parameters);
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
        }
        return;
    }
    case 7: {
        if (selected > 0) {
            selected--;
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
        STARTUPINFOA info = { sizeof(info) };
        PROCESS_INFORMATION processInfo;
        CreateProcessA(path[selected], args[selected], NULL, NULL,
            TRUE, 0, NULL, NULL, &info, &processInfo);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
        launched = TRUE;
        return;
    }
    case 3: {
        exit(0);
        break;
    }
    }
}
