#ifndef GUI_H
#define GUI_H

#include <windows.h>
#include <commctrl.h>
#include "mp3player.h"
#include "audio.h"

// Alias per compatibilità
typedef MP3File* MP3FileList;
typedef int SortType;  // Per compatibilità con il tipo di ordinamento, utilizzando l'enum in mp3player.h

// Dimensioni della finestra
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

// ID dei controlli
#define TOOLBAR_ID 100
#define LISTVIEW_ID 101
#define STATUSBAR_ID 102
#define VOLUMEBAR_ID 103
#define VOLUMELABEL_ID 104
#define PROGRESSBAR_ID 105
#define EQ_CONTAINER_ID 106
#define ALBUMART_ID 107
#define DETAIL_VIEW_ID 108
#define TABS_ID 109

// ID dei comandi menu/toolbar
#define ID_FILE_OPEN 200
#define ID_FILE_OPEN_FOLDER 200 // Alias per retrocompatibilità
#define ID_FILE_EXIT 201

#define ID_PLAY_START 300
#define ID_PLAY_STOP 301
#define ID_PLAY_PREV 302
#define ID_PLAY_NEXT 303
#define ID_PLAY_VOLUME 304
#define ID_PLAY_MODE_NORMAL 305
#define ID_PLAY_MODE_REPEAT_ONE 306
#define ID_PLAY_MODE_REPEAT_ALL 307
#define ID_PLAY_MODE_SHUFFLE 308
#define ID_PLAY_SHOW_EQ 309

#define ID_VIEW_SORT_TITLE 401
#define ID_VIEW_SORT_ARTIST 402
#define ID_VIEW_SORT_ALBUM 403
#define ID_VIEW_SORT_YEAR 404
#define ID_VIEW_SORT_GENRE 405
#define ID_VIEW_SORT_TRACK 406
#define ID_VIEW_LIST_MODE 407
#define ID_VIEW_GRID_MODE 408
#define ID_VIEW_ALBUM_MODE 409
#define ID_VIEW_SHOW_DETAILS 410

// Indice delle colonne ListView (enum invece di define per evitare conflitti)
enum {
    COLUMN_NUMBER = 0,
    COLUMN_INDEX = 0,  // alias per retrocompatibilità
    COLUMN_TITLE,
    COLUMN_ARTIST,
    COLUMN_ALBUM,
    COLUMN_YEAR,
    COLUMN_GENRE,
    COLUMN_TRACK,
    COLUMN_COUNT
};

// Modalità di visualizzazione
enum {
    VIEW_MODE_LIST = 0,
    VIEW_MODE_GRID,
    VIEW_MODE_ALBUM
};

// Messaggi personalizzati
#define WM_AUDIO_NOTIFY (WM_USER + 1)

// Costanti per gli ID dei menu
#define ID_PLAY_PAUSE 2202

// Struttura per memorizzare lo stato della GUI
typedef struct {
    HWND hWnd;               // Finestra principale
    HWND hToolBar;           // Toolbar
    HWND hListView;          // ListView per i file MP3
    HWND hStatusBar;         // Barra di stato
    HWND hProgressBar;       // Barra di avanzamento
    HWND hVolumeBar;         // Controllo volume
    HWND hAlbumArt;          // Controllo per mostrare l'immagine dell'album
    HWND hDetailView;        // View per mostrare dettagli del brano
    HWND hTabs;              // Tab control per cambiare vista
    
    // Finestra di dialogo dell'equalizzatore
    HWND hEqDialog;          // Dialog dell'equalizzatore
    HWND hEqSliders[10];     // Slider per le bande dell'equalizzatore
    
    MP3Library* library;     // Riferimento alla libreria MP3
    AudioPlayer* player;     // Riferimento al player audio
    
    int selected_item;       // Indice dell'elemento selezionato
    SortType sort_type;      // Tipo di ordinamento corrente
    int view_mode;           // Modalità di visualizzazione corrente
    
    BOOL using_filtered_list; // Indica se stiamo visualizzando una lista filtrata
    MP3FileList* current_list; // Lista attualmente visualizzata
    
    UINT_PTR timer_id;       // ID del timer per aggiornamento della progress bar
    
    HBITMAP hAlbumBitmap;    // Handle per il bitmap dell'album
} GUIData;

// Funzioni per l'interfaccia grafica
BOOL init_gui_controls();                        // Inizializza i controlli comuni di Windows
HWND create_main_window(HINSTANCE hInstance);    // Crea la finestra principale
void create_controls(HWND hWnd, GUIData* gui);   // Crea i controlli all'interno della finestra
void populate_list_view(GUIData* gui);           // Popola la ListView con i file MP3
void update_status_bar(GUIData* gui);            // Aggiorna la barra di stato
void update_details_view(GUIData* gui);          // Aggiorna la vista dei dettagli
void create_menu(HWND hWnd);                     // Crea il menu principale
void handle_menu_action(HWND hWnd, WPARAM wParam, GUIData* gui); // Gestisce le azioni del menu
void handle_list_view_notification(HWND hWnd, LPARAM lParam, GUIData* gui); // Gestisce le notifiche della ListView
void show_filter_dialog(HWND hWnd, GUIData* gui); // Mostra la finestra di dialogo per il filtro
void create_toolbar(HWND hWnd, GUIData* gui);    // Crea la barra degli strumenti

// Funzioni per la riproduzione audio
void play_selected_file(GUIData* gui);           // Riproduce il file selezionato
void handle_playback_controls(HWND hWnd, int control_id, GUIData* gui); // Gestisce i controlli di riproduzione
void update_playback_ui(GUIData* gui);           // Aggiorna l'UI in base allo stato di riproduzione
void create_volume_control(HWND hWnd, GUIData* gui); // Crea il controllo del volume
void create_progress_bar(HWND hWnd, GUIData* gui);  // Crea la barra di avanzamento
void update_progress_bar(GUIData* gui);           // Aggiorna la barra di avanzamento
void handle_playback_notification(HWND hWnd, WPARAM wParam, LPARAM lParam, GUIData* gui); // Gestisce le notifiche di riproduzione

// Funzioni per la visualizzazione avanzata (Fase 6)
void create_album_art_view(HWND hWnd, GUIData* gui);  // Crea il controllo per l'immagine dell'album
void update_album_art(GUIData* gui, MP3File* file);   // Aggiorna l'immagine dell'album
void create_detail_view(HWND hWnd, GUIData* gui);     // Crea la vista dettagli
void create_tabs(HWND hWnd, GUIData* gui);            // Crea i tabs per cambiare visualizzazione
void switch_view_mode(GUIData* gui, int view_mode);   // Cambia modalità di visualizzazione
HBITMAP create_album_art_bitmap(MP3File* file);       // Crea un bitmap dall'album art
void free_album_art_bitmap(GUIData* gui);             // Libera il bitmap dell'album art

// Procedura della finestra principale
LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Funzione di entry point per avviare l'interfaccia grafica
int start_gui(HINSTANCE hInstance, MP3Library* library);

// Funzione per attivare/disattivare l'equalizzatore
void toggle_equalizer(GUIData* gui);

#endif // GUI_H 