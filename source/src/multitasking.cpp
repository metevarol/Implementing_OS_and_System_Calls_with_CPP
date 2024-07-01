#include <multitasking.h>
using namespace myos;
using namespace myos::common;

void printf(char*);
void print_integer(int num);

//************************************ TASK  ********************************************


Task::Task() {
    createProcess();
}

Task::Task(GlobalDescriptorTable *gdt, void entrypoint()) {
    createProcess(gdt, entrypoint);
}

void Task::createProcess(GlobalDescriptorTable* gdt, void (*entryPoint)(), int pid, int ppid, int priority) {
    
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));

    // Register initializations
    cpustate->eax = 0;
    cpustate->ebx = 0;
    cpustate->ecx = 0;
    cpustate->edx = 0;
    cpustate->esi = 0;
    cpustate->edi = 0;
    cpustate->ebp = 0;

    // Set instruction pointer and code segment
    cpustate->eip = (uint32_t)entryPoint;
    cpustate->cs = gdt->CodeSegmentSelector();
    cpustate->eflags = 0x202;  // Default flags

    // Task identifiers and initial state
    this->pid = pid;
    this->ppid = ppid;
    this->priority = priority;
    this->state = Ready;  // Default state
}



//******************************* TASK MANAGER ********************************************


common::uint32_t TaskManager::next_pid = 0;  // changed 12 may 16.33 -> waitingprocess ve schedular waitpid de degisti

TaskManager::TaskManager(GlobalDescriptorTable *gdt) {
    this->gdt = gdt;
    currentTask = -1;
    numTasks = 0;
}

bool TaskManager::AddTask(Task* task) {
    if (numTasks >= 256) {
        return false;
    }

    task->pid = next_pid;
    task->ppid = -1; // changed 12 may 16.33
    task->priority = -1; 
    task->state = Ready;

    Task* dest = &tasks[numTasks];

    // copy process table entries
    dest->pid = task->pid;
    dest->ppid = task->ppid;
    dest->state = task->state;
    
    // copy stack
    for (int i = 0; i < 4096; ++i) {
        dest->stack[i] = task->stack[i];
    }
    
    // copy CPU state
    *(dest->cpustate) = *(task->cpustate);

    ++numTasks;
    ++next_pid;
    return true;
}



void TaskManager::fork(CPUState* parentState) {

    Task* parentTask = &tasks[currentTask];

    if (numTasks >= MAX_TASKS) {
        parentTask->eax_value = -1; // double return
        return;
    }

    Task* childTask = &tasks[numTasks];

    childTask->createProcess(gdt, (void(*)())parentTask->cpustate->eip,next_pid,parentTask->pid);

    for (int i = 0; i < 4096; ++i) {
    childTask->stack[i] = parentTask->stack[i];
    }

    *(childTask->cpustate) = *parentState; 

    childTask->cpustate->eax = 0;
    parentState->eax = childTask->getPID();
    
    // ------------------- double return ------------------
    childTask->eax_value = 0;
    parentTask->eax_value = childTask->getPID();
    // ------------------- double return ------------------

    numTasks++,next_pid++;
    
}



// Helper function to find task by PID
int TaskManager::findTaskByPID(common::uint32_t pid) {
    for (int i = 0; i < numTasks; ++i) {
        if (tasks[i].getPID() == pid) {
            return i;
        }
    }
    return -1;
}


int TaskManager::waitpid(common::uint32_t pid) {

    int index = findTaskByPID(pid);

    if (index == -1) return -1;  // Process not found

    Task& child = tasks[index];

    if (child.getState() == Terminated) {
        return 0; // Child already terminated
    }

    // Set the current task to Blocked state
    tasks[currentTask].setState(Blocked);

    // Set current task as waiting process of the child
    child.setWaitingProcess(tasks[currentTask].getPID());

    // force a reschedule
    asm volatile("int $0x20");

    return 0; 
}

void TaskManager::exit() {

    Task* currentTask = &tasks[this->currentTask];

    // terminate current task
    currentTask->state = Terminated;

    // wake up parent
    for (int i = 0; i < numTasks; i++) {
        if (tasks[i].waitingProcess == currentTask->pid && tasks[i].state == Blocked) {
            tasks[i].state = Ready;
        }
    }

    // force reschedule
    asm volatile("int $0x20");
}

common::uint32_t TaskManager::execve(void (*entryPoint)()) {
    
    Task* currentTask = &tasks[this->currentTask];

    // set process with new entrypoint
    currentTask->createProcess(this->gdt, entryPoint, currentTask->pid, currentTask->ppid, currentTask->priority);
    
    return (uint32_t)currentTask->cpustate;
}



void TaskManager::printState(State s) {
    switch (s) {
        case Ready:
            printf("Ready");
            break;
        case Running:
            printf("Running");
            break;
        case Blocked:
            printf("Blocked");
            break;
        case Terminated:
            printf("Terminated");
            break;
        default:
            printf("Unknown State");
    }
}

void TaskManager::printProcessTable(){
    printf("\nPid     ");printf("PPid     ");printf("State     ");printf("Priority     \n");
    printf("--------------------------------\n");

    for(int i = 0;i < numTasks;i++){
        printProcessRow(i);
    }

    printf("--------------------------------\n");
}


void TaskManager::printProcessRow(int index){
   
    print_integer(tasks[index].pid);
    printf("       ");
    print_integer(tasks[index].ppid);
    printf("        ");
    printState(tasks[index].state);
    printf("        ");
    print_integer(tasks[index].priority);
    printf("\n");
}

void TaskManager::setCurrenttaskPriority(int priority){
    Task* currentTask = &tasks[this->currentTask];
    currentTask->priority = priority;
}

void TaskManager::setAgingPid(int pid){
    this->aging_pid = pid;
}

/*
CPUState* TaskManager::Schedule(CPUState* cpustate) {
    if (numTasks <= 0) {
        return cpustate;
    }

    if(currentTask >= 0)
        tasks[currentTask].cpustate = cpustate;

    //printProcessTable();

    if (++currentTask >= numTasks) {
        currentTask = 0;
    }

    return tasks[currentTask].cpustate;
}

*/

/*
// ROUND ROBIN SCHEDULAR WITH STATES AND WAITPID CHECK
CPUState* TaskManager::Schedule(CPUState* cpustate) {
    if (numTasks <= 0) {
        return cpustate;
    }

    if (currentTask >= 0)
        tasks[currentTask].cpustate = cpustate;

    if (tasks[currentTask].getState() == Terminated && tasks[currentTask].getWaitingProcess() != 0) {
        int waitingIndex = findTaskByPID(tasks[currentTask].getWaitingProcess());
        if (waitingIndex != -1) {
            tasks[waitingIndex].setState(Ready);
        }
    }

    // Select next task
    do {
        currentTask = (currentTask + 1) % numTasks;
    } while (tasks[currentTask].getState() == Blocked || tasks[currentTask].getState() == Terminated);

    tasks[currentTask].state = Running;

    return tasks[currentTask].cpustate;
}
*/


/*
// PRIORITY BASED SCHEDULAR WITH STATES AND WAITPID CHECK
CPUState* TaskManager::Schedule(CPUState* cpustate) {
    if (numTasks <= 0) {
        return cpustate;
    }

    if (currentTask >= 0)
        tasks[currentTask].cpustate = cpustate;

    if (tasks[currentTask].getState() == Terminated && tasks[currentTask].getWaitingProcess() != 0) {
        int waitingIndex = findTaskByPID(tasks[currentTask].getWaitingProcess());
        if (waitingIndex != -1) {
            tasks[waitingIndex].setState(Ready);
        }
    }

    int highestPriority = 255; // Başlangıç için yüksek bir değer.
    int nextTaskIndex = currentTask;

    // --------------------------- aging ------------------
    int index = findTaskByPID(aging_pid);

    if(interrupt_count > 20 && tasks[index].getState() != Terminated && index != -1){
        tasks[index].priority = 1;
    }

    //---------------------------- aging ---------------------

    // En yüksek öncelikli görevi bul
    for (int i = 0; i < numTasks; i++) {

        if ((tasks[i].getState() != Blocked && tasks[i].getState() != Terminated) &&
            tasks[i].priority <= highestPriority) {
            highestPriority = tasks[i].priority;
            nextTaskIndex = i;
        }
    }

    currentTask = nextTaskIndex;
    tasks[currentTask].state = Running;

    this->interrupt_count++;
    return tasks[currentTask].cpustate;
}
*/
// PRIORITY BASED ROUND ROBIN SCHEDULAR WITH STATES AND WAITPID CHECK
CPUState* TaskManager::Schedule(CPUState* cpustate) {
    if (numTasks <= 0) {
        return cpustate;
    }

    if (currentTask >= 0)
        tasks[currentTask].cpustate = cpustate;

    // waitpid mechanism
    if (tasks[currentTask].getState() == Terminated && tasks[currentTask].getWaitingProcess() != -1) {
        int waitingIndex = findTaskByPID(tasks[currentTask].getWaitingProcess());
        if (waitingIndex != -1) {
            tasks[waitingIndex].setState(Ready);
        }
    }

    int highestPriority = 255; // max priority
    int nextTaskIndex = currentTask;

    // --------------------------- aging ------------------ // for part b dynamic strategy
    int index = findTaskByPID(aging_pid);

    if(interrupt_count == 20 && tasks[index].getState() != Terminated && index != -1){
        tasks[index].priority = 1;
        printf("----------------- AGING ------------- \n");
    }

    //---------------------------- aging ---------------------

    int startIndex = (currentTask + 1) % numTasks;

    // find highest priority
    for (int i = 0; i < numTasks; i++) {
        int idx = (startIndex + i) % numTasks; // for round robin

        if ((tasks[idx].getState() != Blocked && tasks[idx].getState() != Terminated) &&
            tasks[idx].priority < highestPriority) {

            highestPriority = tasks[idx].priority;
            nextTaskIndex = idx;

            // Reset loop to find the next tasks if multiple with same priority
            i = -1;
            startIndex = (nextTaskIndex + 1) % numTasks;
        }
    }

    currentTask = nextTaskIndex;
    tasks[currentTask].state = Running;

    this->interrupt_count++; // for part b third strategy

    return tasks[currentTask].cpustate;
}
















