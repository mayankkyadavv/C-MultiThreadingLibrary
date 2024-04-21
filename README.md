# C-MultiThreadingLibrary

## Introduction
Welcome to my threading library project that implements basic threading features using the C programming language. 
This library provides the functionality to create and manage threads, handle synchronization using semaphores, and simulate concurrent execution, mimicking higher-level threading capabilities found in operating systems.

## Features
- Thread creation and destruction
- Thread joining and exiting
- Semaphore initialization, wait, and post operations
- Custom scheduling and context switching using signal handling

## Usage
To use this library, include the `threads.h` header file in your C program and compile it with the provided source files. Here's a simple example that demonstrates creating a thread:

```c
#include "threads.h"

void *thread_function(void *arg) {
    printf("Thread running with argument: %d\n", *((int*)arg));
    return 0;
}

int main() {
    pthread_t thread;
    int argument = 42;
    pthread_create(&thread, NULL, thread_function, &argument);
    pthread_join(thread, NULL);
    return 0;
}
```
## Make
You can make the library object by running the command: make all

