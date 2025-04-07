#ifndef SETTINGS_H
#define SETTINGS_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Settings structure
typedef struct {
    // Library settings
    char library_path[MAX_PATH];
    BOOL auto_scan;
    int scan_interval;  // in seconds
    
    // Playback settings
    int volume;         // 0-100
    int playback_mode;  // 0=normal, 1=repeat one, 2=repeat all, 3=shuffle
    BOOL equalizer_enabled;
    float eq_bands[10]; // -10.0 to +10.0
    
    // UI settings
    int view_mode;      // 0=list, 1=grid, 2=album
    int sort_type;      // 0=title, 1=artist, 2=album, 3=year, 4=genre, 5=track
    BOOL show_details;
    int window_width;
    int window_height;
    BOOL maximized;
    
    // Column widths
    int column_widths[7];
} UserSettings;

// Initialize settings with default values
void settings_init(UserSettings* settings);

// Load settings from file
BOOL settings_load(UserSettings* settings, const char* filename);

// Save settings to file
BOOL settings_save(UserSettings* settings, const char* filename);

// Apply loaded settings to application
void settings_apply(UserSettings* settings);

// Default settings filename
#define DEFAULT_SETTINGS_FILE "mp3player.ini"

#endif // SETTINGS_H 