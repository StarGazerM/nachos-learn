#include "kernel.h"
#include "main.h"
#include "thread.h"
#include "filehdr.h"

void
TestCreate(int which)
{
    // int num;
    
    // for (num = 0; num < 5; num++) {
    //     printf("*** thread %d looped %d times\n", which, num);
    //     kernel->currentThread->Yield();
    // }
    if(kernel->fileSystem->Create("f1"))
    {
        cout << "success" << endl;
    }
    OpenFile * file = kernel->fileSystem->Open("f1");
    // file->AppendOneSector("aaa", 3);
    array<char, 128> oneSector;
    oneSector.fill('a');
    for(int i = 0; i < 20; i++)
    {
        file->AppendOneSector(&(oneSector[0]), 128);
    }
    file->AppendOneSector(&(oneSector[0]), 128);
    // file->AppendOneSector(&(oneSector[0]), 128);

    char tmp[18];
    file->ReadAt(tmp,16, 2560);
    tmp[16] = '\0';
    cout << tmp << endl;
    kernel->interrupt->Halt();
}
void
TestRestore(int ww)
{
    OpenFile * file = kernel->fileSystem->Open("f1");
    char tmp[18];
    file->ReadAt(tmp,16, 0);
    tmp[16] = '\0';
    cout << tmp << endl;
    kernel->interrupt->Halt();
}

void
ThreadTest()
{
    Thread *t = new Thread("forked thread");
    t->Fork((VoidFunctionPtr)TestRestore, (void *) 1);
    
    // SimpleThread(0);
}
