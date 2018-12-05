// openfile.cc
//	Routines to manage an open Nachos file.  As in UNIX, a
//	file must be open before we can read or write to it.
//	Once we're all done, we can close it (in Nachos, by deleting
//	the OpenFile data structure).
//
//	Also as in UNIX, for convenience, we keep the file header in
//	memory while the file is open.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.
#ifndef FILESYS_STUB

#include "copyright.h"
#include "main.h"
#include "filehdr.h"
#include "openfile.h"
#include "synchdisk.h"
#include <ctime>

//----------------------------------------------------------------------
// OpenFile::OpenFile
// 	Open a Nachos file for reading and writing.  Bring the file header
//	into memory while the file is open.
//
//	"sector" -- the location on disk of the file header for this file
//----------------------------------------------------------------------

OpenFile::OpenFile(int sector)
{
    hdr = new FileHeader;
    hdr->FetchFrom(sector);
    seekPosition = 0;
}

OpenFile::OpenFile(int sector, char *name)
{
    hdr = new FileHeader;
    fileName = name;
    hdr->FetchFrom(sector);
    seekPosition = 0;
}
//----------------------------------------------------------------------
// OpenFile::~OpenFile
// 	Close a Nachos file, de-allocating any in-memory data structures.
//----------------------------------------------------------------------

OpenFile::~OpenFile()
{
    delete hdr;
}

//----------------------------------------------------------------------
// OpenFile::Seek
// 	Change the current location within the open file -- the point at
//	which the next Read or Write will start from.
//
//	"position" -- the location within the file for the next Read/Write
//----------------------------------------------------------------------

void OpenFile::Seek(int position)
{
    seekPosition = position;
}

//----------------------------------------------------------------------
// OpenFile::Read/Write
// 	Read/write a portion of a file, starting from seekPosition.
//	Return the number of bytes actually written or read, and as a
//	side effect, increment the current position within the file.
//
//	Implemented using the more primitive ReadAt/WriteAt.
//
//	"into" -- the buffer to contain the data to be read from disk
//	"from" -- the buffer containing the data to be written to disk
//	"numBytes" -- the number of bytes to transfer
//----------------------------------------------------------------------

int OpenFile::Read(char *into, int numBytes)
{
    kernel->currentThread->SetCurrentFD(this);
    int result = ReadAt(into, numBytes, seekPosition);
    seekPosition += result;
    return result;
}

int OpenFile::Write(char *into, int numBytes)
{
    int result = WriteAt(into, numBytes, seekPosition);
    seekPosition += result;
    return result;
}

//----------------------------------------------------------------------
// OpenFile::ReadAt/WriteAt
// 	Read/write a portion of a file, starting at "position".
//	Return the number of bytes actually written or read, but has
//	no side effects (except that Write modifies the file, of course).
//
//	There is no guarantee the request starts or ends on an even disk sector
//	boundary; however the disk only knows how to read/write a whole disk
//	sector at a time.  Thus:
//
//	For ReadAt:
//	   We read in all of the full or partial sectors that are part of the
//	   request, but we only copy the part we are interested in.
//	For WriteAt:
//	   We must first read in any sectors that will be partially written,
//	   so that we don't overwrite the unmodified portion.  We then copy
//	   in the data that will be modified, and write back all the full
//	   or partial sectors that are part of the request.
//
//	"into" -- the buffer to contain the data to be read from disk
//	"from" -- the buffer containing the data to be written to disk
//	"numBytes" -- the number of bytes to transfer
//	"position" -- the offset within the file of the first byte to be
//			read/written
//----------------------------------------------------------------------

int OpenFile::ReadAt(char *into, int numBytes, int position)
{
    kernel->currentThread->SetCurrentFD(this);
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    char *buf;

    if ((numBytes <= 0) || (position >= fileLength))
        return 0; // check request
    if ((position + numBytes) > fileLength)
        numBytes = fileLength - position;
    DEBUG(dbgFile, "Reading " << numBytes << " bytes at " << position << " from file of length " << fileLength);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    // read in all the full and partial sectors that we need
    buf = new char[numSectors * SectorSize];
    for (i = firstSector; i <= lastSector; i++)
    {
        int sec = hdr->ByteToSector(i * SectorSize);
        kernel->synchDisk->ReadSector(sec, &buf[(i - firstSector) * SectorSize]);
    }

    // copy the part we want
    bcopy(&buf[position - (firstSector * SectorSize)], into, numBytes);
    delete[] buf;
    return numBytes;
}

#ifndef LOG_FS
// Since original Nachos restricts file size into one sector, original WriteAt restrics file length.
// Thus the numBytes could be updated to not exceed one sector.
// We would like to change as little original code as possible,
// so we add a variable "realNumBytes" to store the value of "numBytes" input.
int OpenFile::WriteAt(char *from, int numBytes, int position)
{
    int realNumBytes = numBytes;
    // Sounds ambigious variable, just used to keep the original input value.
    // The reason refers to Line #164.
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    bool firstAligned, lastAligned;
    char *buf;

    if (numBytes <= 0)
        return 0; // check request
    if ((position + numBytes) > fileLength)
        // We don't want to change the original code
        // Instead of remove the restriction, we use a dummy variable called realNumBytes to keep original value.
        numBytes = fileLength - position;
    DEBUG(dbgFile, "Writing " << numBytes << " bytes at " << position << " from file of length " << fileLength);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    // Our design is to allow one more extra sector for write.
    buf = new char[(numSectors + 1) * SectorSize];

    firstAligned = (position == (firstSector * SectorSize));
    lastAligned = ((position + numBytes) == ((lastSector + 1) * SectorSize));

    // read in first and last sector, if they are to be partially modified
    if (!firstAligned)
        ReadAt(buf, SectorSize, firstSector * SectorSize);
    if (!lastAligned && ((firstSector != lastSector) || firstAligned))
        ReadAt(&buf[(lastSector - firstSector) * SectorSize],
               SectorSize, lastSector * SectorSize);

    // copy in the bytes we want to change
    if ((position + realNumBytes) > fileLength)
    {
        // numBytes has been modified, now we change it back
        ASSERT(position + numBytes < (numSectors + 1) * PageSize)
        lastSector++;
        numBytes = realNumBytes;
        // update new file length
        fileLength = position + realNumBytes;
    }

    bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

    // The original Nachos will only write back the modified sector
    // We changed the iteration style, because when writing back the last sector,
    // the input of ByteToSector() needs to be the last byte of the file.
    // And when writing the other sectors, the input of ByteToSector() is 0.
    // SO we cannot stay on the original iteration style.
    for (i = 0; i <= lastSector * SectorSize; i += SectorSize)
    {
        int readPosition = i;
        int writePosition = i;
        if ((i + SectorSize) > fileLength)
        // When writing back the last sector
        {
            writePosition = fileLength;
        }
        kernel->synchDisk->WriteSector(hdr->ByteToSector(writePosition),
                                       &buf[readPosition]);
    }
    delete[] buf;
    return numBytes;
}

#else
// In LFS write operation should not exceed file length
// if you want to add content to a file, use Append instead
int OpenFile::WriteAt(char *from, int numBytes, int position)
{
    kernel->currentThread->SetCurrentFD(this);
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    bool firstAligned, lastAligned;
    char *buf;
    int segNum;
    DiskSegment* seg;
    std::hash<std::string> hash_fn;
    int fileHashCode = hash_fn(std::string(fileName));

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;
    if (numBytes <= 0 && (position+numBytes) <= (fileLength/SectorSize)*SectorSize)
        return 0; // check request

    buf = new char[numSectors * SectorSize];
    
    firstAligned = (position == (firstSector * SectorSize));
    lastAligned = ((position + numBytes) == ((lastSector + 1) * SectorSize));

    // read in first and last sector, if they are to be partially modified
    if (!firstAligned)
        ReadAt(buf, SectorSize, firstSector * SectorSize);
    if (!lastAligned && ((firstSector != lastSector) || firstAligned))
        ReadAt(&buf[(lastSector - firstSector) * SectorSize],
               SectorSize, lastSector * SectorSize);

    bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

    int version = std::time(nullptr);
    for (i = 0; i <= lastSector * SectorSize; i += SectorSize)
    {
        // FIXME: version here would be miss match!
        // kernel->synchDisk->WriteSector(hdr->ByteToSector(i),
        //                                &buf[i]); 
        segNum = kernel->fileSystem->currentSeg;
        seg = kernel->fileSystem->segTable[segNum];
        int newSector = seg->AllocateSector(fileName, version);
        // we need to know which position we have modified
        hdr->UpdateSectorNum(i, newSector, fileHashCode);
        // seg->Write(SectorSize, &buf[i]);
        kernel->synchDisk->WriteSector(newSector, &(buf[i]));
    }
    // write back changed file header
    segNum = kernel->fileSystem->currentSeg;
    seg = kernel->fileSystem->segTable[segNum];
    int newHdrSec = seg->AllocateSector(fileName, version);
    hdr->WriteBack(newHdrSec);
    kernel->fileSystem->GetFileHdrMap()->UpdateFileHdr(fileName, version, newHdrSec);
    delete[] buf;
    return numBytes;
}
#endif
//----------------------------------------------------------------------
// OpenFile::Length
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int OpenFile::Length()
{
    return hdr->FileLength();
}

#ifdef LOG_FS
//--------------------------------------------------------
// OpenFile::AppendOneSector
//  append some data into a file
//  if the fileSize is full a new sector will be allocate
//  NOTE:
//  in order to simplify this function, max_size append 
//  size at once should be under one sector
//  return value is the size of written data
//--------------------------------------------------------
int OpenFile::AppendOneSector(char *from, int numBytes)
{
    int newSector;
    kernel->currentThread->SetCurrentFD(this);
    int fileLength = hdr->FileLength(); 
    int numSectors = hdr->GetSectorNum();
    if(numBytes > SectorSize)
        return 0;
    
    int lastSec = divRoundDown(fileLength, SectorSize);
  
    if((fileLength + numBytes) > numSectors*SectorSize)// need new sector
    {
        int currentSeg = kernel->fileSystem->currentSeg;
        DiskSegment *segptr = kernel->fileSystem->segTable[currentSeg];
        newSector = segptr->AllocateSector(fileName, std::time(nullptr));
        char empty[SectorSize] = {'\0'}; // initial the new allocated sector with empty content
                                // TODO: this should be optimized
        kernel->synchDisk->WriteSector(newSector, empty);
        hdr->AppendOne(fileName, newSector);
    }
    hdr->SetFileLength(fileLength + numBytes);
    WriteAt(from, numBytes, fileLength);
    return numBytes;
}

#endif

#endif //FILESYS_STUB
