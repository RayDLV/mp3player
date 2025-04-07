/*
 * Per utilizzare BASS audio library:
 * 1. Scaricare BASS da http://www.un4seen.com/bass.html
 * 2. Aggiungere bass.lib (o bass.dll) alle dipendenze nel Makefile
 * 3. Copiare bass.dll nella cartella del progetto o in system32
 */

#include "../include/audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
// Includi bass.h
#include <bass.h>

/*
 * Questo è un codice di esempio che mostra come implementare la riproduzione
 * usando BASS. Per utilizzarlo, è necessario:
 * - Scaricare BASS da http://www.un4seen.com/bass.html
 * - Decommentare l'include di bass.h sopra
 * - Collegare bass.lib nel Makefile
 * - Copiare bass.dll nella directory del progetto
 * 
 * // Esempio di modifica al Makefile:
 * LIBS = -lwinmm -lcomctl32 -lbass
 */

// Questo è un placeholder per HSTREAM che normalmente sarebbe definito in bass.h
typedef DWORD HSTREAM;

// Struttura per tenere traccia dello stream attuale
typedef struct {
    HSTREAM handle;
    BOOL isPlaying;
} BassStream;

static BassStream currentStream = {0};

// Inizializza BASS - chiamare all'avvio del programma
static BOOL init_bass(void) {
    printf("DEBUG: Initializing BASS\n");
    if (!BASS_Init(-1, 44100, 0, 0, NULL)) {
        printf("DEBUG: Error initializing BASS: %d\n", BASS_ErrorGetCode());
        return FALSE;
    }
    printf("DEBUG: BASS initialized successfully\n");
    return TRUE;
}

// Libera BASS - chiamare alla chiusura del programma
static void free_bass(void) {
    printf("DEBUG: Closing BASS\n");
    BASS_Free();
}

// Riproduce un file audio usando BASS
static BOOL play_sound_file(const char* filepath) {
    printf("DEBUG: Attempting playback with BASS: %s\n", filepath);
    
    // Prima ferma qualsiasi riproduzione precedente
    if (currentStream.handle != 0) {
        printf("DEBUG: Closing previous stream\n");
        BASS_StreamFree(currentStream.handle);
        currentStream.handle = 0;
        currentStream.isPlaying = FALSE;
    }
    
    // Carica il file
    printf("DEBUG: Loading file with BASS_StreamCreateFile\n");
    HSTREAM stream = BASS_StreamCreateFile(FALSE, filepath, 0, 0, 0);
    if (!stream) {
        printf("DEBUG: Error loading file: %d\n", BASS_ErrorGetCode());
        return FALSE;
    }
    
    // Avvia la riproduzione
    printf("DEBUG: Starting playback BASS_ChannelPlay\n");
    if (!BASS_ChannelPlay(stream, FALSE)) {
        printf("DEBUG: Error starting playback: %d\n", BASS_ErrorGetCode());
        BASS_StreamFree(stream);
        return FALSE;
    }
    
    // Salva lo stream corrente
    currentStream.handle = stream;
    currentStream.isPlaying = TRUE;
    
    printf("DEBUG: Playback started successfully with BASS\n");
    return TRUE;
}

// Crea un nuovo audio player
AudioPlayer* create_audio_player(HWND notifyWindow, UINT notifyMessage) {
    printf("DEBUG: Creating audio player\n");
    
    // Inizializza BASS
    if (!init_bass()) {
        printf("DEBUG: Unable to initialize BASS, player not created\n");
        return NULL;
    }
    
    AudioPlayer* player = (AudioPlayer*)malloc(sizeof(AudioPlayer));
    if (!player) {
        printf("DEBUG: Error allocating audio player\n");
        return NULL;
    }
    
    // Inizializza il player
    memset(player, 0, sizeof(AudioPlayer));
    player->queue = (MP3Queue*)malloc(sizeof(MP3Queue));
    if (!player->queue) {
        printf("DEBUG: Error allocating playback queue\n");
        free(player);
        return NULL;
    }
    
    // Inizializza la coda
    memset(player->queue, 0, sizeof(MP3Queue));
    player->queue->head = NULL;
    player->queue->current = NULL;
    player->queue->count = 0;
    player->queue->repeat = FALSE;
    player->queue->shuffle = FALSE;
    
    // Imposta le proprietà del player
    player->notifyWindow = notifyWindow;
    player->notifyMessage = notifyMessage;
    player->state = PLAYBACK_STOPPED;
    player->mode = PLAYBACK_MODE_NORMAL;
    player->volume = 80; // Volume predefinito all'80%
    player->isOpen = FALSE;
    
    // Inizializza il generatore di numeri casuali per la modalità shuffle
    srand((unsigned int)time(NULL));
    
    printf("DEBUG: Audio player created successfully\n");
    return player;
}

// Libera la memoria del player
void free_audio_player(AudioPlayer* player) {
    if (!player) return;
    
    // Stop e chiusura del file corrente
    if (player->isOpen) {
        stop_playback(player);
        player->isOpen = FALSE;
    }
    
    // Libera la coda
    if (player->queue) {
        clear_queue(player);
        free(player->queue);
    }
    
    free(player);
    
    // Libera BASS
    free_bass();
}

// Riproduce un file specifico
BOOL play_file(AudioPlayer* player, const char* filepath) {
    if (!player || !filepath) {
        printf("DEBUG: player or filepath NULL\n");
        return FALSE;
    }
    
    printf("DEBUG: Attempting to play file: %s\n", filepath);
    
    // Ferma qualsiasi riproduzione precedente
    if (player->isOpen) {
        stop_playback(player);
    }
    
    // Avvia la riproduzione
    if (play_sound_file(filepath)) {
        player->isOpen = TRUE;
        player->state = PLAYBACK_PLAYING;
        
        // Applica il volume attuale
        set_volume(player, player->volume);
        
        return TRUE;
    }
    
    return FALSE;
}

// Riproduce il file corrente nella coda
BOOL play_current(AudioPlayer* player) {
    if (!player || !player->queue || !player->queue->current) return FALSE;
    
    return play_file(player, player->queue->current->filepath);
}

// Mette in pausa la riproduzione
BOOL pause_playback(AudioPlayer* player) {
    if (!player || !player->isOpen || player->state != PLAYBACK_PLAYING) return FALSE;
    
    // Se stiamo usando BASS, metti in pausa lo stream
    if (currentStream.handle) {
        // Metti in pausa la riproduzione usando BASS_ChannelPause
        if (BASS_ChannelPause(currentStream.handle)) {
            // Aggiorna lo stato della riproduzione
            player->state = PLAYBACK_PAUSED;
            currentStream.isPlaying = FALSE;
            printf("DEBUG: Playback paused\n");
            return TRUE;
        } else {
            printf("DEBUG: Error pausing playback: %d\n", BASS_ErrorGetCode());
        }
    }
    
    return FALSE;
}

// Riprende la riproduzione
BOOL resume_playback(AudioPlayer* player) {
    if (!player || !player->isOpen || player->state != PLAYBACK_PAUSED) return FALSE;
    
    // Se stiamo usando BASS, riprendi lo stream
    if (currentStream.handle) {
        // Riprendi la riproduzione usando BASS_ChannelPlay
        if (BASS_ChannelPlay(currentStream.handle, FALSE)) {  // FALSE = non ripartire dall'inizio
            // Aggiorna lo stato della riproduzione
            player->state = PLAYBACK_PLAYING;
            currentStream.isPlaying = TRUE;
            printf("DEBUG: Playback resumed\n");
            return TRUE;
        } else {
            printf("DEBUG: Error resuming playback: %d\n", BASS_ErrorGetCode());
        }
    }
    
    return FALSE;
}

// Stampa lo stato corrente del player (per debug)
void print_player_status(AudioPlayer* player) {
    if (!player) {
        printf("DEBUG: Player not initialized\n");
        return;
    }
    
    printf("DEBUG: Player status:\n");
    printf("  - isOpen: %s\n", player->isOpen ? "TRUE" : "FALSE");
    printf("  - state: %s\n", 
           player->state == PLAYBACK_STOPPED ? "STOPPED" : 
           player->state == PLAYBACK_PLAYING ? "PLAYING" : 
           player->state == PLAYBACK_PAUSED ? "PAUSED" : "UNKNOWN");
    
    if (currentStream.handle) {
        printf("  - Stream: %lu\n", (unsigned long)currentStream.handle);
        printf("  - isPlaying: %s\n", currentStream.isPlaying ? "TRUE" : "FALSE");
    } else {
        printf("  - Nessuno stream attivo\n");
    }
}

// Ferma la riproduzione
BOOL stop_playback(AudioPlayer* player) {
    if (!player || !player->isOpen) return FALSE;
    
    printf("DEBUG: Stopping playback\n");
    print_player_status(player);
    
    // Se non c'è nessuno stream attivo o la riproduzione è già ferma, restituisci TRUE
    if (!currentStream.handle || player->state == PLAYBACK_STOPPED) {
        player->state = PLAYBACK_STOPPED;
        return TRUE;
    }
    
    // Prova a fermare lo stream
    if (BASS_ChannelStop(currentStream.handle)) {
        printf("DEBUG: Playback stopped successfully\n");
        player->state = PLAYBACK_STOPPED;
        currentStream.isPlaying = FALSE;
        return TRUE;
    } else {
        printf("DEBUG: Error stopping playback: %d\n", BASS_ErrorGetCode());
        return FALSE;
    }
}

// Passa al brano successivo
BOOL next_track(AudioPlayer* player) {
    if (!player || !player->queue || !player->queue->current) return FALSE;
    
    MP3File* next = NULL;
    
    // Determina il prossimo brano in base alla modalità di riproduzione
    switch (player->mode) {
        case PLAYBACK_MODE_NORMAL:
        case PLAYBACK_MODE_REPEAT_ALL:
            next = player->queue->current->next;
            if (!next && player->mode == PLAYBACK_MODE_REPEAT_ALL) {
                next = player->queue->head; // Torna all'inizio
            }
            break;
            
        case PLAYBACK_MODE_REPEAT_ONE:
            next = player->queue->current; // Riproduce lo stesso brano
            break;
            
        case PLAYBACK_MODE_SHUFFLE:
            {
                // Scegli un brano casuale dalla coda
                if (player->queue->count <= 1) {
                    next = player->queue->current;
                } else {
                    int random_index = rand() % player->queue->count;
                    MP3File* temp = player->queue->head;
                    for (int i = 0; i < random_index && temp; i++) {
                        temp = temp->next;
                    }
                    next = temp;
                }
            }
            break;
    }
    
    if (next) {
        player->queue->current = next;
        return play_current(player);
    }
    
    return FALSE;
}

// Passa al brano precedente
BOOL previous_track(AudioPlayer* player) {
    if (!player || !player->queue || !player->queue->current) return FALSE;
    
    // La gestione del brano precedente è più complessa perché la lista è unidirezionale
    // In una implementazione reale andremmo con una lista doppiamente collegata
    // Per ora, se siamo all'inizio, torniamo alla fine se in modalità repeat all
    if (player->queue->current == player->queue->head) {
        if (player->mode == PLAYBACK_MODE_REPEAT_ALL) {
            // Trova l'ultimo brano
            MP3File* last = player->queue->head;
            while (last && last->next) {
                last = last->next;
            }
            player->queue->current = last;
            return play_current(player);
        } else {
            // Se non siamo in modalità repeat all, riavviamo il brano corrente
            return play_current(player);
        }
    } else {
        // Dobbiamo trovare il brano precedente
        MP3File* prev = player->queue->head;
        while (prev && prev->next != player->queue->current) {
            prev = prev->next;
        }
        
        if (prev) {
            player->queue->current = prev;
            return play_current(player);
        }
    }
    
    return FALSE;
}

// Aggiungi un file alla coda
BOOL add_to_queue(AudioPlayer* player, MP3File* file) {
    if (!player || !player->queue || !file) return FALSE;
    
    // Crea una copia del file MP3
    MP3File* new_file = (MP3File*)malloc(sizeof(MP3File));
    if (!new_file) return FALSE;
    
    // Copia le informazioni del file
    memcpy(new_file, file, sizeof(MP3File));
    new_file->next = NULL;
    
    // Se c'è l'immagine dell'album, copia anche quella
    if (file->metadata.album_art && file->metadata.album_art_size > 0) {
        new_file->metadata.album_art = (char*)malloc(file->metadata.album_art_size);
        if (new_file->metadata.album_art) {
            memcpy(new_file->metadata.album_art, file->metadata.album_art, file->metadata.album_art_size);
        } else {
            new_file->metadata.album_art = NULL;
            new_file->metadata.album_art_size = 0;
        }
    } else {
        new_file->metadata.album_art = NULL;
        new_file->metadata.album_art_size = 0;
    }
    
    // Aggiungi il file alla fine della coda
    if (player->queue->head == NULL) {
        player->queue->head = new_file;
        player->queue->current = new_file;
    } else {
        MP3File* temp = player->queue->head;
        while (temp->next) {
            temp = temp->next;
        }
        temp->next = new_file;
    }
    
    player->queue->count++;
    return TRUE;
}

// Svuota la coda
BOOL clear_queue(AudioPlayer* player) {
    if (!player || !player->queue) return FALSE;
    
    // Ferma la riproduzione
    stop_playback(player);
    
    // Libera la memoria dei file nella coda
    MP3File* current = player->queue->head;
    while (current) {
        MP3File* next = current->next;
        
        // Libera la memoria dell'immagine dell'album, se presente
        if (current->metadata.album_art) {
            free(current->metadata.album_art);
        }
        
        free(current);
        current = next;
    }
    
    player->queue->head = NULL;
    player->queue->current = NULL;
    player->queue->count = 0;
    
    return TRUE;
}

// Ottieni il numero di elementi nella coda
int get_queue_size(AudioPlayer* player) {
    if (!player || !player->queue) return 0;
    return player->queue->count;
}

// Ottieni il file attualmente in riproduzione
MP3File* get_current_file(AudioPlayer* player) {
    if (!player || !player->queue) return NULL;
    return player->queue->current;
}

// Imposta il volume
BOOL set_volume(AudioPlayer* player, int volume) {
    if (!player) return FALSE;
    
    // Limita il volume tra 0 e 100
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    player->volume = volume;
    
    // Con BASS possiamo impostare il volume direttamente (0.0-1.0)
    if (player->isOpen && currentStream.handle != 0) {
        float bassVolume = volume / 100.0f;
        printf("DEBUG: Setting BASS volume to %.2f (volume: %d)\n", bassVolume, volume);
        
        if (!BASS_ChannelSetAttribute(currentStream.handle, BASS_ATTRIB_VOL, bassVolume)) {
            printf("DEBUG: Error setting volume: %d\n", BASS_ErrorGetCode());
            return FALSE;
        }
    } else {
        printf("DEBUG: Volume set to %d (will be applied on next playback)\n", volume);
    }
    
    return TRUE;
}

// Ottieni il volume attuale
int get_volume(AudioPlayer* player) {
    if (!player) return 0;
    return player->volume;
}

// Imposta la modalità di riproduzione
BOOL set_playback_mode(AudioPlayer* player, PlaybackMode mode) {
    if (!player) return FALSE;
    
    player->mode = mode;
    
    // Aggiorna le impostazioni della coda
    if (player->queue) {
        player->queue->repeat = (mode == PLAYBACK_MODE_REPEAT_ONE || mode == PLAYBACK_MODE_REPEAT_ALL);
        player->queue->shuffle = (mode == PLAYBACK_MODE_SHUFFLE);
    }
    
    return TRUE;
}

// Ottieni la modalità di riproduzione corrente
PlaybackMode get_playback_mode(AudioPlayer* player) {
    if (!player) return PLAYBACK_MODE_NORMAL;
    return player->mode;
}

// Ottieni lo stato di riproduzione corrente
PlaybackState get_playback_state(AudioPlayer* player) {
    if (!player) return PLAYBACK_STOPPED;
    return player->state;
}

// Ottieni la posizione attuale della riproduzione in millisecondi
int get_current_position(AudioPlayer* player) {
    if (!player || !player->isOpen) return 0;
    
    if (currentStream.handle != 0) {
        QWORD pos = BASS_ChannelGetPosition(currentStream.handle, BASS_POS_BYTE);
        double seconds = BASS_ChannelBytes2Seconds(currentStream.handle, pos);
        return (int)(seconds * 1000);
    }
    
    return 0;
}

// Ottieni la durata totale del file corrente in millisecondi
int get_total_duration(AudioPlayer* player) {
    if (!player || !player->isOpen) return 0;
    
    if (currentStream.handle != 0) {
        QWORD length = BASS_ChannelGetLength(currentStream.handle, BASS_POS_BYTE);
        double seconds = BASS_ChannelBytes2Seconds(currentStream.handle, length);
        return (int)(seconds * 1000);
    }
    
    return 0;
}

// Imposta una banda dell'equalizzatore
void set_eq_band(AudioPlayer* player, int frequency, float gain) {
    if (!player || !currentStream.handle) return;
    
    // Converti il gain da -10/+10 a formati BASS (0.0 a 2.0, dove 1.0 è normale)
    float bass_gain = 1.0f + (gain / 10.0f);
    
    // Imposta la banda dell'equalizzatore
    int eq_handle = BASS_ChannelSetFX(currentStream.handle, BASS_FX_DX8_PARAMEQ, 0);
    
    if (eq_handle) {
        BASS_DX8_PARAMEQ eq;
        eq.fCenter = (float)frequency;   // Frequenza centrale
        eq.fBandwidth = 1.0f;           // Larghezza di banda (in ottave)
        eq.fGain = gain;                // Guadagno (-15...+15 dB)
        
        BASS_FXSetParameters(eq_handle, &eq);
        
        printf("DEBUG: Set EQ band at %d Hz with gain %.1f dB\n", frequency, gain);
    }
} 