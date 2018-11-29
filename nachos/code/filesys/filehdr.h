// filehdr.h 
//	Data structures for managing a disk file header.  
//
//	A file header describes where on disk to find the data in a file,
//	along with other information about the file (for instance, its
//	length, owner, etc.)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "pbitmap.h"

#define NumDoubleIndirect   1
#define NumIndirect 9
#define NumDirect 	((SectorSize - (2+10) * sizeof(int)) / sizeof(int))
#define NumData     (SectorSize / sizeof(int))
// Project requires the max file size is 128kb
#define MaxFileSize 	1024 * SectorSize

// The following class defines the Nachos "file header" (in UNIX terms,  
// the "i-node"), describing where on disk to find all of the data in the file.
// The file header is organized as a simple table of pointers to
// data blocks. 
//
// The file header data structure can be stored in memory or on disk.
// When it is on disk, it is stored in a single sector -- this means
// that we assume the size of this data structure to be the same
// as one disk sector.  Without indirect addressing, this
// limits the maximum file length to just under 4K bytes.
//
// There is no constructor; rather the file header can be initialized
// by allocating blocks for the file (if it is a new file), or by
// reading it from disk.

// in order to store a file larger than 4k bytes, I use a Indirect node
// combine withs doublely direct node. which work as 3 layer data structure
// which is similar to multi-path tree structure.
// although one double node can satisfy the file large as whole disk
// right now, in order to decrease the time expanse of find a block
// direct access and one level access is still kept in current file header

// work as direct data block
class IndirectHeader
{
  public:
    int dataSectors[NumData];
    IndirectHeader()
    {
        for(int i = 0; i < NumData; i++)
        {
            dataSectors[i] = -1;
        }
    }

    void Deallocate(PersistentBitmap *freeMap);
    void FetchFrom(int sectorNumber); 	
    void WriteBack(int sectorNumber); 	
    int ByteToSector(int offset, PersistentBitmap *freeMap);
};

// a block full of pointer to directs data block, whcih
// will cost one 
class DoubleIndirectHeader
{
  public:
    int dataHeaders[NumData];
    DoubleIndirectHeader()
    {
        for(int i = 0; i < NumData; i++)
            dataHeaders[i] = -1;
    }
    void Deallocate(PersistentBitmap *freeMap);
    void FetchFrom(int sectorNumber); 	
    void WriteBack(int sectorNumber);
    int ByteToSector(int offset, PersistentBitmap *freeMap);
};

// // full of pointer to indirect datd block 
// class DoubleIndirectHeader
// {
//   public:
//     int indirectHeaders[NumData];
//     DoubleIndirectHeader()
//     {
//         for(int i = 0; i < NumData; i++)
//             indirectHeaders[0] = -1;
//     }
//     void Deallocate(PersistentBitmap *freeMap);
//     // void FetchFrom(int sectorNumber); 	
//     // void WriteBack(int sectorNumber);    
// };

class FileHeader {
  public:
    FileHeader();
    bool Allocate(PersistentBitmap *bitMap, int fileSize);// Initialize a file header, 
						//  including allocating space 
						//  on disk for the file data
    void Deallocate(PersistentBitmap *bitMap);  // De-allocate this file's 
						//  data blocks

    void FetchFrom(int sectorNumber); 	// Initialize file header from disk
    void WriteBack(int sectorNumber); 	// Write modifications to file header
					//  back to disk

    int ByteToSector(int offset);	// Convert a byte offset into the file
					// to the disk sector containing
					// the byte

    int FileLength();			// Return the length of the file 
					// in bytes

    void Print();			// Print the contents of the file.

  private:
    int numBytes;			// Number of bytes in the file
    int numSectors;			// Number of data sectors in the file
    int dataSectors[NumDirect];		// Disk sector numbers for each data 
					// block in the file
    int indirects[NumIndirect];
    int doubleIndirect;

    void initAllData(PersistentBitmap *freeMap, int num);
};

#endif // FILEHDR_H
