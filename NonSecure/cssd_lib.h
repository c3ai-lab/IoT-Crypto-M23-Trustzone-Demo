#ifndef __CSSD_LIB_H__
#define __CSSD_LIB_H__

/*----------------------------------------------------------------------------
  NonSecure Callable Functions from Secure Region
 *----------------------------------------------------------------------------*/
extern int32_t S_get_secure_value(void);
extern int32_t S_get_cipher_value(void);
extern int32_t S_encrypt_secure_value(void);
extern int32_t S_decrypt_cipher_value(void);

#endif //__CSSD_LIB_H__
