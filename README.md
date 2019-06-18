# Xinpp

Xinpp is not a process pool. It is a minimalist framework for creating
applications that use Linux processes. It abstracts the usage of fork
and other OS functions to create and monitor processes.

The idea behind Xinpp is that sometimes you would like to start a process
and forget about it. The process should recover from errors and restart 
itself again if something really bad happens. Those bad things might not
be under the control of the programmer, for example, some bug in a third
party library. 

More than that, the idea of using fork to create processes manually
may be daunting if you need more than just a few processes. Xinpp 
eliminates this by abstracting processes via functions.

There is still some extra work to do. Future releases, if someone uses
this besides me, might include:

1. Allowing spawned processes to run Xinpp themselves
2. Transparent support to shared memory

## Usage

To use Xinpp, you need to copy the xinpp.c and xinpp.h to your application
directories and implement two functions:

1. `xinpp_before()`
2. `xinpp_after()`

`xinpp_before()` is executed before your processes start to run. It should
be used to register your processes into Xinpp, using `xinpp_register_worker`
and initialize whatever your processes require to be initialized before
running. If `xinpp_before()` returns a value other than 0 (zero), the Xinpp
application does not continues further and returns the value returned by
`xinpp_before()`. 

`xinpp_after()` is executed just before the Xinpp application finishes. 
A Xinpp application is finished when it receives a signal to finish
(SIGTERM, SIGKILL, SIGINT) or when all monitored processes finish. If
The Xinpp application finishes it executes `xinpp_after()` except in the
case of a SIGKILL, which finishes the application immediately. After the
execution of `xinpp_after()`, Xinpp ensures that all created processes
terminate. To this end, after the function execution it sends a SIGKILL
to all lingering processes. 

## Sample code

```c
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/wait.h> 
#include "xinpp.h"

int* exec_count_c1;
int* exec_count_c2;
int* exec_count_c3;

void first_child( void* user_parameter ) {
       
    int * exec_count = ( int * ) user_parameter;
    
    printf( "first: %d\n", ++(*exec_count) );
    
    sleep(1);
    
    if ( ( *exec_count ) < 10 ) {
        raise(SIGINT);
    }
}

void second_child( void* user_parameter ) {
    
    int * exec_count = ( int * ) user_parameter;
    
    printf( "second: %d\n", ++(*exec_count) );
    
    sleep(1);
    
    if ( ( *exec_count ) < 13 ) {
        raise(SIGINT);
    }
}

void third_child( void* user_parameter ) {
       
    int * exec_count = ( int * ) user_parameter;
    
    printf( "third: %d\n", ++(*exec_count) );
    
    sleep(1);
    
    if ( ( *exec_count ) < 13 ) {
        raise(SIGINT);
    }
}

int xinpp_before(int argc, char** argv) {
   
    (void)argc;
    (void)argv;
    
    exec_count_c1 = xinpp_create_shared_memory(sizeof(int));
    exec_count_c2 = xinpp_create_shared_memory(sizeof(int));
    exec_count_c3 = xinpp_create_shared_memory(sizeof(int));
    
    xinpp_register_worker(first_child, exec_count_c1, true);
    xinpp_register_worker(second_child, exec_count_c2, true);
    xinpp_register_worker(third_child, exec_count_c3, false);
    
    printf("before\n");
    
    return 0;
}

void xinpp_after() {
    
    xinpp_free_shared_memory(exec_count_c1, sizeof(int));
    xinpp_free_shared_memory(exec_count_c2, sizeof(int));
    xinpp_free_shared_memory(exec_count_c3, sizeof(int));
    
    printf("after\n");
}
```
