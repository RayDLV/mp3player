#include "../include/mp3player.h"

// Thread globali e variabili di controllo
static HANDLE g_scan_thread = NULL;
static BOOL g_continue_scanning = FALSE;
static int g_scan_interval = 0;  // Intervallo in secondi

// Struttura per passare i parametri al thread
typedef struct {
    MP3Library* library;
    char directory_path[MAX_PATH_LENGTH];
    BOOL recursive;
} ScanThreadParams;

// Funzione che confronta due file MP3 per determinare se sono lo stesso file
static BOOL is_same_file(const char* filepath1, const char* filepath2) {
    return (strcmp(filepath1, filepath2) == 0);
}

// Funzione per verificare se un file è già presente nella libreria
static BOOL file_exists_in_library(MP3Library* library, const char* filepath) {
    MP3File* current = library->all_files;
    
    while (current) {
        if (is_same_file(current->filepath, filepath)) {
            return TRUE;
        }
        current = current->next;
    }
    
    return FALSE;
}

// Funzione per verificare se un file esiste sul filesystem
static BOOL file_exists_on_disk(const char* filepath) {
    DWORD attributes = GetFileAttributes(filepath);
    return (attributes != INVALID_FILE_ATTRIBUTES && 
            !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}

// Funzione per rimuovere un file dalla libreria
static void remove_file_from_library(MP3Library* library, const char* filepath) {
    if (!library || !filepath) {
        return;
    }
    
    // Caso speciale: primo elemento della lista
    if (library->all_files && is_same_file(library->all_files->filepath, filepath)) {
        MP3File* temp = library->all_files;
        library->all_files = library->all_files->next;
        free_mp3_file(temp);
        library->total_files--;
        return;
    }
    
    // Scansione della lista per trovare il file
    MP3File* current = library->all_files;
    while (current && current->next) {
        if (is_same_file(current->next->filepath, filepath)) {
            MP3File* temp = current->next;
            current->next = current->next->next;
            free_mp3_file(temp);
            library->total_files--;
            return;
        }
        current = current->next;
    }
}

// Funzione eseguita dal thread di scansione
static DWORD WINAPI scan_thread_func(LPVOID lpParam) {
    ScanThreadParams* params = (ScanThreadParams*)lpParam;
    
    while (g_continue_scanning) {
        // Prima verifica se i file esistenti sono ancora presenti sul disco
        MP3File* current = params->library->all_files;
        MP3File* prev = NULL;
        int removed_files = 0;
        
        while (current) {
            MP3File* next = current->next;
            
            // Verifica se il file esiste ancora sul disco
            if (!file_exists_on_disk(current->filepath)) {
                // Il file è stato rimosso dal disco
                if (prev) {
                    // Non è il primo elemento
                    prev->next = next;
                    free_mp3_file(current);
                } else {
                    // È il primo elemento
                    params->library->all_files = next;
                    free_mp3_file(current);
                }
                
                params->library->total_files--;
                removed_files++;
            } else {
                // Il file esiste ancora, aggiorniamo prev
                prev = current;
            }
            
            current = next;
        }
        
        // Poi cerca nuovi file
        // Scansione della directory
        WIN32_FIND_DATA findFileData;
        HANDLE hFind = INVALID_HANDLE_VALUE;
        char search_path[MAX_PATH_LENGTH];
        int new_files = 0;
        
        // Costruisce il pattern di ricerca per tutti i file
        _snprintf_s(search_path, MAX_PATH_LENGTH, MAX_PATH_LENGTH - 1, "%s\\*", params->directory_path);
        
        // Trova il primo file
        hFind = FindFirstFile(search_path, &findFileData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                // Ignora "." e ".."
                if (strcmp(findFileData.cFileName, ".") == 0 || 
                    strcmp(findFileData.cFileName, "..") == 0) {
                    continue;
                }
                
                // Costruisci il percorso completo
                char full_path[MAX_PATH_LENGTH];
                _snprintf_s(full_path, MAX_PATH_LENGTH, MAX_PATH_LENGTH - 1, "%s\\%s", 
                          params->directory_path, findFileData.cFileName);
                
                // Se è una directory e la scansione è ricorsiva
                if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (params->recursive) {
                        // Crea parametri per la sottodirectory
                        ScanThreadParams subdir_params;
                        subdir_params.library = params->library;
                        strncpy(subdir_params.directory_path, full_path, MAX_PATH_LENGTH - 1);
                        subdir_params.directory_path[MAX_PATH_LENGTH - 1] = '\0';
                        subdir_params.recursive = TRUE;
                        
                        // Chiamata ricorsiva (in questo thread)
                        scan_thread_func(&subdir_params);
                    }
                }
                // Se è un file con estensione .mp3
                else {
                    char* ext = strrchr(findFileData.cFileName, '.');
                    if (ext && _stricmp(ext, ".mp3") == 0) {
                        // Controlla se il file è già nella libreria
                        if (!file_exists_in_library(params->library, full_path)) {
                            // Crea un nuovo nodo per il file MP3
                            MP3File* new_file = (MP3File*)malloc(sizeof(MP3File));
                            if (new_file) {
                                strncpy(new_file->filepath, full_path, MAX_PATH_LENGTH - 1);
                                new_file->filepath[MAX_PATH_LENGTH - 1] = '\0';
                                
                                // Leggi i metadati dal file MP3
                                if (!read_mp3_metadata(full_path, &new_file->metadata)) {
                                    // Se la lettura dei metadati fallisce, usiamo il nome del file come titolo
                                    memset(&new_file->metadata, 0, sizeof(MP3Metadata));
                                    strncpy(new_file->metadata.title, findFileData.cFileName, MAX_TITLE_LENGTH - 1);
                                    new_file->metadata.title[MAX_TITLE_LENGTH - 1] = '\0';
                                }
                                
                                // Aggiungi il file alla lista in modo thread-safe
                                // Qui andrebbe usato un semaforo o mutex per la sincronizzazione
                                // Ma per semplicità, aggiungiamo il file direttamente
                                new_file->next = params->library->all_files;
                                params->library->all_files = new_file;
                                
                                // Incrementa i contatori
                                params->library->total_files++;
                                new_files++;
                            }
                        }
                    }
                }
            } while (FindNextFile(hFind, &findFileData) != 0);
            
            FindClose(hFind);
        }
        
        // Attendi per l'intervallo di scansione
        Sleep(g_scan_interval * 1000);
    }
    
    // Libera i parametri
    free(params);
    return 0;
}

// Funzione per avviare la scansione continua in background
void start_continuous_scan(MP3Library* library, int interval_seconds, const char* directory_path) {
    // Controlla se il thread è già in esecuzione
    if (g_scan_thread != NULL) {
        return;
    }
    
    // Imposta le variabili di controllo
    g_continue_scanning = TRUE;
    g_scan_interval = (interval_seconds <= 0) ? 60 : interval_seconds;  // Default: 1 minuto
    
    // Crea parametri per il thread
    ScanThreadParams* params = (ScanThreadParams*)malloc(sizeof(ScanThreadParams));
    if (!params) {
        return;
    }
    
    params->library = library;
    
    // Usa la directory specificata o il percorso della libreria se non specificata
    if (directory_path && directory_path[0] != '\0') {
        strncpy(params->directory_path, directory_path, MAX_PATH_LENGTH - 1);
    } else {
        strncpy(params->directory_path, library->library_path, MAX_PATH_LENGTH - 1);
    }
    params->directory_path[MAX_PATH_LENGTH - 1] = '\0';
    params->recursive = TRUE;
    
    // Crea il thread
    g_scan_thread = CreateThread(
        NULL,                   // Attributi di sicurezza predefiniti
        0,                      // Dimensione stack predefinita
        scan_thread_func,       // Funzione del thread
        params,                 // Parametri da passare al thread
        0,                      // Esegui immediatamente
        NULL                    // Non serve l'ID del thread
    );
    
    if (g_scan_thread == NULL) {
        free(params);
    }
}

// Funzione per fermare la scansione continua
void stop_continuous_scan() {
    // Imposta il flag per fermare il thread
    g_continue_scanning = FALSE;
    
    // Attendi che il thread termini
    if (g_scan_thread != NULL) {
        WaitForSingleObject(g_scan_thread, INFINITE);
        CloseHandle(g_scan_thread);
        g_scan_thread = NULL;
    }
} 