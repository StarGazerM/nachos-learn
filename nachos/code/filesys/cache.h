// this is work as the buffer for log-structured file system
// If we want to achieve our goal of fast sequencial write, a
// circular buffer is needed, becasue if there is lot of read
// operation (inode map, segment usage map, and file content),
// it impossible to do sequencial move of cylindr.
// for here a array based circular buffer is used.
// when data need persist to disk, we will directly find the end
// of the data on disk

// persist and flush operation will cause a check of segment usage
// table, if not enough space on disk an Segment clean task will be
// called
#ifndef FS_CACHE_H
#define FS_CACHE_H

#include <array>
#include <vector>
#include <map>
#include <exception>
#include "disk.h"

// buffer size is two sector
#define BUFFER_SIZE 2 * SectorSize * 16

using SectorArray = std::array<char, SectorSize>;
using NameHashCode = int; 

class LogCache
{
private:
  int begin; // the start place of current buffer
  int end;   // the end place, because it is an circular buffer so it
             // is possible end num is less than  start
  int dirtybits;  // keep track how many data hasn't been persisted
  std::array<char, BUFFER_SIZE> buffer;
  std::map<int, std::pair<int,NameHashCode>> sectorMap;  // key is sector number
                                                // value is it's address in log and it's file name hash code 

public:
  LogCache();
  void Read(int sectorNum, char* dest) throw(std::out_of_range); // read a sector from cache
  void Append(int sectorNum, char *data, NameHashCode filename, IDisk *disk);
  void Persist(IDisk *disk); // save all data in memory into disk
                  // it will be triggered if buffer is full
                  // or system shutdown
                  // this will not clear all data in buffer
                  // only dirty counter will be affected here
  void Flush(IDisk *disk);   // save all data and then clean!
};

#endif