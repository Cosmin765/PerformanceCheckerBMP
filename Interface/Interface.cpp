#pragma once

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <Windows.h>
#include <ShlObj.h>

#include "bmp_processing.hpp"

#include "header_field.cpp"
#include "macros.hpp"
#include "handlers.hpp"
#include "interface_utils.hpp"


HWND hUploadButton = NULL, hStartButton = NULL;
HWND hFilenamesPanel = NULL, hInfoPanel = NULL;
HWND hGrayscaleInput = NULL;
HWND hInvertInput = NULL;
HWND hOperationsListBox = NULL, hModesListBox = NULL;
int width = 1024, height = 576;

std::vector<std::u16string> bmpFilepaths;

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
            auto filepaths = HandlePickImages();
            if (filepaths.size())
            {
                bmpFilepaths = std::move(filepaths);
                HandleDisplayImages(hFilenamesPanel, bmpFilepaths);
            }
        }
        break;
        case START_BUTTON_ID:
        {
            displayedImagesInfo.clear();
            SetWindowTextA(hBMPInfoPanel, std::string().c_str());

            HANDLE hCsv = CreateFileA(COMP_OUTPUT_PATH, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            EXPECT(hCsv != INVALID_HANDLE_VALUE);

            std::string csvHeader = "filepath,operation,duration(ms),workers\r\n";
            EXPECT_ELSE(WriteFile(hCsv, csvHeader.c_str(), csvHeader.size(), NULL, NULL));

            DWORD opsMask = 0;
            DWORD modesMask = 0;

            {
                int itemCount = SendMessage(hOperationsListBox, LB_GETCOUNT, 0, 0);
                for (int i = 0; i < itemCount; ++i) {
                    if (SendMessage(hOperationsListBox, LB_GETSEL, i, 0) > 0) {
                        opsMask |= (1 << i);
                    }
                }
            }

            {
                int itemCount = SendMessage(hModesListBox, LB_GETCOUNT, 0, 0);
                for (int i = 0; i < itemCount; ++i) {
                    if (SendMessage(hModesListBox, LB_GETSEL, i, 0) > 0) {
                        modesMask |= (1 << i);
                    }
                }
            }

            if (!opsMask)
            {
                MessageBox(hWnd, L"Select at least one operation", L"Error", MB_OK | MB_ICONERROR);
                break;

            }

            if (!modesMask)
            {
                MessageBox(hWnd, L"Select at least one mode", L"Error", MB_OK | MB_ICONERROR);
                break;
            }

            WCHAR inputBuffer[MAX_PATH];

            if (!bmpFilepaths.size())
            {
                MessageBoxA(hWnd, "Upload at least one image", "Error", MB_OK | MB_ICONERROR);
                break;
            }

            for (const auto& filepath : bmpFilepaths)
            {
                memset(inputBuffer, 0, MAX_PATH);
                GetWindowText(hGrayscaleInput, inputBuffer, MAX_PATH);
                std::u16string grayscaleOutputPath = (char16_t*)inputBuffer;

                memset(inputBuffer, 0, MAX_PATH);
                GetWindowText(hInvertInput, inputBuffer, MAX_PATH);
                std::u16string invertOutputPath = (char16_t*)inputBuffer;

                try {
                    HandleProcessingImage(filepath, grayscaleOutputPath, invertOutputPath, opsMask, modesMask, hCsv);
                }
                catch (std::runtime_error error) {
                    MessageBoxA(hWnd, error.what(), "Error", MB_OK | MB_ICONERROR);
                }
            }

            CloseHandle(hCsv);
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

        if (hUploadButton) {
            // set the style for the button
            int x = (sepX - UPLOAD_BUTTON_W) / 2;
            MoveWindow(hUploadButton, x, PADDING, UPLOAD_BUTTON_W, UPLOAD_BUTTON_H, TRUE);
        }

        if (hStartButton) {
            int x = (sepX - UPLOAD_BUTTON_W) / 2;
            int y = 6 * PADDING + UPLOAD_BUTTON_H + 4 * LABEL_H + LIST_BOX_H;
            MoveWindow(hStartButton, x, y, UPLOAD_BUTTON_W, UPLOAD_BUTTON_H, TRUE);
        }

        if (hInfoPanel) {
            int sepY = SEPARATOR_PERCENT_Y_2 * height / 100;
            MoveWindow(hInfoPanel, sepX, sepY, width - sepX, height - sepY, TRUE);
        }

        if (hBMPInfoPanel) {
            int sepY1 = SEPARATOR_PERCENT_Y_1 * height / 100;
            int sepY2 = SEPARATOR_PERCENT_Y_2 * height / 100;
            MoveWindow(hBMPInfoPanel, sepX, sepY1, width - sepX, sepY2 - sepY1, TRUE);
        }

        if (hFilenamesPanel) {
            int sepY = SEPARATOR_PERCENT_Y_1 * height / 100;
            MoveWindow(hFilenamesPanel, sepX, 0, width - sepX, sepY, TRUE);
        }
        
        if (hPerformancePanel) {
            int sepY = SEPARATOR_PERCENT_Y_1 * height / 100;
            int y = 7 * PADDING + 2 * UPLOAD_BUTTON_H + 4 * LABEL_H + LIST_BOX_H;
            MoveWindow(hPerformancePanel, PADDING, y, sepX - PADDING, height - PADDING - y, TRUE);
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
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    HWND hWnd = CreateWindowW(szWindowClass, L"Performance Checker", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, width, height, nullptr, nullptr, hInstance, nullptr);
    EXPECT(hWnd);

    ShowWindow(hWnd, 1);
    UpdateWindow(hWnd);

    return hWnd;
}

VOID SetInfoPanelContent()
{
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
    
    EXPECT(SetCurrentDirectoryA(HOME_DIR));

    HANDLE hFile = CreateFileA(INFO_FILEPATH, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    EXPECT(hFile != INVALID_HANDLE_VALUE);

    std::string dumpBuffer;
    for (char16_t c : infoBuffer)
        dumpBuffer += (char)c;
    EXPECT(WriteFile(hFile, dumpBuffer.c_str(), (DWORD)dumpBuffer.size(), NULL, NULL));
    EXPECT(CloseHandle(hFile));
}

VOID PopulateWindow(HWND hWnd) 
{
    // button for uploading images
    hUploadButton = CreateButton(hWnd, L"Upload image(s)", (HMENU)UPLOAD_BUTTON_ID);

    // inputs for specifying output paths

    // grayscale
    HWND hGrayscaleLabel = CreateLabel(hWnd, L"Grayscale output path:");
    hGrayscaleInput = CreateInput(hWnd);

    // invert
    HWND hInvertLabel = CreateLabel(hWnd, L"Invert output path:");
    hInvertInput = CreateInput(hWnd);

    // area for selecting tests
    HWND hTestsSelectionLabel = CreateLabel(hWnd, L"Tests selection");
    
    HWND hOperationsSelectionLabel = CreateLabel(hWnd, L"Select operations:");

    HWND hModesSelectionLabel = CreateLabel(hWnd, L"Select modes:");

    hOperationsListBox = CreateListBox(hWnd, { L"✯ Grayscale", L"✯ Invert" });
    hModesListBox = CreateListBox(hWnd, { L"✯ Sequential", L"✯ Statically parallelized", L"✯ Dinamically parallelized"});

    hStartButton = CreateButton(hWnd, L"Start processing", (HMENU)START_BUTTON_ID);

    hFilenamesPanel = CreateTextPanel(hWnd);
    hBMPInfoPanel = CreateTextPanel(hWnd);
    hInfoPanel = CreateTextPanel(hWnd);
    hPerformancePanel = CreateTextPanel(hWnd);

    // set the content for the info panel
    SetInfoPanelContent();
    SetWindowTextA(hGrayscaleInput, GRAYSCALE_OUTPUT_DIR);
    SetWindowTextA(hInvertInput, INVERT_OUTPUT_DIR);

    // ----------------------------------------

    // style for the inputs
    MoveWindow(hGrayscaleLabel, PADDING, 2 * PADDING + UPLOAD_BUTTON_H, LABEL_W, LABEL_H, TRUE);
    MoveWindow(hGrayscaleInput, 2 * PADDING + LABEL_W, 2 * PADDING + UPLOAD_BUTTON_H, INPUT_W, INPUT_H, TRUE);

    MoveWindow(hInvertLabel, PADDING, 3 * PADDING + UPLOAD_BUTTON_H + LABEL_H, LABEL_W, LABEL_H, TRUE);
    MoveWindow(hInvertInput, 2 * PADDING + LABEL_W, 3 * PADDING + UPLOAD_BUTTON_H + LABEL_H, INPUT_W, INPUT_H, TRUE);

    // style for tests selection
    MoveWindow(hTestsSelectionLabel, PADDING, 4 * PADDING + UPLOAD_BUTTON_H + 2 * LABEL_H, width, LABEL_H, TRUE);
    
    MoveWindow(hOperationsSelectionLabel, PADDING, 5 * PADDING + UPLOAD_BUTTON_H + 3 * LABEL_H, LABEL_W, LABEL_H, TRUE);
    MoveWindow(hModesSelectionLabel, 2 * PADDING + LABEL_W, 5 * PADDING + UPLOAD_BUTTON_H + 3 * LABEL_H, LABEL_W, LABEL_H, TRUE);
 
    MoveWindow(hOperationsListBox, PADDING, 5 * PADDING + UPLOAD_BUTTON_H + 4 * LABEL_H, LABEL_W, LIST_BOX_H, TRUE);
    MoveWindow(hModesListBox, 2 * PADDING + LABEL_W, 5 * PADDING + UPLOAD_BUTTON_H + 4 * LABEL_H, LABEL_W, LIST_BOX_H, TRUE);
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    HWND hWnd = InitInstance(hInstance, nShowCmd);
    PopulateWindow(hWnd);

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
