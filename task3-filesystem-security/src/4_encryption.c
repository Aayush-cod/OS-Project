#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Encrypts (or decrypts - XOR is symmetric) a buffer in place, using
// a repeating key. Each byte of data is XOR'd with the corresponding
// byte of the key, cycling the key if data is longer than the key.
void xor_cipher(unsigned char* data, int data_len, const char* key) {
    int key_len = strlen(key);
    for (int i = 0; i < data_len; i++) {
        data[i] = data[i] ^ key[i % key_len];
    }
}

// Reads a plaintext file, encrypts its contents, and writes the
// result to a new file (typically with a .enc extension).
int encrypt_file(const char* input_path, const char* output_path, const char* key) {
    FILE* in = fopen(input_path, "rb");
    if (!in) {
        printf("[ENCRYPT] Failed: could not open '%s'.\n", input_path);
        return 0;
    }

    fseek(in, 0, SEEK_END);
    long size = ftell(in);
    fseek(in, 0, SEEK_SET);

    unsigned char* buffer = malloc(size);
    fread(buffer, 1, size, in);
    fclose(in);

    xor_cipher(buffer, size, key);

    FILE* out = fopen(output_path, "wb");
    fwrite(buffer, 1, size, out);
    fclose(out);

    free(buffer);
    printf("[ENCRYPT] '%s' -> '%s' (%ld bytes encrypted)\n", input_path, output_path, size);
    return 1;
}

// Reads an encrypted file and decrypts it back to the original
// plaintext (XOR-ing again with the same key reverses the operation).
int decrypt_file(const char* input_path, const char* output_path, const char* key) {
    FILE* in = fopen(input_path, "rb");
    if (!in) {
        printf("[DECRYPT] Failed: could not open '%s'.\n", input_path);
        return 0;
    }

    fseek(in, 0, SEEK_END);
    long size = ftell(in);
    fseek(in, 0, SEEK_SET);

    unsigned char* buffer = malloc(size);
    fread(buffer, 1, size, in);
    fclose(in);

    xor_cipher(buffer, size, key); // same operation reverses it

    FILE* out = fopen(output_path, "wb");
    fwrite(buffer, 1, size, out);
    fclose(out);

    free(buffer);
    printf("[DECRYPT] '%s' -> '%s' (%ld bytes decrypted)\n", input_path, output_path, size);
    return 1;
}

// Prints a file's contents, showing raw bytes as-is - used here to
// visually demonstrate that the encrypted file is unreadable garbage.
void print_file_raw(const char* path, const char* label) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        printf("Could not open '%s'.\n", path);
        return;
    }
    printf("%s: \"", label);
    int c;
    while ((c = fgetc(f)) != EOF) {
        // print printable characters normally, show others as [xx] hex
        if (c >= 32 && c <= 126) printf("%c", c);
        else printf("[%02x]", c);
    }
    printf("\"\n");
    fclose(f);
}

int main() {
    const char* key = "S3cretKey";

    // Create a "sensitive" plaintext file
    FILE* f = fopen("salary.txt", "w");
    fprintf(f, "Employee: Alice, Salary: $95000");
    fclose(f);

    printf("--- Before encryption ---\n");
    print_file_raw("salary.txt", "salary.txt (plaintext)");

    printf("\n--- Encrypting ---\n");
    encrypt_file("salary.txt", "salary.txt.enc", key);
    print_file_raw("salary.txt.enc", "salary.txt.enc (ciphertext)");

    printf("\n--- Decrypting ---\n");
    decrypt_file("salary.txt.enc", "salary.txt.dec", key);
    print_file_raw("salary.txt.dec", "salary.txt.dec (recovered plaintext)");

    printf("\n--- Attempting decryption with WRONG key ---\n");
    decrypt_file("salary.txt.enc", "salary.txt.wrongkey", "WrongKey");
    print_file_raw("salary.txt.wrongkey", "salary.txt.wrongkey (garbage - wrong key)");

    return 0;
}