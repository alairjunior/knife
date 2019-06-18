/****************************************************************************
 Copyright (c) 2019 Alair Dias Junior

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*****************************************************************************/
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
