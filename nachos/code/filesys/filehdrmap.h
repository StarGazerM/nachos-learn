// filehdrmap.h
// store the location of all file in log
// this will take up 4 sector in disk to store this
// (in check point)
// there is trick here, index in array is the index of
// sector in disk ,while the value is

#ifndef FILEHDRMAP_H
#define FILEHDRMAP_H

#include <array>
#include <algorithm>

#define NumSectors 128

class FileHdrMap
{
  private:
    std::array<int, NumSectors> fileHdrs;

  public:
    FileHdrMap();
    bool AddFileHdr(int fileHashCode, int sectorNum); // add or update the position of
                                                      // an file header
    bool DeleteFileHdr(int fileHashCode);
};

#endif