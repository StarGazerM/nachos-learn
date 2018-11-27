// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "filehdr.h"
#include "debug.h"
#include "synchdisk.h"
#include "main.h"

void 
IndirectHeader::Deallocate(PersistentBitmap *freeMap)
{
    for(int i = 0; i < NumData; i++)
    {
        if(dataSectors[i] != -1)
        {
            ASSERT(freeMap->Test(dataSectors[i]))
            freeMap->Clear(dataSectors[i]);
        }
    }
}

void
IndirectHeader::FetchFrom(int sector)
{
    kernel->synchDisk->ReadSector(sector, (char *)this);
}

void
IndirectHeader::WriteBack(int sector)
{
    kernel->synchDisk->WriteSector(sector, (char *)this); 
}

int 
IndirectHeader::ByteToSector(int offset, PersistentBitmap *freeMap)
{
    if (dataSectors[offset] = -1)   // byte on new sector
    {
        dataSectors[offset] = freeMap->FindAndSet();
        freeMap->WriteBack(kernel->fileSystem->GetFreeMap());
    }
    return dataSectors[offset];
}

void
DoubleIndirectHeader::Deallocate(PersistentBitmap *freeMap)
{
    for(int i = 0; i < NumData; i++)
    {
        if(dataHeaders[i] != -1)
        {
            IndirectHeader *tmp = new IndirectHeader;   // user heap avoid stack over flow
            kernel->synchDisk->ReadSector(dataHeaders[i], (char *)tmp);
            tmp->Deallocate(freeMap);
            ASSERT(freeMap->Test(dataHeaders[i]))
            freeMap->Clear(dataHeaders[i]);
            delete tmp;
        }
    }
}

void
DoubleIndirectHeader::FetchFrom(int sector)
{
    kernel->synchDisk->ReadSector(sector, (char *)this);
}

void
DoubleIndirectHeader::WriteBack(int sector)
{
    kernel->synchDisk->WriteSector(sector, (char *)this); 
}

int
DoubleIndirectHeader::ByteToSector(int offset, PersistentBitmap *freeMap)
{
    int ret;
    int indirectOff = offset / (NumData * SectorSize);
    int directOff = (offset % (NumData * SectorSize)) / SectorSize;
    IndirectHeader *idtmp = new IndirectHeader;
    if(dataHeaders[indirectOff] == -1)  // byte on new indirect node
    {
        dataHeaders[indirectOff] = freeMap->FindAndSet();
        idtmp->dataSectors[0] = freeMap->FindAndSet();
        freeMap->WriteBack(kernel->fileSystem->GetFreeMap());
    }
    else
    {
        kernel->synchDisk->ReadSector(dataHeaders[indirectOff], (char *)idtmp);
    }
    ret = idtmp->ByteToSector(directOff, freeMap);
    delete idtmp;
    return ret;
}

FileHeader::FileHeader()
{
    for(int i = 0; i < NumDirect; i++)
        dataSectors[i] = -1;
    for (int i=0; i < NumIndirect; i++)
        indirects[i] = -1;
    doubleIndirect = -1;
}

//----------------------------------------------------------------
// FileHeader::initalAlldata
// inital given size block of data, this should be called in allocation
//-----------------------------------------------------------------
void
FileHeader::initAllData(PersistentBitmap *freeMap, int num)
{
    for(int i =0; i < NumIndirect; i++)
        indirects[i] = -1;
    doubleIndirect = -1;
    // init direct data node;
    int counter = 0;
    for(int i = 0; i < NumDirect; i++)
    {
        if(counter >= num)
            return;
        dataSectors[i] = freeMap->FindAndSet();
        counter++;
    }
    // init indirect
    for(int i = 0; i < NumIndirect; i++)
    {
        IndirectHeader *newIndricts = new IndirectHeader;
        // allocate a sector for poniter, in order to keep it it must be flushed
        // to disk @REFECTOR here
        indirects[i] = freeMap->FindAndSet();  
        for(int j = 0; j < NumData; j++)
        {
            newIndricts->dataSectors[j] = freeMap->FindAndSet();
            counter++;
            if(counter >= num)
            {
                newIndricts->WriteBack(indirects[i]);
                delete newIndricts;
                return;
            }
        }
        newIndricts->WriteBack(indirects[i]);
        delete newIndricts;
    }
    // init double indirect
    doubleIndirect = freeMap->FindAndSet(); 
    DoubleIndirectHeader *newDoubleIndirectHeader = new DoubleIndirectHeader;
    for (int i = 0; i < NumData; i++)
    {
        IndirectHeader *newIndricts = new IndirectHeader;
        // allocate a sector for poniter, in order to keep it it must be flushed
        // to disk @REFECTOR here
        indirects[i] = freeMap->FindAndSet();  
        for(int j = 0; j < NumData; j++)
        {
            newIndricts->dataSectors[j] = freeMap->FindAndSet();
            counter++;
            if(counter >= num)
            {
                newIndricts->WriteBack(indirects[i]);
                newDoubleIndirectHeader->WriteBack(doubleIndirect);
                delete newIndricts;
                delete newDoubleIndirectHeader;
                return;
            }
        }
        newIndricts->WriteBack(indirects[i]);
        delete newIndricts;
    }
    newDoubleIndirectHeader->WriteBack(doubleIndirect);
    delete newDoubleIndirectHeader;
    return;
}

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------
bool
FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize)
{
    numBytes = fileSize;
    numSectors = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
        return FALSE; // not enough space

    this->initAllData(freeMap, numSectors);
    // since we checked that there was enough free space,
    // we expect this to succeed
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(PersistentBitmap *freeMap)
{
    for(int i = 0; i < NumDirect; i ++)
    {
        if(dataSectors[i] != -1)
        {
            ASSERT(freeMap->Test(dataSectors[i]))
            freeMap->Clear(dataSectors[i]);
        }
    }
    
    for(int i = 0; i < NumIndirect; i++)
    {
        if(indirects[i] != -1)
        {
            ASSERT(freeMap->Test(indirects[i]))
            IndirectHeader *tmp = new IndirectHeader;
            tmp->FetchFrom(indirects[i]);
            tmp->Deallocate(freeMap);
            freeMap->Clear(indirects[i]);
            delete tmp;
        }
    }
    
    if(doubleIndirect != -1)
    {
        ASSERT(freeMap->Test(doubleIndirect))
        DoubleIndirectHeader *tmp = new DoubleIndirectHeader;
        tmp->FetchFrom(doubleIndirect);
        tmp->Deallocate(freeMap);
        freeMap->Clear(doubleIndirect);
        delete tmp;
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    kernel->synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    kernel->synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
// if offset value is larger than file size, a new sector will be asigned
// meawhle all file header associate with it would be updated.
// However the new sector should no more than one, which means none-contigeous
// append of a file is not allowed!
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    int ret;
    ASSERT(offset <= (numSectors + 1)*SectorSize)
    if(offset > numBytes)
    {
        numSectors++;
        numBytes += offset % SectorSize;
    }
    else
    {
        return dataSectors[offset / SectorSize];
    }
    PersistentBitmap *freeMap = new PersistentBitmap(kernel->fileSystem->GetFreeMap(),NumSectors);
    if(offset < NumDirect*SectorSize)
    {
        if(dataSectors[offset / SectorSize] == -1)
        {
            dataSectors[offset / SectorSize] = freeMap->FindAndSet();
            freeMap->WriteBack(kernel->fileSystem->GetFreeMap());
        }
        delete freeMap;
        return(dataSectors[offset / SectorSize]);
    }
    int current = offset - NumDirect*SectorSize;
    if(current < NumIndirect * NumData * SectorSize)
    {
        int indirectOff =  current / (NumData * SectorSize);
        // read that indirect node form disk
        IndirectHeader *idtmp = new IndirectHeader;
        if(indirects[indirectOff] == -1)    // new byte is append on a new indirect node
        {
            indirects[indirectOff] = freeMap->FindAndSet();
            idtmp->dataSectors[0] = freeMap->FindAndSet();
            idtmp->WriteBack(indirects[indirectOff]);
            freeMap->WriteBack(kernel->fileSystem->GetFreeMap());
            delete idtmp;
            delete freeMap;
        }
        else    // no data appended on indirect node
        {   
            idtmp->FetchFrom(indirects[indirectOff]);
        }
        current = current % (NumData * SectorSize);
        int directOff = current / SectorSize;
        ret = idtmp->ByteToSector(directOff, freeMap);
        delete idtmp;
        delete freeMap;
        return ret;
    }
    else
    {
        current = current - NumIndirect * NumData * SectorSize;
        DoubleIndirectHeader *dtmp = new DoubleIndirectHeader;
        if(doubleIndirect = -1) // on new  double indirect node
        {
            doubleIndirect = freeMap->FindAndSet();
            dtmp->dataHeaders[0] = freeMap->FindAndSet();
            dtmp->WriteBack(doubleIndirect);
            IndirectHeader *idtmp = new IndirectHeader;
            idtmp->dataSectors[0] = freeMap->FindAndSet();
            idtmp->WriteBack(dtmp->dataHeaders[0]);
            freeMap->WriteBack(kernel->fileSystem->GetFreeMap());
            ret = idtmp->dataSectors[0];
            delete dtmp;
            delete idtmp;
            delete freeMap;
            return ret;
        }
        // kernel->synchDisk->ReadSector(doubleIndirect, (char*)&dtmp);
        // int indirectOff = current / (NumData * SectorSize);
        // IndirectHeader idtmp;
        // kernel->synchDisk->ReadSector(dtmp.dataHeaders[indirectOff], (char*)&idtmp);
        // current = current % (NumData * SectorSize);
        // int directOff = current / SectorSize;
        
        ret = dtmp->ByteToSector(current, freeMap);
        delete dtmp;
        delete freeMap;
        return ret;
    }
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
	printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
	kernel->synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		printf("%c", data[j]);
            else
		printf("\\%x", (unsigned char)data[j]);
	}
        printf("\n"); 
    }
    delete [] data;
}
