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
#include "knife.h"


#define KNIFE_INVALID_FD        -1
#define KNIFE_ZERO_OFFSET        0

volatile sig_atomic_t knife_keep_running = true;
bool knife_can_register = true;
int knife_worker_count = 0;

typedef struct knife_worker {
    bool restart_on_sig;
    int pid;
    knife_work_func func;
    void* user_parameter;
} knife_worker;

void knife_sig_handler(int flag) {
    (void)flag;
    knife_keep_running = false;
}

static knife_worker knife_workers[KNIFE_MAX_WORKERS];

bool knife_is_parent(int pid) {
    return pid > 0;
}

bool knife_is_child(int pid) {
    return pid == 0;
}

int knife_fork_worker( int index ) {
    int pid = fork();
    if ( knife_is_child( pid ) ) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        knife_workers[index].func( knife_workers[index].user_parameter );
    } else {
        knife_workers[index].pid = pid;
    }
    return pid;
}

int knife_index_from_pid( int pid ) {
    for ( int i = 0; i < knife_worker_count; ++i ) {
        if ( knife_workers[i].pid == pid ) return i;
    }
    
    return -1;
}

bool knife_register_worker( knife_work_func func, void* user_parameter, 
                                        bool restart_on_sig ) {
    if ( knife_can_register && knife_worker_count < KNIFE_MAX_WORKERS) {
        int i = knife_worker_count++;
        
        knife_worker _worker = {
            .func = func,
            .user_parameter = user_parameter,
            .restart_on_sig = restart_on_sig
        };
        
        knife_workers[i] = _worker;
        return true;
    } else {
        return false;
    }
}

void knife_kill_children() {
    for (int i = 0; i < knife_worker_count; ++i) {
        kill(knife_workers[i].pid, SIGKILL);
        int status;
        waitpid(knife_workers[i].pid, &status, 0);
    }
}

void knife_terminate_children() {
    for (int i = 0; i < knife_worker_count; ++i) {
        kill(knife_workers[i].pid, SIGTERM);
    }
}

bool knife_parent_has_no_children(int child_pid) {
    return child_pid == -1 && errno == ECHILD;
}

bool knife_exited_abnormally(int wstatus) {
    return !WIFEXITED( wstatus );
}

int main(int argc, char** argv) {
    
    knife_worker_count = 0;  
    knife_can_register = true; 
    
    int before_return = knife_before(argc, argv);
    if ( before_return ) {
        return before_return;
    }
    
    signal(SIGINT, knife_sig_handler);
    signal(SIGTERM, knife_sig_handler);
    
    knife_can_register = false;
    
    int pid;
    
    for ( int i = 0; i < knife_worker_count; ++i ) {
        if ( knife_is_child( pid = knife_fork_worker( i ) ) ) 
            break;
    }
    
    if ( knife_is_parent( pid ) ) {
        int wstatus;
        while( true ) {
            
            int child_pid = wait( &wstatus );
            
            if (!knife_keep_running) break;
            
            if (knife_parent_has_no_children(child_pid)) break;
            
            if ( knife_exited_abnormally( wstatus ) ) {
                int index = knife_index_from_pid( child_pid );
                if ( knife_workers[index].restart_on_sig && 
                     knife_is_child( pid = knife_fork_worker( index ) ) )
                    return 0;
            } 
        }
        knife_terminate_children();
        knife_after();
        knife_kill_children();
    }
    
    return 0;
}

void* knife_create_shared_memory(size_t size) {
    int prot = PROT_READ | PROT_WRITE;
    int map = MAP_ANONYMOUS | MAP_SHARED;
    return mmap(NULL, size, prot, map, KNIFE_INVALID_FD, KNIFE_ZERO_OFFSET);
}

int knife_free_shared_memory(void* addr, size_t size) {
    return munmap(addr, size);
}
