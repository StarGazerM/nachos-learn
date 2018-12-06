// filesys.cc 
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk 
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them 
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.
#ifndef FILESYS_STUB

#include "copyright.h"
#include "debug.h"
#include "disk.h"
#include "pbitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"
#include "synchdisk.h"
#include "main.h"
#include <ctime>
#include <set>

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known 
// sectors, so that they can be located on boot-up.
#define FreeMapSector 		0
#define DirectorySector 	1

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number 
// of files that can be loaded onto the disk.
#define FreeMapFileSize 	(NumSectors / BitsInByte)
#define NumDirEntries 		10
#define DirectoryFileSize 	(sizeof(DirectoryEntry) * NumDirEntries)

//----------------------------------------------------------------------
// FileSystem::FileSystem
//  NOTE: before use a disk, format it first! 
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).  
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{
    DEBUG(dbgFile, "Initializing the file system.");
    if (format)
    {
    #ifndef LOG_FS
        PersistentBitmap *freeMap = new PersistentBitmap(NumSectors);
        Directory *directory = new Directory(NumDirEntries);
        FileHeader *mapHdr = new FileHeader;
        FileHeader *dirHdr = new FileHeader;

        DEBUG(dbgFile, "Formatting the file system.");

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FreeMapSector);
        freeMap->Mark(DirectorySector);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!

        ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
        ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));

        // Flush the bitmap and directory FileHeaders back to disk
        // We need to do this before we can "Open" the file, since open
        // reads the file header off of disk (and currently the disk has garbage
        // on it!).

        DEBUG(dbgFile, "Writing headers back to disk.");
        mapHdr->WriteBack(FreeMapSector);
        dirHdr->WriteBack(DirectorySector);

        // OK to open the bitmap and directory files now
        // The file system operations assume these two files are left open
        // while Nachos is running.

        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);

        // Once we have the files "open", we can write the initial version
        // of each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.

        DEBUG(dbgFile, "Writing bitmap and directory back to disk.");
        freeMap->WriteBack(freeMapFile); // flush changes to disk
        directory->WriteBack(directoryFile);

        if (debug->IsEnabled('f'))
        {
            freeMap->Print();
            directory->Print();
        }
        delete freeMap;
        delete directory;
        delete mapHdr;
        delete dirHdr;
    #else
        fileHrdMap = new FileHdrMap;
        currentSeg = 0;
        // init all segment
        // TODO: if possible change to RAII style
        for(int i= 0; i < NumSeg; i++)
        {
            // calculate the start sector of each segment
            segTable[i] = new DiskSegment(i*SegSize + DataSegStart);
        }
        // SaveToCheckPoint();
    #endif
    }
    else
    {
        // if we are not formatting the disk, just open the files representing
        // the bitmap and directory; these are left open while Nachos is running
    #ifndef LOG_FS
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
    #else

        // if not format restore fileheader map and segment table from checkpoint
        fileHrdMap = new FileHdrMap;
        RestoreFromCheckPoint();
    #endif
    }
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk 
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file 
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------

bool
FileSystem::Create(char *name, int initialSize)
{
    Directory *directory;
    PersistentBitmap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;

    DEBUG(dbgFile, "Creating file " << name << " size " << initialSize);

    directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);

    if (directory->Find(name) != -1)
        success = FALSE; // file is already in directory
    else
    {
        freeMap = new PersistentBitmap(freeMapFile, NumSectors);
        sector = freeMap->FindAndSet(); // find a sector to hold the file header
        if (sector == -1)
            success = FALSE; // no free block for file header
        else if (!directory->Add(name, sector))
            success = FALSE; // no space in directory
        else
        {
            hdr = new FileHeader;
            if (!hdr->Allocate(freeMap, initialSize))
                success = FALSE; // no space on disk for data
            else
            {
                success = TRUE;
                // everthing worked, flush all changes back to disk
                hdr->WriteBack(sector);
                directory->WriteBack(directoryFile);
                freeMap->WriteBack(freeMapFile);
            }
            delete hdr;
        }
        delete freeMap;
    }
    delete directory;
    return success;
}

#ifdef LOG_FS
//---------------------------------------------------------------------
/// Create a file whose inital size zero, in another word, this will only
//  1. create a file header
//  2. change segment usage table( in current desgin, which means segment 
//     summary, will be changed  at the same time).
//  3. write to cache disk
//  4. add it into file header map
//---------------------------------------------------------------------
bool
FileSystem::Create(char* name)
{
    //TODO: check whether the file is already exist
    FileHeader *hdr = new FileHeader;
    // change segment info
    DiskSegment *segptr = segTable[currentSeg];
    int version = std::time(nullptr);
    int newSector = segptr->AllocateSector(name, version);
    hdr->WriteBack(newSector);
    // add into file header map
    fileHrdMap->UpdateFileHdr(name, version, newSector);
    return true;
}

#endif

//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.  
//	To open a file:
//	  Find the location of the file's header, using the directory 
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------
#ifndef LOG_FS
OpenFile *
FileSystem::Open(char *name)
{ 
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *openFile = NULL;
    int sector;

    DEBUG(dbgFile, "Opening file" << name);
    directory->FetchFrom(directoryFile);
    sector = directory->Find(name); 
    if (sector >= 0) 		
	openFile = new OpenFile(sector);	// name was found in directory 
    delete directory;
    return openFile;				// return NULL if not found
}

#else
//----------------------------------------------------------------------
// open file in LFS
// 1. first get file header sector number in map
// 2. fetch that block from cache
//----------------------------------------------------------------------
OpenFile *
FileSystem::Open(char *name)
{
    // Directory *directory = new Directory(NumDirEntries);
    int sector; 
    OpenFile *openFile = NULL;
    try
    {
        sector = fileHrdMap->FindFileHeader(name);
    }
    catch(const std::out_of_range &e)
    {
        e.what();
        Abort();   
    }
    openFile = new OpenFile(sector, name);
    return openFile;
}

#endif

//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------

#ifndef LOG_FS
bool
FileSystem::Remove(char *name)
{ 
    Directory *directory;
    PersistentBitmap *freeMap;
    FileHeader *fileHdr;
    int sector;
    
    directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);
    sector = directory->Find(name);
    if (sector == -1) {
       delete directory;
       return FALSE;			 // file not found 
    }
    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);

    freeMap = new PersistentBitmap(freeMapFile,NumSectors);

    fileHdr->Deallocate(freeMap);  		// remove data blocks
    freeMap->Clear(sector);			// remove header block
    directory->Remove(name);

    freeMap->WriteBack(freeMapFile);		// flush to disk
    directory->WriteBack(directoryFile);        // flush to disk
    delete fileHdr;
    delete directory;
    delete freeMap;
    return TRUE;
} 

#else
//----------------------------------------------------------------------
// FileSystem::Remove
// 	In LFS, we just need to remove it from our inode map( file header map)
//----------------------------------------------------------------------
bool
FileSystem::Remove(char *name)
{
    fileHrdMap->DeleteFileHdr(name);
}

#endif
//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void
FileSystem::List()
{
    Directory *directory = new Directory(NumDirEntries);

    directory->FetchFrom(directoryFile);
    directory->List();
    delete directory;
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void
FileSystem::Print()
{
    FileHeader *bitHdr = new FileHeader;
    FileHeader *dirHdr = new FileHeader;
    PersistentBitmap *freeMap = new PersistentBitmap(freeMapFile,NumSectors);
    Directory *directory = new Directory(NumDirEntries);

    printf("Bit map file header:\n");
    bitHdr->FetchFrom(FreeMapSector);
    bitHdr->Print();

    printf("Directory file header:\n");
    dirHdr->FetchFrom(DirectorySector);
    dirHdr->Print();

    freeMap->Print();

    directory->FetchFrom(directoryFile);
    directory->Print();

    delete bitHdr;
    delete dirHdr;
    delete freeMap;
    delete directory;
} 

#ifdef LOG_FS
//--------------------------------------------------------------------
// Save maps into disk
// Currently, file header map will be save to sector 3 and sector 4
// the first 4 byte in sector 3 is number of entries in file header
// map 
// REFECTOR: change the type operation here to sstream way!
// nachos's code style is terrible, we need a more cpp way
//--------------------------------------------------------------------
void FileSystem::SaveToCheckPoint()
{
    auto headers = fileHrdMap->GetAllEntries();
    int len = headers.size();

    HdrInfo *hdrbuf = new HdrInfo[len];
    for(int i = 0; i < len; i++)
    {
        hdrbuf[i] = headers[i];
    }
    char *hdrcheckp = new char[2*SectorSize];
    memcpy(&(hdrcheckp[0]), &len, sizeof(int));  // copy the number of entries to 
                                                // begin of the sector
    memcpy(&(hdrcheckp[sizeof(int)]), hdrbuf, sizeof(HdrInfo)*len);
                                                // copy the content of map
    kernel->synchDisk->WriteSector(5 ,hdrcheckp);
    kernel->synchDisk->WriteSector(6, &(hdrcheckp[SectorSize]));

    // save the status of segment summary to the head of each segment
    int start = SegSize - 1; // the start of all writable data is at 15 sector
    for(int i = 0; i < NumSeg; i++)
    {
        char* summaryTmp = new char[SectorSize];
        SegmentSummary& sumref = segTable[i]->GetSummay();
        unsigned int* usageBits = segTable[i]->GetUsage()->GetBits();
        int size = (segTable[i]->GetUsage()->GetNumWords()) * sizeof(unsigned int);
        memcpy(&(summaryTmp[0]), &(sumref[0]), sizeof(SegmentSummary));         // save summary entry
        memcpy(&(summaryTmp[sizeof(SegmentSummary)]), usageBits, size);         // save the bit map which mark the live data
        int seg_pos = i*SegSize+start;
        kernel->synchDisk->WriteSector(seg_pos, summaryTmp);
    }

    delete hdrbuf;
    delete hdrcheckp;
}


//--------------------------------------------------------------------
// read all info from checkpoint, this will be evoked when file system
// initalize
//--------------------------------------------------------------------
void FileSystem::RestoreFromCheckPoint()
{
    char *hdrcheckp = new char[2*SectorSize];
    kernel->synchDisk->ReadSector(5, hdrcheckp);
    kernel->synchDisk->ReadSector(6, &(hdrcheckp[SectorSize]));
    int len;        // read the number of entries first
    memcpy(&len, &(hdrcheckp[0]), sizeof(int));
    HdrInfo *hdrbuf = new HdrInfo[len];
    memcpy(hdrbuf, &(hdrcheckp[sizeof(int)]),sizeof(HdrInfo)*len);

    std::vector<HdrInfo> tmpv;
    for(int i = 0; i < len; i++)
    {
        tmpv.push_back(hdrbuf[i]);
    }
    fileHrdMap->SetAllEntires(tmpv);

    // read all SegmentSummay into memory
    // it is not necessary, but it can speed up the running of system
    // this is eager version, lazy version can be useful somehow, when 
    // the disk is large
    int start = SegSize - 1; 
    for(int i = 0; i < NumSeg; i++)
    {
        char* summaryTmp = new char[SectorSize];
        kernel->synchDisk->ReadSector((start+i*SegSize), summaryTmp);
        DiskSegment* segment = new DiskSegment(i*SegSize+start);
        SegmentSummary& sumref = segment->GetSummay();
        unsigned int* usageBits = segment->GetUsage()->GetBits();
        int size = (segment->GetUsage()->GetNumWords()) * sizeof(unsigned int);
        memcpy(&(sumref[0]), &(summaryTmp[0]), sizeof(SegmentSummary));         
        memcpy(usageBits, &(summaryTmp[sizeof(SegmentSummary)]), size);

        // calculate the tail of current segment
        int counter = 0;
        int end = segment->GetEnd();
        while((segment->GetSummay()[counter]).fileHashCode == -1)
        {
            counter++;
            end++;
        }
        segment->SetEnd(end);
        segTable[i] = segment;         
    }
    // find the first clean segment as current write cursor
    currentSeg = 0;
    for(auto s :  segTable)
    {
        currentSeg++;
        if(s->IsClean())
        {
            break;
        }
    }

    delete hdrbuf;
    delete hdrcheckp;
}

//--------------------------------------------------
// FileSystem::CleanSegments
//--------------------------------------------------
void FileSystem::CleanSegments()
{
    // check whether condition is satisfied
    int cleanNum = std::count_if(segTable.begin(), segTable.end(), [](DiskSegment *sptr) { return sptr->IsClean(); });

    // in real use open this, set flag to 1
    // if (cleanNum > 20)
    //     return;

    std::vector<DiskSegment *> to_be_clean;
    // need clean
    // find the segment need to be clean
    auto dit = segTable.begin();
    // for(int i = 0; i < (20-cleanNum); i++)
    // {
    //     dit = std::find_if_not(dit, segTable.end()
    //                     , [](DiskSegment* sptr){ return sptr->IsClean();});
    //     to_be_clean.push_back(*dit);
    // }

    // cleamn policy will clean the 5 dirty block whose clean sector is
    // less than other
    // find all dirty segments
    for(auto seg : segTable)
    {
        if(!seg->IsClean())
            to_be_clean.push_back(seg);
    }
    // get most clean 5 segment as the segment to be clean
    std::nth_element(to_be_clean.begin(), to_be_clean.begin()+4, to_be_clean.end(), 
                        [](DiskSegment* s1, DiskSegment* s2){ return s1->NumAlive()<s2->NumAlive();});

    // collect all live data
    std::map<int, SummaryEntry> live_sec;
    for (auto sptr : to_be_clean)
    {
        Bitmap *usageMap = sptr->GetUsage();
        for (int i = 0; i < NumDataSeg; i++)
        {
            if (usageMap->Test(i))
            {
                // SummaryEntry s = (sptr->GetSummay())[i];
                live_sec[sptr->GetBegin() + 1] = (sptr->GetSummay())[i];
            }
        }
    }

    // rewrite all these data to empty segment and update file header map
    std::map<FileHeader *, SummaryEntry> hdrBuf; // use ptr ....it is bad idea but the stack size is percious
    for (auto p : live_sec)
    {
        int originalSec = p.first;
        int fileName = p.second.fileHashCode;
        int version = p.second.last_access;
        // find a clean segment
        auto cleanSeg = std::find_if(segTable.begin(), segTable.end(), [](DiskSegment *sptr) { return sptr->IsClean(); });
        // read data out
        char datatmp[SectorSize];
        kernel->synchDisk->ReadSector(originalSec, datatmp);
        // update file hdr map, the write back of file header should be done at last
        try
        {
            int origanlHdrSec = fileHrdMap->FindFileHeader(fileName);
            FileHeader *hdr = new FileHeader;
            hdr->FetchFrom(origanlHdrSec);
            // write data
            int newSec = (*cleanSeg)->AllocateSector(fileName, version);
            // (*cleanSeg)->Write(SectorSize, datatmp);
            kernel->synchDisk->WriteSector(newSec, datatmp);
            hdr->ReplaceSectorNum(originalSec, newSec, fileName);
            hdrBuf[hdr] = p.second;
            
        }
        catch (const exception &e)
        {
            // it happens when file is deleted
        }
    }

    // write back file header
    for (auto it : hdrBuf)
    {
        int newSec = segTable[currentSeg]->AllocateSector(it.second.fileHashCode, it.second.last_access);
        it.first->WriteBack(newSec);
        fileHrdMap->UpdateFileHdr(it.second.fileHashCode, it.second.last_access, newSec);
    }

    // finally release original segment
    for (auto segptr : to_be_clean)
    {
        segptr->Clear();
    }
}
#endif

#endif // FILESYS_STUB
