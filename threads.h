#ifndef __threads__
#define __threads__

#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h>
#include <unistd.h>
#define JB_RBX 0
#define JB_RBP 1
#define JB_R12 2
#define JB_R13 3
#define JB_R14 4
#define JB_R15 5
#define JB_RSP 6
#define JB_PC 7

int pthread_create(
    pthread_t *thread,
    const pthread_attr_t *attr,
    void *(*start_routine) (void *),
    void *arg
);

void pthread_exit(void *value_ptr);

pthread_t pthread_self(void);

unsigned long int ptr_demangle(unsigned long int p)
{
    unsigned long int ret;

    __asm__("movq %1, %%rax;\n"
        "rorq $0x11, %%rax;"
        "xorq %%fs:0x30, %%rax;"
        "movq %%rax, %0;"
    : "=r"(ret)
    : "r"(p)
    : "%rax"
    );
    return ret;
}

unsigned long int ptr_mangle(unsigned long int p)
{
    unsigned long int ret;

    __asm__("movq %1, %%rax;\n"
        "xorq %%fs:0x30, %%rax;"
        "rolq $0x11, %%rax;"
        "movq %%rax, %0;"
    : "=r"(ret)
    : "r"(p)
    : "%rax"
    );
    return ret;
}

void *start_thunk() {
  __asm__("popq %%rbp;\n"           //clean up the function prolog
      "movq %%r13, %%rdi;\n"    //put arg in $rdi
      "pushq %%r12;\n"          //push &start_routine
      "retq;\n"                 //return to &start_routine
      :
      :
      : "%rdi"
  );
  __builtin_unreachable();
}

void pthread_exit_wrapper(){
    unsigned long int res;
    asm("movq %%rax, %0\n":"=r"(res));
    pthread_exit((void *) res);
}

#endif