#include <stdint.h>  
#include <stdio.h>

int main(int argc, char** argv){
  uint8_t bytes[] = {0xAD, 0xDF, 0xEA, 0x1D};
  // bytes[0] is now 0xAD, bytes[1] is 0xDF etc.
  // Cast uint8_t pointer to a uint32_t pointer 
  uint32_t* wordpoint = (uint32_t *)bytes;
  // interpret above bytes as a word by dereferencing pointer
  uint32_t word = *wordpoint;
  int reversed = 0x1DEADFAD; // hex number where 1D is the most significant byte
  int straight = 0xADDFEA1D; // hex number where AD is the most significant byte
  if(word == reversed) {
    printf("This is a little endian machine\n");
  } else if (word == straight) {
    printf("This is a big endian machine\n");
  } else {
    printf("This machine defies any logic\n");
  }
}