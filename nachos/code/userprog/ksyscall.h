/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls 
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__
#define __USERPROG_KSYSCALL_H__

// #include "kernel.h"
#include "syscall.h"
#include "main.h"

int SysWrite(char *buffer, int size, OpenFileId id)
{
  WriteFile(id, buffer, size);
  return size;
}

OpenFileId SysOpen(char *name)
{
  return OpenForReadWrite(name, TRUE);
}

void SysHalt()
{
  kernel->interrupt->Halt();
}

int SysAdd(int op1, int op2)
{
  return op1 + op2;
}

////////////////////////////////////////

void Exit_POS(int id);

void ForkTest1(int id)
{
  printf("ForkTest1 is called, its PID is %d\n", id);
  for (int i = 0; i < 3; i++)
  {
    // printf("now in %s >>>\n", kernel->currentThread->getName());

    printf("ForkTest1 is in loop %d\n", i);
    for (int j = 0; j < 100; j++)
      kernel->interrupt->OneTick();
  }
  Exit_POS(id);
}

void ForkTest2(int id)
{
  // printf("now in %s >>>", kernel->currentThread->getName());
  printf("ForkTest2 is called, its PID is %d\n", id);
  for (int i = 0; i < 3; i++)
  {
    printf("ForkTest2 is in loop %d\n", i);
    for (int j = 0; j < 100; j++)
      kernel->interrupt->OneTick();
  }
  Exit_POS(id);
}

void ForkTest3(int id)
{
  // printf("now in %s >>>", kernel->currentThread->getName());
  printf("ForkTest3 is called, its PID is %d\n", id);
  for (int i = 0; i < 3; i++)
  {
    printf("ForkTest3 is in loop %d\n", i);
    for (int j = 0; j < 100; j++)
      kernel->interrupt->OneTick();
  }
  Exit_POS(id);
}

// exit child process and awake their parent
void Exit_POS(int id)
{
  IntStatus oldlevel = kernel->interrupt->SetLevel(IntOff);
  Thread *parent = NULL;
  try
  {
    parent = kernel->waitingList->GetParent(id);
    kernel->waitingList->DeletePair(id);
    if(!kernel->waitingList->CheckBlocking(parent))
    {
      kernel->scheduler->ReadyToRun(parent);
    }
    kernel->currentThread->Finish();
    kernel->interrupt->SetLevel(oldlevel);

  }
  catch (const std::exception &ne)
  {
    std::cerr << ne.what() << '\n';
    return;
  }
  
  // return;
}

int SysFork_POS(int i)
{
  VoidFunctionPtr func;
  switch (i)
  {
  case 1:
    func = (VoidFunctionPtr)ForkTest1;
    break;
  case 2:
    func = (VoidFunctionPtr)ForkTest2;
    break;
  case 3:
    func = (VoidFunctionPtr)ForkTest3;
    break;
  default:
    cout << "error!!!";
  }
  Thread *threadtest = new Thread(kernel->currentThread->getName());
  int tid = threadtest->GetPid();
  threadtest->Fork(func, (void *)tid);
  return tid;
}

/*****************************************************
 * SysWait_POS
 *  **THROWABLE!!!!!!!!!!**
 * work as unix system call wait pid, if the child process
 * exist, it will block until child  finish
 * if thread not exit an InvaildIDException  will
 * be arouse
*/
void SysWait_POS(int child_id)
{
  // check whether thread alive
  IntStatus oldlevel = kernel->interrupt->SetLevel(IntOff);

  if (!kernel->scheduler->CheckThreadAlive(child_id))
  {
    throw InvaildIDException(child_id);
  }

  kernel->waitingList->InsertPair(child_id, kernel->currentThread);

  kernel->currentThread->Sleep(FALSE);
  kernel->interrupt->SetLevel(oldlevel);
}

#endif /* ! __USERPROG_KSYSCALL_H__ */
