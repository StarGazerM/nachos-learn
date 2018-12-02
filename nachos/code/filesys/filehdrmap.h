// filehdrmap.h
// store the location of all file in log
// this will take up 4 sector in disk to store this
// (in check point)
// the version in filehdr is implement by last access time
// each operation to this file header would change this.

#ifndef FILEHDRMAP_H
#define FILEHDRMAP_H

#include <vector>
#include <algorithm>
#include <stdexcept>

class HdrInfo
{
  public:
    HdrInfo(){};
    HdrInfo(int fileHashCode, int version, int logOffset);

    int last_access;
    int fileHashCode;
    int logOffset;

    bool operator==(const HdrInfo &other);
};

class FileHdrMap
{
  private:
    std::vector<HdrInfo> fileHdrs;

  public:
    FileHdrMap(){};
    int FindFileHeader(char* name) throw (std::out_of_range);                      // get the sector number of a file by it's name     
    void UpdateFileHdr(char* name, int version,int logOffset); // add or update the position of
                                                      // an file header
    void FileContentModified(char* name, int version);  // this will update the version of a file 
    bool DeleteFileHdr(char* name);               // delete an entry in map
    std::vector<HdrInfo>& GetAllEntries(){ return fileHdrs; } 
    void SetAllEntires(std::vector<HdrInfo>& entires){ fileHdrs = entires;};
    void ClearAll();
};

#endif