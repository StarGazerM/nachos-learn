// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions
//	is in machine.h.
//----------------------------------------------------------------------

void ExceptionHandler(ExceptionType which)
{
	int type = kernel->machine->ReadRegister(2);

	DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");
	IntStatus old = kernel->interrupt->SetLevel(IntOff);

	switch (which)
	{
	case SyscallException:
	{
		switch (type)
		{
		case SC_Exit:
		{
			// Exit_POS(kernel->currentThread->GetPid());
			/* Modify return point */
			int res = (int)kernel->machine->ReadRegister(4);
			cout << "Thread " << kernel->currentThread->GetPid() << " exit with " << res << "\n";
			Thread *oldThread = kernel->currentThread;
			oldThread->Finish();
			kernel->machine->WriteRegister(2, 0);
			kernel->interrupt->SetLevel(old);
			break;
		}

		case SC_Halt:
		{
			DEBUG(dbgSys, "Shutdown, initiated by user program.\n");

			SysHalt();

			ASSERTNOTREACHED();
		}
		break;
		case SC_Add:
		{
			DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");

			/* Process SysAdd Systemcall*/
			int result;
			result = SysAdd(/* int op1 */ (int)kernel->machine->ReadRegister(4),
							/* int op2 */ (int)kernel->machine->ReadRegister(5));

			DEBUG(dbgSys, "Add returning with " << result << "\n");
			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)result);

			kernel->interrupt->SetLevel(old);
			break;
		}

		case SC_Open:
		{
			int nbuf_addr = (int)kernel->machine->ReadRegister(4);
			string name = "";
			int tmp = -1;
			OpenFileId fid = 0;
			while ((int)tmp != 0)
			{
				if (!kernel->machine->ReadMem(nbuf_addr, sizeof(char), &tmp))
				{
					cerr << "valid memory access!!"
						 << "/n";
					fid = -1;
					break;
				}
				nbuf_addr += sizeof(char);
				name += (char)tmp;
			}
			fid = SysOpen(const_cast<char *>(name.c_str()));
			kernel->machine->WriteRegister(2, fid);
			cout << name << "\n";
			
			kernel->interrupt->SetLevel(old);
			break;
		}
		case SC_Write:
		{
			int nbuf_addr = (int)kernel->machine->ReadRegister(4);
			int size = (int)kernel->machine->ReadRegister(5);
			int openfid = (int)kernel->machine->ReadRegister(6);
			// assume the buffer here is not very large , read at one time
			string buf = "";
			int res = 0;
			for (int i = 0; i < size; i++)
			{
				int temp;
				if (!kernel->machine->ReadMem(nbuf_addr, sizeof(char), &temp))
				{
					cerr << "valid memory access!!"
						 << "/n";
					res = -1;
					break;
				}
				nbuf_addr += sizeof(char);
				buf += (char)temp;
			}
			//char* m = buf.c_str();
			res = SysWrite(const_cast<char *>(buf.c_str()), size, openfid);
			kernel->machine->WriteRegister(2, res);
			// cout << buf << endl;

			kernel->interrupt->SetLevel(old);
			break;
			ASSERTNOTREACHED();
		}
		case SC_Fork_POS:
		{
			int i = (int)kernel->machine->ReadRegister(4);
			int tid = SysFork_POS(i);

			// kernel->waitingList->InsertPair(tid, kernel->currentThread);

			kernel->machine->WriteRegister(2, tid);

			kernel->interrupt->SetLevel(old);
		}
		break;
		case SC_Wait_POS:
		{
			int child_id = (int)kernel->machine->ReadRegister(4);

			// cout << "evoke wait " << child_id << endl;
			try
			{
				SysWait_POS(child_id);
			}
			catch (const std::exception &e)
			{
				std::cerr << e.what() << '\n';
			}

			kernel->interrupt->SetLevel(old);

			break;
			ASSERTNOTREACHED();
		}
		case SC_ThreadYield:
		{
			cout << kernel->currentThread->GetPid() << "\n";
		}
		break;
		default:
		{
			cerr << "Unexpected system call " << type << "\n";
			kernel->interrupt->SetLevel(old);

			break;
		}
		}
		break;
	}
	case PageFaultException:
	{
		old = kernel->interrupt->SetLevel(IntOff);
		// handle page fault here
		int vaddr = kernel->machine->ReadRegister(BadVAddrReg);
		AddrSpace *currentPcb = kernel->currentThread->space; // current pcb
		unsigned int faultPage;
		ExceptionType ex = currentPcb->Translate(vaddr, &faultPage, FALSE);
		// ASSERT(ex == NoException)
		// unsigned int faultPage = phyaddr / PageSize;
		// cout << "fault page is " << faultPage << endl;
		int toBeReplaced;
		bool samePcbFlag = false;
		if (faultPage >= 128 && faultPage < 256) // page is in swap, replace page here
		{
			// read original page out
			std::string faultPageContent = kernel->swapdisk->ReadAt(faultPage);
			// kernel->swapdisk->ClearAPage(faultPage);	
			if (kernel->memMap->NumClear() > 0)
			{
				toBeReplaced = kernel->memMap->FindAndSet();
			}
			else
			{
				// ASSERT(!kernel->phyPageList.empty());
				auto head = (kernel->phyPageList).begin();
				toBeReplaced = (*head).first;
				AddrSpace *prevPcb = (*head).second;
				kernel->phyPageList.pop_front();
				if(currentPcb == prevPcb)
				{
					samePcbFlag = true;
				}
				// read the block need to be swap from main memory
				std::string content = "";
				for (int i = 0; i < PageSize; i++)
				{
					content += kernel->machine->mainMemory[(toBeReplaced*PageSize)+i];
				}
				// write a physical memory into swap!
				// int swappage = kernel->swapdisk->AllocatePage(content);
				kernel->swapdisk->WriteAt(faultPage, content);
				if(!samePcbFlag) // if in different pcb change booth
				{
					prevPcb->ModifyPTE(toBeReplaced, faultPage);
				}
			}
			// modify page table in current thread to make it translatable
			// AddrSpace *currentPcb = kernel->currentThread->space;
			if(!samePcbFlag)
			{
				currentPcb->ModifyPTE(faultPage, toBeReplaced);
			}
			else	// in same pcb olny once need to be changed!
			{
				currentPcb->SwitchPTE(toBeReplaced, faultPage);
			}
			kernel->phyPageList.push_back(make_pair(toBeReplaced, currentPcb));

			// write fault page to memory
			auto it = faultPageContent.begin();
			int i = 0;
			while (it != faultPageContent.end())
			{
				kernel->machine->mainMemory[(toBeReplaced*PageSize)+i] = (*it);
				i++;
				it++;
			}
		}
		kernel->interrupt->SetLevel(old);
		return;
	}
	case AddressErrorException:
	{
		// it may happen is one program itself is too big, vm may larger than mem size
		// this condition is actual will not effect translation result..... it is just
		// catched here, no need for handle!
		// OutOfMemory's condition is handled when program is loaded, in current nachos
		// heap hasn't realized and stack is fixed, which means we do not need to worry
		// about OOM in runtime.

		return;
	}
	default:
	{
		cerr << "Unexpected user mode exception" << (int)which << "\n";
		break;
	}
	}
	/* Modify return point */
	{
		/* set previous programm counter (debugging only)*/
		kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

		/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
		kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

		/* set next programm counter for brach execution */
		kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
	}
	kernel->interrupt->SetLevel(old);
}
