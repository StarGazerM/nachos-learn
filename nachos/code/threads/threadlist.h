#include "thread.h"
#include <sstream>
#include <map>
#include <vector>

class InvaildIDException : public std::exception
{
    private:
        int tid;
        std::string msg;
    public:
        InvaildIDException(int id){ 
            tid = id;
            std::stringstream s;
            s << "Access an invaild child thread id " << tid;
            msg = s.str();
        }
        const char* what() const throw()
        {    
            return msg.c_str();
        }
};

class NoParentWaitingException: public std::exception
{
    private:
        int tid;
        std::string msg;
    public:
        NoParentWaitingException(int id){ 
            tid = id;
            std::stringstream s;
            s << "child thread id " << tid << " has no parent waiting";
            msg = s.str();
        }
        const char* what() const throw()
        {
            return msg.c_str();
        }
};

class ThreadWaitingList
{
  private:
    std::map<int, Thread *> childlist;  // all alive thread
    std::map<int, Thread *> waitinglist; // key is some thread id 
                                              // value is parent's ptr
  public:
    Thread *GetParent(int  childid);        // an InvaildIDException will be thrown here
    void DeletePair(int child_id);
    void InsertPair(int child_id, Thread *parent);
    bool CheckBlocking(Thread *parent);
};