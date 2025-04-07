#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mp3player.h"

// Playlist structure
typedef struct {
    char name[100];            // Playlist name
    char description[256];     // Optional description
    MP3File** tracks;          // Array of pointers to MP3File objects
    int track_count;           // Number of tracks in the playlist
    int capacity;              // Allocated capacity for tracks array
} Playlist;

// Playlist collection
typedef struct {
    Playlist** playlists;      // Array of playlists
    int count;                 // Number of playlists
    int capacity;              // Allocated capacity
} PlaylistManager;

// Create a new playlist manager
PlaylistManager* playlist_manager_create(void);

// Free a playlist manager and all its playlists
void playlist_manager_free(PlaylistManager* manager);

// Create a new empty playlist
Playlist* playlist_create(const char* name, const char* description);

// Add a track to a playlist
BOOL playlist_add_track(Playlist* playlist, MP3File* track);

// Remove a track from a playlist
BOOL playlist_remove_track(Playlist* playlist, int index);

// Move a track within a playlist (change order)
BOOL playlist_move_track(Playlist* playlist, int from_index, int to_index);

// Get the track at the specified index
MP3File* playlist_get_track(Playlist* playlist, int index);

// Clear all tracks from a playlist
void playlist_clear(Playlist* playlist);

// Free a playlist and its resources
void playlist_free(Playlist* playlist);

// Save a playlist to file
BOOL playlist_save(Playlist* playlist, const char* filename);

// Load a playlist from file
Playlist* playlist_load(const char* filename, MP3Library* library);

// Add a playlist to the manager
BOOL playlist_manager_add(PlaylistManager* manager, Playlist* playlist);

// Remove a playlist from the manager
BOOL playlist_manager_remove(PlaylistManager* manager, int index);

// Get a playlist by index
Playlist* playlist_manager_get(PlaylistManager* manager, int index);

// Get a playlist by name
Playlist* playlist_manager_find_by_name(PlaylistManager* manager, const char* name);

// Save all playlists to files
BOOL playlist_manager_save_all(PlaylistManager* manager, const char* directory);

// Load all playlists from files in a directory
BOOL playlist_manager_load_all(PlaylistManager* manager, const char* directory, MP3Library* library);

#endif // PLAYLIST_H 