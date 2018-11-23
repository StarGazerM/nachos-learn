#include "kernel.h"
#include "main.h"
#include "thread.h"

void
SimpleThread(int which)
{
    //int num;
    
    // for (num = 0; num < 5; num++) {
    //     printf("*** thread %d looped %d times\n", which, num);
    //     kernel->currentThread->Yield();
    // }
    bool res = kernel->fileSystem->Create("test", 1024);
    if(res)
    {
        cout << "success\n";
    }
    else{
        cout << "fail\n";
    }
}

void
ThreadTest()
{
    Thread *t = new Thread("forked thread");
    t->Fork((VoidFunctionPtr) SimpleThread, (void *) 1);
    
    //SimpleThread(0);
}
