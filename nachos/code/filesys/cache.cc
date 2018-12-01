#include "cache.h"
#include <cstring>
#include "synchdisk.h"
#include "main.h"

LogCache::LogCache()
{
    // begin = 0;
    end = 0;
    dirtybits = 0;
}

void LogCache::Read(int sectorNum, char* dest) throw(std::out_of_range)
{
    if(sectorMap.find(sectorNum) == sectorMap.end())
    {
        // cache miss
        throw std::out_of_range("cache miss");
    }
    int buf_pos = sectorMap[sectorNum].first;
    // TODO change the sector size hard code to macro define
    // which a totolly refactor of fs require is needed
    std::memcpy(&(dest[0]), &(buffer[buf_pos]), SectorSize);  // read data from buffer
}

void LogCache::Append(int sectorNum, char *data, NameHashCode filename, IDisk *disk)
{
    if(dirtybits + SectorSize >= BUFFER_SIZE) 
    {                                         
        // check whether buffer is full, if it is full
        // persist all data in buffer, and reset cursor
        this->Persist(disk);
        sectorMap.erase(sectorMap.begin()); // the orignal head is overlaped
    }
    sectorMap[sectorNum] = make_pair(end, filename);
    for(int i = 0; i < SectorSize; i++)
    {
        if( (end+i) < SectorSize)
        {
            buffer[end+i] = data[i];
        }
        else
        {
            sectorMap.erase(sectorMap.begin());
            buffer[i+end-SectorSize] = data[i];
        }
        dirtybits++;
    }
}

void LogCache::Persist(IDisk *disk)
{
    int persisted = 0;
    int start = 0;
    if((end - dirtybits) >= 0)
    {
        start = end - dirtybits;
    }
    else
    {
        start = BUFSIZ - (dirtybits - end);
    }
    while(persisted < dirtybits)
    {
        // get the write place from current segment table
        // REFECTOR:extract this logic to an ADT
        // if(kernel->fileSystem->segTable[kernel->fileSystem->currentSeg]->IsFull())
        // {

        // }
        // persist to the end of current sector
        int seg = kernel->fileSystem->currentSeg;
        NameHashCode name; // get the file name of the block need to be persisted
        for(auto s : sectorMap)
        {
            if(start + persisted == s.first)
            {
                name = s.second.second;
                break;
            }
        }
        kernel->fileSystem->segTable[seg]->Write(SectorSize, &(buffer[start+persisted]), name);

        persisted += SectorSize;
    }
}
