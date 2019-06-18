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
 
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <errno.h>
#include <sys/mman.h>
#include "xinpp.h"


#define XINPP_INVALID_FD        -1
#define XINPP_ZERO_OFFSET        0

volatile sig_atomic_t xinpp_keep_running = true;
bool xinpp_can_register = true;
int xinpp_worker_count = 0;

typedef struct xinpp_worker {
    bool restart_on_sig;
    int pid;
    xinpp_work_func func;
    void* user_parameter;
} xinpp_worker;

void xinpp_sig_handler(int flag) {
    (void)flag;
    xinpp_keep_running = false;
}

static xinpp_worker xinpp_workers[XINPP_MAX_WORKERS];

bool xinpp_is_parent(int pid) {
    return pid > 0;
}

bool xinpp_is_child(int pid) {
    return pid == 0;
}

int xinpp_fork_worker( int index ) {
    int pid = fork();
    if ( xinpp_is_child( pid ) ) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        xinpp_workers[index].func( xinpp_workers[index].user_parameter );
    } else {
        xinpp_workers[index].pid = pid;
    }
    return pid;
}

int xinpp_index_from_pid( int pid ) {
    for ( int i = 0; i < xinpp_worker_count; ++i ) {
        if ( xinpp_workers[i].pid == pid ) return i;
    }
    
    return -1;
}

bool xinpp_register_worker( xinpp_work_func func, void* user_parameter, 
                                        bool restart_on_sig ) {
    if ( xinpp_can_register && xinpp_worker_count < XINPP_MAX_WORKERS) {
        int i = xinpp_worker_count++;
        
        xinpp_worker _worker = {
            .func = func,
            .user_parameter = user_parameter,
            .restart_on_sig = restart_on_sig
        };
        
        xinpp_workers[i] = _worker;
        return true;
    } else {
        return false;
    }
}

void xinpp_kill_children() {
    for (int i = 0; i < xinpp_worker_count; ++i) {
        kill(xinpp_workers[i].pid, SIGKILL);
        int status;
        waitpid(xinpp_workers[i].pid, &status, 0);
    }
}

void xinpp_terminate_children() {
    for (int i = 0; i < xinpp_worker_count; ++i) {
        kill(xinpp_workers[i].pid, SIGTERM);
    }
}

bool xinpp_parent_has_no_children(int child_pid) {
    return child_pid == -1 && errno == ECHILD;
}

bool xinpp_exited_abnormally(int wstatus) {
    return !WIFEXITED( wstatus );
}

int main(int argc, char** argv) {
    
    xinpp_worker_count = 0;  
    xinpp_can_register = true; 
    
    int before_return = xinpp_before(argc, argv);
    if ( before_return ) {
        return before_return;
    }
    
    signal(SIGINT, xinpp_sig_handler);
    signal(SIGTERM, xinpp_sig_handler);
    
    xinpp_can_register = false;
    
    int pid;
    
    for ( int i = 0; i < xinpp_worker_count; ++i ) {
        if ( xinpp_is_child( pid = xinpp_fork_worker( i ) ) ) 
            break;
    }
    
    if ( xinpp_is_parent( pid ) ) {
        int wstatus;
        while( true ) {
            
            int child_pid = wait( &wstatus );
            
            if (!xinpp_keep_running) break;
            
            if (xinpp_parent_has_no_children(child_pid)) break;
            
            if ( xinpp_exited_abnormally( wstatus ) ) {
                int index = xinpp_index_from_pid( child_pid );
                if ( xinpp_workers[index].restart_on_sig && 
                     xinpp_is_child( pid = xinpp_fork_worker( index ) ) )
                    return 0;
            } 
        }
        xinpp_terminate_children();
        xinpp_after();
        xinpp_kill_children();
    }
    
    return 0;
}

void* xinpp_create_shared_memory(size_t size) {
    int prot = PROT_READ | PROT_WRITE;
    int map = MAP_ANONYMOUS | MAP_SHARED;
    return mmap(NULL, size, prot, map, XINPP_INVALID_FD, XINPP_ZERO_OFFSET);
}

int xinpp_free_shared_memory(void* addr, size_t size) {
    return munmap(addr, size);
}
