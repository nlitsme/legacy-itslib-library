/* Copyright 2005 xda-developers.com
 * All Rights Reserved
 *
 *  $Header$
 *
 */
#include <util/wintypes.h>
#include "debug.h"
#include "vectorutils.h"
#include "stringutils.h"
#include "FileFunctions.h"

#ifdef _UNIX
#include <sys/statvfs.h>
#endif

bool ReadDword(FFHANDLE f, uint32_t &w)
{
#ifdef _WIN32
    DWORD read;
    if (!ReadFile(f, &w, sizeof(uint32_t), &read, NULL))
    {
        error("ReadFile");
        return false;
    }
#else
    if (1!=fread(&w, sizeof(w), 1, f)) {
        error("ReadDword-fread");
        return false;
    }
#endif
    return true;
}
bool ReadData(FFHANDLE f, ByteVector& data, size_t size/*=~0*/)
{
#ifdef _WIN32
    if (size==size_t(~0)) {
        // read remaining part of file.
        size= GetFileSize(f, NULL) - SetFilePointer(f, 0, NULL, FILE_CURRENT);
    }
    data.resize(size);

    DWORD read;
    if (!ReadFile(f, vectorptr(data), size, &read, NULL))
    {
        error("ReadFile");
        return false;
    }
#else
    if (size==size_t(~0)) {
	    uint64_t curpos=ftello(f);
//    if (fgetpos(f, &curpos)) {
//	error("fgetpos");
//	return false;
//    }
	    if (fseeko(f, 0, SEEK_END)) {
		error("fseek");
		return false;
	    }
	    uint64_t eofpos=ftello(f);
//    if (fgetpos(f, &eofpos)) {
//	error("fgetpos");
//	return false;
//    }
	    if (fseeko(f, curpos, SEEK_SET)) {
		error("fseek");
		return false;
	    }
	    size= eofpos-curpos;
    }

    data.resize(size);

    if (1!=fread(vectorptr(data), data.size(), 1, f)) {
        error("ReadData-fread");
        return false;
    }
#endif
    return true;
}

bool WriteData(FFHANDLE f, const ByteVector& data)
{
    if (data.empty())
        return true;
#ifdef _WIN32
    DWORD wrote;
    if (!WriteFile(f, &data.front(), data.size(), &wrote, NULL))
    {
        error("WriteFile");
        return false;
    }
#else
    if (1!=fwrite(&data.front(), data.size(), 1, f)) {
        error("fwrite");
        return false;
    }
#endif
    return true;
}
bool WriteDword(FFHANDLE f, uint32_t w)
{
#ifdef _WIN32
    DWORD wrote;
    if (!WriteFile(f, &w, sizeof(uint32_t), &wrote, NULL))
    {
        error("WriteFile");
        return false;
    }
#else
    if (1!=fwrite(&w, sizeof(w), 1, f)) {
        error("fwrite");
        return false;
    }
#endif
    return true;
}

uint64_t GetFilesystemFreeSpace(const std::string& path)
{
#ifdef _WIN32
    ULARGE_INTEGER ul;
    if (!GetDiskFreeSpaceEx(ToTString(path).c_str(), &ul, 0, 0))
        return uint64_t(-1);

    return ul.QuadPart;
#endif
#ifdef _UNIX
    struct statvfs st;
    if (-1==statvfs(path.c_str(), &st))
        return uint64_t(-1);
    return st.f_bavail * st.f_frsize;
#endif
}
