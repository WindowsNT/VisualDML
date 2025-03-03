#include "pch.h"



// Functions

std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}
std::vector<std::wstring>& split(const std::wstring& s, wchar_t delim, std::vector<std::wstring>& elems) {
    std::wstringstream ss(s);
    std::wstring item;
    while (std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::wstring> split(const std::wstring& s, wchar_t delim) 
{
    std::vector<std::wstring> elems;
    split(s, delim, elems);
    return elems;
}





#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


void Locate([[maybe_unused]] const wchar_t* fi)
{
#ifndef _DEBUG
    wchar_t buf[1024] = {};
    swprintf_s(buf,1024,L"/select,\"%s\"", fi);
    ShellExecute(0, L"open", L"explorer.exe", buf, 0, SW_SHOWMAXIMIZED);
#endif

}

bool PutFile(const wchar_t* f, std::vector<char>& d, bool Fw = true)
{
    HANDLE hX = CreateFile(f, GENERIC_WRITE, 0, 0, Fw ? CREATE_ALWAYS : CREATE_NEW, 0, 0);
    if (hX == INVALID_HANDLE_VALUE)
        return false;
    DWORD A = 0;
    WriteFile(hX, d.data(), (DWORD)(d.size() * sizeof(char)), &A, 0);
    CloseHandle(hX);
    if (A != d.size())
        return false;
    return true;
}


bool LoadFile(const wchar_t* f, std::vector<unsigned char>& d)
{
    HANDLE hX = CreateFile(f, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    if (hX == INVALID_HANDLE_VALUE)
        return false;
    LARGE_INTEGER sz = { 0 };
    GetFileSizeEx(hX, &sz);
    d.resize((size_t)(sz.QuadPart / sizeof(char)));
    DWORD A = 0;
    ReadFile(hX, d.data(), (DWORD)sz.QuadPart, &A, 0);
    CloseHandle(hX);
    if (A != sz.QuadPart)
        return false;
    return true;
}


