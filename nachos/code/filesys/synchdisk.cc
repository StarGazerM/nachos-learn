// synchdisk.cc 
//	Routines to synchronously access the disk.  The physical disk 
//	is an asynchronous device (disk requests return immediately, and
//	an interrupt happens later on).  This is a layer on top of
//	the disk providing a synchronous interface (requests wait until
//	the request completes).
//
//	Use a semaphore to synchronize the interrupt handlers with the
//	pending requests.  And, because the physical disk can only
//	handle one operation at a time, use a lock to enforce mutual
//	exclusion.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synchdisk.h"
#include "cache.h"
#include "main.h"

//----------------------------------------------------------------------
// SynchDisk::SynchDisk
// 	Initialize the synchronous interface to the physical disk, in turn
//	initializing the physical disk.
//
//----------------------------------------------------------------------

SynchDisk::SynchDisk()
{
    semaphore = new Semaphore("synch disk", 0);
    lock = new Lock("synch disk lock");
    disk = new Disk(this);
}

//----------------------------------------------------------------------
// SynchDisk::~SynchDisk
// 	De-allocate data structures needed for the synchronous disk
//	abstraction.
//----------------------------------------------------------------------

SynchDisk::~SynchDisk()
{
    delete disk;
    delete lock;
    delete semaphore;
}

//----------------------------------------------------------------------
// SynchDisk::ReadSector
// 	Read the contents of a disk sector into a buffer.  Return only
//	after the data has been read.
//
//	"sectorNumber" -- the disk sector to read
//	"data" -- the buffer to hold the contents of the disk sector
//----------------------------------------------------------------------

void
SynchDisk::ReadSector(int sectorNumber, char* data)
{
    lock->Acquire();			// only one disk I/O at a time
    disk->ReadRequest(sectorNumber, data);
    semaphore->P();			// wait for interrupt
    lock->Release();
}

//----------------------------------------------------------------------
// SynchDisk::WriteSector
// 	Write the contents of a buffer into a disk sector.  Return only
//	after the data has been written.
//
//	"sectorNumber" -- the disk sector to be written
//	"data" -- the new contents of the disk sector
//----------------------------------------------------------------------

void
SynchDisk::WriteSector(int sectorNumber, char* data)
{
    lock->Acquire();			// only one disk I/O at a time
    disk->WriteRequest(sectorNumber, data);
    semaphore->P();			// wait for interrupt
    lock->Release();
}

//----------------------------------------------------------------------
// SynchDisk::CallBack
// 	Disk interrupt handler.  Wake up any thread waiting for the disk
//	request to finish.
//----------------------------------------------------------------------

void
SynchDisk::CallBack()
{ 
    semaphore->V();
}

IDiskDecorator::~IDiskDecorator()
{
    delete _disk;
}

WithLogCache::WithLogCache(IDisk *disk):IDiskDecorator(disk)
{
    write_cache = new LogCache;
    read_cache = new ReadCache;
}

void WithLogCache::ReadSector(int sectorNumber, char* data)
{
    try
    {
        read_cache->Read(sectorNumber, data);
    }
    catch(const std::out_of_range& e)
    {
        // handle cache miss here, dirctely read from disk
        // and the add data to cache
        IDiskDecorator::ReadSector(sectorNumber, data);
        read_cache->UpdateOrAdd(sectorNumber, data);
    }
    
}

void WithLogCache::WriteSector(int sectorNumber, char* data)
{
    // TODO: plz check whether file name is realy useful.....
    char* name = kernel->currentThread->GetCurrentFD()->GetFileName();
    std::hash<std::string> hash_fn;
    int nameHash = hash_fn(std::string(name));
    write_cache->Append(sectorNumber, data, nameHash, _disk);
    read_cache->UpdateOrAdd(sectorNumber, data);
}

WithLogCache::~WithLogCache()
{
    delete  write_cache;
    delete read_cache;
}
