#include "filehdrmap.h"

FileHdrMap::FileHdrMap()
{
    fileHdrs.fill(-1);
}

bool FileHdrMap::AddFileHdr(int fileHashCode, int sectorNum)
{
    if (fileHdrs[sectorNum] != -1 || fileHdrs[sectorNum] != fileHashCode)
    {
        return false;
    }
    fileHdrs[sectorNum] = fileHashCode;
    return true;
}

bool FileHdrMap::DeleteFileHdr(int fileHashCode)
{
    auto pos = std::find(fileHdrs.begin(), fileHdrs.end(), fileHashCode);
    if (pos == fileHdrs.end())
    {
        return false;
    }
    else
    {
        (*pos) = -1;
        return true;
    }
}
