#include "../include/gui.h"
#include "../include/mp3player.h"
#include <windows.h>
#include <locale.h>

// Entry point per l'applicazione GUI
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Percorso predefinito per la libreria musicale
    char library_path[MAX_PATH_LENGTH] = ".";
    
    // Se viene fornito un percorso come argomento, usalo
    if (lpCmdLine && *lpCmdLine) {
        strncpy(library_path, lpCmdLine, MAX_PATH_LENGTH - 1);
        library_path[MAX_PATH_LENGTH - 1] = '\0'; // Assicura terminazione
    }
    
    // Creazione della libreria MP3
    MP3Library* library = create_library(library_path);
    if (!library) {
        MessageBox(NULL, "Error: unable to create MP3 library.", "Error", MB_ICONERROR);
        return 1;
    }
    
    // Scansione iniziale della directory
    int found_files = scan_directory(library, library_path, TRUE);
    
    if (found_files == 0) {
        char message[512];
        sprintf(message, "No MP3 files found in directory %s.\n"
                         "The application will start anyway.", library_path);
        MessageBox(NULL, message, "Information", MB_ICONINFORMATION);
    }
    
    // Avvia l'interfaccia grafica
    int result = start_gui(hInstance, library);
    
    // Pulizia della memoria
    free_mp3_library(library);
    
    return result;
}

// Funzione per avviare la versione GUI dall'interfaccia a linea di comando
int start_gui_from_cli(MP3Library* library) {
    // Imposta la codepage per la visualizzazione dei caratteri accentati
    SetConsoleOutputCP(65001); // UTF-8 code page
    SetConsoleCP(65001);       // Input code page
    setlocale(LC_ALL, "it_IT.UTF-8");
    
    // Questa funzione viene chiamata dal main.c per avviare l'interfaccia grafica
    // e poi tornare all'interfaccia a linea di comando
    
    HINSTANCE hInstance = GetModuleHandle(NULL);
    return start_gui(hInstance, library);
} 