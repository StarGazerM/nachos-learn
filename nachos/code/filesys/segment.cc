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

bool DiskSegment::Write(int len, char* data, int fileNameHash)
{
    int version = std::time(nullptr);
    int pos = end + 1;
    int currentData = 0;
    for(int i = 0; i <  len; i++)
    {
        kernel->synchDisk->WriteSector(end+i, &(data[currentData]));
        summary[end+i] = SummaryEntry(version, fileNameHash);   
        currentData +=  SectorSize;
    }
}