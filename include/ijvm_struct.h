
#ifndef IJVM_STRUCT_H
#define IJVM_STRUCT_H

#include <stdio.h>  /* contains type FILE * */

#include "ijvm_types.h"
/**
 * All the state of your IJVM machine goes in this struct!
 **/

// Implement stack
typedef struct {
    word *data;
    int top;
    int capacity;
} Stack;

typedef struct IJVM {
    // do not changes these two variables
    FILE *in;   // use fgetc(ijvm->in) to get a character from in.
                // This will return EOF if no char is available.
    FILE *out;  // use for example fprintf(ijvm->out, "%c", value); to print value to out

    // your variables go here

    unsigned int const_pool_size;
    uint8_t *const_pool;
    unsigned int text_size;
    byte *text;

    unsigned int pc;

    Stack stack;

    int lv;

} ijvm;

#endif 
