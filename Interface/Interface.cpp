#include <Windows.h>
#include <ShlObj.h>

#include "bmp_parser.hpp"
#include "macros.hpp"
#include "handlers.hpp"
#include "utils.hpp"
#include "debugging.hpp"

HWND hUploadButton = NULL;
HWND hInfoPanel = NULL;
HWND hFilenamesPanel = NULL;
HWND hGrayscaleLabel = NULL, hGrayscaleInput = NULL;
HWND hInvertLabel = NULL, hInvertInput = NULL;
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

        if (hUploadButton) {
            // set the style for the button
            int x = (sepX - UPLOAD_BUTTON_W) / 2;
            MoveWindow(hUploadButton, x, PADDING, UPLOAD_BUTTON_W, UPLOAD_BUTTON_H, TRUE);
        }

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

    // inputs for specifying output paths

    // grayscale
    hGrayscaleLabel = CreateWindow(
        L"STATIC", L"Grayscale output path:", 
        WS_CHILD | WS_VISIBLE, 
        0, 0, 0, 0, 
        hWnd, NULL, NULL, NULL);
    hGrayscaleInput = CreateWindow(
        L"Edit", L"", 
        WS_CHILD | WS_VISIBLE | WS_BORDER, 
        0, 0, 0, 0, 
        hWnd, NULL, NULL, NULL);
    EXPECT(hGrayscaleLabel && hGrayscaleInput);

    // invert
    hInvertLabel = CreateWindow(
        L"STATIC", L"Invert output path:",
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0,
        hWnd, NULL, NULL, NULL);
    hInvertInput = CreateWindow(
        L"Edit", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        0, 0, 0, 0,
        hWnd, NULL, NULL, NULL);
    EXPECT(hInvertLabel && hInvertInput);

    hFilenamesPanel = CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
        ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        0, 0, 0, 0,
        hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    EXPECT(hFilenamesPanel);

    hInfoPanel = CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
        ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        0, 0, 0, 0,
        hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    EXPECT(hInfoPanel);

    // set the content for the info panel
    std::u16string infoBuffer;
    infoBuffer += u"------ SID info ------\r\n";
    infoBuffer += u"Current user's SID: " + GetCurrentUserSID() + u"\r\n";
    infoBuffer += u"Everyone's group SID: " + GetGroupSID("Administrators") + u"\r\n";
    infoBuffer += u"Administrator's group SID: " + GetGroupSID("Everyone") + u"\r\n";
    infoBuffer += u"------ HT info ------\r\n";
    infoBuffer += GetHTInfo() + u"\r\n";
    infoBuffer += u"------ NUMA info ------\r\n";
    infoBuffer += GetNUMAInfo() + u"\r\n";
    infoBuffer += u"------ CPU sets ------\r\n";

    std::vector<std::u16string> cpuSetEntriesInfo;

    infoBuffer += GetCPUSetsInfo(cpuSetEntriesInfo) + u"\r\n";

    std::wstring_convert<std::codecvt_utf8_utf16<char16_t, 0x10ffff, std::little_endian>, char16_t> conv;
    for (DWORD entry = 0; entry < cpuSetEntriesInfo.size(); ++entry) 
    {
        const auto& entryInfo = cpuSetEntriesInfo[entry];
        infoBuffer += u"\r\n---- Entry #" + conv.from_bytes(std::to_string(entry)) + u" ----\r\n";
        infoBuffer += entryInfo;
    }

    SetWindowText(hInfoPanel, (LPCWSTR)infoBuffer.c_str());

    DWORD code = SHCreateDirectoryExA(NULL, HOME_DIR, NULL);
    EXPECT(code == ERROR_SUCCESS || code == ERROR_ALREADY_EXISTS);

    HANDLE hFile = CreateFileA(INFO_FILEPATH, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    EXPECT(hFile != INVALID_HANDLE_VALUE);

    std::string dumpBuffer;
    for (char16_t c : infoBuffer)
        dumpBuffer += (char)c;
    EXPECT(WriteFile(hFile, dumpBuffer.c_str(), (DWORD)dumpBuffer.size(), NULL, NULL));
    EXPECT(CloseHandle(hFile));
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    HWND hWnd = InitInstance(hInstance, nShowCmd);
    PopulateWindow(hWnd);

    // set the style for the inputs
    MoveWindow(hGrayscaleLabel, PADDING, 2 * PADDING + UPLOAD_BUTTON_H, LABEL_W, LABEL_H, TRUE);
    MoveWindow(hGrayscaleInput, 2 * PADDING + LABEL_W, 2 * PADDING + UPLOAD_BUTTON_H, INPUT_W, INPUT_H, TRUE);

    MoveWindow(hInvertLabel, PADDING, 3 * PADDING + UPLOAD_BUTTON_H + LABEL_H, LABEL_W, LABEL_H, TRUE);
    MoveWindow(hInvertInput, 2 * PADDING + LABEL_W, 3 * PADDING + UPLOAD_BUTTON_H + LABEL_H, INPUT_W, INPUT_H, TRUE);

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
