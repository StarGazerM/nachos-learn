#include "segment.h"
#include "synchdisk.h"
#include "main.h"
#include <ctime>

DiskSegment::DiskSegment(int begin)
{
    this->begin = begin;
    usageTable = new Bitmap(NumDataSeg);
    end = begin+1; // data segment start at second sector
}

int DiskSegment::NumAlive()
{
    return NumDataSeg - usageTable->NumClear();
}

bool DiskSegment::IsClean()
{
    return (NumAlive() == 0);
}

bool DiskSegment::IsFull()
{
    return (end == NumDataSeg);
}

//-----------------------------------------------------------
// DiskSegment::AllocateSector
//   assign a sector on in this segment to a file
//   1. increase current end(an allocation in segment is "copy")
//   2. mark this pos alive in usage table
//   3. add an entry to summary
//   is current sector is full, an overflow error
//   will be thrown 
//   return value is the sector number on disk 
//---------------------------------------------------------
int DiskSegment::AllocateSector(char* name, int version)throw(std::overflow_error)
{
    std::hash<std::string> hash_fn;
    int fileHashCode = hash_fn(std::string(name));
    if(IsFull())
        throw std::overflow_error("this segment is full!");
    // change usageTable
    end++;
    usageTable->Mark(end);
    summary[end] =  SummaryEntry(version, fileHashCode);
    return end;
}

int DiskSegment::AllocateSector(int nameHash, int version)throw(std::overflow_error)
{
    if(IsFull())
        throw std::overflow_error("this segment is full!");
    // change usageTable
    end++;
    usageTable->Mark(end);
    summary[end] =  SummaryEntry(version, nameHash);
    return end;
}

//-----------------------------------------------------------
// DiskSegment::Write
//   this kind of write is just write a sector to disk, 
//   plz note that, the correct of this  block is not guaranteed 
//   here!
//   TODO: merge allocate logic in this write operation
//---------------------------------------------------------
bool DiskSegment::Write(int len, char* data)
{
    int version = std::time(nullptr);
    int pos = end + 1;
    int currentData = 0;
    for(int i = 0; i <  len; i++)
    {
        kernel->synchDisk->WriteSector(end+i, &(data[currentData]));
        // summary[end+i] = SummaryEntry(version, fileNameHash);   
        currentData +=  SectorSize;
    }
}