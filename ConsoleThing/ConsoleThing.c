#define _CRT_SECURE_NO_WARNINGS
#define MAX_BUTTONS		128
#define CHECK(exp)		{ if(!(exp)) goto Error; }
#include <windows.h>
#include <stdio.h>
#include <hidsdi.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI Painter(LPVOID lpParam);
DWORD WINAPI Input(LPVOID lpParam);

HBRUSH brush[4];
HBRUSH white;
int X;
int Y;
int selected = 1;
BOOL launched = FALSE;
char path[3][4096];
char args[3][4096];


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow) {
    RAWINPUTDEVICE rid;
    ZeroMemory(&rid, sizeof(RAWINPUTDEVICE));
    rid.usUsagePage = 0x01;          // HID_USAGE_PAGE_GENERIC
    rid.usUsage = 0x05;              // HID_USAGE_GENERIC_GAMEPAD
    rid.dwFlags = 0;                 // adds game pad
    rid.hwndTarget = 0;
    BOOL a = RegisterRawInputDevices(&rid, 1, sizeof(rid));
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
    white = CreateSolidBrush(RGB(255, 255, 255));
    brush[0] = CreatePatternBrush((HBITMAP)LoadImageA(NULL, "ConsoleThing\\ConsoleThing.bmp", IMAGE_BITMAP, X, Y, LR_LOADFROMFILE));
    brush[1] = CreatePatternBrush((HBITMAP)LoadImageA(NULL, "ConsoleThing\\nintendo.bmp", IMAGE_BITMAP, X, Y, LR_LOADFROMFILE));
    brush[2] = CreatePatternBrush((HBITMAP)LoadImageA(NULL, "ConsoleThing\\ps.bmp", IMAGE_BITMAP, X, Y, LR_LOADFROMFILE));
    brush[3] = CreatePatternBrush((HBITMAP)LoadImageA(NULL, "ConsoleThing\\xbox.bmp", IMAGE_BITMAP, X, Y, LR_LOADFROMFILE));
    ShowWindow(hwnd, nCmdShow);
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return 0;
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
        return 0;
    }

    //process nav
    int val = 0;
    rt = HidP_GetUsageValue(HidP_Input, 1, 0, 0x39, &val, pPreparsedData, (PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid);
    if (rt != HIDP_STATUS_SUCCESS) {
        return 0;
    }
    switch (val) {
    case 3: {
        if (selected < 2) {
            selected++;
        }
        return 0;
    }
    case 7: {
        if (selected > 0) {
            selected--;
        }
        return 0;
    }
    }
    
    //process btn
    int usageLength = 1;
    USAGE usage = 0;
    rt = HidP_GetUsages(HidP_Input, 9, 0, &usage, &usageLength, pPreparsedData,
        (PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid);
    if (rt != HIDP_STATUS_SUCCESS) {
        return 0;
    }
    switch (usage) {
    case 1: {
        STARTUPINFOA info = { sizeof(info) };
        PROCESS_INFORMATION processInfo;
        CreateProcessA(path[selected], args[selected], NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
        launched = TRUE;
        return 0;
    }
    case 3: {
        exit(0);
        break;
    }
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_INPUT: {
        PRAWINPUT pRawInput;
        UINT      bufferSize = 0;;
        HANDLE    hHeap;

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
        Sleep(0);
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (launched) {
            FillRect(hdc, &ps.rcPaint, brush[selected + 1]);
            EndPaint(hwnd, &ps);
            Sleep(2000);
            exit(0);
        }
        RECT rc;
        ZeroMemory(&rc, sizeof(RECT));
        FillRect(hdc, &ps.rcPaint, brush[0]);
        int offset = (int)(selected * X / 3.0);
        for (int i = 0; i < 10; i++) {
            rc.left = i + offset;
            rc.top = 0 + i;
            rc.right = (LONG)(offset + X / 3.0 - i);
            rc.bottom = Y - i;
            FrameRect(hdc, &rc, white);
        }
        EndPaint(hwnd, &ps);
        break;
    }
    return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}
