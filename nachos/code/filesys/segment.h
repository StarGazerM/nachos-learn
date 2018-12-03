// segment.h
// the space in disk, which can be used by user is divided into different Segment
// in current design, 16 special sector is reserved for file system itself or
// some special files.
// all 1008 Sector will be divided into 63 segment, each  contain 16 sector,
// it is just for simulation, in actually file system, segment size should be large
// as 256kb to 2mb
//  the first sector in segment is SummarySector

#ifndef SEGMENT_FS_H
#define SEGMENT_FS_H
#include "bitmap.h"
#include <array>

// all this should be in a  name space to avoid colide with segment in memory
// management
#define SegSize 16
#define NumSeg 63
#define NumDataSeg 15
#define DataSegStart 16       // 16 is the first data segment

// the summary need to keep track for every live sector in segment
class SummaryEntry
{
  public:
    SummaryEntry()
    {
        last_access = -1;
        fileHashCode = -1;
    }
    SummaryEntry(int last_access, int fileHash)
    {
        this->last_access = last_access;
        this->fileHashCode = fileHash;
    }
    int last_access;  // working as version
    int fileHashCode; // file name
                      // bool isAlive;
};

using SegmentSummary = std::array<SummaryEntry, NumDataSeg>;

class DiskSegment
{
  public:
    DiskSegment(){};
    DiskSegment(int start);
    int NumAlive();
    bool IsClean();
    bool IsFull();
    int AllocateSector(char* name, int version)throw(std::overflow_error);
    int AllocateSector(int nameHash, int version)throw(std::overflow_error);            
                                      // alloacte a sector to a file 
    bool Write(int len, char *data); // write a sector into segment. this kind of 
                                    // write will cause a real disk io
    SegmentSummary &GetSummay() { return summary; }
    Bitmap *GetUsage() { return usageTable; }
    void SetEnd(int end) { this->end = end; }
    int GetEnd() { return end; }
    int SetBegin(int begin) { this->begin = begin; }
    int GetBegin() { return begin; }

    ~DiskSegment(){ delete usageTable; }

  private:
    int begin;              // the start offset of this segment. real segnumber in
                            // summary can be calculated from this
    int end;                // track the end of the Segment, it will be convient for write
    SegmentSummary summary; // all summary is stored in the first sector in segment
    Bitmap *usageTable;     // usagtable in each seg will be saved together in an special sector
                            // in reserved sector
                            // TODO: when a file is written this need to be clear
};

#endif