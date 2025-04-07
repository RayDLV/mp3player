#include "../include/mp3player.h"
#include "../include/bass.h"

// Dimensione dell'header ID3v2
#define ID3V2_HEADER_SIZE 10

// Encoding per ID3v2
#define ID3V2_ISO_8859_1   0  // ISO-8859-1 [ISO-8859-1]. Terminated with $00.
#define ID3V2_UTF16_BOM    1  // UTF-16 encoded Unicode [UTF-16]. Terminated with $00 00.
#define ID3V2_UTF16_BE     2  // UTF-16BE encoded Unicode without BOM [UTF-16]. Terminated with $00 00.
#define ID3V2_UTF8         3  // UTF-8 encoded Unicode [UTF-8]. Terminated with $00.

// Funzioni di utilità per la lettura dei tag ID3v2
static unsigned int read_syncsafe_integer(const unsigned char* bytes) {
    return ((bytes[0] & 0x7F) << 21) |
           ((bytes[1] & 0x7F) << 14) |
           ((bytes[2] & 0x7F) << 7) |
           (bytes[3] & 0x7F);
}

static void read_utf8_or_iso(const char* src, char* dest, size_t dest_size, int encoding) {
    // Implementazione migliorata per i diversi encoding
    if (encoding == ID3V2_ISO_8859_1) {
        // ISO-8859-1 può essere direttamente copiato per i caratteri ASCII
        strncpy(dest, src, dest_size - 1);
        dest[dest_size - 1] = '\0';
    } 
    else if (encoding == ID3V2_UTF8) {
        // Per UTF-8, già ben supportato in ambienti moderni
        strncpy(dest, src, dest_size - 1);
        dest[dest_size - 1] = '\0';
    }
    else if (encoding == ID3V2_UTF16_BOM || encoding == ID3V2_UTF16_BE) {
        // UTF-16 (con o senza BOM)
        // Per semplicità prendiamo solo i caratteri ASCII
        // Un'implementazione completa richiederebbe la conversione da UTF-16 a UTF-8
        const unsigned char* utf16 = (const unsigned char*)src;
        int i = 0, j = 0;
        
        // Salta il BOM se presente (2 byte: FF FE o FE FF)
        if (encoding == ID3V2_UTF16_BOM) {
            utf16 += 2;
        }
        
        // Copia solo i caratteri ASCII (ignorando i byte nulli)
        while (j < dest_size - 1) {
            if (utf16[i*2] == 0 && utf16[i*2 + 1] == 0) {
                break; // Fine della stringa
            }
            
            if (utf16[i*2] == 0) {
                // Prendi il secondo byte (assumendo little-endian)
                dest[j++] = utf16[i*2 + 1];
            } else {
                // Carattere non-ASCII, sostituisci con '?'
                dest[j++] = '?';
            }
            i++;
        }
        dest[j] = '\0';
    }
    else {
        // In caso di encoding sconosciuto, copiamo direttamente
        strncpy(dest, src, dest_size - 1);
        dest[dest_size - 1] = '\0';
    }
}

// Funzione migliorata per analizzare il numero della traccia
static int parse_track_number(const char* track_str) {
    int track = 0;
    // Converte solo la parte prima del separatore (/) se presente
    if (track_str) {
        char buffer[10] = {0};
        int i = 0;
        // Copia fino a trovare un separatore o un carattere non numerico
        while (track_str[i] && track_str[i] != '/' && track_str[i] != '-' && i < 9) {
            buffer[i] = track_str[i];
            i++;
        }
        buffer[i] = '\0';
        track = atoi(buffer);
    }
    return track;
}

// Estrae e salva l'immagine dell'album dal frame APIC (ID3v2.4)
static void extract_album_art(const unsigned char* frame_data, size_t frame_size, MP3Metadata* metadata) {
    if (frame_size < 10) {
        return; // Frame troppo piccolo
    }
    
    int pos = 0;
    int encoding = frame_data[pos++]; // Primo byte è l'encoding
    
    // Salta il MIME type (termina con 00)
    while (pos < frame_size && frame_data[pos] != 0) {
        pos++;
    }
    pos++; // Salta il byte null terminatore
    
    // Il byte successivo è il tipo di immagine
    if (pos < frame_size) {
        unsigned char pic_type = frame_data[pos++];
        metadata->album_art_type = pic_type;
    }
    
    // Salta la descrizione (termina con 00 o 00 00 a seconda dell'encoding)
    if (encoding == ID3V2_UTF16_BOM || encoding == ID3V2_UTF16_BE) {
        // UTF-16: terminatore è 00 00
        while (pos < frame_size - 1) {
            if (frame_data[pos] == 0 && frame_data[pos + 1] == 0) {
                pos += 2;
                break;
            }
            pos++;
        }
    } else {
        // Altro encoding: terminatore è 00
        while (pos < frame_size && frame_data[pos] != 0) {
            pos++;
        }
        pos++; // Salta il byte null terminatore
    }
    
    // Il resto è l'immagine vera e propria
    size_t image_data_size = frame_size - pos;
    if (image_data_size > 0) {
        // Libera memoria precedente se esistente
        if (metadata->album_art) {
            free(metadata->album_art);
            metadata->album_art = NULL;
            metadata->album_art_size = 0;
        }
        
        // Alloca memoria e copia i dati dell'immagine
        metadata->album_art = (char*)malloc(image_data_size);
        if (metadata->album_art) {
            memcpy(metadata->album_art, frame_data + pos, image_data_size);
            metadata->album_art_size = image_data_size;
            
            // Determina il formato dell'immagine in base ai magic number
            if (image_data_size >= 3 && 
                (unsigned char)metadata->album_art[0] == 0xFF && 
                (unsigned char)metadata->album_art[1] == 0xD8 && 
                (unsigned char)metadata->album_art[2] == 0xFF) {
                metadata->album_art_format = ALBUM_ART_JPEG;
            } else if (image_data_size >= 8 && 
                (unsigned char)metadata->album_art[0] == 0x89 && 
                metadata->album_art[1] == 'P' && 
                metadata->album_art[2] == 'N' && 
                metadata->album_art[3] == 'G' && 
                (unsigned char)metadata->album_art[4] == 0x0D && 
                (unsigned char)metadata->album_art[5] == 0x0A && 
                (unsigned char)metadata->album_art[6] == 0x1A && 
                (unsigned char)metadata->album_art[7] == 0x0A) {
                metadata->album_art_format = ALBUM_ART_PNG;
            } else {
                metadata->album_art_format = ALBUM_ART_OTHER;
            }
        }
    }
}

// Legge i tag ID3v2
static int read_id3v2_tag(FILE* file, MP3Metadata* metadata) {
    unsigned char header[ID3V2_HEADER_SIZE];
    
    // Posiziona all'inizio del file
    rewind(file);
    
    // Leggi l'header ID3v2
    if (fread(header, 1, ID3V2_HEADER_SIZE, file) != ID3V2_HEADER_SIZE) {
        return 0;
    }
    
    // Verifica se è un header ID3v2 valido
    if (strncmp((char*)header, "ID3", 3) != 0) {
        return 0;
    }
    
    // Ottieni la versione
    unsigned char version = header[3];
    unsigned char revision = header[4];
    
    // Calcola la dimensione del tag
    unsigned int tag_size = read_syncsafe_integer(&header[6]);
    
    // Log per debug
    // printf("ID3v2.%d.%d trovato, dimensione: %u bytes\n", version, revision, tag_size);
    
    // Buffer per i frame
    unsigned char* frame_buffer = NULL;
    unsigned int frame_size = 0;
    unsigned int frame_header_size = (version == 2) ? 6 : 10;
    
    // Leggi i frame ID3v2
    unsigned int position = 10; // Posizione dopo l'header
    while (position < tag_size + ID3V2_HEADER_SIZE) {
        // Leggi l'header del frame
        char frame_id[5] = {0};
        unsigned char frame_header[10] = {0};
        
        if (fread(frame_header, 1, frame_header_size, file) != frame_header_size) {
            break;
        }
        position += frame_header_size;
        
        // Copia l'ID del frame
        if (version == 2) {
            strncpy(frame_id, (char*)frame_header, 3);
            frame_id[3] = '\0';
            
            // Calcola la dimensione del frame (per ID3v2.2)
            frame_size = (frame_header[3] << 16) | (frame_header[4] << 8) | frame_header[5];
        } else {
            strncpy(frame_id, (char*)frame_header, 4);
            frame_id[4] = '\0';
            
            // Calcola la dimensione del frame (per ID3v2.3 e ID3v2.4)
            if (version == 3) {
                frame_size = (frame_header[4] << 24) | (frame_header[5] << 16) | 
                             (frame_header[6] << 8) | frame_header[7];
            } else { // v2.4 - formato syncsafe
                frame_size = read_syncsafe_integer(&frame_header[4]);
            }
        }
        
        // Salta frame vuoti o invalidi
        if (frame_size == 0 || position + frame_size > tag_size + ID3V2_HEADER_SIZE) {
            break;
        }
        
        // Alloca/rialloca il buffer per i dati del frame
        if (frame_buffer) {
            free(frame_buffer);
        }
        frame_buffer = (unsigned char*)malloc(frame_size + 1);
        if (!frame_buffer) {
            break;
        }
        frame_buffer[frame_size] = '\0';
        
        // Leggi i dati del frame
        if (fread(frame_buffer, 1, frame_size, file) != frame_size) {
            free(frame_buffer);
            break;
        }
        position += frame_size;
        
        // Estrai i metadati in base all'ID del frame
        if (version == 2) {
            // ID3v2.2
            if (strcmp(frame_id, "TT2") == 0 && frame_size > 1) {
                read_utf8_or_iso((char*)&frame_buffer[1], metadata->title, MAX_TITLE_LENGTH, frame_buffer[0]);
            } 
            else if (strcmp(frame_id, "TP1") == 0 && frame_size > 1) {
                read_utf8_or_iso((char*)&frame_buffer[1], metadata->artist, MAX_ARTIST_LENGTH, frame_buffer[0]);
            } 
            else if (strcmp(frame_id, "TAL") == 0 && frame_size > 1) {
                read_utf8_or_iso((char*)&frame_buffer[1], metadata->album, MAX_ALBUM_LENGTH, frame_buffer[0]);
            } 
            else if (strcmp(frame_id, "TYE") == 0 && frame_size > 1) {
                char year_str[5] = {0};
                read_utf8_or_iso((char*)&frame_buffer[1], year_str, 5, frame_buffer[0]);
                metadata->year = atoi(year_str);
            } 
            else if (strcmp(frame_id, "TCO") == 0 && frame_size > 1) {
                read_utf8_or_iso((char*)&frame_buffer[1], metadata->genre, MAX_GENRE_LENGTH, frame_buffer[0]);
            } 
            else if (strcmp(frame_id, "TRK") == 0 && frame_size > 1) {
                char track_str[10] = {0};
                read_utf8_or_iso((char*)&frame_buffer[1], track_str, sizeof(track_str), frame_buffer[0]);
                metadata->track_number = parse_track_number(track_str);
            }
            else if (strcmp(frame_id, "PIC") == 0 && frame_size > 4) {
                // Esegui l'estrazione dell'immagine dell'album
                extract_album_art(frame_buffer, frame_size, metadata);
            }
        } 
        else {
            // ID3v2.3 e ID3v2.4
            if (strcmp(frame_id, "TIT2") == 0 && frame_size > 1) {
                read_utf8_or_iso((char*)&frame_buffer[1], metadata->title, MAX_TITLE_LENGTH, frame_buffer[0]);
            } 
            else if (strcmp(frame_id, "TPE1") == 0 && frame_size > 1) {
                read_utf8_or_iso((char*)&frame_buffer[1], metadata->artist, MAX_ARTIST_LENGTH, frame_buffer[0]);
            } 
            else if (strcmp(frame_id, "TALB") == 0 && frame_size > 1) {
                read_utf8_or_iso((char*)&frame_buffer[1], metadata->album, MAX_ALBUM_LENGTH, frame_buffer[0]);
            } 
            else if ((strcmp(frame_id, "TYER") == 0 || strcmp(frame_id, "TDRC") == 0) && frame_size > 1) {
                // ID3v2.4 usa TDRC per la data (può includere più dell'anno), ID3v2.3 usa TYER
                char year_str[5] = {0};
                read_utf8_or_iso((char*)&frame_buffer[1], year_str, 5, frame_buffer[0]);
                metadata->year = atoi(year_str);
            } 
            else if (strcmp(frame_id, "TCON") == 0 && frame_size > 1) {
                read_utf8_or_iso((char*)&frame_buffer[1], metadata->genre, MAX_GENRE_LENGTH, frame_buffer[0]);
            } 
            else if (strcmp(frame_id, "TRCK") == 0 && frame_size > 1) {
                char track_str[10] = {0};
                read_utf8_or_iso((char*)&frame_buffer[1], track_str, sizeof(track_str), frame_buffer[0]);
                metadata->track_number = parse_track_number(track_str);
            }
            else if ((strcmp(frame_id, "APIC") == 0) && frame_size > 10) {
                // Esegui l'estrazione dell'immagine dell'album
                extract_album_art(frame_buffer, frame_size, metadata);
            }
        }
    }
    
    // Libera il buffer
    if (frame_buffer) {
        free(frame_buffer);
    }
    
    return 1;
}

// Funzione principale per leggere i metadati di un file MP3
int read_mp3_metadata(const char* filepath, MP3Metadata* metadata) {
    FILE* file = NULL;
    int success = 0;
    
    // Inizializza i metadati
    memset(metadata, 0, sizeof(MP3Metadata));
    
    // Usa solo il nome del file come titolo predefinito (se la lettura dei tag fallisce)
    const char* filename = strrchr(filepath, '\\');
    if (filename) {
        filename++; // Salta il backslash
    } else {
        filename = filepath; // Se non c'è un backslash, usa il percorso completo
    }
    strncpy(metadata->title, filename, MAX_TITLE_LENGTH - 1);
    metadata->title[MAX_TITLE_LENGTH - 1] = '\0';
    
    // Imposta un genere predefinito
    strncpy(metadata->genre, "Unknown", MAX_GENRE_LENGTH - 1);
    metadata->genre[MAX_GENRE_LENGTH - 1] = '\0';
    
    // Apri il file
    if (fopen_s(&file, filepath, "rb") != 0 || file == NULL) {
        return 0;
    }
    
    // Leggi i tag ID3v2 (ottimizzato per ID3v2.4)
    success = read_id3v2_tag(file, metadata);
    
    // Chiudi il file
    fclose(file);
    
    // Ottieni la durata del file usando BASS
    HSTREAM stream = BASS_StreamCreateFile(FALSE, filepath, 0, 0, BASS_STREAM_DECODE | BASS_STREAM_PRESCAN);
    if (stream) {
        QWORD length = BASS_ChannelGetLength(stream, BASS_POS_BYTE);
        double seconds = BASS_ChannelBytes2Seconds(stream, length);
        metadata->duration = (int)seconds;
        BASS_StreamFree(stream);
    }
    
    return success;
}

// Funzione per ordinare i file MP3 in base a diversi criteri
void sort_mp3_files(MP3File** file_list, int sort_type) {
    if (!file_list || !(*file_list) || !(*file_list)->next) {
        return; // Niente da ordinare
    }
    
    // Implementa un semplice bubble sort per ora
    int swapped;
    MP3File* ptr1;
    MP3File* lptr = NULL;
    
    do {
        swapped = 0;
        ptr1 = *file_list;
        
        while (ptr1->next != lptr) {
            int compare_result = 0;
            
            // Seleziona il campo di ordinamento
            switch (sort_type) {
                case SORT_BY_TITLE:
                    compare_result = strcmp(ptr1->metadata.title, ptr1->next->metadata.title);
                    break;
                case SORT_BY_ARTIST:
                    compare_result = strcmp(ptr1->metadata.artist, ptr1->next->metadata.artist);
                    break;
                case SORT_BY_ALBUM:
                    compare_result = strcmp(ptr1->metadata.album, ptr1->next->metadata.album);
                    break;
                case SORT_BY_YEAR:
                    compare_result = ptr1->metadata.year - ptr1->next->metadata.year;
                    break;
                case SORT_BY_GENRE:
                    compare_result = strcmp(ptr1->metadata.genre, ptr1->next->metadata.genre);
                    break;
                case SORT_BY_TRACK:
                    compare_result = ptr1->metadata.track_number - ptr1->next->metadata.track_number;
                    break;
                default:
                    compare_result = strcmp(ptr1->metadata.title, ptr1->next->metadata.title);
                    break;
            }
            
            // Scambia se necessario
            if (compare_result > 0) {
                // Scambia i metadati
                MP3Metadata temp_metadata = ptr1->metadata;
                ptr1->metadata = ptr1->next->metadata;
                ptr1->next->metadata = temp_metadata;
                
                // Scambia i percorsi dei file
                char temp_filepath[MAX_PATH_LENGTH];
                strncpy(temp_filepath, ptr1->filepath, MAX_PATH_LENGTH);
                strncpy(ptr1->filepath, ptr1->next->filepath, MAX_PATH_LENGTH);
                strncpy(ptr1->next->filepath, temp_filepath, MAX_PATH_LENGTH);
                
                swapped = 1;
            }
            
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    } while (swapped);
}

// Funzione per filtrare i file MP3 in base a un criterio
MP3File* filter_mp3_files(MP3Library* library, MP3Filter* filter) {
    if (!library || !filter || !library->all_files) {
        return NULL;
    }
    
    MP3File* filtered_list = NULL;
    MP3File* current = library->all_files;
    
    while (current) {
        int match = 0;
        
        // Controlla se il file corrisponde al filtro
        switch (filter->filter_type) {
            case FILTER_BY_TITLE:
                match = (strstr(current->metadata.title, filter->filter_text) != NULL);
                break;
            case FILTER_BY_ARTIST:
                match = (strstr(current->metadata.artist, filter->filter_text) != NULL);
                break;
            case FILTER_BY_ALBUM:
                match = (strstr(current->metadata.album, filter->filter_text) != NULL);
                break;
            case FILTER_BY_GENRE:
                match = (strstr(current->metadata.genre, filter->filter_text) != NULL);
                break;
            case FILTER_BY_YEAR:
                match = (current->metadata.year == atoi(filter->filter_text));
                break;
            default:
                match = 0;
                break;
        }
        
        // Se corrisponde, aggiungi alla lista filtrata
        if (match) {
            MP3File* new_file = (MP3File*)malloc(sizeof(MP3File));
            if (new_file) {
                // Copia i dati
                strncpy(new_file->filepath, current->filepath, MAX_PATH_LENGTH);
                new_file->metadata = current->metadata;
                
                // Aggiungi all'inizio della lista filtrata
                new_file->next = filtered_list;
                filtered_list = new_file;
            }
        }
        
        current = current->next;
    }
    
    return filtered_list;
} 