#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAGIC_SIGNATURE "BENDER01"
#define MAGIC_SIZE 8
#define MAGIC_XOR_KEY 0x7F


void encrypt_data(unsigned char* data, int size, unsigned char key) {
    for (int i = 0; i < size; i++) {
        data[i] ^= key ^ (i & 0xFF);
    }
}


int copy_encrypt_file_content(FILE* dest, const char* src_path, unsigned char encrypt_key) {
    FILE* src = fopen(src_path, "rb");
    if (!src) {
        printf("Error: Cannot open source file '%s'\n", src_path);
        return 0;
    }
    
    fseek(src, 0, SEEK_END);
    long size = ftell(src);
    fseek(src, 0, SEEK_SET);
    
    unsigned char* buffer = malloc(size);
    if (!buffer) {
        printf("Error: Memory allocation failed\n");
        fclose(src);
        return 0;
    }
    
    fread(buffer, 1, size, src);
    

    encrypt_data(buffer, size, encrypt_key);
    
    fwrite(buffer, 1, size, dest);
    
    free(buffer);
    fclose(src);
    return size;
}


int bind_files(const char* file1_path, const char* file2_path, const char* output_path) {

    char exe_path[MAX_PATH];
    GetModuleFileName(NULL, exe_path, sizeof(exe_path));
    

    char* last_slash = strrchr(exe_path, '\\');
    if (last_slash) {
        *last_slash = '\0';
    }
    

    char stub_path[MAX_PATH];
    snprintf(stub_path, sizeof(stub_path), "%s\\stub.exe", exe_path);
    

    if (GetFileAttributes(stub_path) == INVALID_FILE_ATTRIBUTES) {
        printf("Error: stub.exe not found at '%s'\n", stub_path);
        printf("Please make sure stub.exe is in the same directory as binder.exe\n");
        return 0;
    }
    

    if (GetFileAttributes(file1_path) == INVALID_FILE_ATTRIBUTES) {
        printf("Error: File 1 '%s' not found\n", file1_path);
        return 0;
    }
    
    if (GetFileAttributes(file2_path) == INVALID_FILE_ATTRIBUTES) {
        printf("Error: File 2 '%s' not found\n", file2_path);
        return 0;
    }
    

    printf("Copying stub executable...\n");
    if (!CopyFile(stub_path, output_path, FALSE)) {
        printf("Error: Failed to create output file '%s'\n", output_path);
        return 0;
    }
    

    FILE* output = fopen(output_path, "ab");
    if (!output) {
        printf("Error: Failed to open output file for writing\n");
        return 0;
    }
    

    FILE* f1 = fopen(file1_path, "rb");
    FILE* f2 = fopen(file2_path, "rb");
    
    if (!f1 || !f2) {
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        fclose(output);
        printf("Error: Failed to open input files\n");
        return 0;
    }
    
    fseek(f1, 0, SEEK_END);
    unsigned int file1_size = ftell(f1);
    fseek(f2, 0, SEEK_END);
    unsigned int file2_size = ftell(f2);
    
    fclose(f1);
    fclose(f2);
    
    printf("Encrypting and binding file 1 (%u bytes)...\n", file1_size);
    copy_encrypt_file_content(output, file1_path, 0x42);
    
    printf("Encrypting and binding file 2 (%u bytes)...\n", file2_size);
    copy_encrypt_file_content(output, file2_path, 0x24);
    

    unsigned int encrypted_file1_size = file1_size ^ 0x12345678;
    unsigned int encrypted_file2_size = file2_size ^ 0x87654321;
    

    fwrite(&encrypted_file1_size, 4, 1, output);
    fwrite(&encrypted_file2_size, 4, 1, output);
    
   
    unsigned char encrypted_magic[MAGIC_SIZE];
    for (int i = 0; i < MAGIC_SIZE; i++) {
        encrypted_magic[i] = MAGIC_SIGNATURE[i] ^ MAGIC_XOR_KEY;
    }
    fwrite(encrypted_magic, 1, MAGIC_SIZE, output);
    
    fclose(output);
    
    printf("Files bound successfully! Output: '%s'\n", output_path);
    return 1;
}

void print_usage(const char* program_name) {
    printf("Bender - File Binding Tool (Command Line)\n");
    printf("Usage: %s <file1> <file2> <output.exe>\n\n", program_name);
    printf("  file1    - First file to bind (typically an executable)\n");
    printf("  file2    - Second file to bind (any file type)\n");
    printf("  output   - Output executable filename\n\n");
    printf("Example:\n");
    printf("  %s myapp.exe readme.txt bound_app.exe\n", program_name);
}

int main(int argc, char* argv[]) {
    printf("Bender File Binder v1.0\n");
    printf("========================\n\n");
    
    if (argc != 4) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* file1_path = argv[1];
    const char* file2_path = argv[2];
    const char* output_path = argv[3];
    
    printf("File 1: %s\n", file1_path);
    printf("File 2: %s\n", file2_path);
    printf("Output: %s\n\n", output_path);
    
    if (bind_files(file1_path, file2_path, output_path)) {
        printf("\nBinding completed successfully!\n");
        return 0;
    } else {
        printf("\nBinding failed!\n");
        return 1;
    }
} 