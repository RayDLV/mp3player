#include "../include/playlist.h"
#include "../include/memory.h"

#define INITIAL_PLAYLIST_CAPACITY 16
#define INITIAL_MANAGER_CAPACITY 8
#define PLAYLIST_FILE_EXTENSION ".m3plist"

// Create a new playlist manager
PlaylistManager* playlist_manager_create(void) {
    PlaylistManager* manager = (PlaylistManager*)MEM_ALLOC(sizeof(PlaylistManager));
    if (!manager) return NULL;
    
    manager->playlists = (Playlist**)MEM_ALLOC(INITIAL_MANAGER_CAPACITY * sizeof(Playlist*));
    if (!manager->playlists) {
        MEM_FREE(manager);
        return NULL;
    }
    
    manager->count = 0;
    manager->capacity = INITIAL_MANAGER_CAPACITY;
    
    return manager;
}

// Free a playlist manager and all its playlists
void playlist_manager_free(PlaylistManager* manager) {
    if (!manager) return;
    
    // Free all playlists
    for (int i = 0; i < manager->count; i++) {
        playlist_free(manager->playlists[i]);
    }
    
    // Free the array and the manager
    MEM_FREE(manager->playlists);
    MEM_FREE(manager);
}

// Create a new empty playlist
Playlist* playlist_create(const char* name, const char* description) {
    Playlist* playlist = (Playlist*)MEM_ALLOC(sizeof(Playlist));
    if (!playlist) return NULL;
    
    // Initialize fields
    strncpy(playlist->name, name ? name : "New Playlist", sizeof(playlist->name) - 1);
    playlist->name[sizeof(playlist->name) - 1] = '\0';
    
    strncpy(playlist->description, description ? description : "", sizeof(playlist->description) - 1);
    playlist->description[sizeof(playlist->description) - 1] = '\0';
    
    playlist->tracks = (MP3File**)MEM_ALLOC(INITIAL_PLAYLIST_CAPACITY * sizeof(MP3File*));
    if (!playlist->tracks) {
        MEM_FREE(playlist);
        return NULL;
    }
    
    playlist->track_count = 0;
    playlist->capacity = INITIAL_PLAYLIST_CAPACITY;
    
    return playlist;
}

// Ensure the playlist has enough capacity for a new track
static BOOL ensure_playlist_capacity(Playlist* playlist) {
    if (playlist->track_count >= playlist->capacity) {
        int new_capacity = playlist->capacity * 2;
        MP3File** new_tracks = (MP3File**)MEM_REALLOC(playlist->tracks, new_capacity * sizeof(MP3File*));
        
        if (!new_tracks) return FALSE;
        
        playlist->tracks = new_tracks;
        playlist->capacity = new_capacity;
    }
    
    return TRUE;
}

// Ensure the manager has enough capacity for a new playlist
static BOOL ensure_manager_capacity(PlaylistManager* manager) {
    if (manager->count >= manager->capacity) {
        int new_capacity = manager->capacity * 2;
        Playlist** new_playlists = (Playlist**)MEM_REALLOC(manager->playlists, new_capacity * sizeof(Playlist*));
        
        if (!new_playlists) return FALSE;
        
        manager->playlists = new_playlists;
        manager->capacity = new_capacity;
    }
    
    return TRUE;
}

// Add a track to a playlist
BOOL playlist_add_track(Playlist* playlist, MP3File* track) {
    if (!playlist || !track) return FALSE;
    
    // Make sure we have enough capacity
    if (!ensure_playlist_capacity(playlist)) return FALSE;
    
    // Add the track to the playlist (just store the pointer)
    playlist->tracks[playlist->track_count++] = track;
    
    return TRUE;
}

// Remove a track from a playlist
BOOL playlist_remove_track(Playlist* playlist, int index) {
    if (!playlist || index < 0 || index >= playlist->track_count) return FALSE;
    
    // Shift all tracks after the removed one
    for (int i = index; i < playlist->track_count - 1; i++) {
        playlist->tracks[i] = playlist->tracks[i + 1];
    }
    
    playlist->track_count--;
    return TRUE;
}

// Move a track within a playlist (change order)
BOOL playlist_move_track(Playlist* playlist, int from_index, int to_index) {
    if (!playlist || from_index < 0 || from_index >= playlist->track_count || 
        to_index < 0 || to_index >= playlist->track_count) {
        return FALSE;
    }
    
    if (from_index == to_index) return TRUE; // Nothing to do
    
    // Save the track being moved
    MP3File* track = playlist->tracks[from_index];
    
    // Move tracks up or down as needed
    if (from_index < to_index) {
        // Move tracks up
        for (int i = from_index; i < to_index; i++) {
            playlist->tracks[i] = playlist->tracks[i + 1];
        }
    } else {
        // Move tracks down
        for (int i = from_index; i > to_index; i--) {
            playlist->tracks[i] = playlist->tracks[i - 1];
        }
    }
    
    // Insert the track at its new position
    playlist->tracks[to_index] = track;
    
    return TRUE;
}

// Get the track at the specified index
MP3File* playlist_get_track(Playlist* playlist, int index) {
    if (!playlist || index < 0 || index >= playlist->track_count) return NULL;
    return playlist->tracks[index];
}

// Clear all tracks from a playlist
void playlist_clear(Playlist* playlist) {
    if (!playlist) return;
    playlist->track_count = 0;
}

// Free a playlist and its resources
void playlist_free(Playlist* playlist) {
    if (!playlist) return;
    
    // Free the tracks array (not the tracks themselves, as those belong to the library)
    MEM_FREE(playlist->tracks);
    MEM_FREE(playlist);
}

// Find a track in the library by its filepath
static MP3File* find_track_by_filepath(MP3Library* library, const char* filepath) {
    if (!library || !filepath) return NULL;
    
    MP3File* current = library->all_files;
    while (current) {
        if (strcmp(current->filepath, filepath) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// Save a playlist to file
BOOL playlist_save(Playlist* playlist, const char* filename) {
    if (!playlist || !filename) return FALSE;
    
    FILE* file = fopen(filename, "w");
    if (!file) return FALSE;
    
    // Write the playlist name and description
    fprintf(file, "[Playlist]\n");
    fprintf(file, "Name=%s\n", playlist->name);
    fprintf(file, "Description=%s\n", playlist->description);
    fprintf(file, "TrackCount=%d\n\n", playlist->track_count);
    
    // Write each track's filepath
    fprintf(file, "[Tracks]\n");
    for (int i = 0; i < playlist->track_count; i++) {
        MP3File* track = playlist->tracks[i];
        if (track) {
            fprintf(file, "%d=%s\n", i, track->filepath);
        }
    }
    
    fclose(file);
    return TRUE;
}

// Load a playlist from file
Playlist* playlist_load(const char* filename, MP3Library* library) {
    if (!filename || !library) return NULL;
    
    FILE* file = fopen(filename, "r");
    if (!file) return NULL;
    
    char buffer[MAX_PATH];
    char name[100] = "Unnamed Playlist";
    char description[256] = "";
    int track_count = 0;
    
    // Read the playlist header
    while (fgets(buffer, sizeof(buffer), file)) {
        buffer[strcspn(buffer, "\r\n")] = 0; // Remove newline
        
        if (strncmp(buffer, "[Tracks]", 8) == 0) {
            break; // Start of tracks section
        }
        
        if (strncmp(buffer, "Name=", 5) == 0) {
            strncpy(name, buffer + 5, sizeof(name) - 1);
            name[sizeof(name) - 1] = '\0';
        } else if (strncmp(buffer, "Description=", 12) == 0) {
            strncpy(description, buffer + 12, sizeof(description) - 1);
            description[sizeof(description) - 1] = '\0';
        } else if (strncmp(buffer, "TrackCount=", 11) == 0) {
            track_count = atoi(buffer + 11);
        }
    }
    
    // Create the playlist
    Playlist* playlist = playlist_create(name, description);
    if (!playlist) {
        fclose(file);
        return NULL;
    }
    
    // Read each track
    while (fgets(buffer, sizeof(buffer), file)) {
        buffer[strcspn(buffer, "\r\n")] = 0; // Remove newline
        
        // Find the '=' separator
        char* separator = strchr(buffer, '=');
        if (!separator) continue;
        
        // Get the filepath
        char* filepath = separator + 1;
        
        // Find the track in the library
        MP3File* track = find_track_by_filepath(library, filepath);
        if (track) {
            playlist_add_track(playlist, track);
        }
    }
    
    fclose(file);
    return playlist;
}

// Add a playlist to the manager
BOOL playlist_manager_add(PlaylistManager* manager, Playlist* playlist) {
    if (!manager || !playlist) return FALSE;
    
    // Make sure we have enough capacity
    if (!ensure_manager_capacity(manager)) return FALSE;
    
    // Add the playlist
    manager->playlists[manager->count++] = playlist;
    
    return TRUE;
}

// Remove a playlist from the manager
BOOL playlist_manager_remove(PlaylistManager* manager, int index) {
    if (!manager || index < 0 || index >= manager->count) return FALSE;
    
    // Free the playlist
    playlist_free(manager->playlists[index]);
    
    // Shift all playlists after the removed one
    for (int i = index; i < manager->count - 1; i++) {
        manager->playlists[i] = manager->playlists[i + 1];
    }
    
    manager->count--;
    return TRUE;
}

// Get a playlist by index
Playlist* playlist_manager_get(PlaylistManager* manager, int index) {
    if (!manager || index < 0 || index >= manager->count) return NULL;
    return manager->playlists[index];
}

// Get a playlist by name
Playlist* playlist_manager_find_by_name(PlaylistManager* manager, const char* name) {
    if (!manager || !name) return NULL;
    
    for (int i = 0; i < manager->count; i++) {
        if (strcmp(manager->playlists[i]->name, name) == 0) {
            return manager->playlists[i];
        }
    }
    
    return NULL;
}

// Save all playlists to files
BOOL playlist_manager_save_all(PlaylistManager* manager, const char* directory) {
    if (!manager || !directory) return FALSE;
    
    // Create the directory if it doesn't exist
    CreateDirectory(directory, NULL);
    
    BOOL success = TRUE;
    
    // Save each playlist
    for (int i = 0; i < manager->count; i++) {
        Playlist* playlist = manager->playlists[i];
        
        // Create a filename from the playlist name
        char filename[MAX_PATH];
        sprintf(filename, "%s\\%s%s", directory, playlist->name, PLAYLIST_FILE_EXTENSION);
        
        // Replace invalid filename characters
        for (char* p = filename + strlen(directory) + 1; *p; p++) {
            if (*p == '\\' || *p == '/' || *p == ':' || *p == '*' || 
                *p == '?' || *p == '"' || *p == '<' || *p == '>' || *p == '|') {
                *p = '_';
            }
        }
        
        // Save the playlist
        if (!playlist_save(playlist, filename)) {
            success = FALSE;
        }
    }
    
    return success;
}

// Load all playlists from files in a directory
BOOL playlist_manager_load_all(PlaylistManager* manager, const char* directory, MP3Library* library) {
    if (!manager || !directory || !library) return FALSE;
    
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char search_path[MAX_PATH];
    BOOL success = FALSE;
    
    // Make sure the directory exists
    if (GetFileAttributes(directory) == INVALID_FILE_ATTRIBUTES) {
        // Create it if it doesn't exist
        if (!CreateDirectory(directory, NULL)) {
            return FALSE;
        }
    }
    
    // Construct the search pattern for playlist files
    sprintf(search_path, "%s\\*%s", directory, PLAYLIST_FILE_EXTENSION);
    
    // Find the first file
    hFind = FindFirstFile(search_path, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    
    success = TRUE; // At least one file was found
    
    do {
        // Skip directories
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        
        // Construct the full path
        char filepath[MAX_PATH];
        sprintf(filepath, "%s\\%s", directory, findFileData.cFileName);
        
        // Load the playlist
        Playlist* playlist = playlist_load(filepath, library);
        if (playlist) {
            playlist_manager_add(manager, playlist);
        }
        
    } while (FindNextFile(hFind, &findFileData) != 0);
    
    FindClose(hFind);
    return success;
} 