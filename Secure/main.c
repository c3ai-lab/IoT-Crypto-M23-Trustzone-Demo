#include "main.h"

uint8_t secret_value[256];
uint8_t cipher_value[1024];
bool is_encrypt; 											//true = encrypt, false = decrypt

uint8_t aes_key[32];									// AES 128 secure key

/* AES 128 initial vector 
* TODO: randomize the IV for best security of AES
*/
uint32_t aes_iv[4] = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 };


/*----------------------------------------------------------------------------
  Secure functions exported to NonSecure application
  Must place in Non-secure Callable
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
Secure API: Read secure value from STDIN
 *----------------------------------------------------------------------------*/
__NONSECURE_ENTRY
int32_t S_get_secure_value(void)
{
	printf("Enter Secret Value: \n");
	fgets((char *)secret_value, sizeof(secret_value), stdin);
	printf("Secret Value:%s \n",secret_value);

	return 0;
}

/*----------------------------------------------------------------------------
Secure API: Read cipher value from STDIN
 *----------------------------------------------------------------------------*/
__NONSECURE_ENTRY
int32_t S_get_cipher_value(void)
{
	char input[sizeof(cipher_value)];
	
	printf("Enter Ciphertext: \n");
	fgets((char *)input, sizeof(input), stdin);
	hex_to_byte(input, cipher_value, sizeof(cipher_value));

	return 0;
}

/*----------------------------------------------------------------------------
Secure API: Encrypt stored secure value
 *----------------------------------------------------------------------------*/
__NONSECURE_ENTRY
int32_t S_encrypt_secure_value(void)
{
	encrypt();
	return 0;
}

/*----------------------------------------------------------------------------
Secure API: Decrypt stored cipher value
 *----------------------------------------------------------------------------*/
__NONSECURE_ENTRY
int32_t S_decrypt_cipher_value(void)
{
	decrypt();
	return 0;
}

/*----------------------------------------------------------------------------
  Crypto Interrupt handler
 *----------------------------------------------------------------------------*/
void CRPT_IRQHandler(void)
{
    if(AES_GET_INT_FLAG(CRPT))
    {
				printf("AES work done.\n\n");
			  if(is_encrypt) //Check crypto mode
				{
					printf("Cipher value = ");
					print_hex((char *)cipher_value, strlen((char *)cipher_value));
				}
				else
				{
					printf("Secret value = %s", secret_value);
				}
			  printf("\n");
        AES_CLR_INT_FLAG(CRPT);
    }
}


/*----------------------------------------------------------------------------
  Main function
 *----------------------------------------------------------------------------*/
int main(void)
{
		printf("Secure code is running...This cannot be seen, as the debug port isn't initialzed yet!\n");
    SYS_UnlockReg();

    init_system();

		init_debug_port(); //Configure UART as non-secure for debug in both secure and non-secure region -> partition_M2351.h
	
		printf("\nDebug port initialized!\n\n");
	
    itsc_startup();
	
		init_crypto();

		SYS_LockReg();
	
    init_non_secure(); //switch to non-secure world

    do
    {		
        __WFI();		// Wait For Interrupt
    }
    while(1);
}

/*---------------------------------------
*  AES-128 ECB mode encrypt
*---------------------------------------*/
void encrypt()
{
		is_encrypt = true;
    
    AES_Open(CRPT, 0, 1, AES_MODE_ECB, AES_KEY_SIZE_128, AES_IN_OUT_SWAP);
    AES_SetKey(CRPT, 0, (uint32_t*)aes_key, AES_KEY_SIZE_128);
		AES_SetInitVect(CRPT, 0, aes_iv);
		AES_SetDMATransfer(CRPT, 0, (uint32_t)secret_value, (uint32_t)cipher_value, strlen((char *)secret_value));
	
    /* Start AES Encrypt */
		printf("AES encrypt started.\n\n");
    AES_Start(CRPT, 0, CRYPTO_DMA_ONE_SHOT);
}

/*---------------------------------------
*  AES-128 ECB mode decrypt
*---------------------------------------*/
void decrypt()
{
	  is_encrypt = false;
		
		AES_Open(CRPT, 0, 0, AES_MODE_ECB, AES_KEY_SIZE_128, AES_IN_OUT_SWAP);
    AES_SetKey(CRPT, 0, (uint32_t*)aes_key, AES_KEY_SIZE_128);
    AES_SetInitVect(CRPT, 0, aes_iv);
    AES_SetDMATransfer(CRPT, 0, (uint32_t)cipher_value, (uint32_t)secret_value, strlen((char *)cipher_value));

    /* Start AES decrypt */
		printf("AES decrypt started.\n\n");
    AES_Start(CRPT, 0, CRYPTO_DMA_ONE_SHOT);
}

/*---------------------------------------
*  Initialize cryptography and generate key
*---------------------------------------*/
void init_crypto()
{
		NVIC_EnableIRQ(CRPT_IRQn); //Enable crypto hardware interrupt inside the Nested Vector Interrupt Controller
		AES_ENABLE_INT(CRPT);	
	
		int32_t nbits = 256;	// Generate random number
    BL_RNG_T rng;
	
	   /* Initial TRNG */
    BL_RandomInit(&rng, BL_RNG_PRNG | BL_RNG_LIRC32K);

		/* Generate random number for private key */
		BL_Random(&rng, aes_key, nbits / 8);
		
		// Here you can print the secret key .. for debug reasons
	  //printf("\nSecret key = ");
		//print_hex((char *) aes_key, sizeof(aes_key));
		//printf("\n");
}

/*---------------------------------------
*  Switch to normal world
*---------------------------------------*/
void init_non_secure(void)
{
    NonSecure_funcptr fp;

    /* SCB_NS.VTOR points to the Non-secure vector table base address. */
    SCB_NS->VTOR = NEXT_BOOT_BASE;

    /* 1st Entry in the vector table is the Non-secure Main Stack Pointer. */
    __TZ_set_MSP_NS(*((uint32_t *)SCB_NS->VTOR)); /* Set up MSP in Non-secure code */

    /* 2nd entry contains the address of the Reset_Handler (CMSIS-CORE) function */
    fp = ((NonSecure_funcptr)(*(((uint32_t *)SCB_NS->VTOR) + 1)));

    /* Clear the LSB of the function address to indicate the function-call
       will cause a state switch from Secure to Non-secure */
    fp = cmse_nsfptr_create(fp);

    /* Check if the Reset_Handler address is in Non-secure space */
    if(cmse_is_nsfptr(fp) && (((uint32_t)fp & 0xf0000000) == 0x10000000))
    {
        printf("Execute non-secure code ...\n");
        fp(0); /* Non-secure function call */
    }
    else
    {
        /* Something went wrong */
        printf("No code in non-secure region!\n");
        printf("CPU will halted at non-secure state\n");

        /* Set nonsecure MSP in nonsecure region */
        __TZ_set_MSP_NS(NON_SECURE_SRAM_BASE + 512);

        /* Try to halt in non-secure state (SRAM) */
        M32(NON_SECURE_SRAM_BASE) = JUMP_HERE;
        fp = (NonSecure_funcptr)(NON_SECURE_SRAM_BASE + 1);
        fp(0);

        while(1);
    }
}

/*---------------------------------------
*  Initialize system
*---------------------------------------*/
void init_system(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable PLL */
    CLK->PLLCTL = CLK_PLLCTL_128MHz_HIRC;

    /* Waiting for PLL stable */
    while((CLK->STATUS & CLK_STATUS_PLLSTB_Msk) == 0);

    /* Set HCLK divider to 2 */
    CLK->CLKDIV0 = (CLK->CLKDIV0 & (~CLK_CLKDIV0_HCLKDIV_Msk)) | 1;

    /* Switch HCLK clock source to PLL */
    CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLKSEL_Msk)) | CLK_CLKSEL0_HCLKSEL_PLL;

		/* Select IP clock source */
    CLK->CLKSEL1 = CLK_CLKSEL1_UART0SEL_HIRC;

    /* Enable IP clock */
    CLK->AHBCLK  |= CLK_AHBCLK_CRPTCKEN_Msk;
    CLK->APBCLK0 |= CLK_APBCLK0_UART0CKEN_Msk | CLK_APBCLK0_TMR0CKEN_Msk;
	
    /* Enable UART clock */
    //CLK->APBCLK0 |= CLK_APBCLK0_UART0CKEN_Msk;

    /* Select UART clock source */
    CLK->CLKSEL1 = (CLK->CLKSEL1 & (~CLK_CLKSEL1_UART0SEL_Msk)) | CLK_CLKSEL1_UART0SEL_HIRC;

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate PllClock, SystemCoreClock and CycylesPerUs automatically. */
    //SystemCoreClockUpdate();
    PllClock        = 128000000;           // PLL
    SystemCoreClock = 64000000 / 1;        // HCLK
    CyclesPerUs     = 64000000 / 1000000;  // For SYS_SysTickDelay()

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & (~(UART0_RXD_PB12_Msk | UART0_TXD_PB13_Msk))) | UART0_RXD_PB12 | UART0_TXD_PB13;
		
		GPIO_SetMode(PB_NS, BIT0, GPIO_MODE_INPUT); //Enable GPIO Port B Non-Secure
}

/*---------------------------------------
*  Initialize debug port
*---------------------------------------*/
void init_debug_port(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Configure UART and set UART Baudrate */
    DEBUG_PORT->BAUD = UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HIRC, 115200);
    DEBUG_PORT->LINE = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
}

/*---------------------------------------
*  Print byte buffer as hex
*---------------------------------------*/
void print_hex(char * buff, int size)
{
	for(int i = 0; i < size; i++)
	{
		printf("%02x", buff[i]);
	}
}

/*---------------------------------------
*  Convert hex string to byte buffer
*---------------------------------------*/
void hex_to_byte(char* str, uint8_t* out, int out_size)
{
	int o = 0;
	int len = strlen(str);
	if(len % 2 != 0) //string needs to be aligned by 2
		return;
	
	for(int i = 0; str[i] != 0 && o < out_size; i+=2) //read 2 characters and convert to single byte
	{
		
		sscanf(&str[i], "%2hhx", &out[o++]);
	}
}

void itsc_startup(void)
{
	printf("\n");
	printf(" __     ______   ______     ______    \n");
	printf("/\\ \\   /\\__  _\\ /\\  ___\\   /\\  ___\\   \n");
	printf("\\ \\ \\  \\/_/\\ \\/ \\ \\___  \\  \\ \\ \\____  \n");
	printf(" \\ \\_\\    \\ \\_\\  \\/\\_____\\  \\ \\_____\\ \n");
	printf("  \\/_/     \\/_/   \\/_____/   \\/_____/ \n");
  printf("          F L E N S B U R G         \n");
	
	printf("Push button SW2 to call the menue by the GPIO interrupt!\n\n");
}