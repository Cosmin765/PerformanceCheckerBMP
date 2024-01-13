#pragma once

#include <Windows.h>
#include <sddl.h>
#include <string>
#include <sstream>
#include <unordered_set>

#include "macros.hpp"

std::u16string GetCurrentUserSID() 
{
    HANDLE hToken;
    DWORD dwTokenInfoSize;

    EXPECT(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken));
    if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwTokenInfoSize)) {
        EXPECT(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
    }

    PTOKEN_USER pTokenUser = reinterpret_cast<PTOKEN_USER>(malloc(dwTokenInfoSize));
    EXPECT(pTokenUser);

    EXPECT(GetTokenInformation(hToken, TokenUser, pTokenUser, dwTokenInfoSize, &dwTokenInfoSize));

    CloseHandle(hToken);

    LPWSTR pszSid;
    EXPECT(ConvertSidToStringSidW(pTokenUser->User.Sid, &pszSid));
    
    std::u16string sid = (char16_t*)pszSid;

    LocalFree(pszSid);
    free(pTokenUser);

    return sid;
}

std::u16string GetGroupSID(LPCSTR groupName)
{
    SID_NAME_USE sidNameUse;
    DWORD cbSid = 0;
    DWORD cchReferencedDomainName = 0;

    LookupAccountNameA(nullptr, groupName, nullptr, &cbSid, nullptr, &cchReferencedDomainName, &sidNameUse);

    PSID pSid = (PSID)malloc(cbSid);
    EXPECT(pSid);
    LPSTR referencedDomainName = (LPSTR)malloc(cchReferencedDomainName * sizeof(TCHAR));

    EXPECT(LookupAccountNameA(nullptr, groupName, pSid, &cbSid, referencedDomainName, &cchReferencedDomainName, &sidNameUse));

    LPWSTR sidString;
    ConvertSidToStringSid(pSid, &sidString);
    std::u16string sid = (char16_t*)sidString;

    free(pSid);
    free(referencedDomainName);
    LocalFree((HLOCAL)sidString);
    
    return sid;
}

std::u16string GetHTInfo() 
{
    DWORD returnedLength = NULL;
    GetLogicalProcessorInformation(NULL, &returnedLength);

    SYSTEM_LOGICAL_PROCESSOR_INFORMATION* buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(returnedLength);
    EXPECT(buffer);

    GetLogicalProcessorInformation(buffer, &returnedLength);

    std::u16string infoBuffer;
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t, 0x10ffff, std::little_endian>, char16_t> conv;

    DWORD count = returnedLength / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
    for (DWORD i = 0; i < count; ++i)
    {
        const auto& item = buffer[i];
        infoBuffer += u"Entry #" + conv.from_bytes(std::to_string(i)) + u":\r\n";
        infoBuffer += u"Processor mask: " + conv.from_bytes(std::to_string(item.ProcessorMask)) + u"\r\n";
        infoBuffer += u"Relationship: " + conv.from_bytes(std::to_string(item.Relationship)) + u"\r\n";
        infoBuffer += u"---\r\n";
    }

    free(buffer);

    return infoBuffer;
}


std::u16string GetNUMAInfo()
{
    std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> converter;
    DWORD_PTR processAffinityMask, systemAffinityMask;


    GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask);

    ULONG highestNodeNumber;
    EXPECT(GetNumaHighestNodeNumber(&highestNodeNumber));

    std::unordered_set<UCHAR> nodeNumbers;

    for (UCHAR processor = 0; processor < MAXIMUM_PROCESSORS; ++processor)
    {
        UCHAR nodeNumber;
        GetNumaProcessorNode(processor, &nodeNumber);

        if (nodeNumber == 0xFF) {
            continue;
        }

        EXPECT(nodeNumber <= highestNodeNumber);
        nodeNumbers.insert(nodeNumber);
    }

    std::stringstream stream;
    stream << "Process affinity mask: 0x" << std::hex << processAffinityMask << "\r\n";
    stream << "System affinity mask: 0x" << std::hex << systemAffinityMask << "\r\n";
    stream << "Total nodes count: " << std::dec << (DWORD) nodeNumbers.size();

    std::u16string buffer = converter.from_bytes(stream.str());

    return buffer;
}

std::u16string GetCPUSetsInfo(std::vector<std::u16string>& cpuSetEntriesInfo) {
    ULONG requiredIdCount;
    HANDLE hProc = GetCurrentProcess();

    // firstly get the capacity needed
    EXPECT(GetProcessDefaultCpuSets(hProc, NULL, 0, &requiredIdCount));

    ULONG* cpuSetIds = (PULONG)malloc(requiredIdCount * sizeof(ULONG));
    EXPECT(cpuSetIds);
    EXPECT(GetProcessDefaultCpuSets(hProc, cpuSetIds, requiredIdCount, &requiredIdCount));

    std::wstring_convert<std::codecvt_utf8_utf16<char16_t, 0x10ffff, std::little_endian>, char16_t> conv;

    std::u16string buffer;
    buffer += u"Process default CPU sets count: " + conv.from_bytes(std::to_string(requiredIdCount)) + u"\r\n";

    for (ULONG id = 0; id < requiredIdCount; ++id)
    {
        std::u16string idStr = conv.from_bytes(std::to_string(id));
        std::u16string valueStr = conv.from_bytes(std::to_string(cpuSetIds[id]));
        buffer += u"CPU set #" + idStr + u": " + valueStr + u"\r\n";
    }

    free(cpuSetIds);

    ULONG returnedLength;

    GetSystemCpuSetInformation(NULL, 0, &returnedLength, hProc, 0);
    
    SYSTEM_CPU_SET_INFORMATION* cpuSetsInfo = (SYSTEM_CPU_SET_INFORMATION*)malloc(returnedLength);
    EXPECT(cpuSetsInfo);

    GetSystemCpuSetInformation(cpuSetsInfo, returnedLength, &returnedLength, hProc, 0);

    ULONG entriesCount = returnedLength / sizeof(SYSTEM_CPU_SET_INFORMATION);
    buffer += u"---\r\n";
    buffer += u"System CPU sets count: " + conv.from_bytes(std::to_string(entriesCount));
    for (ULONG entry = 0; entry < entriesCount; ++entry)
    {
        const SYSTEM_CPU_SET_INFORMATION& cpuSetInfo = cpuSetsInfo[entry];
        std::u16string entryStr = conv.from_bytes(std::to_string(entry));

        std::u16string sizeStr = conv.from_bytes(std::to_string(cpuSetInfo.Size));
        std::u16string typeStr = conv.from_bytes(std::to_string(cpuSetInfo.Type));

        const auto& entryInfo = cpuSetInfo.CpuSet;
        std::u16string cpuSetEntryInfoBuffer;
        
        cpuSetEntryInfoBuffer += u"Id: " + conv.from_bytes(std::to_string(entryInfo.Id)) + u"\r\n";
        cpuSetEntryInfoBuffer += u"Size: " + conv.from_bytes(std::to_string(cpuSetInfo.Size)) + u"\r\n";
        cpuSetEntryInfoBuffer += u"Type: " + conv.from_bytes(std::to_string(cpuSetInfo.Type)) + u"\r\n";
        cpuSetEntryInfoBuffer += u"AllFlags: " + conv.from_bytes(std::to_string(entryInfo.AllFlags)) + u"\r\n";
        cpuSetEntryInfoBuffer += u"AllocationTag: " + conv.from_bytes(std::to_string(entryInfo.AllocationTag)) + u"\r\n";
        cpuSetEntryInfoBuffer += u"Allocated: " + conv.from_bytes(std::to_string(entryInfo.Allocated)) + u"\r\n";
        cpuSetEntryInfoBuffer += u"AllocatedToTargetProcess: " + conv.from_bytes(std::to_string(entryInfo.AllocatedToTargetProcess)) + u"\r\n";
        cpuSetEntryInfoBuffer += u"CoreIndex: " + conv.from_bytes(std::to_string(entryInfo.CoreIndex)) + u"\r\n";
        cpuSetEntryInfoBuffer += u"EfficiencyClass: " + conv.from_bytes(std::to_string(entryInfo.EfficiencyClass)) + u"\r\n";
        cpuSetEntryInfoBuffer += u"Group: " + conv.from_bytes(std::to_string(entryInfo.Group)) + u"\r\n";
        cpuSetEntryInfoBuffer += u"LastLevelCacheIndex: " + conv.from_bytes(std::to_string(entryInfo.LastLevelCacheIndex)) + u"\r\n";
        cpuSetEntryInfoBuffer += u"LogicalProcessorIndex: " + conv.from_bytes(std::to_string(entryInfo.LogicalProcessorIndex)) + u"\r\n";
        cpuSetEntryInfoBuffer += u"NumaNodeIndex: " + conv.from_bytes(std::to_string(entryInfo.NumaNodeIndex)) + u"\r\n";
        cpuSetEntryInfoBuffer += u"Parked: " + conv.from_bytes(std::to_string(entryInfo.Parked)) + u"\r\n";
        cpuSetEntryInfoBuffer += u"RealTime: " + conv.from_bytes(std::to_string(entryInfo.RealTime)) + u"\r\n";
        cpuSetEntryInfoBuffer += u"SchedulingClass: " + conv.from_bytes(std::to_string(entryInfo.SchedulingClass)) + u"\r\n";

        cpuSetEntriesInfo.push_back(std::move(cpuSetEntryInfoBuffer));
    }

    free(cpuSetsInfo);

    return buffer;
}

std::string ConvertFromU16(std::u16string str) {
    static std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> converter;
    return converter.to_bytes(str);
}

std::u16string ConvertToU16(std::string str) {
    static std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> converter;
    return converter.from_bytes(str);
}


HWND CreateLabel(HWND hParent, LPCWSTR content)
{
    HWND hWnd = CreateWindow(
        L"STATIC", content,
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0,
        hParent, NULL, NULL, NULL);
    EXPECT(hWnd);
    return hWnd;
}

HWND CreateTextPanel(HWND hParent, LPCWSTR content = L"")
{
    HWND hWnd = CreateWindow(L"EDIT", content,
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
        ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        0, 0, 0, 0,
        hParent, NULL, (HINSTANCE)GetWindowLongPtr(hParent, GWLP_HINSTANCE), NULL);
    EXPECT(hWnd);
    return hWnd;
}

HWND CreateInput(HWND hParent)
{
    HWND hWnd = CreateWindow(
        L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        0, 0, 0, 0,
        hParent, NULL, (HINSTANCE)GetWindowLongPtr(hParent, GWLP_HINSTANCE), NULL);
    EXPECT(hWnd);
    return hWnd;
}

HWND CreateListBox(HWND hParent, const std::vector<LPCWSTR>& options = {})
{
    HWND hWnd = CreateWindowEx(
        0,
        L"LISTBOX", L"",
        LBS_MULTIPLESEL | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
        0, 0, 0, 0,
        hParent,
        NULL,
        (HINSTANCE)GetWindowLongPtr(hParent, GWLP_HINSTANCE),
        NULL
    );

    EXPECT(hWnd);

    for (const auto& option : options)
        SendMessage(hWnd, LB_ADDSTRING, 0, (LPARAM)option);

    return hWnd;
}

HWND CreateButton(HWND hParent, LPCWSTR content, HMENU buttonId = NULL)
{
    HWND hWnd = CreateWindow(L"BUTTON", content,
        WS_TABSTOP | WS_VISIBLE | WS_CHILD,
        0, 0, 0, 0,
        hParent, buttonId, (HINSTANCE)GetWindowLongPtr(hParent, GWLP_HINSTANCE), NULL);
    EXPECT(hWnd);
    return hWnd;
}
