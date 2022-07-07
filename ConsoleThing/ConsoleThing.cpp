#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <hidsdi.h>
#include <d2d1_1.h>
#include <d3d11_1.h>


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Init();
void InitDX();
void InitBitmaps();
void GetSwapChain(HWND hwnd);
void ParseRawInput(PRAWINPUT pRawInput);

HRESULT hr;
ID3D11Device* device3D;
ID2D1DeviceContext* context2D;
ID2D1SolidColorBrush* white;
DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
IDXGISwapChain1* swapChain;
IDXGIFactory2* factoryDXGI;
ID2D1Bitmap1* bmp[4] = {{},{},{},{}};
DXGI_PRESENT_PARAMETERS parameters = { 0, NULL, NULL, NULL };

int X;
int Y;
int selected = 1;
BOOL launched = FALSE;
char path[3][4096];
char args[3][4096];

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow) {
    Init();
    InitDX();
    InitBitmaps();
    LPCSTR CLASS_NAME = "ConsoleThing";
    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(WNDCLASSA));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = NULL;
    RegisterClassA(&wc);
    X = GetSystemMetrics(SM_CXSCREEN);
    Y = GetSystemMetrics(SM_CYSCREEN);
    HWND hwnd = CreateWindowExA(0, CLASS_NAME, CLASS_NAME, WS_BORDER, 0, 0, X, Y, NULL, NULL, hInstance, NULL);
    SetWindowLongA(hwnd, GWL_STYLE, 0);
    SetCursor(NULL);
    ShowWindow(hwnd, nCmdShow);
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));
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
        context2D->BeginDraw();
        context2D->DrawBitmap(bmp[(1 + selected) * launched], D2D1::RectF(0.0f, 0.0f, X + 0.0f, Y + 0.0f), 1.0f, D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC, NULL, NULL);
        float onethird = X / 3.0f;
        context2D->DrawRectangle(D2D1::RectF(selected * onethird, 0, selected * onethird + onethird, Y + 0.0f), white, !launched * 10.0f, NULL);
        hr = context2D->EndDraw();
        swapChain->Present1(1, 0, &parameters);
        if (launched) {
            Sleep(2000);
            exit(0);
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
        char buff[4096];
        for (int i = 0; i < 3; i++) {
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
    ID2D1Factory1* factory2D;
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1Factory1), (void**)&factory2D);
    ID2D1Device* device2D;
    hr = factory2D->CreateDevice(deviceDXGI, &device2D);
    hr = device2D->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, &context2D);
    hr = context2D->CreateSolidColorBrush(D2D1::ColorF(255, 255, 255), &white);
    swapChainDesc.Width = 0;
    swapChainDesc.Height = 0;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.Stereo = false;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Flags = 0;
    IDXGIAdapter* adapterDXGI;
    hr = deviceDXGI->GetAdapter(&adapterDXGI);
    hr = adapterDXGI->GetParent(IID_PPV_ARGS(&factoryDXGI));
}

void InitBitmaps() {
    LPCSTR files[4] = {
        "ConsoleThing\\ConsoleThing.bmp",
        "ConsoleThing\\nintendo.bmp",
        "ConsoleThing\\ps.bmp",
        "ConsoleThing\\xbox.bmp"
    };

    for (int i = 0; i < 4; i++) {
        BITMAP bt = { 0 };
        BITMAPINFO inf = { 0 };
        HDC DC = CreateCompatibleDC(NULL);
        HBITMAP qwe = (HBITMAP)LoadImageA(NULL, files[i], IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        HBITMAP OldBitmap = (HBITMAP)SelectObject(DC, qwe);
        GetObject(qwe, sizeof(bt), &bt);
        inf.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        inf.bmiHeader.biWidth = bt.bmWidth;
        inf.bmiHeader.biHeight = -bt.bmHeight;
        inf.bmiHeader.biPlanes = 1;
        inf.bmiHeader.biBitCount = bt.bmBitsPixel;
        inf.bmiHeader.biCompression = BI_RGB;
        inf.bmiHeader.biSizeImage = ((bt.bmWidth * bt.bmBitsPixel + 31) / 32) * 4 * bt.bmHeight;
        void* pixels = malloc(inf.bmiHeader.biSizeImage);
        GetDIBits(DC, qwe, 0, bt.bmHeight, pixels, &inf, DIB_RGB_COLORS);
        SelectObject(DC, OldBitmap);
        D2D1_PIXEL_FORMAT fmt = {
            DXGI_FORMAT_B8G8R8A8_UNORM,
            D2D1_ALPHA_MODE_IGNORE
        };
        D2D1_BITMAP_PROPERTIES1 bitmapProperties = {
            fmt,
            0,
            0,
            D2D1_BITMAP_OPTIONS_NONE
        };
        hr = context2D->CreateBitmap(D2D1::SizeU(bt.bmWidth, bt.bmHeight), pixels, bt.bmWidthBytes, bitmapProperties, &bmp[i]);
        free(pixels);
    }
}

void GetSwapChain(HWND hwnd) {
    hr = factoryDXGI->CreateSwapChainForHwnd(device3D, hwnd, &swapChainDesc, NULL, NULL, &swapChain);
    ID3D11Texture2D* backBuffer;
    hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    D2D1_BITMAP_PROPERTIES1 bitmapProperties = { };
    bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
    bitmapProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    IDXGISurface* backBufferDXGI;
    hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&backBufferDXGI));
    if (backBufferDXGI == NULL) {
        return;
    }
    ID2D1Bitmap1* bitmap2D;
    hr = context2D->CreateBitmapFromDxgiSurface(backBufferDXGI, &bitmapProperties, &bitmap2D);
    context2D->SetTarget(bitmap2D);
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
    rt = HidP_GetUsageValue(HidP_Input, 1, 0, 0x39, &val, pPreparsedData, (PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid);
    if (rt != HIDP_STATUS_SUCCESS) {
        return;
    }
    switch (val) {
    case 3: {
        if (selected < 2) {
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
        CreateProcessA(path[selected], args[selected], NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo);
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
