#ifndef __MYOS__SYSCALLS_H
#define __MYOS__SYSCALLS_H

#include <common/types.h>  // uint8_t için gerekli olan dosya
#include <hardwarecommunication/interrupts.h>
#include <multitasking.h>

#define SYS_FORK 2  
#define SYS_GETPID 3
#define SYS_PRINT_TABLE 4
#define SYS_DOUBLE_RETURN 5
#define SYS_WAITPID 6
#define SYS_EXIT 7
#define SYS_EXECVE 8
#define SYS_SET_PRIORITY 9
#define SYS_INTRPT_COUNT 10
#define SYS_ADD_AGING 11
#define SYS_GET_PRIORITY 12
#define SYS_SET_STATE 13


namespace myos {
    
    class SyscallHandler : public hardwarecommunication::InterruptHandler {
        TaskManager* taskManager;  // TaskManager referansı

    public:
        SyscallHandler(hardwarecommunication::InterruptManager* interruptManager, myos::common::uint8_t InterruptNumber, TaskManager* tm)
        : InterruptHandler(interruptManager, InterruptNumber + interruptManager->HardwareInterruptOffset()),
          taskManager(tm) {}
        
        virtual myos::common::uint32_t HandleInterrupt(myos::common::uint32_t esp);
    };


}

#endif
