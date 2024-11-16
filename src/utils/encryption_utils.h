#ifndef ENCRYPTION_UTILS_H_ 
#define ENCRYPTION_UTILS_H_

#include <linux/limits.h>

#ifndef ENCRYPTION_UTILS_SALT_MAX_LENGTH
    #define ENCRYPTION_UTILS_SALT_MAX_LENGTH 16
#endif
#if(ENCRYPTION_UTILS_SALT_MAX_LENGTH > 16)
    #error "ENCRYPTION_UTILS_SALT_MAX_LENGTH > 16"
#endif

void EncryptionUtils__printHex(char * title, unsigned char * data, int length);
void EncryptionUtils__printReadable(unsigned char * data, int length, char * title);

int EncryptionUtils__encrypt(unsigned char * pass, 
                                        int pass_len, 
                                        unsigned char * decrypted_data, 
                                        int decrypted_data_length,
                                        unsigned char * salt,
                                        unsigned char * encrypted_data);

int EncryptionUtils__decrypt(unsigned char * pass, 
                                        int pass_len,
                                        unsigned char * encrypted_data, 
                                        int encrypted_data_length, 
                                        unsigned char * salt,
                                        unsigned char * decrypted_data);
                                         
int EncryptionUtils__write_encrypted(unsigned char * pass, 
                                        int pass_len, 
                                        unsigned char * data, 
                                        int data_len, 
                                        char * path);

int EncryptionUtils__read_encrypted(unsigned char * pass, 
                                        int pass_len, 
                                        char * path,
                                        int (*callback) (unsigned char * buffer, int buffer_length, void * user_data),
                                        void * user_data);

#endif