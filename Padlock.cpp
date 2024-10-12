#include <windows.h>
#include <shellapi.h>
#include <iostream>
#include <vector>
#include <string>
#include <locale>
#include <codecvt>
#include <cctype>
#include <commctrl.h>
#include "resource.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")


HINSTANCE hInst;
HWND hWnd;
bool isArmed = false;
bool isLocked = false; 
std::vector<int> keySequence = { 'a', 'b', 'c' }; 
std::vector<int> pressedKeys; 


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HICON CreateIconColor(ULONG color);
void CreateTrayIcon();
void RemoveTrayIcon();
void TryLockComputer();
void Arm();
void Disarm();
void ShowMessage(const wchar_t* msg);
static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
static void SetHooks();
static void UnhookHooks();
INT_PTR CALLBACK DisarmDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
std::vector<int> wstringToVectorChar(const std::wstring& wstr);
std::wstring vectorCharToWstring(const std::vector<int>& vec);
std::vector<int> wcharBufferToVectorChar(const wchar_t* wcharBuffer);
bool isKeyInSequence(int key);
bool compareKeySequences(const std::vector<int>& seq1, const std::vector<int>& seq2);
bool isValidSubstring(const std::vector<int>& pressedKeys, const std::vector<int>& passCode);


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"GlobalHookApp";

    RegisterClass(&wc);
    hWnd = CreateWindow(L"GlobalHookApp", L"Global Hook Application", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 300, 200, NULL, NULL, hInstance, NULL);

    CreateTrayIcon();
    ShowWindow(hWnd, SW_HIDE);
    UpdateWindow(hWnd);

    SetHooks(); 

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookHooks(); 
    RemoveTrayIcon();
    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_APP + 1: 
        if (lParam == WM_LBUTTONDOWN) { 
            if (isArmed) {
                Disarm();
            }
            else {
                Arm();
            }
        }
        else if (lParam == WM_RBUTTONDOWN) { 
            DialogBox(hInst, MAKEINTRESOURCE(IDD_DISARM_DIALOG), hWnd, DisarmDialogProc);
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        UnhookHooks();
        RemoveTrayIcon();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0; 
}


std::vector<int> wstringToVectorChar(const std::wstring& wstr) {
    
    std::vector<int> vec;
    for (wchar_t wc : wstr) {
        vec.push_back(static_cast<int>(wc)); 
    }
    return vec;
}

std::wstring vectorCharToWstring(const std::vector<int>& vec) {
    std::wstring wstr;
    for (int c : vec) {
        wstr.push_back(static_cast<wchar_t>(c)); 
    }
    return wstr;
}

std::vector<int> wcharBufferToVectorChar(const wchar_t* wcharBuffer) {
    if (wcharBuffer == nullptr) {
        return {}; 
    }

    
    std::wstring wstr(wcharBuffer);
    return wstringToVectorChar(wstr); 
}


void CreateTrayIcon() {
    NOTIFYICONDATA nid = { 0 };
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    nid.uCallbackMessage = WM_APP + 1; 
    wcscpy_s(nid.szTip, L"Click to arm/disarm");
    nid.hIcon = CreateIconColor(isArmed ? RGB(255, 0, 0) : RGB(0, 255, 0)); 
    Shell_NotifyIcon(NIM_ADD, &nid);
}

HICON CreateIconColor(ULONG color) {
    const int width = 32;    
    const int height = 32;   
    HBITMAP hBitmap;
    HDC hdc, memDC;
    BITMAP bmp;
    ICONINFO iconInfo;

    
    hdc = GetDC(NULL);
    memDC = CreateCompatibleDC(hdc);

    
    hBitmap = CreateCompatibleBitmap(hdc, width, height);
    SelectObject(memDC, hBitmap);

    
    HBRUSH TransparentBrush = (HBRUSH)::GetStockObject(NULL_BRUSH);
    RECT rect = { 0, 0, width, height };
    FillRect(memDC, &rect, TransparentBrush);

    
    HBRUSH brush = CreateSolidBrush(color);
    SelectObject(memDC, brush);

    
    Ellipse(memDC, 0, 0, width, height);

    
    DeleteObject(brush);

    
    GetObject(hBitmap, sizeof(bmp), &bmp);
    iconInfo.fIcon = TRUE; 
    iconInfo.xHotspot = width / 2;
    iconInfo.yHotspot = height / 2;
    iconInfo.hbmMask = hBitmap; 
    iconInfo.hbmColor = hBitmap;

    
    HICON hIcon = CreateIconIndirect(&iconInfo);

    
    DeleteDC(memDC);
    DeleteObject(hBitmap);
    ReleaseDC(NULL, hdc);

    return hIcon;
}




void RemoveTrayIcon() {
    NOTIFYICONDATA nid = { 0 };
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    Shell_NotifyIcon(NIM_DELETE, &nid);
}


void Arm() {
    isArmed = true;
    pressedKeys.clear();
    
    RemoveTrayIcon(); 
    CreateTrayIcon(); 
}


void Disarm() {
    isArmed = false;
    pressedKeys.clear();
    
    RemoveTrayIcon(); 
    CreateTrayIcon(); 
}


void ShowMessage(const wchar_t* msg) {
    MessageBox(NULL, msg, L"Notification", MB_OK);
}


void TryLockComputer() {
    if (isArmed) {
        LockWorkStation();
        Disarm();
    }
}


HHOOK keyboardHook;
HHOOK mouseHook;


static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        if (wParam == WM_LBUTTONDOWN ||
            wParam == WM_RBUTTONDOWN ||
            wParam == WM_MBUTTONDOWN ||
            wParam == WM_XBUTTONDOWN 
            ) {
               TryLockComputer();
        }
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKbStruct = (KBDLLHOOKSTRUCT*)lParam;
        if (wParam == WM_KEYDOWN) {
            int currentKey = pKbStruct->vkCode;

            
            if ((GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_SHIFT) & 0x8000) &&
                (currentKey == 'L' || currentKey == 'l')) {
                Arm(); 
                return 1; 
            }

            if (isArmed) {
                
                if (isKeyInSequence(currentKey)) {
                    pressedKeys.push_back(currentKey);

                    if (!isValidSubstring(pressedKeys, keySequence)) {
                        pressedKeys.clear();
                        TryLockComputer();
                    }
                    else if (compareKeySequences(pressedKeys, keySequence)) {
                        Disarm();
                    }
                    return 1;
                }
                
                
                TryLockComputer(); 
            }
        }
    }
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

bool isKeyInSequence(int key) {
    
    int upperCurrentKey = (key >= 'a' && key <= 'z') ? std::toupper(key) : key;

    for (int k : keySequence) {
        
        int upperKeyInSequence = (k >= 'a' && k <= 'z') ? std::toupper(k) : k;
        if (upperKeyInSequence == upperCurrentKey) {
            return true;
        }
    }
    return false;
}

bool isValidSubstring(const std::vector<int>& pressedKeys, const std::vector<int>& passCode) {
    
    if (pressedKeys.size() > passCode.size()) {
        return false; 
    }

    for (size_t i = 0; i < pressedKeys.size(); ++i) {
        if (std::toupper(pressedKeys[i]) != std::toupper(passCode[i])) {
            return false; 
        }
    }

    return true; 
}

bool compareKeySequences(const std::vector<int>& seq1, const std::vector<int>& seq2) {
    
    if (seq1.size() != seq2.size()) {
        return false;
    }

    for (size_t i = 0; i < seq1.size(); ++i) {
        
        int upperKey1 = (seq1[i] >= 'a' && seq1[i] <= 'z') ? std::toupper(seq1[i]) : seq1[i];
        int upperKey2 = (seq2[i] >= 'a' && seq2[i] <= 'z') ? std::toupper(seq2[i]) : seq2[i];

        if (upperKey1 != upperKey2) {
            return false; 
        }
    }

    return true; 
}


INT_PTR CALLBACK DisarmDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        SetDlgItemText(hDlg, IDC_EDIT_DISARM_CODE, vectorCharToWstring(keySequence).c_str());
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            wchar_t buffer[100];
            GetDlgItemText(hDlg, IDC_EDIT_DISARM_CODE, buffer, 100);
            keySequence = wcharBufferToVectorChar(buffer); 
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        if (LOWORD(wParam) == IDC_QUIT_BUTTON) {
            DestroyWindow(hWnd);
        }
        break;
    }
    return FALSE;
}


static void SetHooks() {
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInst, 0);
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, hInst, 0);
}


static void UnhookHooks() {
    UnhookWindowsHookEx(keyboardHook);
    UnhookWindowsHookEx(mouseHook);
}