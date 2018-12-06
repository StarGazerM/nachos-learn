#include "kernel.h"
#include "main.h"
#include "thread.h"
#include "filehdr.h"

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

        // test read
        int i = 1;

        test->Seek(126);
        char into[10];
        test->Read(into, 4);
        into[0] = char(i+48);
        into[4] = '\0';
        cout<<into<<"\n";
    }
}

void
SyncCreate(int which)
{
    if(kernel->fileSystem->Create("test2", 128))
        {
            cout << "Create successfully. Start testing sync." << endl;
        }
}

void
SyncWriteTest(int which)
{   
    char writerNumber = char(which + 48);
    char testText[4];
    testText[0] = 'a';  
    testText[1] = 'b';
    testText[2] = 'c';
    testText[3] = writerNumber;
    OpenFile* test = kernel->fileSystem->Open("test2");
    test->Seek(1);
    test->Write(testText, 4);

    char into[10];
    test->Seek(1);
    test->Read(into, 4);
    into[4] = '\0';
    cout<<"Writer #"<<which<<" wrote "<<into<<"\n";
}

void
SyncReadTest(int which)
{   
    char intor[10];
    OpenFile* rtest = kernel->fileSystem->Open("test2");
    rtest->Seek(1);
    rtest->Read(intor, 4);
    //intor[0] = 'r';
    intor[4] = '\0';
    cout<<"Reader #"<<which<<" read "<<intor<<"\n";
}

void
ThreadTest()
{   
 
    Thread *t = new Thread("forked thread");
    t->Fork((VoidFunctionPtr) SyncCreate, (void *) 1);
    // Create 5 writer
    for(int i = 0; i < 5; i++){
        Thread *t = new Thread("forked thread");
        t->Fork((VoidFunctionPtr) SyncWriteTest, (void *) i);
    }
    // Create 10 reader
    for(int i = 0; i < 10; i++){
        Thread *t = new Thread("forked thread");
        t->Fork((VoidFunctionPtr) SyncReadTest, (void *) i);
    }


    // Thread *t1 = new Thread("forked thread");
    // t1->Fork((VoidFunctionPtr) TestCreate, (void *) 1);
    
    // SimpleThread(0);
}
