#include "kernel.h"
#include "main.h"
#include "thread.h"
#include "filehdr.h"
#include <sstream>

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
    cout << "writting below will on indirect and double indirect node, which has exceed the writing limit of original file" << endl;
    file1->AppendOneSector(&(oneSector[0]), 128);
    cout << "start test writing f1----------------" << endl;

    cout << "start test writing f2----------------" << endl;
    OpenFile * file2 = kernel->fileSystem->Open("f2");
    for(int i = 0; i < 20; i++)
    {
        file2->AppendOneSector(&(oneSector[0]), 128);
    }

    char tmp[18]; // read tmp
    cout << "start test reading f1 tail----------------" << endl;
    file1->ReadAt(tmp,16, 2560);
    tmp[16] = '\0';
    cout << tmp << endl;
    checkSegment();

    cout << "start test reading f2 tail----------------" << endl;
    file2->ReadAt(tmp,16, 2304);
    tmp[16] = '\0';
    cout << tmp << endl;
    checkSegment();

    cout << "test creating more than 10 file, which can be hold by original nachos dictornay" << endl;
    for(int i = 3; i < 12; i++)
    {
        char* ntmp = new char[8]; 
        sprintf(ntmp, "f%d", i);
        if(kernel->fileSystem->Create(ntmp))
        {
            // OpenFile * f = kernel->fileSystem->Open(ntmp);
            // f->AppendOneSector(&(oneSector[0]), 128);
            cout << "create " << ntmp << " success" << endl;
        }
    }

    cout << "delete file 2----------------" << endl;
    kernel->fileSystem->Remove("f2");

    cout << "test clean segment-----------------" << endl;
    kernel->fileSystem->CleanSegments(); // for test inllustration, I manually call
                                        // this, in kernel, there is a daemon process
                                        // will clean it when clean segment size drop
                                        // bellow 20.
    cout << "block in file2 should be cleaned" << endl;
    checkSegment();

    cout << "start test reading f1 middle----------------" << endl;
    file1->ReadAt(tmp,16, 128);
    tmp[16] = '\0';
    cout << tmp << endl;
    cout << "test finish, shutdown machine, I/O stat will shows cache is working!" << endl;
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

#else

void
TestThreadX(int which)
{
    //----------------------------------
    // test case
    // Part 1 Create file at any size, use direct, indirect, double indirect first block
    //------------------------------------
    //in order to meet 128 MB file size, change disk sector size from 128 to 256.
    //since exsitence of directory file header and freeMap file header and file header, the 
    //largest file size cannot meet excat 1024*128.
    if(kernel->fileSystem->Create("maxFile",128*1024)){
        //do not print bitmap in here
        //change disk size to 256
        cout<<"\nmaxFile file ith a file size of 128*1024 has been created successfully"<< endl;
        OpenFile* test = kernel->fileSystem->Open("maxFile");
        if(kernel->fileSystem->Remove("maxFile"))
        {
            cout<<"maxFile has been removed successfully.\n" <<endl;
        }
    }
    else {
        cout<< "file not create"<<endl;
    }
    if(kernel->fileSystem->Create("test0",256*15)){
        
        cout<<"\ntest1 file ith a file size of 256*31 has been created successfully"<< endl;
        OpenFile* test0 = kernel->fileSystem->Open("test0");
        test0->Seek(255);
        test0->Write("Case11234567890123456789012345678901234567890", 45);
        
        char buffer[45];
        test0->Seek(255);
        test0->Read(buffer, 45);
        
        buffer[45] = '\0';
        cout<<"File contents: "<< buffer << endl;
        //kernel->fileSystem->Print();
        if(kernel->fileSystem->Remove("test0"))
        {
            cout<<"test1 has been removed successfully.\n" <<endl;
        }
    }
    if(kernel->fileSystem->Create("test1",256*31)){
        
        cout<<"\ntest1 file ith a file size of 256*31 has been created successfully"<< endl;
        OpenFile* test1 = kernel->fileSystem->Open("test1");
        test1->Seek(255);
        test1->Write("Case11234567890123456789012345678901234567890", 45);
      
        char buffer[45];
        test1->Seek(255);
        test1->Read(buffer, 45);
       
        buffer[45] = '\0';
        cout<<"File contents: "<< buffer << endl;
        //kernel->fileSystem->Print();
        if(kernel->fileSystem->Remove("test1"))
        {
            cout<<"test1 has been removed successfully.\n" <<endl;
        }
    }
    if(kernel->fileSystem->Create("test2",256*53)){
        //BITMAP PRINTS 0 TO 60 BLOCK
        cout<<"\ntest 2 file ith a file size of 256*153 has been created successfully"<< endl;
        OpenFile* test2 = kernel->fileSystem->Open("test2");
        cout<<"first four bitmap is occupied by directory and freeMap.\n"<<endl;
        //FILE BLOCK 2 is bit map file header
        //FILE BLOCK 3 ARE DIRECTORY FILE HEADER
        //SECTOR 5 IS DIRECTORY CONTENT
        //file block 5 file header(unconfirm)
        //FILE BLOCK 6 TO 60
        // kernel->fileSystem->Print();
        test2->Seek(255);
        test2->Write("Case21234567890123456789012345678901234567890", 45);
        char buffer[45];
        test2->Seek(255);
        test2->Read(buffer, 45);
        buffer[45] = '\0';
        cout<<"File contents: "<< buffer << endl;
        
        if(kernel->fileSystem->Remove("test2"))
        {
            cout<<"test 2 has been removed successfully.\n" <<endl;
        }
    }
    if(kernel->fileSystem->Create("test3",256*308)){
        
        cout<<"\ntest 4 file ith a file size of 256*308 has been created successfully"<< endl;
        OpenFile* test3 = kernel->fileSystem->Open("test3");
        test3->Seek(255);
        test3->Write("Case3", 5);
        char buffer[5];
        test3->Seek(255);
        test3->Read(buffer, 5);
        buffer[5] = '\0';
        cout<<"File contents: "<< buffer << endl;
        if(kernel->fileSystem->Remove("test3"))
        {
            cout<<"test 3 has been removed successfully.\n" <<endl;
        }
    }
    //Maximum file size in sector size 256
    if(kernel->fileSystem->Create("test4",256*1012)){
        cout<<"Maximum file size for sector size 256 bytes\n";
        cout<<"\ntest 4 file, with a file size of 256*1012, that occupies every free disk has been created successfully"<< endl;
        OpenFile* test4 = kernel->fileSystem->Open("test4");
        
        test4->Seek(255);
        test4->Write("Case4", 5);
        char buffer[5];
        test4->Seek(255);
        
        test4->Read(buffer, 5);
        buffer[5] = '\0';
        cout<<"File contents: "<< buffer << endl;
        if(kernel->fileSystem->Remove("test4"))
        {
            cout<<"test 4 has been removed successfully.\n" <<endl;
        }
    }
    else
    {
        cout<<"Test file 4 cannot create"<<endl;
    }

}

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
#endif


void
ThreadTest()
{   
 
 #ifndef LOG_FS
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
#else
    Thread *t1 = new Thread("forked thread");
    t1->Fork((VoidFunctionPtr) TestCreate, (void *) 1);
#endif
    // SimpleThread(0);
}
