// SwapDisk.h
// this is a simulation of a hard disk in nachos, it is extension of main memory!
// but this swap disk is not in nachos's file system, it is in **host**
// physical memory. It mapped to physical page number 128~256.(virtual memory in
// windows, swap in linux). 
// it will work as /swap partition in *NIX operation system, when user program 
// try to access physical memory 128 ~ 256 page, it will read from here 
// this ram disk can also support fulsh back into nachos disk with a serialized
// swaped page entry(virtual page number and memeroy content)
// (in this case I will use normal format stringfy to do serialzation with
// stl's sstream, if boost is possile, plz change to that).

#ifndef RAM_DISK_H
#define RAM_DISK_H

#include <map>
#include <sstream>
#include <array>
#include <list>
#include "bitmap.h"
#include "machine.h"
#include "openfile.h"

using SwapedPageEnty = std::map<int, std::string>;

const int SwapSize = 128;             // max page number
const int SwapStartPageNum = 128;     // Swap's mapped address in memory start from the 
                                      // end of main memory,

// class OpenFile;

class SwapDisk
{
  private:
    std::array<std::string, SwapSize> swapMemory;
    Bitmap* bitmap;

  public:
    SwapDisk();
    ~SwapDisk();
    int AllocatePage(std::string content);
    // char* SwitchPage(int swapaddr, char *content);
    string ReadAt(unsigned int swapaddr);
    void WriteAt(unsigned int swapaddr, std::string);
    void ClearAPage(int swapaddr);

    bool CopyFromFile(OpenFile *file, int numBytes, int position, std::list<int> &dest_addrs);
                                      // copy the content of a file to some address
                                      // the address value in dest_addrs should be valid!
                                      // REFECTOR: some exception need to be thrown here
    // void Load();
    void Flush();
    int Remains();
};

#endif