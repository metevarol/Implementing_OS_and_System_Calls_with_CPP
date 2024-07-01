#ifndef __MYOS__MULTITASKING_H
#define __MYOS__MULTITASKING_H

#include <common/types.h>
#include <gdt.h>

namespace myos {

    enum State {Ready, Running, Blocked, Terminated};

    struct CPUState {
        common::uint32_t eax;
        common::uint32_t ebx;
        common::uint32_t ecx;
        common::uint32_t edx;
        common::uint32_t esi;
        common::uint32_t edi;
        common::uint32_t ebp;
        common::uint32_t error;
        common::uint32_t eip;
        common::uint32_t cs;
        common::uint32_t eflags;
        common::uint32_t esp;
        common::uint32_t ss;        
    } __attribute__((packed));


    //******************************* TASK (PROCESS) ********************************************

    class Task {

    friend class TaskManager;
    
    private:
        
        CPUState* cpustate;                                             // CPU_STATE
        common::uint8_t stack[4096];                                    // STACK
        int eax_value;                                                  // EAX_VALUE

        common::uint32_t pid;                                           // PID
        common::uint32_t ppid;                                          // P_PID
        int priority;                                      // PRIORITY
        State state;                                                    // STATE
                                                       
        common::uint32_t waitingProcess = -1;                                // WAITING_PID


    //________________________________________________________________________________

    public:
        

        Task(GlobalDescriptorTable *gdt, void entrypoint());   // CONSTRUCTOR

        Task();                                                         // CONSTRUCTOR
        
        ~Task();                                                        // DESTRUCTOR

        // **************************** HELPER FUNCS *************************

        common::uint32_t getPID() const { return pid; }                 // GET_PID            
        
        common::uint32_t getPPID() const { return ppid; }               // GET_PPID
        
        void createProcess(GlobalDescriptorTable *gdt = nullptr, 
            void (*entryPoint)() = nullptr, int pid = -1, int ppid = -1, int priority = -1);
        
        int getEaxVal(){return eax_value;}                              // GET_EAX_VALUE

        State getState() const { return state; }                        // GET_STATE

        void setState(State newState) { state = newState; }             // SET_STATE

        void setWaitingProcess(common::uint32_t pid) { waitingProcess = pid; } // SET_WAIT

        common::uint32_t getWaitingProcess() const { return waitingProcess; }  // GET_WAIT


    };

    
    //******************************* TASK MANAGER ********************************************

    class TaskManager {
    
    private:
        
        static const int MAX_TASKS = 256;                            
        
        Task tasks[MAX_TASKS];                                      // PROCESS_TABLE
        GlobalDescriptorTable *gdt;                                 // GDT
        
        int numTasks;                                               // NUMBER_OF_TASKS
        int currentTask;                                            // CURRENT_TASK
        int interrupt_count = 0;

        int aging_pid = -1;


        
    // _________________________________________________________________________________

    public:
        
        static common::uint32_t next_pid;                           // STATIC NEXT_PID
        
        TaskManager(GlobalDescriptorTable *gdt);                    // CONSTRUCTOR
            
        ~TaskManager();                                             // DESTRUCTOR

        // **************************** SYSCALLS *************************
        
        
        void fork(CPUState* parentState);                            // FORK SYSCALL

        void exit();                                                // EXIT SYSCALL

        common::uint32_t execve(void (*entryPoint)());              // EXECVE SYSCALL

        int getPID(){return tasks[currentTask].getPID();}           // GET_PID SYSCALL

        int waitpid(common::uint32_t pid);                          // WAIT_PID SYSCALL

        void setCurrenttaskPriority(int priority);

        void setCurrenttaskState(State newState){tasks[currentTask].setState(newState);}

        // **************************** ADDTASK *************************

        bool AddTask(Task* task);                                   // ADD_TASK
                
        // **************************** HELPER FUNCS *************************

        void printProcessTable();                                   // PRINT_PROCESS_TABLE
        
        void printProcessRow(int index);                            // PRINT_PROCESS
        
        int getEaxVal(){return tasks[currentTask].getEaxVal();}     // GET_EAX_VALUE

        int findTaskByPID(common::uint32_t pid);                    // FIND_PROCESS (for WAIT_PID)

        void printState(State s);                                   // PRINT_PROCESS_STATE

        int getIntrptCount(){return interrupt_count;}               // GET_INTERRUPT_COUNT

        int getPriority(){return tasks[currentTask].priority;}               // GET_PRIORITY

        void setAgingPid(int pid);

        // **************************** SCHEDULAR *************************

        CPUState* Schedule(CPUState* cpustate);                     // SCHEDULAR


    };

}

#endif
