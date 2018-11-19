#include "threadlist.h"

void ThreadWaitingList::InsertPair(int child_id, Thread *parent)
{
    // childlist[child->GetPid()] = child;
    waitinglist[child_id] = parent;
}

void ThreadWaitingList::DeletePair(int id)
{
    std::map<int, Thread*>::iterator wit = waitinglist.find(id);
    if(wit == waitinglist.end())
    {
        throw InvaildIDException(id);
    }
    if( (*wit).second == NULL)
    {
        throw NoParentWaitingException(id);
    }
    waitinglist.erase(wit);
}

Thread *ThreadWaitingList::GetParent(int id)
{
    auto wit = waitinglist.find(id);
    // if()
    // {
    //     throw InvaildIDException(id);
    // }
    if( wit == waitinglist.end() || (*wit).second == NULL)
    {
        throw NoParentWaitingException(id);
    }
    return (*wit).second;
}

bool ThreadWaitingList::CheckBlocking(Thread *parent)
{
    auto wit = waitinglist.begin();
    while(wit != waitinglist.end())
    {
        if((*wit).second == parent)
        {
            return true;
        }
        wit++;
    }
    return false;
}