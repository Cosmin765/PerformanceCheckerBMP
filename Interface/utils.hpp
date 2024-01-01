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
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer;
    DWORD returnedLength = sizeof(buffer);

    GetLogicalProcessorInformation(&buffer, &returnedLength);

    buffer.ProcessorMask;
    buffer.Relationship;

    std::u16string infoBuffer;

    std::wstring_convert<std::codecvt_utf8_utf16<char16_t, 0x10ffff, std::little_endian>, char16_t> conv;
    
    infoBuffer += u"Processor mask: " + conv.from_bytes(std::to_string(buffer.ProcessorMask)) + u"\n";
    infoBuffer += u"Relationship: " + conv.from_bytes(std::to_string(buffer.Relationship));

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
    stream << "Process affinity mask: 0x" << std::hex << processAffinityMask << "\n";
    stream << "System affinity mask: 0x" << std::hex << systemAffinityMask << "\n";
    stream << "Total nodes count: " << std::dec << (DWORD) nodeNumbers.size();

    std::u16string buffer = converter.from_bytes(stream.str());

    return buffer;
}

std::u16string GetCPUSetsInfo() {
    ULONG requiredIdCount;
    HANDLE hProc = GetCurrentProcess();

    // firstly get the capacity needed
    EXPECT(GetProcessDefaultCpuSets(hProc, NULL, 0, &requiredIdCount));

    ULONG* cpuSetIds = (PULONG)malloc(requiredIdCount * sizeof(ULONG));
    EXPECT(cpuSetIds);
    EXPECT(GetProcessDefaultCpuSets(hProc, cpuSetIds, requiredIdCount, &requiredIdCount));

    std::wstring_convert<std::codecvt_utf8_utf16<char16_t, 0x10ffff, std::little_endian>, char16_t> conv;

    std::u16string buffer;
    buffer += u"Process default CPU sets count: " + conv.from_bytes(std::to_string(requiredIdCount)) + u"\n";

    for (ULONG id = 0; id < requiredIdCount; ++id)
    {
        std::u16string idStr = conv.from_bytes(std::to_string(id));
        std::u16string valueStr = conv.from_bytes(std::to_string(cpuSetIds[id]));
        buffer += u"CPU set #" + idStr + u": " + valueStr + u"\n";
    }

    free(cpuSetIds);

    ULONG returnedLength;

    GetSystemCpuSetInformation(NULL, 0, &returnedLength, hProc, 0);
    
    SYSTEM_CPU_SET_INFORMATION* cpuSetsInfo = (SYSTEM_CPU_SET_INFORMATION*)malloc(returnedLength);
    EXPECT(cpuSetsInfo);

    GetSystemCpuSetInformation(cpuSetsInfo, returnedLength, &returnedLength, hProc, 0);

    ULONG entriesCount = returnedLength / sizeof(SYSTEM_CPU_SET_INFORMATION);
    buffer += u"---\n";
    buffer += u"System CPU sets count: " + conv.from_bytes(std::to_string(entriesCount)) + u"\n";
    for (ULONG entry = 0; entry < entriesCount; ++entry)
    {
        const SYSTEM_CPU_SET_INFORMATION& cpuSetInfo = cpuSetsInfo[entry];
        std::u16string entryStr = conv.from_bytes(std::to_string(entry));

        std::u16string sizeStr = conv.from_bytes(std::to_string(cpuSetInfo.Size));
        std::u16string typeStr = conv.from_bytes(std::to_string(cpuSetInfo.Type));

        // TODO: display info for the cpuSet field as well
        std::u16string data = u"Size - " + sizeStr + u"; Type - " + typeStr;
        buffer += u"Entry #" + entryStr + u": " + data + u"\n";
    }

    free(cpuSetsInfo);

    return buffer;
}
