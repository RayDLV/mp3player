#include "../include/gui.h"
#include "../include/mp3player.h"
#include "../include/memory.h"
#include "../include/settings.h"
#include <windows.h>
#include <locale.h>

// Global settings
static UserSettings g_settings;

// Entry point per l'applicazione GUI
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize memory tracking
    mem_init();
    
    // Initialize and load user settings
    settings_init(&g_settings);
    settings_load(&g_settings, DEFAULT_SETTINGS_FILE);
    
    // Percorso predefinito per la libreria musicale
    char library_path[MAX_PATH_LENGTH] = ".";
    
    // If command line has a path, use it; otherwise use the path from settings
    if (lpCmdLine && *lpCmdLine) {
        strncpy(library_path, lpCmdLine, MAX_PATH_LENGTH - 1);
        library_path[MAX_PATH_LENGTH - 1] = '\0'; // Assicura terminazione
    } else if (g_settings.library_path[0] != '\0') {
        strncpy(library_path, g_settings.library_path, MAX_PATH_LENGTH - 1);
        library_path[MAX_PATH_LENGTH - 1] = '\0';
    }
    
    // Creazione della libreria MP3
    MP3Library* library = create_library(library_path);
    if (!library) {
        MessageBox(NULL, "Error: unable to create MP3 library.", "Error", MB_ICONERROR);
        mem_shutdown();
        return 1;
    }
    
    // Update settings with the current library path
    strncpy(g_settings.library_path, library_path, MAX_PATH - 1);
    g_settings.library_path[MAX_PATH - 1] = '\0';
    
    // Scansione iniziale della directory
    int found_files = scan_directory(library, library_path, TRUE);
    
    if (found_files == 0) {
        char message[512];
        sprintf(message, "No MP3 files found in directory %s.\n"
                         "The application will start anyway.", library_path);
        MessageBox(NULL, message, "Information", MB_ICONINFORMATION);
    }
    
    // Start automatic scanning if enabled in settings
    if (g_settings.auto_scan) {
        start_continuous_scan(library, g_settings.scan_interval, library_path);
    }
    
    // Avvia l'interfaccia grafica
    int result = start_gui(hInstance, library, &g_settings);
    
    // Before exiting, save settings
    settings_save(&g_settings, DEFAULT_SETTINGS_FILE);
    
    // Pulizia della memoria
    free_mp3_library(library);
    
    // Report any memory leaks
    mem_report();
    mem_shutdown();
    
    return result;
}

// Funzione per avviare la versione GUI dall'interfaccia a linea di comando
int start_gui_from_cli(MP3Library* library) {
    // Imposta la codepage per la visualizzazione dei caratteri accentati
    SetConsoleOutputCP(65001); // UTF-8 code page
    SetConsoleCP(65001);       // Input code page
    setlocale(LC_ALL, "it_IT.UTF-8");
    
    // Initialize and load user settings if not already done
    static BOOL settings_loaded = FALSE;
    if (!settings_loaded) {
        settings_init(&g_settings);
        settings_load(&g_settings, DEFAULT_SETTINGS_FILE);
        settings_loaded = TRUE;
    }
    
    // Questa funzione viene chiamata dal main.c per avviare l'interfaccia grafica
    // e poi tornare all'interfaccia a linea di comando
    
    HINSTANCE hInstance = GetModuleHandle(NULL);
    int result = start_gui(hInstance, library, &g_settings);
    
    // Save settings before returning to CLI
    settings_save(&g_settings, DEFAULT_SETTINGS_FILE);
    
    return result;
} 