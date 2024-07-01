#include "syscalls.h"

using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;

void printf(char*);

void print_integer(int num);

int getEAXValue();


uint32_t SyscallHandler::HandleInterrupt(uint32_t esp) {

    CPUState* cpu = (CPUState*)esp;
    
    common::uint32_t entrypoint = 0;

    switch(cpu->eax) {
        
        case SYS_FORK:
            taskManager->fork(cpu); 
            break;

        case SYS_DOUBLE_RETURN:
            cpu->ebx = taskManager->getEaxVal();
            break;
        
        case SYS_GETPID:
            cpu->ecx = taskManager->getPID();
            break;
        
        case SYS_PRINT_TABLE:
            taskManager->printProcessTable();
            break;

        case SYS_WAITPID:
            taskManager->waitpid(cpu->ebx);
            break;
            
        case SYS_EXIT:
            taskManager->exit();
            break;

        case SYS_EXECVE:
            entrypoint = cpu->ebx;
            esp = taskManager->execve((void (*)())entrypoint);
            break;

        case SYS_SET_PRIORITY:
            taskManager->setCurrenttaskPriority(cpu->ebx);
            break;

        case SYS_GET_PRIORITY:
            cpu->edx = taskManager->getPriority();
            break;

        case SYS_INTRPT_COUNT:
            cpu->edx = taskManager->getIntrptCount();
            break;

        case SYS_ADD_AGING:
            taskManager->setAgingPid(cpu->ebx);
            break;

        case SYS_SET_STATE:
            if(cpu->ebx == 0)
            taskManager->setCurrenttaskState(Ready);
            if(cpu->ebx == 2)
            taskManager->setCurrenttaskState(Blocked);
            break;

        default:
            break;
    }

    //print_integer(cpu->eax);
    
    return esp;
}
