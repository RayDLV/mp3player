#include "../include/settings.h"
#include "../include/mp3player.h"
#include "../include/memory.h"
#include "../include/gui.h"
#include "../include/audio.h"

// Define sections for the INI file
#define SECTION_LIBRARY "Library"
#define SECTION_PLAYBACK "Playback"
#define SECTION_UI "UI"
#define SECTION_COLUMNS "Columns"

// Initialize settings with default values
void settings_init(UserSettings* settings) {
    if (!settings) return;
    
    // Zero out the structure
    memset(settings, 0, sizeof(UserSettings));
    
    // Library settings
    GetCurrentDirectory(MAX_PATH, settings->library_path);
    settings->auto_scan = TRUE;
    settings->scan_interval = 60;  // 1 minute
    
    // Playback settings
    settings->volume = 80;
    settings->playback_mode = 0;   // Normal
    settings->equalizer_enabled = FALSE;
    
    // Initialize EQ bands to 0 (flat)
    for (int i = 0; i < 10; i++) {
        settings->eq_bands[i] = 0.0f;
    }
    
    // UI settings
    settings->view_mode = VIEW_MODE_LIST;
    settings->sort_type = SORT_BY_TITLE;
    settings->show_details = TRUE;
    settings->window_width = 800;
    settings->window_height = 600;
    settings->maximized = FALSE;
    
    // Default column widths
    int default_widths[7] = {40, 200, 150, 150, 60, 100, 60};
    memcpy(settings->column_widths, default_widths, sizeof(default_widths));
}

// Load settings from file
BOOL settings_load(UserSettings* settings, const char* filename) {
    if (!settings || !filename) return FALSE;
    
    // First, initialize with defaults in case the file is missing or incomplete
    settings_init(settings);
    
    // Load library settings
    GetPrivateProfileString(
        SECTION_LIBRARY, "Path", settings->library_path, 
        settings->library_path, MAX_PATH, filename);
    
    settings->auto_scan = GetPrivateProfileInt(
        SECTION_LIBRARY, "AutoScan", settings->auto_scan, filename);
    
    settings->scan_interval = GetPrivateProfileInt(
        SECTION_LIBRARY, "ScanInterval", settings->scan_interval, filename);
    
    // Load playback settings
    settings->volume = GetPrivateProfileInt(
        SECTION_PLAYBACK, "Volume", settings->volume, filename);
    
    settings->playback_mode = GetPrivateProfileInt(
        SECTION_PLAYBACK, "Mode", settings->playback_mode, filename);
    
    settings->equalizer_enabled = GetPrivateProfileInt(
        SECTION_PLAYBACK, "EqualizerEnabled", settings->equalizer_enabled, filename);
    
    // Load EQ bands
    char key[20];
    char value[20];
    for (int i = 0; i < 10; i++) {
        sprintf(key, "EQBand%d", i);
        if (GetPrivateProfileString(SECTION_PLAYBACK, key, "", value, sizeof(value), filename)) {
            settings->eq_bands[i] = (float)atof(value);
        }
    }
    
    // Load UI settings
    settings->view_mode = GetPrivateProfileInt(
        SECTION_UI, "ViewMode", settings->view_mode, filename);
    
    settings->sort_type = GetPrivateProfileInt(
        SECTION_UI, "SortType", settings->sort_type, filename);
    
    settings->show_details = GetPrivateProfileInt(
        SECTION_UI, "ShowDetails", settings->show_details, filename);
    
    settings->window_width = GetPrivateProfileInt(
        SECTION_UI, "WindowWidth", settings->window_width, filename);
    
    settings->window_height = GetPrivateProfileInt(
        SECTION_UI, "WindowHeight", settings->window_height, filename);
    
    settings->maximized = GetPrivateProfileInt(
        SECTION_UI, "Maximized", settings->maximized, filename);
    
    // Load column widths
    for (int i = 0; i < 7; i++) {
        sprintf(key, "Column%d", i);
        settings->column_widths[i] = GetPrivateProfileInt(
            SECTION_COLUMNS, key, settings->column_widths[i], filename);
    }
    
    return TRUE;
}

// Save settings to file
BOOL settings_save(UserSettings* settings, const char* filename) {
    if (!settings || !filename) return FALSE;
    
    char value[256];
    
    // Save library settings
    WritePrivateProfileString(SECTION_LIBRARY, "Path", settings->library_path, filename);
    
    sprintf(value, "%d", settings->auto_scan);
    WritePrivateProfileString(SECTION_LIBRARY, "AutoScan", value, filename);
    
    sprintf(value, "%d", settings->scan_interval);
    WritePrivateProfileString(SECTION_LIBRARY, "ScanInterval", value, filename);
    
    // Save playback settings
    sprintf(value, "%d", settings->volume);
    WritePrivateProfileString(SECTION_PLAYBACK, "Volume", value, filename);
    
    sprintf(value, "%d", settings->playback_mode);
    WritePrivateProfileString(SECTION_PLAYBACK, "Mode", value, filename);
    
    sprintf(value, "%d", settings->equalizer_enabled);
    WritePrivateProfileString(SECTION_PLAYBACK, "EqualizerEnabled", value, filename);
    
    // Save EQ bands
    char key[20];
    for (int i = 0; i < 10; i++) {
        sprintf(key, "EQBand%d", i);
        sprintf(value, "%.1f", settings->eq_bands[i]);
        WritePrivateProfileString(SECTION_PLAYBACK, key, value, filename);
    }
    
    // Save UI settings
    sprintf(value, "%d", settings->view_mode);
    WritePrivateProfileString(SECTION_UI, "ViewMode", value, filename);
    
    sprintf(value, "%d", settings->sort_type);
    WritePrivateProfileString(SECTION_UI, "SortType", value, filename);
    
    sprintf(value, "%d", settings->show_details);
    WritePrivateProfileString(SECTION_UI, "ShowDetails", value, filename);
    
    sprintf(value, "%d", settings->window_width);
    WritePrivateProfileString(SECTION_UI, "WindowWidth", value, filename);
    
    sprintf(value, "%d", settings->window_height);
    WritePrivateProfileString(SECTION_UI, "WindowHeight", value, filename);
    
    sprintf(value, "%d", settings->maximized);
    WritePrivateProfileString(SECTION_UI, "Maximized", value, filename);
    
    // Save column widths
    for (int i = 0; i < 7; i++) {
        sprintf(key, "Column%d", i);
        sprintf(value, "%d", settings->column_widths[i]);
        WritePrivateProfileString(SECTION_COLUMNS, key, value, filename);
    }
    
    return TRUE;
}

// Apply loaded settings to the application
void settings_apply(UserSettings* settings) {
    // This function will be expanded as we integrate it into the GUI
    if (!settings) return;
    
    // Apply settings to the appropriate components as they are implemented
} 