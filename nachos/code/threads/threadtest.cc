#include "kernel.h"
#include "main.h"
#include "thread.h"
#include "filehdr.h"
// #include <fstream>

#ifdef LOG_FS
void checkSegment()
{
    int dirty = 0;
    for(DiskSegment* seg : kernel->fileSystem->segTable)
    {
        if(!seg->IsClean())
        {
            dirty ++;
        }
    }
    printf("%d segment is dirty in DISK_0\n", dirty);
}

void
TestCreate(int which)
{
    if(kernel->fileSystem->Create("f1"))
    {
        cout << "create f1 success" << endl;
    }
    else
    {
        cout << "open f1 failed" << endl;
        return;
    }
    if(kernel->fileSystem->Create("f2"))
    {
        cout << "create f2 success" << endl;
    }
    else
    {
        cout << "open f2 failed" << endl;
        return;
    }
    OpenFile * file1 = kernel->fileSystem->Open("f1");
    checkSegment();
    cout << "start test writing f1----------------" << endl;
    array<char, 128> oneSector;
    oneSector.fill('a');
    for(int i = 0; i < 20; i++)
    {
        file1->AppendOneSector(&(oneSector[0]), 128);
    }
    file1->AppendOneSector(&(oneSector[0]), 128);

    cout << "start test writing f2----------------" << endl;
    OpenFile * file2 = kernel->fileSystem->Open("f2");
    for(int i = 0; i < 20; i++)
    {
        file2->AppendOneSector(&(oneSector[0]), 128);
    }

    cout << "start test reading f2----------------" << endl;
    char tmp[18];
    file1->ReadAt(tmp,16, 2560);
    tmp[16] = '\0';
    cout << tmp << endl;
    checkSegment();

    cout << "delete file 2----------------" << endl;
    kernel->fileSystem->Remove("f2");

    cout << "test clean segment-----------------" << endl;
    kernel->fileSystem->CleanSegments(); // for test inllustration, I manually call
                                        // this, in kernel, there is a daemon process
                                        // will clean it when clean segment size drop
                                        // bellow 20.
    checkSegment();
    kernel->interrupt->Halt();
}
void
TestRestore(int ww)
{
    OpenFile * file = kernel->fileSystem->Open("f1");
    if(file == NULL)
    {
        cout << "file not exsist!" << endl;
        return;
    }
    char tmp[18];
    file->ReadAt(tmp,16, 0);
    tmp[16] = '\0';
    cout << tmp << endl;
    kernel->interrupt->Halt();
}

#endif
void
ThreadTest()
{
    Thread *t = new Thread("forked thread");

    // t->Fork((VoidFunctionPtr)TestCreate, (void *) 1);
    
    // SimpleThread(0);
}
