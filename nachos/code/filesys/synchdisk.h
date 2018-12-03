// synchdisk.h 
// 	Data structures to export a synchronous interface to the raw 
//	disk device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef SYNCHDISK_H
#define SYNCHDISK_H

#include "disk.h"
#include "synch.h"
#include "callback.h"

class LogCache;
class ReadCache;
class Semaphore;
class Lock;
// for different kind of file system, the ADT interface of disk 
// operation will be different, In order to make current design 
// more flexable, some design  need to be made on ......
// TODO:
// extract these code to an new file, and make child dictory for 
// different file system!
class IDisk : public CallBackObj
{
  public:
    virtual void ReadSector(int sectorNumber, char* data) = 0; 
                        // Read/write a disk sector, returning
    					// only once the data is actually read 
					    // or written.  These call
    					// Disk::ReadRequest/WriteRequest and
					    // then wait until the request is done.
    virtual void WriteSector(int sectorNumber, char* data) = 0;
    virtual ~IDisk(){};

//   protected:
//     Disk *disk; // Raw disk device
};

// The following class defines a "synchronous" disk abstraction.
// As with other I/O devices, the raw physical disk is an asynchronous device --
// requests to read or write portions of the disk return immediately,
// and an interrupt occurs later to signal that the operation completed.
// (Also, the physical characteristics of the disk device assume that
// only one operation can be requested at a time).
//
// This class provides the abstraction that for any individual thread
// making a request, it waits around until the operation finishes before
// returning.

class SynchDisk : public IDisk {
  public:
    SynchDisk();    		        // Initialize a synchronous disk,
					// by initializing the raw Disk.
    ~SynchDisk();			// De-allocate the synch disk data
    
    void ReadSector(int sectorNumber, char* data);
    					// Read/write a disk sector, returning
    					// only once the data is actually read 
					// or written.  These call
    					// Disk::ReadRequest/WriteRequest and
					// then wait until the request is done.
    void WriteSector(int sectorNumber, char* data);
    
    void CallBack();			// Called by the disk device interrupt
					// handler, to signal that the
					// current disk operation is complete.

  private:
    Disk *disk;		  		// Raw disk device
    Semaphore *semaphore; 		// To synchronize requesting thread 
					// with the interrupt handler
    Lock *lock;		  		// Only one read/write request
					// can be sent to the disk at a time
};

// This is the base class for decorator, it will be useful if you 
// just want to add some feature to exist interface
// the decorator in cpp is not so elegent, but we have to
// cause we don't have AOP support in STL
class IDiskDecorator :  public IDisk
{
  protected:
    IDisk *_disk;
  public: 
    IDiskDecorator(IDisk *disk) : _disk(disk){}
    virtual void ReadSector(int sectorNumber, char* data)
    {
        _disk->ReadSector(sectorNumber, data);
    }
    virtual void WriteSector(int sectorNumber, char* data)
    {
        _disk->WriteSector(sectorNumber, data);
    }
    virtual void CallBack()
    {
        _disk->CallBack();
    }
    virtual ~IDiskDecorator();
};

// log cache decorator
// with this we can make a Disk working with cache
class WithLogCache : public IDiskDecorator
{
  private:
    LogCache *write_cache;
    ReadCache *read_cache;
  public:
    WithLogCache(IDisk *disk);
    void ReadSector(int sectorNumber, char* data);
    void WriteSector(int sectorNumber, char* data);
    // void CallBack();
    // some other disk associated operation
    // casue now all of our file header need to be
    // recorded in seprerated space on disk, this 
    // can be done in this decorator
    int AddFileHdr(FileHeader* hdr);
    void Flush();    // flush the cache to physical disk
    ~WithLogCache();
};

#endif // SYNCHDISK_H
