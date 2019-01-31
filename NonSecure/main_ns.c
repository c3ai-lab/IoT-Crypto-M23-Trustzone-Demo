#include <arm_cmse.h>
#include "NuMicro.h"                    /* Device header */
#include "cssd_lib.h"                   /* Collaborative Secure Software Development Library header */

/*----------------------------------------------------------------------------
  NonSecure Functions from NonSecure Region
 *----------------------------------------------------------------------------*/
void display_menue(void)
{
	int select;

  printf("\n__________________________________________\n");
	printf("|   Trustzone AES Cryptography Demo      |\n");
	printf("__________________________________________\n");
	printf("Select: \n			1 - Encrypt\n			2 - Decrypt\n");
	select = fgetc(stdin);
	
	//clear stdin - otherwise next input will be skipped
	int c;
	while ((c = getchar()) != '\n' && c != EOF) { };
	
	if(select=='1')
	{
		S_get_secure_value();
		S_encrypt_secure_value();
	}
	else if(select=='2')
	{
		S_get_cipher_value();
		S_decrypt_cipher_value();
	}
}
/*----------------------------------------------------------------------------
  GPB IRQ Handler
 *----------------------------------------------------------------------------*/
void GPB_IRQHandler(void)
{
    /* To check if PB.0 (Button SW2) interrupt occurred */
    if(GPIO_GET_INT_FLAG(PB_NS, BIT0))
    {
				printf("Button 0 -SW2- pushed\n");
				GPIO_CLR_INT_FLAG(PB_NS, BIT0);
				display_menue();
		}
		if(GPIO_GET_INT_FLAG(PB_NS, BIT1))
    {
				printf("Button 1 -SW3- pushed\n");
				printf("Not used - get creative!\n");
				GPIO_CLR_INT_FLAG(PB_NS, BIT1);			
		}
		else
    {
        /* Un-expected interrupt. Just clear all PB interrupts */
        PB_NS->INTSRC = PB_NS->INTSRC;
    }
}


void GPIO_INT_Init(void)
{
		printf("\n");
		printf("GPI0 INTERRUPT INIT ......\n");
		printf("\n");

    /* Configure PB.0 as Input mode and enable interrupt by rising edge trigger */
    GPIO_SetMode(PB_NS, BIT0, GPIO_MODE_INPUT);
		GPIO_SetMode(PB_NS, BIT1, GPIO_MODE_INPUT);
    GPIO_EnableInt(PB_NS, 0, GPIO_INT_RISING);
		GPIO_EnableInt(PB_NS, 1, GPIO_INT_RISING);
    NVIC_EnableIRQ(GPB_IRQn);

    /* Enable interrupt de-bounce function and select de-bounce sampling cycle time is 1024 clocks of LIRC clock */
    GPIO_SET_DEBOUNCE_TIME(PB_NS, GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_1024);
    GPIO_ENABLE_DEBOUNCE(PB_NS, BIT0);
		GPIO_ENABLE_DEBOUNCE(PB_NS, BIT1);
	
		printf("GPI0 INTERRUPT INIT SUCCESSFULLY COMPLETED!\n");
}

/*----------------------------------------------------------------------------
  Main function
 *----------------------------------------------------------------------------*/
int main(void)
{		
		printf("Non-secure code is running...\n");
	
		GPIO_INT_Init();

    while(1);
}
