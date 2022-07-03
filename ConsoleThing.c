#include "ConsoleThing.h"
#include <windows.h>
#include <stdio.h>
#include <xinput.h>
#include <ShlObj.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI Painter(LPVOID lpParam);
DWORD WINAPI Input(LPVOID lpParam);
int X;
int Y;
int selected = 1;
char path[3][255];

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    CreateDirectory("ConsoleThing", NULL);
    FILE* f0 = fopen("ConsoleThing\\path.txt", "r");
    if (f0 != NULL) {
        char buff[255];
        for (int i = 0; i < 3; i++) {
            fgets(buff, 255, f0);
            memcpy(path[i], buff, strlen(buff) - 1);
        }
        fclose(f0);
    }
    else {
        f0 = fopen("ConsoleThing\\path.txt", "w");
        fclose(f0);
    }
    X = GetSystemMetrics(SM_CXSCREEN);
    Y = GetSystemMetrics(SM_CYSCREEN);
    const wchar_t CLASS_NAME[] = L"ConsoleThing";
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(WNDCLASS));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = NULL;
    RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "ConsoleThing", WS_BORDER, 0, 0, X, Y, NULL, NULL, hInstance, NULL);
    SetWindowLong(hwnd, GWL_STYLE, 0);
    SetCursor(NULL);
    if (hwnd == NULL)
        return 0;
    ShowWindow(hwnd, nCmdShow);
    CreateThread(NULL, NULL, &Painter, hwnd, NULL, NULL);
    CreateThread(NULL, NULL, &Input, NULL, NULL, NULL);
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        
    }
    return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

DWORD WINAPI Painter(LPVOID lpParam) {
    HWND hwnd = (HWND)lpParam;
    HDC hdc = GetWindowDC(hwnd);
    HBRUSH gold = CreateSolidBrush(RGB(255, 233, 0));
    HBRUSH brush = CreatePatternBrush((HBITMAP)LoadImage(NULL, "ConsoleThing\\ConsoleThing.bmp", IMAGE_BITMAP, X, Y, LR_LOADFROMFILE));
    RECT full;
    ZeroMemory(&full, sizeof(RECT));
    RECT rc;
    ZeroMemory(&rc, sizeof(RECT));
    full.left = 0;
    full.top = 0;
    full.right = X;
    full.bottom = Y;
    int prev = -1;
    while (TRUE) {
        Sleep(1);
        if (prev == selected) {
            continue;
        }
        int offset = selected * X / 3.0;
        FillRect(hdc, &full, brush);
        for (int i = 0; i < 10; i++) {
            rc.left = i + offset;
            rc.top = 0 + i;
            rc.right = offset + X / 3.0 - i;
            rc.bottom = Y - i;
           FrameRect(hdc, &rc, gold);
        }
        prev = selected;
    }
    
    ReleaseDC(hdc, hwnd);
    return 0;   
}

DWORD WINAPI Input(LPVOID lpParams) {
    DWORD dwResult;
    while (TRUE) {
        Sleep(1);
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));
        dwResult = XInputGetState(0, &state);
        if (dwResult == ERROR_SUCCESS) {
            while (TRUE) {
                Sleep(1);
                XINPUT_KEYSTROKE key;
                ZeroMemory(&key, sizeof(XINPUT_KEYSTROKE));
                dwResult = XInputGetKeystroke(0, NULL, &key);
                if (dwResult == ERROR_SUCCESS) {
                    if (key.Flags != XINPUT_KEYSTROKE_KEYUP) {
                        continue;
                    }
                    if (key.VirtualKey == VK_PAD_DPAD_LEFT) {
                        if (selected > 0) {
                            selected--;
                        }
                    }
                    if (key.VirtualKey == VK_PAD_DPAD_RIGHT) {
                        if (selected < 2) {
                            selected++;
                        }
                    }
                    if (key.VirtualKey == VK_PAD_A) {
                        switch (selected) {
                        case 0: {
                            STARTUPINFO info = { sizeof(info) };
                            PROCESS_INFORMATION processInfo;
                            CreateProcess(path[0], NULL, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo);
                            exit(0);
                            break;
                        }
                        case 1: {
                            STARTUPINFO info = { sizeof(info) };
                            PROCESS_INFORMATION processInfo;
                            CreateProcess(path[1], NULL, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo);
                            exit(0);
                            break;
                        }
                        case 2: {
                            STARTUPINFO info = { sizeof(info) };
                            PROCESS_INFORMATION processInfo;
                            CreateProcess(path[2], NULL, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo);
                            exit(0);
                            break;
                        }
                        }
                    }
                    if (key.VirtualKey == VK_PAD_X) {
                        exit(0);
                    }
                }
            }
        }
        else {
            printf("Controller not detected\n");
        }
    }
    return 0;
}