#include "../include/mp3player.h"
#include <locale.h>
#include <windows.h>

// Dichiarazione della funzione di avvio dell'interfaccia grafica
int start_gui_from_cli(MP3Library* library);

int main(int argc, char* argv[]) {
    // Imposta la codepage per la visualizzazione dei caratteri accentati
    SetConsoleOutputCP(65001); // UTF-8 code page
    SetConsoleCP(65001);       // Input code page
    setlocale(LC_ALL, "it_IT.UTF-8");
    
    // Test visualizzazione caratteri accentati
    printf("Test accented characters: à è ì ò ù\n");
    
    // Percorso predefinito per la libreria musicale
    char library_path[MAX_PATH_LENGTH] = ".";
    BOOL continuous_scan_active = FALSE;
    
    // Se viene fornito un percorso come argomento, usalo
    if (argc > 1) {
        strncpy(library_path, argv[1], MAX_PATH_LENGTH - 1);
        library_path[MAX_PATH_LENGTH - 1] = '\0'; // Assicura terminazione
    }
    
    printf("MP3 Player:\n");
    printf("Initializing...\n");
    
    // Creazione della libreria MP3
    MP3Library* library = create_library(library_path);
    if (!library) {
        printf("Error: unable to create MP3 library.\n");
        return 1;
    }
    
    printf("Scanning directory: %s\n", library_path);
    
    // Scansione iniziale della directory
    int found_files = scan_directory(library, library_path, TRUE);
    printf("Found %d MP3 files.\n", found_files);
    
    // Qui andrà il codice per l'interfaccia utente e il loop principale
    // Per ora, mostriamo un semplice menu testuale
    printf("\nAvailable commands:\n");
    printf("  scan [directory] - Manually scan a directory\n");
    printf("  monitor [interval] - Start continuous background scanning (interval in seconds, default: 60)\n");
    printf("  stop - Stop continuous scanning\n");
    printf("  list - Show all detected MP3 files\n");
    printf("  info [number] - Show detailed information about an MP3 file\n");
    printf("  sort [criterion] - Sort MP3 files (title, artist, album, year, genre, track)\n");
    printf("  filter [type] [text] - Filter MP3 files (title, artist, album, genre, year)\n");
    printf("  reset - Reset display to complete list\n");
    printf("  gui - Start graphical interface\n");
    printf("  quit - Exit program\n");
    
    char input[MAX_PATH_LENGTH];
    char command[20];
    char param[MAX_PATH_LENGTH - 20];
    char param2[MAX_PATH_LENGTH - 20];
    MP3File* filtered_list = NULL;
    BOOL using_filtered_list = FALSE;
    
    while (1) {
        printf("\n> ");
        
        // Legge l'intera linea di input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        // Pulisce l'input e rimuove il carattere newline
        input[strcspn(input, "\n")] = 0;
        
        // Estrae il comando e il parametro
        param[0] = '\0'; // Inizializza a stringa vuota
        param2[0] = '\0'; // Inizializza a stringa vuota
        
        // Estrae comando e fino a due parametri
        int params = sscanf(input, "%19s %[^ ] %[^\n]", command, param, param2);
        if (params < 1) {
            continue; // Input vuoto, richiedi nuovo input
        }
        
        if (strcmp(command, "scan") == 0) {
            char scan_path[MAX_PATH_LENGTH];
            
            // Se viene fornito un percorso, usalo
            if (param[0] != '\0') {
                strncpy(scan_path, param, MAX_PATH_LENGTH - 1);
                scan_path[MAX_PATH_LENGTH - 1] = '\0';
            } else {
                strncpy(scan_path, library_path, MAX_PATH_LENGTH - 1);
                scan_path[MAX_PATH_LENGTH - 1] = '\0';
            }
            
            printf("Scanning: %s\n", scan_path);
            int new_files = scan_directory(library, scan_path, TRUE);
            printf("Found %d MP3 files.\n", new_files);
            
            // Reset della lista filtrata
            if (filtered_list) {
                // Liberiamo solo la memoria dei nodi, non i file stessi
                MP3File* current = filtered_list;
                while (current) {
                    MP3File* next = current->next;
                    free(current);
                    current = next;
                }
                filtered_list = NULL;
                using_filtered_list = FALSE;
            }
        } 
        else if (strcmp(command, "monitor") == 0) {
            if (continuous_scan_active) {
                printf("Continuous scanning is already active.\n");
            } else {
                int interval = 60; // Default: 1 minuto
                char monitor_path[MAX_PATH_LENGTH] = {0}; // Percorso da monitorare (opzionale)
                
                // Processa i parametri
                if (params >= 2) {
                    // Il primo parametro è l'intervallo o la directory
                    if (isdigit((unsigned char)param[0])) {
                        // È un numero, lo interpretiamo come intervallo
                        int parsed_interval = atoi(param);
                        if (parsed_interval > 0) {
                            interval = parsed_interval;
                        }
                        
                        // Se c'è un secondo parametro, è la directory
                        if (params >= 3) {
                            strncpy(monitor_path, param2, MAX_PATH_LENGTH - 1);
                        }
                    } else {
                        // Non è un numero, lo interpretiamo come directory
                        strncpy(monitor_path, param, MAX_PATH_LENGTH - 1);
                    }
                }
                
                if (monitor_path[0] != '\0') {
                    printf("Starting continuous scanning (interval: %d seconds, directory: %s)...\n", 
                           interval, monitor_path);
                } else {
                    printf("Starting continuous scanning (interval: %d seconds)...\n", interval);
                }
                
                start_continuous_scan(library, interval, monitor_path);
                continuous_scan_active = TRUE;
            }
        }
        else if (strcmp(command, "stop") == 0) {
            if (!continuous_scan_active) {
                printf("Continuous scanning is not active.\n");
            } else {
                printf("Stopping continuous scanning...\n");
                stop_continuous_scan();
                continuous_scan_active = FALSE;
                printf("Continuous scanning stopped.\n");
            }
        }
        else if (strcmp(command, "list") == 0) {
            MP3File* list_to_show = using_filtered_list ? filtered_list : library->all_files;
            int total_files = 0;
            
            printf("\nMP3 Files%s:\n", using_filtered_list ? " (filtered)" : "");
            MP3File* current = list_to_show;
            int index = 1;
            
            while (current) {
                printf("%d. %s - %s\n", index++, 
                       current->metadata.artist[0] ? current->metadata.artist : "Artista sconosciuto", 
                       current->metadata.title[0] ? current->metadata.title : "Titolo sconosciuto");
                current = current->next;
                total_files++;
            }
            
            if (total_files == 0) {
                printf("No MP3 files found.\n");
            } else {
                printf("Total: %d files.\n", total_files);
            }
        }
        else if (strcmp(command, "info") == 0) {
            if (param[0] == '\0') {
                printf("Specify file number.\n");
                continue;
            }
            
            int index = atoi(param);
            if (index <= 0) {
                printf("Invalid number.\n");
                continue;
            }
            
            MP3File* list_to_show = using_filtered_list ? filtered_list : library->all_files;
            
            // Creiamo un array temporaneo di puntatori per mappare l'indice visualizzato al file reale
            // Questo garantisce che l'indice corrisponda esattamente a quello visualizzato nel comando "list"
            int total_files = 0;
            MP3File* current = list_to_show;
            while (current) {
                total_files++;
                current = current->next;
            }
            
            if (total_files == 0) {
                printf("No MP3 files found.\n");
                continue;
            }
            
            if (index > total_files) {
                printf("Invalid number. There are only %d files.\n", total_files);
                continue;
            }
            
            // Alloca l'array di puntatori
            MP3File** file_array = (MP3File**)malloc(total_files * sizeof(MP3File*));
            if (!file_array) {
                printf("Memory error.\n");
                continue;
            }
            
            // Riempi l'array nello stesso ordine del comando list
            current = list_to_show;
            for (int i = 0; i < total_files; i++) {
                file_array[i] = current;
                current = current->next;
            }
            
            // Accedi direttamente al file utilizzando l'indice dell'array (con indice base 1)
            MP3File* selected_file = file_array[index - 1];
            
            // Visualizza le informazioni
            printf("\nDetailed information:\n");
            printf("Title: %s\n", selected_file->metadata.title[0] ? selected_file->metadata.title : "Unknown");
            printf("Artist: %s\n", selected_file->metadata.artist[0] ? selected_file->metadata.artist : "Unknown");
            printf("Album: %s\n", selected_file->metadata.album[0] ? selected_file->metadata.album : "Unknown");
            printf("Year: %d\n", selected_file->metadata.year);
            printf("Genre: %s\n", selected_file->metadata.genre[0] ? selected_file->metadata.genre : "Unknown");
            printf("Track: %d\n", selected_file->metadata.track_number);
            printf("Path: %s\n", selected_file->filepath);
            
            if (selected_file->metadata.album_art_size > 0) {
                // Corregge il formato di printf per size_t
                printf("Album image: Present (%lu bytes", (unsigned long)selected_file->metadata.album_art_size);
                
                // Aggiungi informazioni sul tipo di immagine
                const char* img_format = "Sconosciuto";
                switch (selected_file->metadata.album_art_format) {
                    case ALBUM_ART_JPEG:
                        img_format = "JPEG";
                        break;
                    case ALBUM_ART_PNG:
                        img_format = "PNG";
                        break;
                    case ALBUM_ART_OTHER:
                        img_format = "Altro";
                        break;
                }
                
                // Tipo di immagine (nel frame APIC)
                const char* img_type = "Altro";
                if (selected_file->metadata.album_art_type == 3) {
                    img_type = "Copertina";
                } else if (selected_file->metadata.album_art_type == 4) {
                    img_type = "Retro copertina";
                }
                
                printf(", %s, %s)\n", img_format, img_type);
            } else {
                printf("Album image: Not present\n");
            }
            
            // Libera la memoria dell'array temporaneo
            free(file_array);
        }
        else if (strcmp(command, "sort") == 0) {
            if (param[0] == '\0') {
                printf("Specify sorting criterion (title, artist, album, year, genre, track).\n");
                continue;
            }
            
            int sort_type = SORT_BY_TITLE; // Default
            
            if (strcmp(param, "title") == 0) {
                sort_type = SORT_BY_TITLE;
            } else if (strcmp(param, "artist") == 0) {
                sort_type = SORT_BY_ARTIST;
            } else if (strcmp(param, "album") == 0) {
                sort_type = SORT_BY_ALBUM;
            } else if (strcmp(param, "year") == 0) {
                sort_type = SORT_BY_YEAR;
            } else if (strcmp(param, "genre") == 0) {
                sort_type = SORT_BY_GENRE;
            } else if (strcmp(param, "track") == 0) {
                sort_type = SORT_BY_TRACK;
            } else {
                printf("Invalid sorting criterion.\n");
                continue;
            }
            
            printf("Sorting by %s...\n", param);
            
            // Ordina la lista principale o la lista filtrata
            MP3File** list_to_sort = using_filtered_list ? &filtered_list : &library->all_files;
            sort_mp3_files(list_to_sort, sort_type);
            
            printf("Sorting completed.\n");
        }
        else if (strcmp(command, "filter") == 0) {
            if (param[0] == '\0') {
                printf("Specify filter type (title, artist, album, genre, year).\n");
                continue;
            }
            
            if (param2[0] == '\0') {
                printf("Specify filter text.\n");
                continue;
            }
            
            MP3Filter filter;
            strncpy(filter.filter_text, param2, MAX_FILTER_LENGTH - 1);
            filter.filter_text[MAX_FILTER_LENGTH - 1] = '\0';
            
            if (strcmp(param, "title") == 0) {
                filter.filter_type = FILTER_BY_TITLE;
            } else if (strcmp(param, "artist") == 0) {
                filter.filter_type = FILTER_BY_ARTIST;
            } else if (strcmp(param, "album") == 0) {
                filter.filter_type = FILTER_BY_ALBUM;
            } else if (strcmp(param, "genre") == 0) {
                filter.filter_type = FILTER_BY_GENRE;
            } else if (strcmp(param, "year") == 0) {
                filter.filter_type = FILTER_BY_YEAR;
            } else {
                printf("Invalid filter type.\n");
                continue;
            }
            
            printf("Filtering by %s = '%s'...\n", param, param2);
            
            // Libera la lista filtrata precedente
            if (filtered_list) {
                MP3File* current = filtered_list;
                while (current) {
                    MP3File* next = current->next;
                    free(current);
                    current = next;
                }
                filtered_list = NULL;
            }
            
            // Applica il nuovo filtro
            filtered_list = filter_mp3_files(library, &filter);
            using_filtered_list = TRUE;
            
            // Conta i risultati
            int count = 0;
            MP3File* current = filtered_list;
            while (current) {
                count++;
                current = current->next;
            }
            
            printf("Found %d matching files.\n", count);
        }
        else if (strcmp(command, "reset") == 0) {
            // Ripristina la visualizzazione alla lista completa
            if (filtered_list) {
                MP3File* current = filtered_list;
                while (current) {
                    MP3File* next = current->next;
                    free(current);
                    current = next;
                }
                filtered_list = NULL;
            }
            using_filtered_list = FALSE;
            printf("Filter removed. All files will be displayed.\n");
        }
        else if (strcmp(command, "gui") == 0) {
            printf("Starting graphical interface...\n");
            
            // Ferma la scansione continua se è attiva
            if (continuous_scan_active) {
                printf("Stopping continuous scanning...\n");
                stop_continuous_scan();
                continuous_scan_active = FALSE;
            }
            
            // Avvia l'interfaccia grafica
            start_gui_from_cli(library);
            
            // Una volta che la GUI viene chiusa, torniamo all'interfaccia a linea di comando
            printf("Graphical interface closed. Returning to command line interface.\n");
            
            // Ripristina la modalità di visualizzazione
            using_filtered_list = FALSE;
            if (filtered_list) {
                MP3File* current = filtered_list;
                while (current) {
                    MP3File* next = current->next;
                    free(current);
                    current = next;
                }
                filtered_list = NULL;
            }
        }
        else if (strcmp(command, "quit") == 0) {
            // Ferma la scansione continua se attiva
            if (continuous_scan_active) {
                printf("Stopping continuous scanning...\n");
                stop_continuous_scan();
            }
            
            // Libera la lista filtrata
            if (filtered_list) {
                MP3File* current = filtered_list;
                while (current) {
                    MP3File* next = current->next;
                    free(current);
                    current = next;
                }
            }
            
            break;
        }
        else {
            printf("Command not recognized.\n");
        }
    }
    
    // Pulizia della memoria
    free_mp3_library(library);
    printf("Program terminated.\n");
    
    return 0;
} 