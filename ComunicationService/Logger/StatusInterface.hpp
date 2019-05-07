#ifndef STATUSFUNC_H
#define STATUSFUNC_H

#include <string>

class StatusInterface
{
public:
    virtual std::string getStatus()const=0;
};

#endif // STATUSFUNC_H
