#include "encryption_utils.h"
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <math.h>
#include "clogger.h"

#ifndef ENCRYPTION_UTILS_PLAIN_BUFFER_SIZE
    #define ENCRYPTION_UTILS_PLAIN_BUFFER_SIZE 32
#endif
#if(ENCRYPTION_UTILS_PLAIN_BUFFER_SIZE < AES_BLOCK_SIZE)
    #error "ENCRYPTION_UTILS_PLAIN_BUFFER_SIZE < AES_BLOCK_SIZE"
#endif
#if((ENCRYPTION_UTILS_PLAIN_BUFFER_SIZE > 0 & (ENCRYPTION_UTILS_PLAIN_BUFFER_SIZE - 1)) == 0)
    #error "ENCRYPTION_UTILS_PLAIN_BUFFER_SIZE isn't a power of 2"
#endif

#define ENCRYPTION_UTILS_ENC_BUFFER_SIZE ENCRYPTION_UTILS_PLAIN_BUFFER_SIZE+AES_BLOCK_SIZE
#define DUMP_PREFIX "--------------- %s ----------------\n"
#define DUMP_SUFIX "-----------------------------------------------"

static void 
printHex(char * title, unsigned char * data, int length){
    int lines = ceil((double)length/(double)16);
    //Extra 64bytes for title buffer
    char dump[length*3 + lines + strlen(DUMP_PREFIX) + strlen(DUMP_SUFIX)+1 + 64];
    sprintf(dump,DUMP_PREFIX, title);
    for(int a = 1; a<=length;a++){
        sprintf(&dump[strlen(dump)],"%.2x ", data[a-1]); //Format hex
        if (!(a % 16)) strcat(dump,"\n"); //Newline at every 16th hex
    }
    if (length % 16)  strcat(dump,"\n"); //Add teriminal newline if the last line isn't complete
    strcat(dump,DUMP_SUFIX);
    C_TRAIL("%s",dump);
}

static EVP_CIPHER_CTX* 
EncryptionUtils__new_context(unsigned char *key_data, int key_data_len, unsigned char * salt, int encrypt){
    int i, round = 5;
    unsigned char key[32], iv[32];
    
    i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), salt, key_data, key_data_len, round, key, iv);
    if (i != 32) {
        C_ERROR("Key size is %d bits - should be 256 bits", i);
        return NULL;
    }

    EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();
    if(encrypt){
        EVP_CIPHER_CTX_init(context);
        EVP_EncryptInit_ex(context, EVP_aes_256_cbc(), NULL, key, iv);
    } else {
        EVP_CIPHER_CTX_init(context);
        EVP_DecryptInit_ex(context, EVP_aes_256_cbc(), NULL, key, iv);
    }

    return context;
}

int 
EncryptionUtils__encrypt(unsigned char * pass, 
                                        int pass_len,
                                         unsigned char * plain_data, 
                                         int plain_data_length, 
                                         unsigned char * salt,
                                         unsigned char * encrypted_data){
    EVP_CIPHER_CTX* context = EncryptionUtils__new_context(pass, pass_len, salt,1);
    if (!context) {
        C_ERROR("Failed to initialize AES cipher\n");
        return -1;
    }

    int encrypted_data_length = plain_data_length + AES_BLOCK_SIZE;
    int tmp_len = 0;
    EVP_EncryptInit_ex(context, NULL, NULL, NULL, NULL);
    EVP_EncryptUpdate(context, encrypted_data, &encrypted_data_length, plain_data, plain_data_length);
    EVP_EncryptFinal_ex(context, encrypted_data+encrypted_data_length, &tmp_len);

    printHex("Encrypted Chunk", encrypted_data, encrypted_data_length);

    EVP_CIPHER_CTX_cleanup(context);

    return encrypted_data_length + tmp_len;
}

int 
EncryptionUtils__decrypt(unsigned char * pass, 
                                        int pass_len, 
                                        unsigned char * encrypted_data, 
                                        int encrypted_data_length, 
                                        unsigned char * salt,
                                        unsigned char * decrypted_data){
    EVP_CIPHER_CTX* context = EncryptionUtils__new_context(pass, pass_len, salt,0);
    if (!context) {
        C_ERROR("Failed to initialize AES cipher\n");
        return 0;
    }

    int decrypted_data_len;
    int tmp_len = 0;
    EVP_DecryptInit_ex(context, NULL, NULL, NULL, NULL);
    EVP_DecryptUpdate(context, decrypted_data, &decrypted_data_len, encrypted_data, encrypted_data_length);
    EVP_DecryptFinal_ex(context, decrypted_data+decrypted_data_len, &tmp_len);

    EVP_CIPHER_CTX_cleanup(context);

    return decrypted_data_len + tmp_len;
}

int EncryptionUtils__write_encrypted(unsigned char * pass, 
                                        int pass_len, 
                                        unsigned char * plain_data, 
                                        int plain_data_len, 
                                        char * path){
    C_DEBUG("Writing encrypted data to '%s'",path);
    int encrypted_data_len = 0;
    FILE * fptr = fopen(path,"w");
    if(fptr != NULL){
        RAND_poll(); //Create new seed
        unsigned char salt[ENCRYPTION_UTILS_SALT_MAX_LENGTH];
        RAND_bytes(salt, ENCRYPTION_UTILS_SALT_MAX_LENGTH);

        //Write salt value
        fwrite(salt,ENCRYPTION_UTILS_SALT_MAX_LENGTH,1,fptr);

        unsigned char encrypted_data[plain_data_len + AES_BLOCK_SIZE];
        int to_encrypt_len = plain_data_len;
        if (to_encrypt_len > ENCRYPTION_UTILS_PLAIN_BUFFER_SIZE) to_encrypt_len=ENCRYPTION_UTILS_PLAIN_BUFFER_SIZE;

        int number_of_chunks = ceil((double)plain_data_len/(double)ENCRYPTION_UTILS_PLAIN_BUFFER_SIZE);
        int total_data_encrypted = 0;
        C_TRACE("Number of encrypted chunks : %d",number_of_chunks);
        for(int i=0;i<number_of_chunks;i++){
            //Handling last chunk that doesn't fill buffer
            if(plain_data_len - total_data_encrypted < ENCRYPTION_UTILS_PLAIN_BUFFER_SIZE) to_encrypt_len = plain_data_len - total_data_encrypted;
            //Encrypt the chunk
            encrypted_data_len = EncryptionUtils__encrypt(pass, pass_len, (unsigned char *) &plain_data[total_data_encrypted], to_encrypt_len, salt, encrypted_data);
            //Write to fil

            if(fwrite(encrypted_data,1,encrypted_data_len,fptr) == 0){
                C_FATAL("Failed to write encypted data to file : Total encypted %d",total_data_encrypted);
                total_data_encrypted = 0;
                goto abort;
            }
            total_data_encrypted += to_encrypt_len;;
        }
        
abort:
        fclose(fptr);
        return total_data_encrypted;
    } else {
        C_ERROR("Failed to write to encrypted setting file! '%s'",path);
        return 0;
    }
}

void EncryptionUtils__read_encrypted(unsigned char * pass, 
                                                int pass_len,
                                                int * decripted_data_len,
                                                char * path,
                                                int (*callback) (unsigned char * buffer, int buffer_length, void * user_data),
                                                void * user_data){
    if(!callback){
        C_ERROR("callback is NULL");
        return;
    }

    char salt[ENCRYPTION_UTILS_SALT_MAX_LENGTH];
    unsigned char encrypted_data[ENCRYPTION_UTILS_ENC_BUFFER_SIZE];
    unsigned char decrypted_data[ENCRYPTION_UTILS_PLAIN_BUFFER_SIZE];
    FILE * fptr = fopen(path,"r");
    if(fptr != NULL){
        //Extract salt
        if(!fread(&salt, 1, ENCRYPTION_UTILS_SALT_MAX_LENGTH, fptr)){
            C_ERROR("Failed to read encrypted setting file! '%s'",path);
            goto drop;
        }
        
        int data_read = 0;
        while((data_read = fread(&encrypted_data, 1, ENCRYPTION_UTILS_ENC_BUFFER_SIZE, fptr)) != 0){
            *decripted_data_len = EncryptionUtils__decrypt(pass,pass_len,encrypted_data,data_read, (unsigned char *) salt, decrypted_data);
            printHex("Decrypted Chunk", decrypted_data, *decripted_data_len);
            if(!callback(decrypted_data, *decripted_data_len, user_data)){
                C_WARN("Decryption aborted by called.");
                goto drop;
            }
        }
drop:
        fclose(fptr);
    } else {
        C_ERROR("Failed to read encrypted setting file! '%s'",path);
        return;
    }
}