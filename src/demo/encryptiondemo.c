#include "../utils/encryption_utils.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include "clogger.h"


int encrypted_chunk_callback (unsigned char * buffer, int buffer_length, void * user_data){

    /*
        The caller can process the chunk and copy the remaining data into user_data pointer.
        The remaining data will be available on the next invokation for further processing
    */

    //Print chunk decrypted with added null terminating character (DEBUG)
    unsigned char null_term_buffer[buffer_length+1];
    memcpy(null_term_buffer, buffer, buffer_length);
    null_term_buffer[buffer_length] = '\0';
    C_DEBUG("Chunk decrypted '%s'",null_term_buffer);

    //Return false if processing failed to abort
    return true;
}

int main(int argc, char *argv[]){
    c_log_set_level(C_ALL_E);

    char cwd[PATH_MAX];
    char * file_path = NULL;
    if(argc <= 1){
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            strcat(cwd,"/encrypted.bin");
            C_WARN("No destination provided.\n\tDefaulting to : %s",cwd);
            file_path = cwd;
        } else {
            C_FATAL("No destination provided and unable to determine current working directory.");
            exit(1);
        }
    } else if(argc > 2){
        C_FATAL("Invalid number of arguments. Please simply provide a valid file location to write to. Args count :%d", argc);
        exit(1);
    } else if(argv[1] && strlen(argv[1]) > 1) {
        file_path = argv[1];
    }

    unsigned char * pass = (unsigned char *) "SomeSuperSecretKey(Can be non-printable characters)";
    int pass_len = strlen((char*)pass);

    char * plain_data = "0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz";

    int plain_data_len = strlen(plain_data)+1;
    EncryptionUtils__write_encrypted(pass, pass_len, (unsigned char *) plain_data, plain_data_len,file_path);

    int decrypted_data_len = 0;
    EncryptionUtils__read_encrypted(pass,pass_len,&decrypted_data_len,file_path,encrypted_chunk_callback, NULL);

    // RAND_poll(); //Create new seed
    // unsigned char salt[ENCRYPTION_UTILS_SALT_MAX_LENGTH];
    // RAND_bytes(salt, ENCRYPTION_UTILS_SALT_MAX_LENGTH);

    // C_DEBUG("plain_data %d - %s\n",plain_data_len, plain_data);
    // unsigned char encrypted_data[1024];
    // int encrypted_data_len = EncryptionUtils__encrypt(pass, pass_len, (unsigned char *) plain_data, plain_data_len, salt, encrypted_data);

    // C_DEBUG("encrypted_data_len %d\n",encrypted_data_len);
    // unsigned char decrypted_data[1024];
    // int decrypted_data_len = EncryptionUtils__decrypt(pass,pass_len,encrypted_data,encrypted_data_len, salt, decrypted_data);
    // C_DEBUG("decrypted_data %d - %s\n",decrypted_data_len,decrypted_data);
}

