#include "JitParser.hpp"

#include <windows.h>

namespace Centaurus
{
template<typename TCHAR>
JitParser<TCHAR>::JitParser(const Grammar<TCHAR>& grammar)
{

}
template<typename TCHAR>
virtual JitParser<TCHAR>::~JitParser()
{

}
template<typename TCHAR>
bool JitParser<TCHAR>::just_parse(const wchar_t *filename)
{
    HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    DWORD dwFileSizeLo, dwFileSizeHi;
    dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi);
     
    HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMapping == INVALID_HANDLE_VALUE)
        return false;

    void *lpView = MapViewOfFile(hMapping, GENERIC_READ, 0, 0, 0);
    if (lpView == NULL)
        return false;

    UnmapViewOfFile(lpView);

    CloseHandle(hMapping);
    CloseHandle(hFile);
    return true;
}
}