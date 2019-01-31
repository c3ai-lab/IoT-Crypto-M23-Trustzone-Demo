#ifndef __MAIN_H__
#define __MAIN_H__

#include <arm_cmse.h>
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"
#include "partition_M2351.h"

#define NEXT_BOOT_BASE  0x10040000		// Non secure vector table base address
#define JUMP_HERE       0xe7fee7ff    // Non secure fallback pointer 

typedef enum { false, true } bool;

/* typedef for NonSecure callback functions */
typedef __NONSECURE_CALL int32_t (*NonSecure_funcptr)(uint32_t);

void encrypt(void);
void decrypt(void);
void init_crypto(void);
void init_non_secure(void);
void init_system(void);
void init_debug_port(void);
void print_hex(char * buff, int size);
void hex_to_byte(char* str, uint8_t* out, int out_size);

void itsc_startup();

#endif //__MAIN_H__