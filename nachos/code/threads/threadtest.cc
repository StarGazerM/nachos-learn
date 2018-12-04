#include "kernel.h"
#include "main.h"
#include "thread.h"
#include "filehdr.h"

void
SimpleThread(int which)
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
    file->AppendOneSector("aaa", 3);
    char tmp[4];
    file->ReadAt(tmp,2, 0);
    cout << tmp << endl;
}

void
ThreadTest()
{
    Thread *t = new Thread("forked thread");
    t->Fork((VoidFunctionPtr) SimpleThread, (void *) 1);
    
    // SimpleThread(0);
}
