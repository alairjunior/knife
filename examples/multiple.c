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
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include "knife.h"

void child1( void* user_parameter ) {
    (void)user_parameter;
    printf( "I'm child 1\n" );
}

void child2( void* user_parameter ) {
    (void)user_parameter;
    printf( "I'm child 2\n" );
}

int knife_before(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("This is executed before everything else\n");
    
    knife_register_worker(child1, NULL, true);
    knife_register_worker(child2, NULL, false);
    
    return 0;
}

void knife_after() {
    printf("This is executed after everything else\n");
}
