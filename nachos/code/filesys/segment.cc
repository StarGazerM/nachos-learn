#include "segment.h"
#include "synchdisk.h"
#include "main.h"
#include <ctime>

DiskSegment::DiskSegment(int begin)
{
    this->begin = begin;
    usageTable = new Bitmap(NumDataSeg);
    end = begin; // data segment start at second sector
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
//   is current sector is full, rest currentSeg to next segment
//   return value is the sector number on disk 
//---------------------------------------------------------
int DiskSegment::AllocateSector(char* name, int version)
{
    std::hash<std::string> hash_fn;
    int fileHashCode = hash_fn(std::string(name));
    if(IsFull())
    {
        // if sector is full, continue on next empty segment
        // for here, next find  will start at directly next seg, so that
        // this can make full use of sequencial write performance
        int seg = kernel->fileSystem->currentSeg + 1;
        while (seg != kernel->fileSystem->currentSeg)
        {
            if (!kernel->fileSystem->segTable[seg]->IsFull())
                break;

            if (seg == NumSeg) // circular
                seg = 0;

            // the disk should not be really full.....
            // in LFS it is prerequsite
            ASSERT(seg != kernel->fileSystem->currentSeg)
            seg++;
        }
        kernel->fileSystem->currentSeg = seg;
    }
    // change usageTable
    end++;
    usageTable->Mark(end-begin-1);
    summary[end-begin-1] =  SummaryEntry(version, fileHashCode);
    return end;
}

int DiskSegment::AllocateSector(int nameHash, int version)
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
// bool DiskSegment::Write(int len, char* data)
// {
//     int version = std::time(nullptr);
//     int pos = end + 1;
//     int currentData = 0;
//     int secNum = divRoundUp(len, SectorSize);
//     // FIXME: right now we can only write a whole sector in...
//     for(int i = 0; i < secNum; i++)
//     {
//         kernel->synchDisk->WriteSector(end+i, &(data[currentData]));
//         // summary[end+i] = SummaryEntry(version, fileNameHash);   
//         currentData +=  SectorSize;
//     }
// }
//-----------------------------------------------------------
// DiskSegment::Clear
//  clear a segment means clear it usage table, marks every 
//  sector clean
//---------------------------------------------------------
void DiskSegment::Clear()
{
    for(int i = 0; i < usageTable->GetNumBits(); i++)
    {
        usageTable->Clear(i);
    }
}