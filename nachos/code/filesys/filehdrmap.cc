#include "filehdrmap.h"
#include <ctime>
#include <functional>

HdrInfo::HdrInfo(int fileHashCode, int version, int logOffset)
{
    this->fileHashCode = fileHashCode;
    this->logOffset =  logOffset;
    this->last_access = version;
}

bool HdrInfo::operator==(const HdrInfo &other)
{
    return (fileHashCode==other.fileHashCode);
}


//--------------------------------------------------------------------
// FileHdrMap::UpdateFileHdr
//  add or update a file header, this should be called after a log is
//  the version of this file need to be provided
//  this should be called when a file header itself is changed
//  created on cache or disk
//--------------------------------------------------------------------
void FileHdrMap::UpdateFileHdr(char* name, int version, int logOffset)
{
    std::hash<std::string> hash_fn;
    int fileHashCode = hash_fn(std::string(name));
    for(auto info : fileHdrs)
    {
        if(info.fileHashCode == fileHashCode)
        {
            info.logOffset = logOffset;
            info.last_access = version;
            return;
        }
    }
    fileHdrs.push_back(HdrInfo(fileHashCode, version, logOffset));
    return;
}
void FileHdrMap::UpdateFileHdr(int fileHashCode, int version, int logOffset)
{;
    for(auto info : fileHdrs)
    {
        if(info.fileHashCode == fileHashCode)
        {
            info.logOffset = logOffset;
            info.last_access = version;
            return;
        }
    }
    fileHdrs.push_back(HdrInfo(fileHashCode, version, logOffset));
    return;
}
//--------------------------------------------------------------------
// FileHdrMap::FileContentModified
//  when a file  content is modified, the timestamp of it's summary 
//  need to be updated  
//--------------------------------------------------------------------
void FileHdrMap::FileContentModified(char* name, int version)
{
    std::hash<std::string> hash_fn;
    int fileHashCode = hash_fn(std::string(name));
    for(auto info : fileHdrs)
    {
        if(info.fileHashCode == fileHashCode)
        {
            info.last_access = version;
            return;
        }
    }
}

bool FileHdrMap::DeleteFileHdr(char* name)
{
    std::hash<std::string> hash_fn;
    int fileHashCode = hash_fn(std::string(name));
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
    int fileHashCode = hash_fn(std::string(name));
    for(auto fd : fileHdrs)
    {
        if(fileHashCode == fd.fileHashCode)
        {
            sector = fd.fileHashCode;
            break;
        }
    }
    if( sector == -1)
        throw std::out_of_range("file header is not in range!");
    
    return sector;
}

int FileHdrMap::FindFileHeader(int nameHash) throw (std::out_of_range)
{
    // calculate the hash code of file name, which is more easy
    // to save in disk
    int sector = -1;
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

