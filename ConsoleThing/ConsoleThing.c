#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <xinput.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI Painter(LPVOID lpParam);
DWORD WINAPI Input(LPVOID lpParam);

HBRUSH brush[4];
int X;
int Y;
int selected = 1;
char path[3][4096];
char args[3][4096];

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow) {
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
    brush[0] = CreatePatternBrush((HBITMAP)LoadImageA(NULL, "ConsoleThing\\ConsoleThing.bmp", IMAGE_BITMAP, X, Y, LR_LOADFROMFILE));
    brush[1] = CreatePatternBrush((HBITMAP)LoadImageA(NULL, "ConsoleThing\\nintendo.bmp", IMAGE_BITMAP, X, Y, LR_LOADFROMFILE));
    brush[2] = CreatePatternBrush((HBITMAP)LoadImageA(NULL, "ConsoleThing\\ps.bmp", IMAGE_BITMAP, X, Y, LR_LOADFROMFILE));
    brush[3] = CreatePatternBrush((HBITMAP)LoadImageA(NULL, "ConsoleThing\\xbox.bmp", IMAGE_BITMAP, X, Y, LR_LOADFROMFILE));
    ShowWindow(hwnd, nCmdShow);
    CreateThread(NULL, 0, &Painter, hwnd, 0, NULL);
    CreateThread(NULL, 0, &Input, hwnd, 0, NULL);
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
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &ps.rcPaint, brush[0]);
        EndPaint(hwnd, &ps);
    }
    return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

DWORD WINAPI Painter(LPVOID lpParam) {
    HWND hwnd = lpParam;
    HDC hdc = GetWindowDC(hwnd);
    HBRUSH white = CreateSolidBrush(RGB(255, 255, 255));
    RECT rc;
    ZeroMemory(&rc, sizeof(RECT));
    RECT full;
    ZeroMemory(&full, sizeof(RECT));
    full.left = 0;
    full.top = 0;
    full.right = X;
    full.bottom = Y;
    int prev = -1;
    while (TRUE) {
        if (prev == selected) {
            continue;
        }
        FillRect(hdc, &full, brush[0]);
        int offset = (int)(selected * X / 3.0);
        for (int i = 0; i < 10; i++) {
            rc.left = i + offset;
            rc.top = 0 + i;
            rc.right = (LONG)(offset + X / 3.0 - i);
            rc.bottom = Y - i;
            FrameRect(hdc, &rc, white);
        }
        prev = selected;
        Sleep(0);
    }
    ReleaseDC(hwnd, hdc);
    return 0;
}

DWORD WINAPI Input(LPVOID lpParams) {
    HWND hwnd = lpParams;
    HDC hdc = GetWindowDC(hwnd);
    DWORD dwResult;
    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(XINPUT_STATE));
    dwResult = XInputGetState(0, &state);
    while (dwResult != ERROR_SUCCESS) {
        dwResult = XInputGetState(0, &state);
        Sleep(0);
    }
    while (TRUE) {
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
                RECT full;
                ZeroMemory(&full, sizeof(RECT));
                full.left = 0;
                full.top = 0;
                full.right = X;
                full.bottom = Y;
                FillRect(hdc, &full, brush[selected + 1]);
                STARTUPINFOA info = { sizeof(info) };
                PROCESS_INFORMATION processInfo;
                CreateProcessA(path[selected], args[selected], NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo);
                Sleep(2000);
                CloseHandle(processInfo.hProcess);
                CloseHandle(processInfo.hThread);
                exit(0);

            }
            if (key.VirtualKey == VK_PAD_X) {
                exit(0);
            }
        }
        Sleep(0);
    }
    ReleaseDC(hwnd, hdc);
    return 0;
}
