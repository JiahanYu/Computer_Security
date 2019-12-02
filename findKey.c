#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/evp.h>

/* Message to be encrypted */
char plaintext[] = "This is a top secret.";
char ciphertext[] = "8d20e5056a8d24d0462ce74e4904c1b513e10d1df4a2ef2ad4540fae1ca0aaf9";
unsigned char iv[16] = { 0 } ;

int convert_hex(unsigned char *, int, FILE *); // get the hex

int main() {
	char tmpKey[1024];
	FILE *letters, *OF;

	unsigned char outbuf[1024 + EVP_MAX_BLOCK_LENGTH]; // output buffer
	int OL, tmplen, i;
	EVP_CIPHER_CTX ctx;

	letters = fopen("words.txt", "r");
	OF = fopen("ciphertext.txt", "w+");
	if(tmpKey<0 || OF<0) {
		perror ("Cannot open file");
		exit(1);
	}

	EVP_CIPHER_CTX_init(&ctx);

	// Try each word in words.txt as the key, with space characters appended 
	// on the end up to 16 chars, if encrypted text matches given_ciphertext, 
	// then stop and that key is the match!   
	while (fgets(tmpKey,16,letters)) {
		if(strlen(tmpKey)>=16) continue;
		// Padding the word by appending space characters at the end
		// also remove the '\n' at the end of the word
		for(i=strlen(tmpKey)-1;i<16;i++) tmpKey[i]=' ';
		tmpKey[i] = '\0';

		// Set up cipher context ctx for encryption 
		// with the cipher type aes-128-cbc
	    EVP_EncryptInit_ex(&ctx, EVP_aes_128_cbc(), NULL, tmpKey, iv);

	    // Provide the message to be encrypted, 
	    // and obtain the encrypted output.
		if(!EVP_EncryptUpdate(&ctx, outbuf, &OL, plaintext, strlen(plaintext))) {
			EVP_CIPHER_CTX_cleanup(&ctx);
			return 0;
		}
  		// Finalise the encryption. Further ciphertext bytes may be written at
   		// this stage.
		if(!EVP_EncryptFinal_ex(&ctx, outbuf + OL, &tmplen)) {
			EVP_CIPHER_CTX_cleanup(&ctx);
			return 0;
		}
		OL += tmplen;
        
        // Compare the ciphertext with the given ciphertext
		if(convert_hex(outbuf, OL, OF)) {
			printf("Key found! It is: %s\n",tmpKey);
			break;
		}
	}

	fclose(letters);
	fclose(OF);

	return 1;
}

int convert_hex(unsigned char *buf, int length, FILE *OF) {
    unsigned char buffer[1024]="";
    unsigned char *pbuffer = buffer;
	int i;

	for (i=0; i<length; i++) {
		fprintf(OF,"%02x",buf[i]);
		sprintf(pbuffer, "%02x", buf[i]);
		pbuffer+=2;
	}
	fprintf(OF,"%c",'\n');

	// Compare the ciphertexts and return true(1) if same
    if(!strcmp(buffer, ciphertext)) return 1;
	
	return 0;
}