#include "Base64.h"
#include "mbedtls/aes.h"

class EncryptionContext {
  public:
    EncryptionContext() {
      LOG0("Initializing Encryption Context\n");
      mbedtls_aes_init(&aes);
      mbedtls_aes_setkey_enc(&aes, (unsigned char*)nello_aes_key, 256);
      LOG0("Encryption Context ready\n");
    }

    String encrypt(String plain_data){   
      int i;
      int len = plain_data.length();
      int numberOfPads = block_size - len % block_size;
      int targetLen = len + numberOfPads;
      uint8_t data[targetLen];
      memcpy(data, plain_data.c_str(), len);
      for(i = len; i < targetLen; i++){
        data[i] = 0;
      }
      uint8_t iv[iv_size];
      memcpy(iv, nello_aes_iv, iv_size);

      unsigned char encrypt_output[targetLen] = {0};
      mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, targetLen, iv, data, encrypt_output);
      char encoded_data[ base64_enc_len(targetLen) ];
      base64_encode(encoded_data, (char *)encrypt_output, targetLen);
      LOG0("Encrypting ");
      LOG0(plain_data.c_str());
      LOG0(" -> ");   
      LOG0(encoded_data);
      LOG0("\n");  
      return String(encoded_data);
    }

    String decrypt(String encoded_data_str){  
      int input_len = encoded_data_str.length();
      char *encoded_data = const_cast<char*>(encoded_data_str.c_str());
      int len = base64_dec_len(encoded_data, input_len);
      uint8_t data[ len ];
      base64_decode((char *)data, encoded_data, input_len);
      
      uint8_t iv[iv_size];
      memcpy(iv, nello_aes_iv, iv_size);

      unsigned char decrypt_output[len] = {0};
      mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, len, iv, data, decrypt_output);
      LOG0("Decrypting ");
      LOG0(encoded_data_str.c_str());
      LOG0(" -> ");      
      LOG0((char*)decrypt_output);
      LOG0("\n");   
      return String((char*)decrypt_output);
    }

  private:
    int block_size = 16;
    int iv_size = 16;
    mbedtls_aes_context aes;
};