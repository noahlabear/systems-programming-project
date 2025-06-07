#include <stdio.h>  // for getc, printf
#include <stdlib.h> // malloc, free
#include "ijvm.h"
#include "util.h" // read this file for debug prints, endianness helper functions


// see ijvm.h for descriptions of the below functions

ijvm* init_ijvm(char *binary_path, FILE* input, FILE* output)
{
  // do not change these first three lines
  ijvm* m = (ijvm *) malloc(sizeof(ijvm));
  // note that malloc gives you memory, but gives no guarantees on the initial
  // values of that memory. It might be all zeroes, or be random data.
  // It is hence important that you initialize all variables in the ijvm
  // struct and do not assume these are set to zero.
  m->in = input;
  m->out = output;
  
  
  // TODO: implement me

  // Open and read .ijvm binary
  FILE *f = fopen(binary_path, "rb");
  if (!f) {
      perror("fopen");
      free(m);
      return NULL;
  }

  uint8_t buf[4];
  // read first 4 bytes for magic number
  if (fread(buf, 1, 4, f) != 4) {
        fprintf(stderr, "Error reading magic number from %s\n", binary_path);
        free(m);
        return NULL;
  }

  uint32_t magic = read_uint32(buf);
  fprintf(m->out, "Magic number: 0x%08x\n", magic);

  fclose(f);

  return m;
}

void destroy_ijvm(ijvm* m) 
{
  // TODO: implement me

  free(m); // free memory for struct
}

byte *get_text(ijvm* m) 
{
  // TODO: implement me
  return NULL;
}

unsigned int get_text_size(ijvm* m) 
{
  // TODO: implement me
  return 0;
}

word get_constant(ijvm* m,int i) 
{
  // TODO: implement me
  return 0;
}

unsigned int get_program_counter(ijvm* m) 
{
  // TODO: implement me
  return 0;
}

word tos(ijvm* m) 
{
  // this operation should NOT pop (remove top element from stack)
  // TODO: implement me
  return -1;
}

bool finished(ijvm* m) 
{
  // TODO: implement me
  return false;
}

word get_local_variable(ijvm* m, int i) 
{
  // TODO: implement me
  return 0;
}

void step(ijvm* m) 
{
  // TODO: implement me

}

byte get_instruction(ijvm* m) 
{ 
  return get_text(m)[get_program_counter(m)]; 
}

ijvm* init_ijvm_std(char *binary_path) 
{
  return init_ijvm(binary_path, stdin, stdout);
}

void run(ijvm* m) 
{
  while (!finished(m)) 
  {
    step(m);
  }
}


// Below: methods needed by bonus assignments, see ijvm.h
// You can leave these unimplemented if you are not doing these bonus 
// assignments.

int get_call_stack_size(ijvm* m) 
{
   // TODO: implement me if doing tail call bonus
   return 0;
}


// Checks if reference is a freed heap array. Note that this assumes that 
// 
bool is_heap_freed(ijvm* m, word reference) 
{
   // TODO: implement me if doing garbage collection bonus
   return 0;
}

// Checks if top of stack is a reference
bool is_tos_reference(ijvm* m)
{
	// TODO: implement me if doing precise garbage collection bonus
	//  using ANEWARRAY, AIALOAD and AIASTORE
	return 0;
}
