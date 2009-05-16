#include <vm/vm.h>

ExecEnv *getExecEnv()
{
       static ExecEnv ee;

       return &ee;
}
