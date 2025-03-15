#include "pch.h"



// Functions

struct RECTANDTIP
{
    D2D1_RECT_F r = {};
    std::wstring tip;
};
std::vector<RECTANDTIP> RectsAndTips;

void ClearRectsAndTips()
{
    RectsAndTips.clear();
    RectsAndTips.reserve(200);
}
void AddRectAndTip(D2D1_RECT_F r, const wchar_t* t)
{
    if (!t)
        return;
    if (wcslen(t) == 0)
        return;
    RECTANDTIP rt;
    if (wcslen(t) <= 57)
    {
        rt.r = r;
        rt.tip = t;
        RectsAndTips.push_back(rt);
    }
}


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







extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}


#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


void Locate([[maybe_unused]] const wchar_t* fi)
{
#ifndef _DEBUGX
    wchar_t buf[1024] = {};
    swprintf_s(buf,1024,L"/select,\"%s\"", fi);
    ShellExecute(0, L"open", L"explorer.exe", buf, 0, SW_SHOWMAXIMIZED);
#endif

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

std::wstring TempFile3()
{
    std::vector<wchar_t> td(1000);
    GetTempPathW(1000, td.data());

    wcscat_s(td.data(), 1000, L"\\");
    std::vector<wchar_t> x(1000);
    GetTempFileNameW(td.data(), L"tmp", 0, x.data());
    DeleteFile(x.data());

    return x.data();
}



void CsvToBinary(const wchar_t* csv, const wchar_t* binout)
{
	std::ifstream f(csv);
    std::string line;
	auto hF = CreateFile(binout, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0); 
	if (hF == INVALID_HANDLE_VALUE)
		return;
    while (std::getline(f, line))
    {
        std::vector<float> v;
        std::vector<std::string> split(const std::string & s, char delim);
        std::vector<std::string> sp = split(line, ',');
        for (auto& s : sp)
        {
            if (isalpha(s[0]))
                continue;
            v.push_back(std::stof(s));
        }
		DWORD A = 0;
		WriteFile(hF, v.data(), (DWORD)(v.size() * sizeof(float)), &A, 0);

    }
	CloseHandle(hF);
}


#include "en.hpp"

const wchar_t* s(size_t idx)
{
    if (idx > MAX_LANG)
        return L"";
    return z_strings[idx];
}





// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.


int CALLBACK BrowseCallbackProc(
    HWND hwnd,
    UINT uMsg,
    LPARAM,
    LPARAM lpData
)
{
    if (uMsg == BFFM_INITIALIZED)
    {
        ITEMIDLIST* sel = (ITEMIDLIST*)lpData;
        if (!sel)
            return 0;
        SendMessage(hwnd, BFFM_SETSELECTION, false, (LPARAM)sel);
    }
    return 0;
}


bool BrowseFolder(HWND hh, const TCHAR* tit, const TCHAR* root, const TCHAR* sel, TCHAR* rv)
{
    IShellFolder* sf = 0;
    SHGetDesktopFolder(&sf);
    if (!sf)
        return false;

    BROWSEINFO bi = { 0 };
    bi.hwndOwner = hh;
    if (root && _tcslen(root))
    {
        PIDLIST_RELATIVE pd = 0;
        wchar_t dn[1000] = { 0 };
        wcscpy_s(dn, 1000, root);
        sf->ParseDisplayName(hh, 0, dn, 0, &pd, 0);
        bi.pidlRoot = pd;
    }
    bi.lpszTitle = tit;
    bi.ulFlags = BIF_EDITBOX | BIF_NEWDIALOGSTYLE;
    bi.lpfn = BrowseCallbackProc;
    if (sel && _tcslen(sel))
    {
        PIDLIST_RELATIVE pd = 0;
        wchar_t dn[1000] = { 0 };
        wcscpy_s(dn, 1000, sel);
        sf->ParseDisplayName(hh, 0, dn, 0, &pd, 0);
        bi.lParam = (LPARAM)pd;
    }

    PIDLIST_ABSOLUTE pd = SHBrowseForFolder(&bi);
    if (!pd)
        return false;

    SHGetPathFromIDList(pd, rv);
    CoTaskMemFree(pd);
    return true;
}



std::wstring TensorStringToString(std::wstring v)
{
    return TensorToString<unsigned int>(TensorFromString<unsigned int>(v.c_str()));
}

bool Hit(float x, float y, D2D1_RECT_F rc)
{
    return x >= rc.left && x <= rc.right && y >= rc.top && y <= rc.bottom;
}

