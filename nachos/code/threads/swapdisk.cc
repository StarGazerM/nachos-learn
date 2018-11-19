#include "swapdisk.h"
#include "main.h"

SwapDisk::SwapDisk()
{
    bitmap = new Bitmap(SwapSize);
}

SwapDisk::~SwapDisk()
{
    delete bitmap;
}


int SwapDisk::AllocatePage(std::string content)
{
    // int n = bitmap->NumClear();
    int addr = bitmap->FindAndSet();
    ASSERT(addr != -1);
    swapMemory[addr] = content;
    return addr + SwapStartPageNum;
}

void SwapDisk::ClearAPage(int swapaddr)
{
    ASSERT(bitmap->Test(swapaddr-SwapStartPageNum))
    bitmap->Clear(swapaddr - SwapStartPageNum);
}

int SwapDisk::Remains()
{
    return bitmap->NumClear();
}

string SwapDisk::ReadAt(unsigned int swapaddr)
{
    ASSERT(bitmap->Test(swapaddr-SwapStartPageNum))
    //ASSERT(swapMemory[(swapaddr - SwapStartPageNum)] == "")
    string ret  = swapMemory[(swapaddr - SwapStartPageNum)];
    if(ret == "")
    {
        cout <<"here";
    }
    return ret;
}

void SwapDisk::WriteAt(unsigned int swapaddr, std::string content)
{
    // it an address is not allocated yet, it should not be written!!!
    // although it's owner haven't be check !
    // for security concern, it's associated PCB should be check later
    ASSERT(bitmap->Test(swapaddr-SwapStartPageNum));
    swapMemory[swapaddr-SwapStartPageNum] = content;
}

#ifdef FILESYS_STUB

bool SwapDisk::CopyFromFile(OpenFile *file, int numBytes, int position, std::list<int> &dest_addrs)
{
    int fileLength = file->Length();
    int firstPage, lastPage, numPages;
    // char *buf;

    if ((numBytes <= 0) || (position >= fileLength))
        return false; // check request
    if ((position + numBytes) > fileLength)
        numBytes = fileLength - position;

    firstPage = divRoundDown(position, PageSize);
    lastPage = divRoundDown(position + numBytes - 1, PageSize);
    numPages = 1 + lastPage - firstPage;

    int current = position;
    for (int i = 0; i < numPages; i++)
    {
        int target = dest_addrs.front();
        dest_addrs.pop_front();
        std::string tmp = "";
        char *bufPage = new char[PageSize];
        bzero(&bufPage[0], PageSize);
        file->ReadAt(&bufPage[0], PageSize, current);
        for(int j = 0; j < PageSize; j++)
        {
            tmp += bufPage[j];
        }
        WriteAt(target, tmp);
        current += PageSize;
    }

    // delete buf;
    return true;
}

#else

#include "synchdisk.h"
#include "filehdr.h"

bool SwapDisk::CopyFromFile(OpenFile *file, int numBytes, int position, const std::vector<int> &dest_addrs)
{
    int fileLength = file->Length();
    int firstPage, lastPage, numPages;
    char *buf;

    if ((numBytes <= 0) || (position >= fileLength))
        return false; // check request
    if ((position + numBytes) > fileLength)
        numBytes = fileLength - position;
    // DEBUG(dbgFile, "Reading " << numBytes << " bytes at " << position << " from file of length " << fileLength);

    firstPage = divRoundDown(position, PageSize);
    lastPage = divRoundDown(position + numBytes - 1, PageSize);
    numPages = 1 + lastPage - firstPage;

    // read in all the full and partial sectors that we need
    buf = new char[numPages * PageSize];
    for (int i = firstPage; i <= lastPage; i++)
    {
        FileHeader *fileheader = file->GetHeader();
        kernel->synchDisk->ReadSector(fileheader->ByteToSector(i * PageSize),
                                      &buf[(i - firstPage) * PageSize]);
    }

    // save what we read into SwapDisk
    if (numPages != dest_addrs.size())
    {
        cerr << "miss match between allocation and real space!! \n";
    }
    ASSERT(numPages != dest_addrs.size())
    std::string tmp(buf);
    for (int i = 0; i < dest_addrs.size(); i++)
    {
        // this is based on virtual address, suppose it is linear!
        WriteAt(dest_addrs[i], tmp.substr(i * PageSize, PageSize); // write one page a time
    }

    delete buf;
    return true;
}

#endif

void SwapDisk::Flush()
{
    // impl
    
}
