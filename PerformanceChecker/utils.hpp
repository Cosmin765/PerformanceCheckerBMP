#pragma once

#include <string>
#include <windows.h>
#include <iostream>
#include <aclapi.h>


std::string getFilename(const std::string& path){
    
    size_t found = path.find_last_of("\\");

    return path.substr(found + 1);
}


std::string getCurrentUser(){

    DWORD bufferSize = 32767;
    char* buffer = new char[bufferSize] {0};

    if(!GetUserNameA(buffer, &bufferSize)){
        delete[] buffer;

        DWORD error = GetLastError();

        throw std::runtime_error("GetUserName error " + std::to_string(error));
    }

    std::string username = buffer;

    delete[] buffer;

    return username;
}

PSID getSidFromUsername(const std::string& username){

    SID_NAME_USE snu;
    DWORD cbSid = 0, domainSize = 0;

    // get required buffer size

    if(!LookupAccountNameA(NULL, username.c_str(), NULL, &cbSid, NULL, &domainSize, &snu)){

        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER){
            
            DWORD error = GetLastError();

            throw std::runtime_error("LookupAccountName error " + std::to_string(error));

        }

    }

    PSID pSid = static_cast<PSID>(malloc(cbSid));
    LPSTR domainName = static_cast<LPSTR>(malloc(domainSize));

    if (!LookupAccountNameA(NULL, username.c_str(), pSid, &cbSid, domainName, &domainSize, &snu)){

        DWORD error = GetLastError();
        free(pSid);
        free(domainName);

        throw std::runtime_error("LookupAccountName error " + std::to_string(error));
    }

    free(domainName);
    return pSid;
}

SECURITY_ATTRIBUTES currentUserReadONLY(){
    
    PACL pACL = NULL;
    EXPLICIT_ACCESSA ea;
    PSECURITY_DESCRIPTOR pSD = NULL;
    SECURITY_ATTRIBUTES sa;
    DWORD result;

    std::string username = getCurrentUser();

    PSID currentUserSID = getSidFromUsername(username);

    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = GENERIC_READ;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance = NO_INHERITANCE;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
    ea.Trustee.ptstrName = static_cast<LPSTR>(currentUserSID);

    result = SetEntriesInAclA(1, &ea, NULL, &pACL);

    if(ERROR_SUCCESS != result){
        DWORD error = GetLastError();

        std::cerr << "SetEntriesInAcl error " + std::to_string(error);

        goto Cleanup;
    }

    pSD = static_cast<PSECURITY_DESCRIPTOR>(malloc(SECURITY_DESCRIPTOR_MIN_LENGTH));

    if (pSD == NULL){
        DWORD error = GetLastError();

        std::cerr << "Malloc error " + std::to_string(error);

        goto Cleanup;

    }

    if(!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)){
        DWORD error = GetLastError();

        std::cerr << "InitializeSecurityDescriptor error " + std::to_string(error);

        goto Cleanup;
    }

    if(!SetSecurityDescriptorDacl(pSD, TRUE, pACL, FALSE )){
        DWORD error = GetLastError();

        std::cerr << "SetSecurityDescriptorDacl error " + std::to_string(error);

        goto Cleanup;
    }

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = FALSE;

    return sa;


    Cleanup:
        if(currentUserSID){
            FreeSid(currentUserSID);
        }
        if(pACL){
            free(pACL);
        }
        if(pSD){
            free(pSD);
        }

        ExitProcess(1);

}

void SaveVectorToFile(const std::string& filepath, const std::vector<BYTE>& buffer)
{
    SECURITY_ATTRIBUTES sa = currentUserReadONLY();

    HANDLE handle = CreateFileA(filepath.c_str(), GENERIC_WRITE, 0, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (handle == INVALID_HANDLE_VALUE)
        throw std::runtime_error("Could not create file " + std::to_string(GetLastError()));

    DWORD dwBytesToWrite = (DWORD)buffer.size();

    if (!(WriteFile(handle, buffer.data(), dwBytesToWrite, NULL, NULL))) {
        CloseHandle(handle);
        throw std::runtime_error("Error writing to file " + std::to_string(GetLastError()));
    }

    CloseHandle(handle);
}

DWORD getProcessorCores(){
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
    DWORD returnLength = 0; 
    DWORD byteOffset = 0;
    DWORD processorCoreCount = 0;


    GetLogicalProcessorInformation(NULL, &returnLength);
    
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION* buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(returnLength);

    ptr = buffer;

    GetLogicalProcessorInformation(buffer, &returnLength);

    while(byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength){
        switch (ptr->Relationship){

            case RelationProcessorCore:
                processorCoreCount++;

                break;
            
            default:
               
                break;
        }
        byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
        
    }

    return processorCoreCount;
}

struct pixel_t {BYTE r, g, b, a;};
struct worker_cs { DWORD index, end, state; };
