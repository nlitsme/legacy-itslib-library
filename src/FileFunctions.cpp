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
        logerror("ReadFile");
        return false;
    }
#endif
#ifdef _UNIX
    if (1!=fread(&w, sizeof(w), 1, f)) {
        logerror("ReadDword-fread");
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
        logerror("ReadFile");
        return false;
    }
#endif
#ifdef _UNIX
    if (size==size_t(~0)) {
	    uint64_t curpos=ftello(f);
//    if (fgetpos(f, &curpos)) {
//	logerror("fgetpos");
//	return false;
//    }
	    if (fseeko(f, 0, SEEK_END)) {
		logerror("fseek");
		return false;
	    }
	    uint64_t eofpos=ftello(f);
//    if (fgetpos(f, &eofpos)) {
//	logerror("fgetpos");
//	return false;
//    }
	    if (fseeko(f, curpos, SEEK_SET)) {
		logerror("fseek");
		return false;
	    }
	    size= eofpos-curpos;
    }

    data.resize(size);

    if (1!=fread(vectorptr(data), data.size(), 1, f)) {
        logerror("ReadData-fread");
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
        logerror("WriteFile");
        return false;
    }
#endif
#ifdef _UNIX
    if (1!=fwrite(&data.front(), data.size(), 1, f)) {
        logerror("fwrite");
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
        logerror("WriteFile");
        return false;
    }
#endif
#ifdef _UNIX
    if (1!=fwrite(&w, sizeof(w), 1, f)) {
        logerror("fwrite");
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
#else
    struct statvfs st;
    if (-1==statvfs(path.c_str(), &st))
        return uint64_t(-1);
    return st.f_bavail * st.f_frsize;
#endif
}
