#include <Windows.h>
#include <sddl.h>
#include <string>
#include <sstream>

#include "macros.hpp"

std::u16string GetCurrentUserSID() 
{
    HANDLE hToken;
    DWORD dwTokenInfoSize;

    HANDLE hProc = GetCurrentProcess();
    EXPECT(OpenProcessToken(hProc, TOKEN_QUERY, &hToken));
    if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwTokenInfoSize)) {
        EXPECT(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
    }
    CloseHandle(hProc);

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


    HANDLE hProc = GetCurrentProcess();
    GetProcessAffinityMask(hProc, &processAffinityMask, &systemAffinityMask);

    CloseHandle(hProc);

    ULONG highestNodeNumber;
    EXPECT(GetNumaHighestNodeNumber(&highestNodeNumber));

    ULONG totalNodesCount = 0;
    for (ULONG nodeNumber = 0; nodeNumber <= highestNodeNumber; ++nodeNumber)
    {
        
    }

    std::stringstream stream;
    stream << "Process affinity mask: 0x" << std::hex << processAffinityMask << "\n";
    stream << "System affinity mask: 0x" << std::hex << systemAffinityMask << "\n";
    stream << "Total nodes count: " << std::dec << totalNodesCount << "\n";

    std::u16string buffer = converter.from_bytes(stream.str());

    return buffer;
}
