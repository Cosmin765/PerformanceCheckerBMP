#include <Windows.h>

#include "bmp_parser.hpp"
#include "macros.hpp"
#include "handlers.hpp"
#include "utils.hpp"
#include "debugging.hpp"

HWND hUploadButton = NULL;
HWND hInfoPanel = NULL;
HWND hFilenamesPanel = NULL;
int width = 1024, height = 576;

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
            HandleDisplayImages(hFilenamesPanel, filepaths);
        }
        break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_SIZE:
    {
        width = LOWORD(lParam);
        height = HIWORD(lParam);

        int sepX = SEPARATOR_PERCENT_X * width / 100;
        int sepY = SEPARATOR_PERCENT_Y * height / 100;

        if (hInfoPanel) {
            MoveWindow(hInfoPanel, sepX, sepY, width - sepX, height - sepY, TRUE);
        }

        if (hFilenamesPanel) {
            MoveWindow(hFilenamesPanel, sepX, 0, width - sepX, sepY, TRUE);
        }

        // trigger paint event
        InvalidateRect(hWnd, NULL, TRUE);
        UpdateWindow(hWnd);
    }
    break;
    case WM_PAINT:
        HandlePaint(hWnd, width, height);
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
        CW_USEDEFAULT, 0, width, height, nullptr, nullptr, hInstance, nullptr);
    EXPECT(hWnd);

    ShowWindow(hWnd, 1);
    UpdateWindow(hWnd);

    return hWnd;
}

VOID PopulateWindow(HWND hWnd) 
{
    // button for uploading images
    hUploadButton = CreateWindow(L"BUTTON", L"Upload image(s)",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD,
        0, 0, 0, 0,
        hWnd, (HMENU)UPLOAD_BUTTON_ID, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    EXPECT(hUploadButton);

    hFilenamesPanel = CreateWindow(L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        0, 0, 0, 0,
        hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    EXPECT(hFilenamesPanel);

    hInfoPanel = CreateWindow(L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        0, 0, 0, 0,
        hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    EXPECT(hInfoPanel);

    // set the content for the info panel

    ;

    std::u16string infoBuffer;
    infoBuffer += u"------ SID info ------\n";
    infoBuffer += u"Current user's SID: " + GetCurrentUserSID() + u"\n";
    infoBuffer += u"Everyone's group SID: " + GetGroupSID("Administrators") + u"\n";
    infoBuffer += u"Administrator's group SID: " + GetGroupSID("Everyone") + u"\n";
    infoBuffer += u"------ HT info ------\n";
    infoBuffer += GetHTInfo() + u"\n";
    infoBuffer += u"------ NUMA info ------\n";
    infoBuffer += GetNUMAInfo() + u"\n";

    SetWindowText(hInfoPanel, (LPCWSTR)infoBuffer.c_str());
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    HWND hWnd = InitInstance(hInstance, nShowCmd);
    PopulateWindow(hWnd);

    MoveWindow(hUploadButton, PADDING, PADDING, 150, 30, TRUE);

    // trigger size event
    MoveWindow(hWnd, 0, 0, width, height, TRUE);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}
