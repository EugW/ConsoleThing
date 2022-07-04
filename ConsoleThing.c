#define _CRT_SECURE_NO_WARNINGS
#include "ConsoleThing.h"
#include <windows.h>
#include <stdio.h>
#include <xinput.h>

DWORD WINAPI Painter(LPVOID lpParam);
DWORD WINAPI Input(LPVOID lpParam);
int X;
int Y;
int selected = 1;
char path[3][4096];
char args[3][4096];

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
    CreateDirectory("ConsoleThing", NULL);
    FILE* f0 = fopen("ConsoleThing\\path.txt", "r");
    if (f0 != NULL) {
        char buff[4096];
        for (int i = 0; i < 3; i++) {
            fgets(buff, 4096, f0);
            char* p = strstr(buff, ".exe") + 4;
            memcpy(path[i], buff, p - buff);
            memcpy(args[i], p, strlen(buff) - strlen(path[i]));
        }
        fclose(f0);
    }
    else {
        f0 = fopen("ConsoleThing\\path.txt", "w");
        fclose(f0);
    }
    X = GetSystemMetrics(SM_CXSCREEN);
    Y = GetSystemMetrics(SM_CYSCREEN);
    LPCSTR CLASS_NAME = "ConsoleThing";
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(WNDCLASS));
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = NULL;
    RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, CLASS_NAME, WS_BORDER, 0, 0, X, Y, NULL, NULL, hInstance, NULL);
    SetWindowLong(hwnd, GWL_STYLE, 0);
    SetCursor(NULL);
    if (hwnd == NULL)
        return 0;
    ShowWindow(hwnd, nCmdShow);
    CreateThread(NULL, 0, &Painter, hwnd, 0, NULL);
    CreateThread(NULL, 0, &Input, NULL, 0, NULL);
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

DWORD WINAPI Painter(LPVOID lpParam) {
    HWND hwnd = lpParam;
    HDC hdc = GetWindowDC(hwnd);
    HBRUSH gold = CreateSolidBrush(RGB(255, 233, 0));
    HBRUSH brush = CreatePatternBrush(LoadImage(NULL, "ConsoleThing\\ConsoleThing.bmp", IMAGE_BITMAP, X, Y, LR_LOADFROMFILE));
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
        int offset = (int)(selected * X / 3.0);
        FillRect(hdc, &full, brush);
        for (int i = 0; i < 10; i++) {
            rc.left = i + offset;
            rc.top = 0 + i;
            rc.right = (LONG)(offset + X / 3.0 - i);
            rc.bottom = Y - i;
           FrameRect(hdc, &rc, gold);
        }
        prev = selected;
    }
    ReleaseDC(hwnd, hdc);
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
                dwResult = XInputGetKeystroke(0, 0, &key);
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
                            CreateProcess(path[0], args[0], NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo);
                            exit(0);
                            break;
                        }
                        case 1: {
                            STARTUPINFO info = { sizeof(info) };
                            PROCESS_INFORMATION processInfo;
                            CreateProcess(path[1], args[1], NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo);
                            exit(0);
                            break;
                        }
                        case 2: {
                            STARTUPINFO info = { sizeof(info) };
                            PROCESS_INFORMATION processInfo;
                            CreateProcess(path[2], args[2], NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo);
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