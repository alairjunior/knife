# Knife

Knife is a minimalist framework for creating
applications that use Linux processes. It abstracts the usage of fork
and other OS functions to create and monitor processes. Creating an
application that use processes in Linux can be simple as this:

```c
void child( void* user_parameter ) {
    printf( "this is the child executing\n" );
}

int xinpp_before(int argc, char** argv) {
    printf("This is executed before everything else\n");
    
    // this register the child process
    xinpp_register_worker(child, NULL, true);
    
    return 0;
}

void xinpp_after() {
    printf("This is executed after everything else\n");
}

```

The idea behind Xinpp is that sometimes you would like to start a process
and forget about it. The process should recover from errors and restart 
itself again if something really bad happens. Those bad things might not
be under the control of the programmer, for example, some bug in a third
party library. You can define in `xinpp_register_worker` if a process
should be restarted or not. The main process itself is never restarted.
To close the application, you can send a INT or TERM signal. The example
bellow will restart the child while the parent process is running. To
finish the parent process, just press CTRL+C.

```c
void child( void* user_parameter ) {
    const char* my_line = (const char*) user_parameter;
    printf( "I'll access an invalid memory position and %s\n", my_line );
    
    for (int* x = 0; *x < 10; ++x)
        printf("Invalid memory access %x.\n", *x); // never printed
        
    printf( "Exited normally.\n" ); // never printed
}

int xinpp_before(int argc, char** argv) {   
    printf("This is executed before everything else\n");
    
    xinpp_register_worker(child, "I'll restart", true);
    xinpp_register_worker(child, "I'll not restart", false);
    
    return 0;
}

void xinpp_after() {
    printf("This is executed after everything else\n");
}

```

You can create up to `XINPP_MAX_WORKERS` processes. If you need more,
edit the `xinpp.h` to increase the number.

Different behaviors can be defined by using different functions for the 
processes.

```c
void child1( void* user_parameter ) {
    printf( "I'm child 1\n" );
}

void child2( void* user_parameter ) {
    printf( "I'm child 2\n" );
}

int xinpp_before(int argc, char** argv) {
    printf("This is executed before everything else\n");
    
    xinpp_register_worker(child1, NULL, true);
    xinpp_register_worker(child2, NULL, false);
    
    return 0;
}

void xinpp_after() {
    printf("This is executed after everything else\n");
}
```

Xinpp use processes, not threads. The lines of execution are pretty 
independent and this makes the application very robust. However,
this cames at a price: there is no native shared memory. If you need
to share data between processes, even between different executions of
the same process, you need do it manualy. Xinpp provides helper functions
to manipulate shared memory.

```c
int* exec_count;
void child( void* user_parameter ) {
    int * exec_count = ( int * ) user_parameter;
    
    printf( "Child executed %d times\n", ++(*exec_count) );
    
    sleep(1);
    
    if ( ( *exec_count ) < 10 ) {
        raise(SIGINT);
    }
}


int xinpp_before(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("This is executed before everything else\n");
    
    exec_count = xinpp_create_shared_memory(sizeof(int));
    
    xinpp_register_worker(child, exec_count, true);
    
    return 0;
}

void xinpp_after() {
    xinpp_free_shared_memory(exec_count, sizeof(int));
    printf("This is executed after everything else\n");
}
```

There is still some extra work to do. Future releases, if someone uses
this besides me, might include:

1. Allowing spawned processes to run Xinpp again and spawn their on children
2. More transparent support to shared memory

## Usage

To use Xinpp, you need to copy the xinpp.c and xinpp.h to your application
directories and implement two functions:

1. `xinpp_before(int argc, char** argv)`
2. `xinpp_after()`

`xinpp_before()` is executed before your processes start to run. It should
be used to register your processes into Xinpp, using `xinpp_register_worker`
and initialize whatever your processes require to be initialized before
running. If you return a value other than 0 in `xinpp_before`, the Xinpp
application will abort and return the value returned in `xinpp_before`. 

`xinpp_after()` is executed just before the Xinpp application finishes. 
A Xinpp application is finished when it receives a signal to finish
(SIGTERM, SIGKILL, SIGINT) or when all monitored processes finish. If
the Xinpp application finishes, `xinpp_after` is executed except in the
case of a SIGKILL, which finishes the application immediately. After the
execution of `xinpp_after`, Xinpp ensures that all created processes
terminate. To this end, after the function execution it sends a SIGKILL
to all lingering processes. Use `xinpp_after` to exit the processes
gracefully.
