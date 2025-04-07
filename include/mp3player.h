#ifndef MP3PLAYER_H
#define MP3PLAYER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_PATH_LENGTH 260
#define MAX_TITLE_LENGTH 100
#define MAX_ARTIST_LENGTH 100
#define MAX_ALBUM_LENGTH 100
#define MAX_GENRE_LENGTH 30
#define MAX_FILTER_LENGTH 100

// Costanti per l'ordinamento dei file MP3
enum {
    SORT_BY_TITLE,
    SORT_BY_ARTIST,
    SORT_BY_ALBUM,
    SORT_BY_YEAR,
    SORT_BY_GENRE,
    SORT_BY_TRACK
};

// Costanti per i formati delle immagini dell'album
enum {
    ALBUM_ART_UNKNOWN,
    ALBUM_ART_JPEG,
    ALBUM_ART_PNG,
    ALBUM_ART_OTHER
};

// Struttura per rappresentare i metadati di un file MP3
typedef struct {
    char title[MAX_TITLE_LENGTH];
    char artist[MAX_ARTIST_LENGTH];
    char album[MAX_ALBUM_LENGTH];
    char genre[MAX_GENRE_LENGTH];
    int year;
    int track_number;
    int duration; // in secondi
    char* album_art; // puntatore all'immagine dell'album (se presente)
    size_t album_art_size; // dimensione dell'immagine dell'album
    int album_art_format; // formato dell'immagine (vedi enum sopra)
    unsigned char album_art_type; // tipo di immagine (0=Other, 3=Cover front)
} MP3Metadata;

// Struttura per rappresentare un file MP3
typedef struct MP3File {
    char filepath[MAX_PATH_LENGTH];
    MP3Metadata metadata;
    struct MP3File* next; // per lista collegata
} MP3File;

// Struttura per la playlist/coda di riproduzione
typedef struct {
    MP3File* head;
    MP3File* current;
    int count;
    BOOL repeat; // modalità ripeti
    BOOL shuffle; // modalità casuale
} MP3Queue;

// Struttura per la libreria MP3
typedef struct {
    MP3File* all_files; // lista collegata di tutti i file MP3
    int total_files;
    char library_path[MAX_PATH_LENGTH]; // percorso della directory principale
} MP3Library;

// Struttura per i filtri
typedef struct {
    char filter_text[MAX_FILTER_LENGTH];
    enum {
        FILTER_BY_TITLE,
        FILTER_BY_ARTIST,
        FILTER_BY_ALBUM,
        FILTER_BY_GENRE,
        FILTER_BY_YEAR
    } filter_type;
} MP3Filter;

// Dichiarazioni delle funzioni principali (da implementare)

// Funzioni di scansione
MP3Library* create_library(const char* directory_path);
int scan_directory(MP3Library* library, const char* directory_path, BOOL recursive);
void start_continuous_scan(MP3Library* library, int interval_seconds, const char* directory_path);
void stop_continuous_scan();

// Funzioni per i metadati
int read_mp3_metadata(const char* filepath, MP3Metadata* metadata);
MP3File* filter_mp3_files(MP3Library* library, MP3Filter* filter);
void sort_mp3_files(MP3File** file_list, int sort_type);

// Funzioni per la coda di riproduzione
MP3Queue* create_queue();

// Funzioni di pulizia
void free_mp3_file(MP3File* file);
void free_mp3_queue(MP3Queue* queue);
void free_mp3_library(MP3Library* library);

#endif // MP3PLAYER_H 