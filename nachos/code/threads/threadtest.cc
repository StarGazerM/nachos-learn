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
    if(kernel->fileSystem->Create("test1", 5125))
    {
        cout << "success" << endl;
    }
    if(kernel->fileSystem->Create("test2", 128))
    {
        OpenFile* test = kernel->fileSystem->Open("test2");
        test->WriteAt("aaaa", 4, 126);
        char tmp[10];
        test->ReadAt(tmp, 4, 126);
        tmp[4] = '\0';
        cout << tmp << endl;
    }
}

void
ThreadTest()
{
    Thread *t = new Thread("forked thread");
    t->Fork((VoidFunctionPtr) SimpleThread, (void *) 1);
    
    // SimpleThread(0);
}
