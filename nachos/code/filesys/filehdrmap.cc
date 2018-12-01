#include "filehdrmap.h"
#include <ctime>
#include <functional>

HdrInfo::HdrInfo(int fileHashCode, int logOffset)
{
    this->fileHashCode = fileHashCode;
    this->logOffset =  logOffset;
    this->last_access = std::time(nullptr);
}

bool HdrInfo::operator==(const HdrInfo &other)
{
    return (fileHashCode==other.fileHashCode);
}

void FileHdrMap::UpdateFileHdr(int fileHashCode, int logOffset)
{
    for(auto info :  fileHdrs)
    {
        if(info.fileHashCode == fileHashCode)
        {
            info.logOffset = logOffset;
            info.last_access = std::time(nullptr);
            return;
        }
    }
    fileHdrs.push_back(HdrInfo(fileHashCode, logOffset));
    return;
}

bool FileHdrMap::DeleteFileHdr(int fileHashCode)
{
    for(auto it = fileHdrs.begin(); it != fileHdrs.end();)
    {
        if((*it).fileHashCode == fileHashCode)
        {
            it = fileHdrs.erase(it);
        }
        else
        {
            it++;
        }
    }
}

// all file header is stored in fileHdrMap, we can get this directly 
// from memory
int FileHdrMap::FindFileHeader(char* name) throw (std::out_of_range)
{
    // calculate the hash code of file name, which is more easy
    // to save in disk
    int sector = -1;
    std::hash<std::string> hash_fn;
    int nameHash = hash_fn(std::string(name));
    for(auto fd : fileHdrs)
    {
        if(nameHash == fd.fileHashCode)
        {
            sector = fd.fileHashCode;
            break;
        }
    }
    if( sector == -1)
        throw std::out_of_range("file header is not in range!");
    
    return sector;
}
