#include <stdio.h>  // for getc, printf
#include <stdlib.h> // malloc, free
#include "ijvm.h"
#include "util.h" // read this file for debug prints, endianness helper functions


// see ijvm.h for descriptions of the below functions

// Stack functions
void stack_init(Stack *s, int capacity) {
    s->data     = malloc(sizeof(word) * capacity);
    if (!s->data) {
        perror("malloc stack");
        exit(1);
    }
    s->capacity = capacity;
    s->top      = -1;
}

void stack_destroy(Stack *s) {
    free(s->data);
    s->data     = NULL;
    s->capacity = 0;
    s->top      = -1;
}

void stack_push(Stack *s, word v) {
    if (s->top + 1 >= s->capacity) {
        fprintf(stderr, "Stack overflow\n");
        exit(1);
    }
    s->data[++s->top] = v;
}

word stack_pop(Stack *s) {
    if (s->top < 0) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }
    return s->data[s->top--];
}

word stack_top(const Stack *s) {
    if (s->top < 0) {
        fprintf(stderr, "Stack is empty\n");
        exit(1);
    }
    return s->data[s->top];
}

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

  if (!m) return NULL;

  // Initialise all variables so cleanup is safe
  m->const_pool = NULL;
  m->const_pool_size = 0;
  m->text = NULL;
  m->text_size = 0;

  // Initialise stack and Program Counter
  m->pc = 0;
  stack_init(&m->stack, 1024);

  // Reserve 256 word slots at stack.data
  for (int i = 0; i < 256; i++) {
    stack_push(&m->stack, 0);
  }

  // Initialise main local variable frame pointer
  m->lv = 0;

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
    goto fail;
  }

  // Validate magic number
  uint32_t magic = read_uint32(buf);
  if (magic != 0x1DEADFAD) {
    fprintf(stderr, "%s: bad magic number: 0x%08x (expected 0x1DEADFAD)\n",
            binary_path, magic);
    goto fail;
  }

  // Skip 4 byte origin of constant pool
  if (fread(buf, 1, 4, f) != 4) {
    fprintf(stderr, "Error skipping const-pool origin in %s\n", binary_path);
    goto fail;
  }

  // Read const-pool size
  if (fread(buf, 1, 4, f) != 4) {
    fprintf(stderr, "Error reading const-pool size in %s\n", binary_path);
    goto fail;
  }

  // convert from big-endian file bytes to ensure endianness is correct
  m->const_pool_size = read_uint32(buf);

  // Allocate memory for the constant pool and read it
  m->const_pool = malloc(m->const_pool_size);
  if (!m->const_pool) {
    perror("malloc const_pool");
    goto fail;
  }
  if (fread(m->const_pool, 1, m->const_pool_size, f) != m->const_pool_size) {
    fprintf(stderr, "Error reading const-pool data in %s\n", binary_path);
    goto fail;
  }

  // Skip the 4 byte origin of text block
  if (fread(buf, 1, 4, f) != 4) {
    fprintf(stderr, "Error skipping text origin in %s\n", binary_path);
    goto fail;
  }

  // Read the 4 byte text size
  if (fread(buf, 1, 4, f) != 4) {
    fprintf(stderr, "Error reading text size in %s\n", binary_path);
    goto fail;
  }
  m->text_size = read_uint32(buf);

  // Allocate and read the text
  m->text = malloc(m->text_size);
  if (!m->text) {
    perror("malloc text");
    goto fail;
  }
  if (fread(m->text, 1, m->text_size, f) != m->text_size) {
    fprintf(stderr, "Error reading text data in %s\n", binary_path);
    goto fail;
  }

  fclose(f);

  return m;

fail:
  // cleanup on failure
  fclose(f);
  free(m->const_pool);
  free(m->text);
  free(m);
  return NULL;
}

void destroy_ijvm(ijvm* m) 
{
  stack_destroy(&m->stack);
  free(m);
}

byte *get_text(ijvm* m) 
{
  return m->text;
}

unsigned int get_text_size(ijvm* m) 
{
  return m->text_size;
}

word get_constant(ijvm* m,int i) 
{
  unsigned int offset = (unsigned int)i * 4;
  // Handle endianness and return value
  return (word)read_uint32(m->const_pool + offset);
}

unsigned int get_program_counter(ijvm* m) 
{
  return m->pc;
}

word tos(ijvm* m) 
{
  return stack_top(&m->stack);
}

bool finished(ijvm* m) 
{
  return m->pc >= m->text_size;
}

word get_local_variable(ijvm* m, int i) 
{
  // index into the current frame at lv + i
  return m->stack.data[m->lv + i];
}

void step(ijvm* m) 
{
  byte instruction = get_instruction(m);
  m->pc++;

  switch (instruction) {

    case OP_BIPUSH: {
      // Push next byte to stack
      int8_t next_byte = (int8_t) m->text[m->pc];
      m->pc++;
      stack_push(&m->stack, (word)next_byte);
      break;
    }

    case OP_IADD: {
      // Pop two numbers, add them, push result
      word val2 = stack_pop(&m->stack);
      word val1 = stack_pop(&m->stack);
      stack_push(&m->stack, val1 + val2);
      break;
    }

    case OP_ISUB: {
      // Subtract the top two values
      word val2 = stack_pop(&m->stack);
      word val1 = stack_pop(&m->stack);
      stack_push(&m->stack, val1 - val2);
      break;
    }

    case OP_DUP: {
      // Duplicate the top value
      word top_val = stack_top(&m->stack);
      stack_push(&m->stack, top_val);
      break;
    }

    case OP_IAND: {
      // AND of the top two values
      word val2 = stack_pop(&m->stack);
      word val1 = stack_pop(&m->stack);
      stack_push(&m->stack, val1 & val2);
      break;
    }

    case OP_IOR: {
      // OR of the top two values
      word val2 = stack_pop(&m->stack);
      word val1 = stack_pop(&m->stack);
      stack_push(&m->stack, val1 | val2);
      break;
    }

    case OP_NOP:
      // Do nothing
      break;

    case OP_POP:
      // Pop
      stack_pop(&m->stack);
      break;

    case OP_SWAP: {
      // Swap top two values
      word val2 = stack_pop(&m->stack);
      word val1 = stack_pop(&m->stack);
      stack_push(&m->stack, val2);
      stack_push(&m->stack, val1);
      break;
    }

    case OP_ERR:
      // Print error and halt
      fprintf(m->out, "Error\n");
      m->pc = m->text_size;
      break;

    case OP_IN: {
      // Read a byte and push it
      int c = fgetc(m->in);
      stack_push(&m->stack, (word)(c == EOF ? 0 : c));
      break;
    }

    case OP_OUT: {
      // Pop and print as character
      word val = stack_pop(&m->stack);
      fprintf(m->out, "%c", (char)val);
      break;
    }

    case OP_GOTO: {
      // Jump the specified number of bytes
      unsigned int orig_pc = m->pc - 1;
      int16_t offset = read_int16(&m->text[m->pc]);
      m->pc += 2;
      m->pc = orig_pc + offset;
      break;
    }

    case OP_IFEQ: {
      // Pop value and branch if == 0
      word val = stack_pop(&m->stack);
      unsigned int orig_pc = m->pc - 1;
      int16_t offset = read_int16(&m->text[m->pc]);
      m->pc += 2;
      if (val == 0) {
        m->pc = orig_pc + offset;
      }
      break;
    }

    case OP_IFLT: {
      // Pop value and branch if < 0
      word val = stack_pop(&m->stack);
      unsigned int orig_pc = m->pc - 1;
      int16_t offset = read_int16(&m->text[m->pc]);
      m->pc += 2;
      if (val < 0) {
        m->pc = orig_pc + offset;
      }
      break;
    }

    case OP_IF_ICMPEQ: {
      // Pop two values and branch if equal
      word val2 = stack_pop(&m->stack);
      word val1 = stack_pop(&m->stack);
      unsigned int orig_pc = m->pc - 1;
      int16_t offset = read_int16(&m->text[m->pc]);
      m->pc += 2;
      if (val1 == val2) {
        m->pc = orig_pc + offset;
      }
      break;
    }

    case OP_LDC_W: {
      // Push the constant at index onto the stack
      uint16_t index = read_uint16(m->text + m->pc);
      m->pc += 2;
      word v = get_constant(m, index);
      stack_push(&m->stack, v);
      break;
    }

    case OP_ISTORE: {
      // Take a single byte index to store a value in the local variable
      uint8_t index = m->text[m->pc++];
      word value = stack_pop(&m->stack);
      m->stack.data[m->lv + index] = value;
      break;
    }

    case OP_ILOAD: {
      // Take a single byte index to load a local variable
      uint8_t index = m->text[m->pc++];
      word value = get_local_variable(m, index);
      stack_push(&m->stack, value);
      break;
    }

    case OP_IINC: {
      // first byte: local variable index. second byte: signed increment
      uint8_t index = m->text[m->pc++];
      int8_t increment = (int8_t)m->text[m->pc++];
      m->stack.data[m->lv + index] += increment;
      break;
    }

    case OP_WIDE: {
      // Consume next opcode and handle it with a 16 bit index
      byte wide_op = m->text[m->pc++];
      switch (wide_op) {
        case OP_ILOAD: {
          uint16_t index = read_uint16(m->text + m->pc);
          m->pc += 2;
          stack_push(&m->stack, get_local_variable(m, index));
          break;
        }
        case OP_ISTORE: {
          uint16_t index = read_uint16(m->text + m->pc);
          m->pc += 2;
          word v = stack_pop(&m->stack);
          m->stack.data[m->lv + index] = v;
          break;
        }
        case OP_IINC: {
          uint16_t index = read_uint16(m->text + m->pc);
          m->pc += 2;
          int8_t delta = (int8_t)m->text[m->pc++];
          m->stack.data[m->lv + index] += delta;
          break;
        }
        default:
          fprintf(stderr, "Invalid WIDE opcode 0x%02x at pc=%u\n", wide_op, m->pc-1);
          exit(1);
      }
      break;
  }

    case OP_HALT:
      // Increase pc to text size so finished() returns true
      m->pc = m->text_size;
      break;


    default:
        fprintf(stderr, "Unimplemented opcode 0x%02x at pc=%u\n", instruction, m->pc-1);
        exit(1);
  }
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
