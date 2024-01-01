#include <Windows.h>

#include "bmp_parser.hpp"
#include "macros.hpp"
#include "handlers.hpp"
#include "debugging.hpp"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case UPLOAD_BUTTON_ID:
        {
            const auto& filepaths = HandlePickImages();
            HandleDisplayImages(filepaths);
        }
        break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
        HandlePaint(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    LPCWSTR szWindowClass = L"PERCHK";

    WNDCLASS wc = { };

    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = szWindowClass;
    RegisterClass(&wc);

    HWND hWnd = CreateWindowW(szWindowClass, L"Performance Checker", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
    EXPECT(hWnd);

    ShowWindow(hWnd, 1);
    UpdateWindow(hWnd);

    return hWnd;
}

VOID PopulateWindow(HWND hWnd) 
{
    // button for uploading images
    HWND hwndButton = CreateWindow(L"BUTTON", L"Upload image(s)",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD,
        10, 10, 150, 30,
        hWnd, (HMENU)UPLOAD_BUTTON_ID, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    EXPECT(hwndButton);
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    HWND hWnd = InitInstance(hInstance, nShowCmd);
    PopulateWindow(hWnd);

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}
