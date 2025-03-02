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


void Locate(const wchar_t* fi)
{
#ifndef _DEBUG88
    wchar_t buf[1024] = {};
    swprintf_s(buf,1024,L"/select,\"%s\"", fi);
    ShellExecute(0, L"open", L"explorer.exe", buf, 0, SW_SHOWMAXIMIZED);
#endif

}

