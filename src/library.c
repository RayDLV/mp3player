#include "../include/mp3player.h"

// Funzione per creare una nuova libreria MP3
MP3Library* create_library(const char* directory_path) {
    if (!directory_path) {
        return NULL;
    }
    
    MP3Library* library = (MP3Library*)malloc(sizeof(MP3Library));
    if (!library) {
        return NULL;
    }
    
    // Inizializzazione della libreria
    library->all_files = NULL;
    library->total_files = 0;
    strncpy(library->library_path, directory_path, MAX_PATH_LENGTH - 1);
    library->library_path[MAX_PATH_LENGTH - 1] = '\0'; // Assicura terminazione
    
    return library;
}

// Funzione per scansionare una directory alla ricerca di file MP3
int scan_directory(MP3Library* library, const char* directory_path, BOOL recursive) {
    if (!library || !directory_path) {
        return 0;
    }
    
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char search_path[MAX_PATH_LENGTH];
    int file_count = 0;
    
    // Costruisce il pattern di ricerca per tutti i file
    _snprintf_s(search_path, MAX_PATH_LENGTH, MAX_PATH_LENGTH - 1, "%s\\*", directory_path);
    
    // Trova il primo file
    hFind = FindFirstFile(search_path, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return 0;
    }
    
    do {
        // Ignora "." e ".."
        if (strcmp(findFileData.cFileName, ".") == 0 || 
            strcmp(findFileData.cFileName, "..") == 0) {
            continue;
        }
        
        // Costruisci il percorso completo
        char full_path[MAX_PATH_LENGTH];
        _snprintf_s(full_path, MAX_PATH_LENGTH, MAX_PATH_LENGTH - 1, "%s\\%s", directory_path, findFileData.cFileName);
        
        // Se è una directory e la scansione è ricorsiva
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (recursive) {
                file_count += scan_directory(library, full_path, recursive);
            }
        }
        // Se è un file con estensione .mp3
        else {
            char* ext = strrchr(findFileData.cFileName, '.');
            if (ext && _stricmp(ext, ".mp3") == 0) {
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
                    
                    // Aggiungi il file alla lista
                    new_file->next = library->all_files;
                    library->all_files = new_file;
                    
                    // Incrementa i contatori
                    library->total_files++;
                    file_count++;
                }
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);
    
    FindClose(hFind);
    return file_count;
}

// Funzione per liberare la memoria di un file MP3
void free_mp3_file(MP3File* file) {
    if (!file) {
        return;
    }
    
    // Libera l'immagine dell'album se presente
    if (file->metadata.album_art) {
        free(file->metadata.album_art);
    }
    
    free(file);
}

// Funzione per liberare la memoria di una coda di riproduzione
void free_mp3_queue(MP3Queue* queue) {
    if (!queue) {
        return;
    }
    
    // Non liberiamo i singoli MP3File qui perché sono referenziati dalla libreria
    free(queue);
}

// Funzione per liberare la memoria di una libreria MP3
void free_mp3_library(MP3Library* library) {
    if (!library) {
        return;
    }
    
    // Libera tutti i file MP3
    MP3File* current = library->all_files;
    while (current) {
        MP3File* next = current->next;
        free_mp3_file(current);
        current = next;
    }
    
    free(library);
} 