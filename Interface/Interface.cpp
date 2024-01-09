#include <Windows.h>
#include <ShlObj.h>

#include "bmp_processing.hpp"

#include "macros.hpp"
#include "handlers.hpp"
#include "interface_utils.hpp"
#include "debugging.hpp"
#include "thread_pool.hpp"

HWND hUploadButton = NULL, hStartButton = NULL;
HWND hInfoPanel = NULL;
HWND hFilenamesPanel = NULL;
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

            for (const auto& filepath : bmpFilepaths)
                HandleProcessingImage(filepath, opsMask, modesMask);
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

        if (hStartButton) {
            int x = (sepX - UPLOAD_BUTTON_W) / 2;
            int y = 6 * PADDING + UPLOAD_BUTTON_H + 4 * LABEL_H + LIST_BOX_H;
            MoveWindow(hStartButton, x, y, UPLOAD_BUTTON_W, UPLOAD_BUTTON_H, TRUE);
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
    hInfoPanel = CreateTextPanel(hWnd);

    // set the content for the info panel
    SetInfoPanelContent();

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

DWORD target(LPVOID arg)
{
    int index = *(int*)arg;

    std::string filepath = "C:\\Facultate\\CSSO\\a";
    filepath[filepath.size() - 1] += index;
    HANDLE handle = CreateFileA(filepath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    EXPECT(handle != INVALID_HANDLE_VALUE);
    CloseHandle(handle);
    return 0;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    constexpr int count = 26;
    ThreadPool tp(count);
    int buffer[count];
    for (int i = 0; i < count; ++i)
    {
        buffer[i] = i;
        tp.Submit(target, (LPVOID)(&buffer[i]));
    }
    tp.Shutdown();

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
