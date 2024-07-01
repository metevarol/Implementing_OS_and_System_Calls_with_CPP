
#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
#include <syscalls.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <drivers/ata.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>

#include <drivers/amd_am79c973.h>



using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;



void printf(char* str)
{
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;

    static uint8_t x=0,y=0;

    for(int i = 0; str[i] != '\0'; ++i)
    {
        switch(str[i])
        {
            case '\n':
                x = 0;
                y++;
                break;
            default:
                VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | str[i];
                x++;
                break;
        }

        if(x >= 80)
        {
            x = 0;
            y++;
        }

        if(y >= 25)
        {
            for(y = 0; y < 25; y++)
                for(x = 0; x < 80; x++)
                    VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
            x = 0;
            y = 0;
        }
    }
}

void printfHex(uint8_t key)
{
    char* foo = "00";
    char* hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}
void printfHex16(uint16_t key)
{
    printfHex((key >> 8) & 0xFF);
    printfHex( key & 0xFF);
}
void printfHex32(uint32_t key)
{
    printfHex((key >> 24) & 0xFF);
    printfHex((key >> 16) & 0xFF);
    printfHex((key >> 8) & 0xFF);
    printfHex( key & 0xFF);
}

void int_to_string(int num, char* buffer, int buffer_size) {
    // Negative vaules
    if (num < 0) {
        num = -num;
        *buffer++ = '-';
        buffer_size--;
    }
    // convert to string
    char temp[20]; 
    int index = 0;
    do {
        temp[index++] = num % 10 + '0';
        num /= 10;
    } while (num != 0 && index < buffer_size);
    
    // copy buffer to inverted order
    for (int i = index - 1; i >= 0; --i) {
        *buffer++ = temp[i];
    }
    
    // end buffer
    *buffer = '\0';
}

void print_integer(int num) {
    char buffer[20]; // Örnek olarak 20 karakterlik bir alan ayrıldı
    int_to_string(num, buffer, sizeof(buffer));
    printf(buffer);
}





class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
    void OnKeyDown(char c)
    {
        char* foo = " ";
        foo[0] = c;
        printf(foo);
    }
};

class IntegerScanfKeyboardEventHandler : public KeyboardEventHandler {
    int value;
    bool completed;
    bool negative;

public:
    IntegerScanfKeyboardEventHandler() : value(0), completed(false), negative(false) {}

    virtual void OnKeyDown(char c) {
        if (completed) {
            return; // if completed do not enter new char
        }

        if (c == '\n') {
            if (negative) {
                value = -value;
            }
            completed = true; // complete input with enter button
        } else if (c == '-' && value == 0 && !negative) {
            negative = true; // negative number
        } else if (c >= '0' && c <= '9') {
            value = value * 10 + (c - '0'); // convert char to number
        }

        // print char to screen for user interaction
        printf(&c);
    }

    bool IsCompleted() const { // check scanf process is completed
        return completed;
    }

    void Reset() {          // reset handler for new scanf process
        value = 0;
        completed = false;
        negative = false;
    }

    int GetValue() const {  // get value of scanned number
        return value;
    }
};

IntegerScanfKeyboardEventHandler intScanfHandler; // SCANF HANDLER




class MouseToConsole : public MouseEventHandler
{
    int8_t x, y;
public:
    
    MouseToConsole()
    {
        uint16_t* VideoMemory = (uint16_t*)0xb8000;
        x = 40;
        y = 12;
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);        
    }
    
    virtual void OnMouseMove(int xoffset, int yoffset)
    {
        static uint16_t* VideoMemory = (uint16_t*)0xb8000;
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);

        x += xoffset;
        if(x >= 80) x = 79;
        if(x < 0) x = 0;
        y += yoffset;
        if(y >= 25) y = 24;
        if(y < 0) y = 0;

        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);
    }

    virtual void OnMouseUp(uint8_t button) {
        printf("Mouse Button ");
        print_integer(button);
        printf("\n");
    }
    
};


void sleep(int s) 
{
    long int d = 100000000;
    for (long int i = 0; i < s * d; i++)
        i++;
}

// ***************************************** SYSCALLS ***********************************************

void sys_process_table(){
    asm("int $0x80" : : "a" (SYS_PRINT_TABLE));
}

void sysfork() {
    asm ("int $0x80" : : "a" (SYS_FORK)); 
}

int sysfork_return(){
    int ebx_value;
    asm("int $0x80" :"=b" (ebx_value): "a" (SYS_DOUBLE_RETURN));
    return ebx_value;
}

int getpid(){
    int ecx_value;
    asm("int $0x80" :"=c" (ecx_value): "a" (SYS_GETPID));
    return ecx_value;
}

void set_priority(int priority){
    asm("int $0x80" : : "a" (SYS_SET_PRIORITY), "b" (priority));
}

void set_state(int state){
    asm("int $0x80" : : "a" (SYS_SET_STATE), "b" (state));
}

void waitpid(int pid){
    asm("int $0x80" : : "a" (SYS_WAITPID), "b" (pid));
}

void sysexit() {
    asm ("int $0x80" : : "a" (SYS_EXIT)); 
}

void sysexecve(void entrypoint()){
    asm("int $0x80" : : "a" (SYS_EXECVE), "b" ((uint32_t)entrypoint));
}

int sys_interrupt_count(){
    int edx_value;
    asm("int $0x80" :"=d" (edx_value): "a" (SYS_INTRPT_COUNT));
    return edx_value;
}

int sys_get_priority(){
    int edx_value;
    asm("int $0x80" :"=d" (edx_value): "a" (SYS_GET_PRIORITY));
    return edx_value;
}

void set_aging(int pid){
    asm("int $0x80" : : "a" (SYS_ADD_AGING), "b" (pid));
}

// ***************************************** SCANF ***********************************************


void scanf(int* outValue) {

    intScanfHandler.Reset();

    while (!intScanfHandler.IsCompleted());

    *outValue = intScanfHandler.GetValue();

    set_state(0); // Set state to READY
}

// ***************************************** TASK FUNCTIONS ***********************************************


void printCollatz(int n) {
    print_integer(n);
    printf(" : ");

    while (n != 1) {
        if (n % 2 == 0) {
            n /= 2;
        } else {
            n = 3 * n + 1;
        }
        printf(", ");
        print_integer(n);
    }

    printf("\n");
}

int binarySearch(int arr[], int n, int x) {
    int low = 0, high = n - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;

        if (arr[mid] == x)
            return mid; // x finded and returned its index
        else if (arr[mid] < x)
            low = mid + 1; // x is right
        else
            high = mid - 1; // x is left
    }
    return -1; // x not find
}

int linearSearch(int arr[], int n, int x) {
    for (int i = 0; i < n; i++) {
        if (arr[i] == x)
            return i; // x finded and returned its index
    }
    return -1; // x not find
}

long long long_running_program(int n) {
    long long result = 0;  
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            result += i * j;
        }
    }
    return result;
}


void calculate_Collatz(int n) {

    while (n != 1) {
        if (n % 2 == 0) {
            n /= 2;
        } else {
            n = 3 * n + 1;
        }
        
    }

}

void collatztask_with_input(){
    while(1){
    printf("\nPID: ");print_integer(getpid());
    printf(" Please enter an integer: ");
    int number;
    scanf(&number);
    //printf("\nYou entered: ");print_integer(number);
    printf("PID: ");print_integer(getpid());printf(" ");printCollatz(number);
    sys_process_table();
    }
    
}

void collatztask(){

    int n = 10;
    for(int i = 0; i < n; i++){
         printf("collatz_task(100) running for ");
         print_integer(i+1);
         printf("/10 times");
         printf(" PID = ");print_integer(getpid());
         printf(" Priority = ");print_integer(sys_get_priority());
         printf("\n");
         calculate_Collatz(100);
         sleep(1);
    }
 
    sysexit();
}

void longrunningtask(){

    int n = 10;
    for(int i = 0; i < n; i++){
        printf("long_running_task(1000) running for ");
        print_integer(i+1);
        printf("/10 times");
        printf(" PID = ");print_integer(getpid());
        printf(" Priority = ");print_integer(sys_get_priority());
        printf("\n");
        long long result = long_running_program(1000);
        sleep(1);
    }

    sysexit();
}

void binar_search_task(){
    int arr[] = {10, 20, 30, 50, 60, 80, 100, 110, 130, 170};
    int n = sizeof(arr)/sizeof(arr[0]);
    int x = 110;

    int m = 10;
    for(int i = 0; i < m; i++){
         printf("binar_search_task(arr) running for ");
         print_integer(i+1);printf("/10 times");
         printf(" PID = ");print_integer(getpid());
         printf(" Priority = ");print_integer(sys_get_priority());
         printf("\n");
         binarySearch(arr, n, x);
         sleep(1);
    }

    sysexit();
}

void linear_search_task(){
    int arr[] = {10, 20, 80, 30, 60, 50, 110, 100, 130, 170};
    int n = sizeof(arr)/sizeof(arr[0]);
    int x = 175;

    int m = 10;
    for(int i = 0; i < m; i++){
         printf("linear_search_task(arr) running for ");
         print_integer(i+1);printf("/10 times");
         printf(" PID = ");print_integer(getpid());
         printf(" Priority = ");print_integer(sys_get_priority());
         printf("\n");
         linearSearch(arr, n, x);
         sleep(1);
    }

    sysexit();
}







// ************************************* FORK TESTS **********************************************


void forktask_3() {
    printf("\nfork() test:\n");

    sysfork();  

    // syscall for double return feature
    int pid = sysfork_return(); 

    if(pid == 0){
        printf("\nHello from child = ");
        print_integer(getpid());
        printf("\n");
    }else{
        printf("\nHello from parent = ");
        print_integer(getpid()); 
        printf("\n");
    }

    while(1);
}

void forktask_4() {
    printf("\nThree times fork() test:\n");
    printf("pid - message - getpid()\n");

    sysfork(); 
    sysfork();
    sysfork();

    int pid = sysfork_return();

    if(pid == 0){
        printf("\n");
        print_integer(pid); 
        printf(" Hello from child  = ");
        print_integer(getpid());
        //sleep(1); 
        sysexit(); 
    }else{
        printf("\n");
        print_integer(pid);
        printf(" Hello from parent = ");
        print_integer(getpid());
        //sleep(1);
        sysexit(); 
    }

    //sys_process_table();
    while(1);
}

// ************************************* WAITPID TESTS **********************************************


void waitpidtask_1(){

    sysfork();  

    int pid = sysfork_return();

    if (pid == 0) {
        // child
        printf("child started PID = ");
        print_integer(getpid());
        printf("\n");

        sleep(5);  // sleep

        printf("child ended\n");
        sysexit();
    } else {
       
        waitpid(pid);  // wait child
        
        printf("parent started\n");
        
    }

    while(1);
}

void waitpidtask_2() {
    printf("\nThree times fork() test with waitpid:\n");

    sysfork();  
    sysfork();
    sysfork();

    int pid = sysfork_return();

    // childs
    if(pid == 0){
        printf("\n");
        printf("Hello from child = ");
        print_integer(getpid()); 
        sysexit();          // terminate childs
    }else{
        waitpid(pid);       // wait childs
        printf("\n");
        printf("Hello from parent = ");
        print_integer(getpid()); 
    }

    while(1);
}

// ************************************* EXECVE TESTS **********************************************

void hello_from_execve(){
    printf("\nHello from execve = ");
    print_integer(getpid()); 
    while(1);
}

void execvetask_1() {
    printf("\nchild-> execve(hello_from_execve) test:\n");

    sysfork();  

    int pid = sysfork_return();

    if(pid == 0){
        sysexecve(hello_from_execve);

        printf("\nHello from child = ");
        print_integer(getpid());  
    }else{
        printf("\nHello from parent = ");
        print_integer(getpid()); 
    }

    while(1);
}

// ************************************* PART A TESTS **********************************************



void partA_strategy() {
    printf("PART A - Strategy - Each program loading 3 times..\n\n");

    for (int i = 0; i < 3; i++) {
        sysfork(); // fork    
        int pid = sysfork_return();
        if(pid == 0){    
            sysexecve(collatztask); // collatztask
            while(1);
        }    
    }

    for (int i = 0; i < 3; i++) {
        sysfork(); // fork    
        int pid = sysfork_return();
        if(pid == 0){
            sysexecve(longrunningtask); // longrunningtask        
            while(1);
        }      
    }

    // Main process waits
    while(1){
       printf("\nMain task waiting for all processes to terminate...\n");  
       sys_process_table(); // Process Table 
       sleep(1);
    }
}

void partA_strategy_waitpid() {
    printf("PART A - Strategy - Each program loading 3 times(with waitpid)..\n\n");
    for (int i = 0; i < 3; i++) {
        sysfork(); // fork
        int pid = sysfork_return(); // double return
        
        if (pid == 0) { // child
            sysexecve(collatztask); // collatztask
            sysexit();
        }
        else{           //parent
            waitpid(pid);
        }
    }

    for (int i = 0; i < 3; i++) {
        sysfork(); // fork
        int pid = sysfork_return(); // double return
        
        if (pid == 0) { // child
            sysexecve(longrunningtask); // longrunningtask
            sysexit();
        }else{          // parent
            waitpid(pid);
        }
    }

    // Main process waits
    printf("\n\n\n\n\n\n\n\n\n\n");
    printf("\nMain task waiting for all processes to terminate...\n");  

    sys_process_table(); // Process Table
    while(1);

}

// ************************************* PART B TESTS **********************************************

// ROUND ROBIN 
void part_b_first_strategy(){
    printf("PART B - First Strategy - program loading 10 times..\n\n");

    for (int i = 0; i < 10; i++) {
        sysfork(); // fork    
        int pid = sysfork_return();
        if(pid == 0){    
            sysexecve(collatztask); // collatztask
            while(1);
        }    
        
    }

    // Main process waits
    while(1){
       printf("\nMain task waiting for all processes to terminate...\n");  
       sys_process_table(); // Process Table 
       sleep(1);
    } 

}

// ROUND ROBIN 
void part_b_second_strategy() {
    printf("PART B - Second Strategy - Each program loading 3 times..\n\n");

    for (int i = 0; i < 3; i++) {
        sysfork(); // fork    
        int pid = sysfork_return();
        if(pid == 0){    
            sysexecve(binar_search_task); // collatztask
            while(1);
        }    
    }

    for (int i = 0; i < 3; i++) {
        sysfork(); // fork    
        int pid = sysfork_return();
        if(pid == 0){
            sysexecve(linear_search_task); // longrunningtask        
            while(1);
        }       
    }

    // Main process waits
    while(1){
       printf("\nMain task waiting for all processes to terminate...\n");  
       sys_process_table(); // Process Table 
       sleep(1);
    }
    
}

// PRIORITY BASED ROUND ROBIN
void part_b_third_strategy() {
    printf("PART B - Third Strategy - Fixed Priority starting with collatztask..\n\n");

    set_priority(5); 
    int pid;
    
    
    // low priority Collatz task
    sysfork();
    pid = sysfork_return(); // double return
    if (pid == 0) {
        set_priority(5);
        sysexecve(collatztask);
        while(1);
    }

    int interrupt_count = 0;

    // after 50th interrupt other programs loading
    while(true){
        interrupt_count = sys_interrupt_count();
        if(interrupt_count > 50){                   
            break;
        }
    }
    
    //BinarySearch 
    sysfork();
    pid = sysfork_return(); // double return
    if (pid == 0) {
        set_priority(5); 
        sysexecve(binar_search_task);
        while(1);
    }

    //LinearSearch
    sysfork();
    pid = sysfork_return(); // double return
    if (pid == 0) {
        set_priority(5); // Orta öncelik
        sysexecve(linear_search_task);
        while(1);
    }

    //long running program
    sysfork();
    pid = sysfork_return(); // double return
    if (pid == 0) {
        set_priority(5); // Orta öncelik
        sysexecve(longrunningtask);
        while(1);
    }
    
    
    set_priority(10);
    while (1) {
        printf("Main task waiting for all processes to terminate...\n");
        sys_process_table();
        sleep(1); // CPU'yu boş yere meşgul etmemek için bekleme
    }
    
}

// PRIORITY BASED ROUND ROBIN
void part_b_dynamic_strategy() {
    printf("PART B - Third Strategy - Dynamic Priority starting..\n\n");

    int numProcesses = 4; // Toplam süreç sayısı
    int initializedProcesses = 0;

    set_priority(5); // Düşük öncelik
    int pid;
    
    // low priority Collatz task
    sysfork();
    pid = sysfork_return(); // double return
    if (pid == 0) {
        set_aging(getpid()); // set aging for collatz task
        set_priority(5);
        while (initializedProcesses < numProcesses);
        sysexecve(collatztask);
        while(1);
    }
    else{
        initializedProcesses++;
    }
    
    //BinarySearch 
    sysfork();
    pid = sysfork_return(); // double return
    if (pid == 0) {
        set_priority(4);
        while (initializedProcesses < numProcesses); 
        sysexecve(binar_search_task);
        while(1);
    }
    else{
        initializedProcesses++;
    }

    //LinearSearch
    sysfork();
    pid = sysfork_return(); // double return
    if (pid == 0) {
        set_priority(2); // Orta öncelik
        while (initializedProcesses < numProcesses);
        sysexecve(linear_search_task);
        while(1);
    }
    else{
        initializedProcesses++;
    }

    //long running program
    sysfork();
    pid = sysfork_return(); // double return
    if (pid == 0) {
        set_priority(3); // Orta öncelik
        while (initializedProcesses < numProcesses);
        sysexecve(longrunningtask);
        while(1);
    }
    else{
        initializedProcesses++;
    }
    

    while (1) {
        printf("Main task waiting for all processes to terminate...\n");
        sys_process_table();
        sleep(1); // CPU'yu boş yere meşgul etmemek için bekleme
    }
    
}



void partb() {
    printf("Initializing system...\n");
    set_priority(10); // Düşük öncelik
    int pid;
    // Düşük öncelikli Collatz görevi
    sysfork();
    pid = sysfork_return(); // double return
    if (pid == 0) {
        set_priority(5); // Düşük öncelik
        sysexecve(collatztask);
        while(1);
    }

    // Yüksek öncelikli BinarySearch görevi
    sysfork();
    pid = sysfork_return(); // double return
    if (pid == 0) {
        set_priority(1); // Yüksek öncelik
        sysexecve(collatztask);
        while(1);
    }

    // Orta öncelikli LinearSearch görevi
    sysfork();
    pid = sysfork_return(); // double return
    if (pid == 0) {
        set_priority(3); // Orta öncelik
        sysexecve(collatztask);
        while(1);
    }

    while (1) {
        printf("Main task waiting for all processes to terminate...\n");
        sleep(1); // CPU'yu boş yere meşgul etmemek için bekleme
    }
}

// ************************************* PART C TESTS **********************************************


void testScanfFunction() {
    printf("Please enter an integer: ");
    int number;
    scanf(&number);
    printf("\nYou entered: ");print_integer(number);
    while(1);
}


void part_c_first_strategy() {

    printf("PART C - First Strategy\n\n");

    for (int i = 0; i < 3; i++) {
        sysfork(); // fork    
        int pid = sysfork_return();
        if(pid == 0){    
         sysexecve(collatztask_with_input); // collatztask
        }
           
    }

    while(1);
   

}

// ************************************* KERNEL MAIN **********************************************


typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}



extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{
    printf("Hello from My OS --- Hikmet Mete Varol -- 1801042608\n");
    printf("----------------------------------------------------\n");

    GlobalDescriptorTable gdt;
    
    uint32_t* memupper = (uint32_t*)(((size_t)multiboot_structure) + 8);
    size_t heap = 10*1024*1024;
    MemoryManager memoryManager(heap, (*memupper)*1024 - heap - 10*1024);
   
    void* allocated = memoryManager.malloc(1024);

    // ****************************************** TESTS ******************************************
    TaskManager taskManager(&gdt);



    // ********************* PART A *********************



    // -------------------Syscalls-------

    //Task initial(&gdt,forktask_3); // sysfork() double return test with sysfork_return()
    Task initial(&gdt,forktask_4); // sysfork() multiple fork test (3 times)

    //Task initial(&gdt,waitpidtask_1);
    //Task initial(&gdt,waitpidtask_2);

    //Task initial(&gdt,execvetask_1); // execve(hello_from_execve) test


    // ---------------- Strrategies------

    //Task initial(&gdt,partA_strategy);  
    //Task initial(&gdt,partA_strategy_waitpid);


    
    // ********************* PART B *********************

    
    //Task initial(&gdt,part_b_first_strategy);
    //Task initial(&gdt,part_b_second_strategy);
    //Task initial(&gdt,part_b_third_strategy);
    //Task initial(&gdt,part_b_dynamic_strategy);


    
    // ********************* PART C *********************

    //Task initial(&gdt,part_c_first_strategy);

    // **************************************************



    taskManager.AddTask(&initial);   // Görev yöneticisine ekle



    InterruptManager interrupts(0x20, &gdt, &taskManager);
    SyscallHandler syscalls(&interrupts, 0x80,&taskManager);


    
    // *********************** KEYBOARD ****************************************************
    DriverManager drvManager;

    PrintfKeyboardEventHandler kbhandler;

    KeyboardDriver keyboard(&interrupts, &intScanfHandler);

    drvManager.AddDriver(&keyboard);

    // **************************************************************************************

    
    // *********************** MOUSE ********************************************************
    MouseToConsole mousehandler;
    MouseDriver mouse(&interrupts, &mousehandler);
        
    drvManager.AddDriver(&mouse);


    
    // ***************************************************************************************
    
    drvManager.ActivateAll();
    
    interrupts.Activate(); 



    while (1); 
   
}
