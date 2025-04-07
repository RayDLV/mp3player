#ifndef AUDIO_H
#define AUDIO_H

#include <windows.h>
#include <bass.h>
#include "mp3player.h"

// Costanti per le notifiche audio
#define AUDIO_NOTIFY_COMPLETE 1

// Tipo di riproduzione
typedef enum {
    PLAYBACK_STOPPED,  // Riproduzione ferma
    PLAYBACK_PLAYING,  // In riproduzione
    PLAYBACK_PAUSED    // In pausa
} PlaybackState;

// Modalità di riproduzione
typedef enum {
    PLAYBACK_MODE_NORMAL,       // Modalità normale
    PLAYBACK_MODE_REPEAT_ONE,   // Ripeti il brano corrente
    PLAYBACK_MODE_REPEAT_ALL,   // Ripeti tutti i brani
    PLAYBACK_MODE_SHUFFLE       // Riproduzione casuale
} PlaybackMode;

// Alias per retrocompatibilità
typedef struct MP3Queue PlaybackQueue;

// Struttura per i dati di riproduzione
typedef struct {
    PlaybackState state;      // Stato attuale della riproduzione
    PlaybackMode mode;        // Modalità di riproduzione
    MP3Queue* queue;          // Coda di riproduzione
    HWND notifyWindow;        // Finestra da notificare per gli eventi di riproduzione
    UINT notifyMessage;       // Messaggio di notifica per la riproduzione
    int volume;               // Volume (0-100)
    BOOL isOpen;              // Se un file è aperto
} AudioPlayer;

// Funzioni per la riproduzione audio
AudioPlayer* create_audio_player(HWND notifyWindow, UINT notifyMessage);
void free_audio_player(AudioPlayer* player);

// Controlli di base
BOOL play_file(AudioPlayer* player, const char* filepath);
BOOL play_current(AudioPlayer* player);
BOOL pause_playback(AudioPlayer* player);
BOOL resume_playback(AudioPlayer* player);
BOOL stop_playback(AudioPlayer* player);
BOOL next_track(AudioPlayer* player);
BOOL previous_track(AudioPlayer* player);

// Gestione della coda
BOOL add_to_queue(AudioPlayer* player, MP3File* file);
BOOL clear_queue(AudioPlayer* player);
int get_queue_size(AudioPlayer* player);
MP3File* get_current_file(AudioPlayer* player);

// Impostazioni
BOOL set_volume(AudioPlayer* player, int volume);
int get_volume(AudioPlayer* player);
BOOL set_playback_mode(AudioPlayer* player, PlaybackMode mode);
PlaybackMode get_playback_mode(AudioPlayer* player);
PlaybackState get_playback_state(AudioPlayer* player);

// Informazioni sul file corrente
int get_current_position(AudioPlayer* player);  // in millisecondi
int get_total_duration(AudioPlayer* player);    // in millisecondi

// Funzioni per l'equalizzatore
void set_eq_band(AudioPlayer* player, int frequency, float gain);  // Imposta una banda dell'equalizzatore

#endif // AUDIO_H 