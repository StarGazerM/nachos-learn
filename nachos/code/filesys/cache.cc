#include "cache.h"
#include <cstring>
#include "main.h"
#include "synchdisk.h"

LogCache::LogCache()
{
    // begin = 0;
    end = 0;
    dirtybits = 0;
}

// TODO: remove this plz
void LogCache::Read(int sectorNum, char *dest) throw(std::out_of_range)
{
    if (sectorMap.find(sectorNum) == sectorMap.end())
    {
        // cache miss
        throw std::out_of_range("cache miss");
    }
    int buf_pos = sectorMap[sectorNum];
    // TODO: change the sector size hard code to macro define
    // which a totolly refactor of fs require is needed
    std::memcpy(&(dest[0]), &(buffer[buf_pos]), SectorSize); // read data from buffer
}

//---------------------------------------------------------------------------------
// LogCache::Append
//  this will  append a sector onto cache, and it's real sector number on
//  disk will be returned
//---------------------------------------------------------------------------------
void LogCache::Append(int sectorNum, char *data, IDisk *disk)
{
    if (dirtybits + SectorSize >= BUFFER_SIZE)
    {
        // check whether buffer is full, if it is full
        // persist all data in buffer, and reset cursor
        this->Persist(disk);
        sectorMap.erase(sectorMap.begin()); // the orignal head is overlaped
    }
    sectorMap[sectorNum] = end;
    for (int i = 0; i < SectorSize; i++)
    {
        if ((end + i) < SectorSize)
        {
            buffer[end + i] = data[i];
        }
        else
        {
            sectorMap.erase(sectorMap.begin());
            buffer[i + end - SectorSize] = data[i];
        }
        dirtybits++;
    }
}

//----------------------------------------------------------------------
// LogCache::Persist
// this operation should just be a pure disk writing operation
// the segment table should be well orignized before this,
// which means, the data on segment summary should be changed
// when a "file write" operation happen
//----------------------------------------------------------------------
void LogCache::Persist(IDisk *disk)
{
    int persisted = 0;
    int start = 0;
    if ((end - dirtybits) >= 0)
    {
        start = end - dirtybits;
    }
    else
    {
        start = BUFSIZ - (dirtybits - end);
    }
    while (persisted < dirtybits)
    {
        // get the write place from current segment table
        // REFECTOR:extract this logic to an ADT
        // if(kernel->fileSystem->segTable[kernel->fileSystem->currentSeg]->IsFull())
        // {

        // }
        // persist to the end of current sector
        int seg = kernel->fileSystem->currentSeg;
        int sec; // get the file name of the block need to be persisted

        for (auto s : sectorMap)
        {
            if (start + persisted == s.first)
            {
                sec = s.second;
                break;
            }
        }
        //kernel->fileSystem->segTable[seg]->Write(SectorSize, &(buffer[start + persisted]));
        // FIXME: here the sector number many be wrong, double check
        disk->WriteSector(sec, &(buffer[start + persisted]));

        // if sector is full, continue on next empty segment
        // for here, next find  will start at directly next seg, casue
        // this can make full use of sequencial write performance
        if (!kernel->fileSystem->segTable[seg]->IsFull())
        {
            while (seg != kernel->fileSystem->currentSeg)
            {
                if (!kernel->fileSystem->segTable[seg]->IsFull())
                    break;
                    
                if(seg == NumSeg)   // circular
                    seg = 0;

                // the disk should not be really full.....
                // in LFS it is prerequsite
                ASSERT(seg != kernel->fileSystem->currentSeg)
                seg++;
            }
            kernel->fileSystem->currentSeg = seg;
        }

        persisted += SectorSize;
    }
}

//--------------------------------------------------------------------
// ReadCache::Read
//    read a data out
//--------------------------------------------------------------------
void ReadCache::Read(int sec, char* dest)throw(std::out_of_range)
{
    auto find = buffer.find(sec);
    if(find == buffer.end())
    {
        // cache miss
        throw out_of_range("this sector is not in read cache");
    }
    else
    {
        SectorArray tmp = buffer[sec];
        std::memcpy(dest, &(tmp[0]), SectorSize);
    }
}

void ReadCache::UpdateOrAdd(int sec, char* data)
{
    SectorArray in_buf;
    for(int i=0; i < SectorSize; i++)
    {
        in_buf[i] = data[i];
    }
    if(buffer.size() == BUFFER_SIZE)    // overflow
    {
        buffer.erase(buffer.begin());   //FIFO
    }
    buffer[sec] = in_buf;
}

void ReadCache::Clear()
{
    buffer.clear();
}
