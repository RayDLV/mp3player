#include "../include/gui.h"
#include <stdio.h>
#include <windowsx.h>
#include <shlobj.h>  // Per la funzione di selezione cartella
#include <objidl.h>  // Per IStream

// Includi GDI+ usando l'interfaccia C
#define WINGDIPAPI __stdcall
#define GDIPCONST const
#include <gdiplus.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib") // Collega GDI+

// Variabile globale per memorizzare i dati della GUI
static GUIData g_gui_data;

// Variabile per tenere traccia dell'elemento attualmente evidenziato
static int currently_highlighted_item = -1;

// Nome della classe della finestra
static const char* const WINDOW_CLASS_NAME = "MP3PlayerWindow";
static const char* const WINDOW_TITLE = "MP3 Player";

// ID dei controlli di equalizzazione
#define ID_EQ_60HZ 500
#define ID_EQ_170HZ 501
#define ID_EQ_310HZ 502
#define ID_EQ_600HZ 503
#define ID_EQ_1KHZ 504
#define ID_EQ_3KHZ 505
#define ID_EQ_6KHZ 506
#define ID_EQ_12KHZ 507
#define ID_EQ_14KHZ 508
#define ID_EQ_16KHZ 509
#define ID_EQ_RESET 510

// Frequenze dell'equalizzatore (in Hz)
static const int eq_freqs[] = {60, 170, 310, 600, 1000, 3000, 6000, 12000, 14000, 16000};
static const char* eq_labels[] = {"60", "170", "310", "600", "1K", "3K", "6K", "12K", "14K", "16K"};
#define EQ_BANDS 10

// Inizializza i controlli comuni di Windows
BOOL init_gui_controls() {
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    return InitCommonControlsEx(&icc);
}

// Crea la finestra principale
HWND create_main_window(HINSTANCE hInstance) {
    // Registra la classe della finestra
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));
    
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Registrazione della classe della finestra fallita", "Errore", MB_ICONERROR);
        return NULL;
    }
    
    // Crea la finestra
    HWND hWnd = CreateWindowEx(
        0,                          // Stile esteso
        WINDOW_CLASS_NAME,          // Nome della classe
        WINDOW_TITLE,               // Titolo della finestra
        WS_OVERLAPPEDWINDOW,        // Stile della finestra
        CW_USEDEFAULT, CW_USEDEFAULT, // Posizione
        WINDOW_WIDTH, WINDOW_HEIGHT, // Dimensioni
        NULL,                       // Finestra genitore
        NULL,                       // Menu (verrà creato successivamente)
        hInstance,                  // Istanza
        NULL                        // Dati aggiuntivi
    );
    
    if (!hWnd) {
        MessageBox(NULL, "Creazione della finestra fallita", "Errore", MB_ICONERROR);
        return NULL;
    }
    
    return hWnd;
}

// Crea i controlli all'interno della finestra
void create_controls(HWND hWnd, GUIData* gui) {
    // Memorizza l'handle della finestra
    gui->hWnd = hWnd;
    
    // Crea la ListView per la lista dei brani
    gui->hListView = CreateWindowEx(
        0,                          // Stile esteso
        WC_LISTVIEW,                // Nome della classe
        "",                         // Testo
        WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDRAWFIXED,
        0, 0, 0, 0,                 // Posizione e dimensioni (saranno impostate in seguito)
        hWnd,                       // Finestra genitore
        (HMENU)LISTVIEW_ID,         // ID
        GetModuleHandle(NULL),      // Istanza
        NULL                        // Dati aggiuntivi
    );
    
    // Imposta un font migliore per la ListView
    HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    SendMessage(gui->hListView, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Aggiungi lo stile esteso alla ListView
    ListView_SetExtendedListViewStyle(
        gui->hListView, 
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP
    );
    
    // Aggiungi le colonne alla ListView
    LVCOLUMN lvc;
    ZeroMemory(&lvc, sizeof(LVCOLUMN));
    
    lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_FMT;
    lvc.fmt = LVCFMT_LEFT;
    
    const char* column_names[] = {
        "#", "Title", "Artist", "Album", "Year", "Genre", "Track"
    };
    
    int column_widths[] = {
        40, 200, 150, 150, 60, 100, 60
    };
    
    for (int i = 0; i < COLUMN_COUNT; i++) {
        lvc.iSubItem = i;
        lvc.pszText = (char*)column_names[i];
        lvc.cx = column_widths[i];
        ListView_InsertColumn(gui->hListView, i, &lvc);
    }
    
    // Crea la StatusBar
    gui->hStatusBar = CreateWindowEx(
        0,                          // Stile esteso
        STATUSCLASSNAME,            // Nome della classe
        "",                         // Testo
        WS_VISIBLE | WS_CHILD,      // Stile
        0, 0, 0, 0,                 // Posizione e dimensioni (saranno impostate in seguito)
        hWnd,                       // Finestra genitore
        (HMENU)STATUSBAR_ID,        // ID
        GetModuleHandle(NULL),      // Istanza
        NULL                        // Dati aggiuntivi
    );
    
    // Crea la barra di avanzamento
    create_progress_bar(hWnd, gui);
    
    // Crea il controllo del volume
    create_volume_control(hWnd, gui);
    
    // Crea la ToolBar
    create_toolbar(hWnd, gui);
    
    // Crea il menu principale
    create_menu(hWnd);
    
    // Crea i nuovi controlli per la visualizzazione avanzata (Fase 6)
    create_album_art_view(hWnd, gui);
    create_detail_view(hWnd, gui);
    create_tabs(hWnd, gui);
    
    // Inizializza la modalità di visualizzazione
    gui->view_mode = VIEW_MODE_LIST;
    
    // Crea il player audio
    gui->player = create_audio_player(hWnd, WM_AUDIO_NOTIFY);
    
    // Imposta il volume iniziale
    set_volume(gui->player, 80);
}

// Crea la barra degli strumenti
void create_toolbar(HWND hWnd, GUIData* gui) {
    gui->hToolBar = CreateWindowEx(
        0,                          // Stile esteso
        TOOLBARCLASSNAME,           // Nome della classe
        "",                         // Testo
        WS_VISIBLE | WS_CHILD | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_ADJUSTABLE,
        0, 0, 0, 0,                 // Posizione e dimensioni (saranno impostate in seguito)
        hWnd,                       // Finestra genitore
        (HMENU)TOOLBAR_ID,          // ID
        GetModuleHandle(NULL),      // Istanza
        NULL                        // Dati aggiuntivi
    );
    
    // Imposta la dimensione dei pulsanti della toolbar
    SendMessage(gui->hToolBar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    
    // Imposta la dimensione dei pulsanti
    SendMessage(gui->hToolBar, TB_SETBUTTONSIZE, 0, MAKELPARAM(60, 40));
    
    // Definizione dei pulsanti della toolbar
    TBBUTTON tbb[6];
    ZeroMemory(tbb, sizeof(tbb));
    
    // Pulsante Play
    tbb[0].iBitmap = I_IMAGENONE;
    tbb[0].idCommand = ID_PLAY_START;
    tbb[0].fsState = TBSTATE_ENABLED;
    tbb[0].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
    tbb[0].iString = (INT_PTR)"Play";
    
    // Pulsante Pause
    tbb[1].iBitmap = I_IMAGENONE;
    tbb[1].idCommand = ID_PLAY_PAUSE;
    tbb[1].fsState = TBSTATE_ENABLED;
    tbb[1].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
    tbb[1].iString = (INT_PTR)"Pause";
    
    // Pulsante Stop
    tbb[2].iBitmap = I_IMAGENONE;
    tbb[2].idCommand = ID_PLAY_STOP;
    tbb[2].fsState = TBSTATE_ENABLED;
    tbb[2].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
    tbb[2].iString = (INT_PTR)"Stop";
    
    // Separatore
    tbb[3].fsStyle = TBSTYLE_SEP;
    
    // Pulsante Precedente
    tbb[4].iBitmap = I_IMAGENONE;
    tbb[4].idCommand = ID_PLAY_PREV;
    tbb[4].fsState = TBSTATE_ENABLED;
    tbb[4].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
    tbb[4].iString = (INT_PTR)"Prev";
    
    // Pulsante Successivo
    tbb[5].iBitmap = I_IMAGENONE;
    tbb[5].idCommand = ID_PLAY_NEXT;
    tbb[5].fsState = TBSTATE_ENABLED;
    tbb[5].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
    tbb[5].iString = (INT_PTR)"Next";
    
    // Aggiungi i pulsanti alla toolbar
    SendMessage(gui->hToolBar, TB_ADDBUTTONS, 6, (LPARAM)&tbb);
    
    // Aggiorna le dimensioni della toolbar
    SendMessage(gui->hToolBar, TB_AUTOSIZE, 0, 0);
}

// Crea il menu principale
void create_menu(HWND hWnd) {
    // Crea un menu
    HMENU hMenu = CreateMenu();
    
    // Crea i sottomenu
    HMENU hFileMenu = CreatePopupMenu();
    HMENU hPlaybackMenu = CreatePopupMenu();
    HMENU hPlaybackModeMenu = CreatePopupMenu();
    HMENU hToolsMenu = CreatePopupMenu();
    HMENU hViewMenu = CreatePopupMenu(); // Nuovo menu per la visualizzazione
    HMENU hSortMenu = CreatePopupMenu(); // Sottomenu per l'ordinamento
    HMENU hLayoutMenu = CreatePopupMenu(); // Sottomenu per il layout
    
    // Aggiungi voci al menu File
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_OPEN_FOLDER, "Open Folder...");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_EXIT, "Exit");
    
    // Aggiungi voci al menu Riproduzione
    AppendMenu(hPlaybackMenu, MF_STRING, ID_PLAY_START, "Play/Pause");
    AppendMenu(hPlaybackMenu, MF_STRING, ID_PLAY_STOP, "Stop");
    AppendMenu(hPlaybackMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hPlaybackMenu, MF_STRING, ID_PLAY_PREV, "Previous");
    AppendMenu(hPlaybackMenu, MF_STRING, ID_PLAY_NEXT, "Next");
    
    // Aggiungi voci al sottomenu Modalità di riproduzione
    AppendMenu(hPlaybackModeMenu, MF_STRING, ID_PLAY_MODE_NORMAL, "Normal");
    AppendMenu(hPlaybackModeMenu, MF_STRING, ID_PLAY_MODE_REPEAT_ONE, "Loop One");
    AppendMenu(hPlaybackModeMenu, MF_STRING, ID_PLAY_MODE_REPEAT_ALL, "Loop All");
    AppendMenu(hPlaybackModeMenu, MF_STRING, ID_PLAY_MODE_SHUFFLE, "Shuffle");
    
    // Aggiungi il sottomenu Modalità al menu Riproduzione
    AppendMenu(hPlaybackMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hPlaybackMenu, MF_POPUP, (UINT_PTR)hPlaybackModeMenu, "Playback Mode");
    
    // Aggiungi voci al menu Strumenti
    AppendMenu(hToolsMenu, MF_STRING, ID_PLAY_SHOW_EQ, "Equalizer");
    
    // Aggiungi voci al sottomenu Ordinamento
    AppendMenu(hSortMenu, MF_STRING, ID_VIEW_SORT_TITLE, "By Title");
    AppendMenu(hSortMenu, MF_STRING, ID_VIEW_SORT_ARTIST, "By Artist");
    AppendMenu(hSortMenu, MF_STRING, ID_VIEW_SORT_ALBUM, "By Album");
    AppendMenu(hSortMenu, MF_STRING, ID_VIEW_SORT_YEAR, "By Year");
    AppendMenu(hSortMenu, MF_STRING, ID_VIEW_SORT_GENRE, "By Genre");
    AppendMenu(hSortMenu, MF_STRING, ID_VIEW_SORT_TRACK, "By Track Number");
    
    // Aggiungi voci al sottomenu Layout
    AppendMenu(hLayoutMenu, MF_STRING, ID_VIEW_LIST_MODE, "List");
    AppendMenu(hLayoutMenu, MF_STRING, ID_VIEW_GRID_MODE, "Grid");
    AppendMenu(hLayoutMenu, MF_STRING, ID_VIEW_ALBUM_MODE, "Album");
    
    // Aggiungi voci al menu Visualizzazione
    AppendMenu(hViewMenu, MF_POPUP, (UINT_PTR)hSortMenu, "Sort");
    AppendMenu(hViewMenu, MF_POPUP, (UINT_PTR)hLayoutMenu, "Layout");
    AppendMenu(hViewMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hViewMenu, MF_STRING, ID_VIEW_SHOW_DETAILS, "Show Details");
    
    // Aggiungi i sottomenu al menu principale
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, "File");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hPlaybackMenu, "Playback");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hViewMenu, "View");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hToolsMenu, "Tools");
    
    // Imposta il menu
    SetMenu(hWnd, hMenu);
}

// Crea il controllo del volume
void create_volume_control(HWND hWnd, GUIData* gui) {
    // Crea l'etichetta "Volume: " prima dello slider
    CreateWindowEx(
        0,                          // Stile esteso
        "STATIC",                   // Nome della classe
        "Volume:",                  // Testo
        WS_VISIBLE | WS_CHILD | SS_RIGHT,
        0, 0, 60, 25,               // Posizione e dimensioni (saranno aggiornate in resize_controls)
        hWnd,                       // Finestra genitore
        (HMENU)VOLUMELABEL_ID,      // ID
        GetModuleHandle(NULL),      // Istanza
        NULL                        // Dati aggiuntivi
    );
    
    // Crea il controllo slider per il volume
    gui->hVolumeBar = CreateWindowEx(
        0,                          // Stile esteso
        TRACKBAR_CLASS,             // Nome della classe
        "",                         // Testo
        WS_VISIBLE | WS_CHILD | TBS_HORZ | TBS_AUTOTICKS | TBS_BOTH,
        0, 0, 0, 0,                 // Posizione e dimensioni (saranno impostate in seguito)
        hWnd,                       // Finestra genitore
        (HMENU)VOLUMEBAR_ID,        // ID
        GetModuleHandle(NULL),      // Istanza
        NULL                        // Dati aggiuntivi
    );
    
    // Imposta il range del volume (0-100)
    SendMessage(gui->hVolumeBar, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    
    // Imposta il valore iniziale
    SendMessage(gui->hVolumeBar, TBM_SETPOS, TRUE, 80);
    
    // Aggiungi i tick marks principali
    SendMessage(gui->hVolumeBar, TBM_SETTICFREQ, 10, 0);
}

// Crea la barra di avanzamento
void create_progress_bar(HWND hWnd, GUIData* gui) {
    // Crea la barra di avanzamento
    gui->hProgressBar = CreateWindowEx(
        0,                          // Stile esteso
        PROGRESS_CLASS,             // Nome della classe
        "",                         // Testo
        WS_VISIBLE | WS_CHILD,      // Stile
        0, 0, 0, 0,                 // Posizione e dimensioni (saranno impostate in seguito)
        hWnd,                       // Finestra genitore
        (HMENU)PROGRESSBAR_ID,      // ID
        GetModuleHandle(NULL),      // Istanza
        NULL                        // Dati aggiuntivi
    );
    
    // Imposta il range della barra di avanzamento (0-1000)
    SendMessage(gui->hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 1000));
}

// Aggiorna la barra di avanzamento
void update_progress_bar(GUIData* gui) {
    if (!gui || !gui->player || !gui->hProgressBar) return;
    
    // Ottiene la posizione corrente e la durata totale
    int position = get_current_position(gui->player);
    int duration = get_total_duration(gui->player);
    
    // Aggiorna la barra di stato con la posizione corrente
    if (duration > 0) {
        // Conversione in formato MM:SS
        int pos_sec = position / 1000;
        int dur_sec = duration / 1000;
        
        // Otteniamo il file corrente in riproduzione
        MP3File* current = get_current_file(gui->player);
        
        if (current) {
            char status[512]; // Buffer più grande per contenere tutte le informazioni
            
            // Formatta una stringa che contiene tutte le informazioni richieste ma senza etichette
            sprintf(status, "%02d:%02d / %02d:%02d | %s | %s | %s | %d", 
                    pos_sec / 60, pos_sec % 60, 
                    dur_sec / 60, dur_sec % 60,
                    current->metadata.title[0] ? current->metadata.title : "Sconosciuto",
                    current->metadata.artist[0] ? current->metadata.artist : "Sconosciuto",
                    current->metadata.album[0] ? current->metadata.album : "Sconosciuto",
                    current->metadata.year > 0 ? current->metadata.year : 0);
            
            SetWindowText(gui->hStatusBar, status);
        } else {
            char status[128];
            sprintf(status, "Riproduzione: %02d:%02d / %02d:%02d", 
                    pos_sec / 60, pos_sec % 60, 
                    dur_sec / 60, dur_sec % 60);
            
            SetWindowText(gui->hStatusBar, status);
        }
        
        // Aggiorna la progress bar (mappando la posizione su 0-1000)
        int progress = (int)((position * 1000.0) / duration);
        SendMessage(gui->hProgressBar, PBM_SETPOS, progress, 0);
    }
}

// Ridimensiona i controlli nella finestra
void resize_controls(HWND hWnd, GUIData* gui) {
    RECT rcClient;
    GetClientRect(hWnd, &rcClient);
    
    // Calcola le dimensioni della StatusBar
    int statusHeight = 20;
    MoveWindow(gui->hStatusBar, 0, rcClient.bottom - statusHeight, 
               rcClient.right, statusHeight, TRUE);
    
    // Calcola le dimensioni della ToolBar
    RECT rcTool;
    GetWindowRect(gui->hToolBar, &rcTool);
    int toolHeight = rcTool.bottom - rcTool.top;
    MoveWindow(gui->hToolBar, 0, 0, rcClient.right, toolHeight, TRUE);
    
    // Calcola le dimensioni della progress bar (sopra la status bar)
    int progressHeight = 15;
    MoveWindow(gui->hProgressBar, 0, rcClient.bottom - statusHeight - progressHeight, 
               rcClient.right, progressHeight, TRUE);
    
    // Calcola le dimensioni del controllo volume (nella parte bassa, sopra la progress bar)
    int volumeHeight = 25;
    int volumeWidth = 200;
    int volumeLabelWidth = 60;
    int volumeY = rcClient.bottom - statusHeight - progressHeight - volumeHeight - 5;
    
    // Posiziona l'etichetta del volume
    HWND hVolumeLabel = GetDlgItem(hWnd, VOLUMELABEL_ID);
    if (hVolumeLabel) {
        MoveWindow(hVolumeLabel, rcClient.right - volumeWidth - volumeLabelWidth - 15, volumeY, 
                   volumeLabelWidth, volumeHeight, TRUE);
    }
    
    // Posiziona lo slider del volume
    MoveWindow(gui->hVolumeBar, rcClient.right - volumeWidth - 10, volumeY, 
               volumeWidth, volumeHeight, TRUE);
    
    // Posiziona il controllo tab
    int tabHeight = 25;
    if (gui->hTabs) {
        MoveWindow(gui->hTabs, 0, toolHeight, rcClient.right, tabHeight, TRUE);
    }
    
    // Calcola lo spazio disponibile per i controlli principali
    int mainY = toolHeight + (gui->hTabs ? tabHeight : 0);
    int mainHeight = volumeY - mainY - 5;
    
    // Verifica se la vista dettagli è visibile
    BOOL detailsVisible = IsWindowVisible(gui->hDetailView);
    int detailWidth = 250; // Larghezza della vista dettagli
    
    // Posiziona la ListView
    int listViewWidth = detailsVisible ? rcClient.right - detailWidth : rcClient.right;
    MoveWindow(gui->hListView, 0, mainY, listViewWidth, mainHeight, TRUE);
    
    // Ridimensiona le colonne in proporzione alla dimensione della finestra
    adjust_column_widths(gui->hListView, listViewWidth);
    
    // Posiziona la vista dettagli e album art (se visibili)
    if (gui->hDetailView && detailsVisible) {
        int albumArtSize = 200;
        
        // Posiziona il controllo Album Art in alto a destra
        if (gui->hAlbumArt) {
            MoveWindow(gui->hAlbumArt, rcClient.right - detailWidth + 25, mainY + 10, 
                       albumArtSize, albumArtSize, TRUE);
        }
        
        // Posiziona la vista dettagli sotto l'album art
        MoveWindow(gui->hDetailView, rcClient.right - detailWidth, mainY + albumArtSize + 20, 
                   detailWidth, mainHeight - albumArtSize - 20, TRUE);
    }
}

// Funzione per regolare dinamicamente la larghezza delle colonne
void adjust_column_widths(HWND hListView, int totalWidth) {
    if (!hListView) return;
    
    int columnCount = Header_GetItemCount(ListView_GetHeader(hListView));
    if (columnCount <= 0) return;
    
    // Proporzioni consigliate per le colonne (in percentuali)
    float columnWidthPercentages[] = {
        5.0f,   // #
        30.0f,  // Title
        20.0f,  // Artist
        20.0f,  // Album
        7.5f,   // Year
        10.0f,  // Genre
        7.5f    // Track
    };
    
    // Assicurati che ci siano abbastanza percentuali per tutte le colonne
    int percentageCount = sizeof(columnWidthPercentages) / sizeof(float);
    if (percentageCount < columnCount) {
        return;
    }
    
    // Calcola le larghezze effettive in base alle percentuali
    for (int i = 0; i < columnCount; i++) {
        int width = (int)(totalWidth * columnWidthPercentages[i] / 100.0f);
        
        // Imposta una larghezza minima per la colonna
        if (width < 30) width = 30;
        
        ListView_SetColumnWidth(hListView, i, width);
    }
}

// Popola la ListView con i file MP3
void populate_list_view(GUIData* gui) {
    if (!gui) return;
    
    // Se siamo in modalità griglia, usa la funzione specifica
    if (gui->view_mode == VIEW_MODE_GRID) {
        prepare_grid_view_items(gui);
        return;
    }
    
    // Altrimenti, usa il metodo standard per la visualizzazione a lista
    // Cancella tutti gli elementi nella ListView
    ListView_DeleteAllItems(gui->hListView);
    
    // Ottieni la lista dei file MP3 da visualizzare
    MP3File* list_to_show = gui->current_list ? gui->current_list : gui->library->all_files;
    
    // Aggiungi gli elementi alla ListView
    LVITEM lvItem;
    ZeroMemory(&lvItem, sizeof(LVITEM));
    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    
    int itemIndex = 0;
    MP3File* current = list_to_show;
    
    while (current) {
        char buffer[32];
        
        // Colonna numero
        lvItem.iItem = itemIndex;
        lvItem.iSubItem = COLUMN_NUMBER;
        sprintf(buffer, "%d", itemIndex + 1);
        lvItem.pszText = buffer;
        lvItem.lParam = (LPARAM)current;  // Memorizza il puntatore al file MP3
        ListView_InsertItem(gui->hListView, &lvItem);
        
        // Colonna titolo
        ListView_SetItemText(gui->hListView, itemIndex, COLUMN_TITLE, 
                             current->metadata.title[0] ? current->metadata.title : "Sconosciuto");
        
        // Colonna artista
        ListView_SetItemText(gui->hListView, itemIndex, COLUMN_ARTIST, 
                             current->metadata.artist[0] ? current->metadata.artist : "Sconosciuto");
        
        // Colonna album
        ListView_SetItemText(gui->hListView, itemIndex, COLUMN_ALBUM, 
                             current->metadata.album[0] ? current->metadata.album : "Sconosciuto");
        
        // Colonna anno
        if (current->metadata.year > 0) {
            sprintf(buffer, "%d", current->metadata.year);
            ListView_SetItemText(gui->hListView, itemIndex, COLUMN_YEAR, buffer);
        } else {
            ListView_SetItemText(gui->hListView, itemIndex, COLUMN_YEAR, "");
        }
        
        // Colonna genere
        ListView_SetItemText(gui->hListView, itemIndex, COLUMN_GENRE, 
                             current->metadata.genre[0] ? current->metadata.genre : "");
        
        // Colonna traccia
        if (current->metadata.track_number > 0) {
            sprintf(buffer, "%d", current->metadata.track_number);
            ListView_SetItemText(gui->hListView, itemIndex, COLUMN_TRACK, buffer);
        } else {
            ListView_SetItemText(gui->hListView, itemIndex, COLUMN_TRACK, "");
        }
        
        itemIndex++;
        current = current->next;
    }
}

// Aggiorna la barra di stato
void update_status_bar(GUIData* gui) {
    char statusText[256];
    int total_files = 0;
    MP3File* current = gui->current_list ? gui->current_list : gui->library->all_files;
    
    while (current) {
        total_files++;
        current = current->next;
    }
    
    sprintf(statusText, "File MP3: %d", total_files);
    SetWindowText(gui->hStatusBar, statusText);
}

// Funzione per selezionare una cartella
char* select_folder(HWND hWnd) {
    static char buffer[MAX_PATH];
    
    BROWSEINFO bi;
    ZeroMemory(&bi, sizeof(BROWSEINFO));
    bi.hwndOwner = hWnd;
    bi.pszDisplayName = buffer;
    bi.lpszTitle = "Seleziona una cartella contenente file MP3";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != NULL) {
        // Ottiene il percorso dalla selezione
        SHGetPathFromIDList(pidl, buffer);
        
        // Libera la memoria
        CoTaskMemFree(pidl);
        
        return buffer;
    }
    
    return NULL;
}

// Gestisce le azioni del menu
void handle_menu_action(HWND hWnd, WPARAM wParam, GUIData* gui) {
    switch (LOWORD(wParam)) {
        case ID_FILE_OPEN_FOLDER:
            {
                // Mostra la finestra di dialogo per selezionare una cartella
                char* folder = select_folder(hWnd);
                if (folder != NULL) {
                    // Resetta la libreria e scansiona la nuova cartella
                    if (gui->library) {
                        // Prima cancella tutti gli elementi dalla ListView
                        ListView_DeleteAllItems(gui->hListView);
                        
                        // Rimuovi la lista filtrata se presente
                        if (gui->using_filtered_list && gui->current_list) {
                            gui->using_filtered_list = FALSE;
                            gui->current_list = NULL;
                        }
                        
                        // Libera la memoria della vecchia libreria
                        free_mp3_library(gui->library);
                        
                        // Crea una nuova libreria con la cartella selezionata
                        gui->library = create_library(folder);
                        
                        // Esegui una scansione completa della cartella (con ricorsione)
                        int found = scan_directory(gui->library, folder, TRUE); // TRUE per abilitare la ricorsione
                        
                        // Ordina i file per traccia (come da default)
                        sort_mp3_files(&gui->library->all_files, SORT_BY_TRACK);
                        
                        // Popola la ListView con i nuovi file
                        populate_list_view(gui);
                        
                        // Aggiorna la barra di stato
                        char statusText[256];
                        sprintf(statusText, "Cartella: %s - %d file MP3 trovati", folder, found);
                        SetWindowText(gui->hStatusBar, statusText);
                        
                        // Mostra un messaggio se non sono stati trovati file
                        if (found == 0) {
                            MessageBox(hWnd, "Nessun file MP3 trovato nella cartella selezionata.", 
                                       "Informazione", MB_OK | MB_ICONINFORMATION);
                        }
                    }
                }
            }
            break;
            
        case ID_FILE_EXIT:
            DestroyWindow(hWnd);
            break;
            
        case ID_PLAY_SHOW_EQ:
            toggle_equalizer(gui);
            break;
            
        case ID_PLAY_START:
        case ID_PLAY_PAUSE:
        case ID_PLAY_STOP:
        case ID_PLAY_NEXT:
        case ID_PLAY_PREV:
            // Gestisci i controlli di riproduzione utilizzando la funzione dedicata
            handle_playback_controls(hWnd, LOWORD(wParam), gui);
            break;
            
        // Gestione dell'ordinamento
        case ID_VIEW_SORT_TITLE:
            gui->sort_type = SORT_BY_TITLE;
            if (gui->using_filtered_list && gui->current_list) {
                sort_mp3_files(gui->current_list, SORT_BY_TITLE);
            } else {
                sort_mp3_files(&gui->library->all_files, SORT_BY_TITLE);
            }
            populate_list_view(gui);
            break;
            
        case ID_VIEW_SORT_ARTIST:
            gui->sort_type = SORT_BY_ARTIST;
            if (gui->using_filtered_list && gui->current_list) {
                sort_mp3_files(gui->current_list, SORT_BY_ARTIST);
            } else {
                sort_mp3_files(&gui->library->all_files, SORT_BY_ARTIST);
            }
            populate_list_view(gui);
            break;
            
        case ID_VIEW_SORT_ALBUM:
            gui->sort_type = SORT_BY_ALBUM;
            if (gui->using_filtered_list && gui->current_list) {
                sort_mp3_files(gui->current_list, SORT_BY_ALBUM);
            } else {
                sort_mp3_files(&gui->library->all_files, SORT_BY_ALBUM);
            }
            populate_list_view(gui);
            break;
            
        case ID_VIEW_SORT_YEAR:
            gui->sort_type = SORT_BY_YEAR;
            if (gui->using_filtered_list && gui->current_list) {
                sort_mp3_files(gui->current_list, SORT_BY_YEAR);
            } else {
                sort_mp3_files(&gui->library->all_files, SORT_BY_YEAR);
            }
            populate_list_view(gui);
            break;
            
        case ID_VIEW_SORT_GENRE:
            gui->sort_type = SORT_BY_GENRE;
            if (gui->using_filtered_list && gui->current_list) {
                sort_mp3_files(gui->current_list, SORT_BY_GENRE);
            } else {
                sort_mp3_files(&gui->library->all_files, SORT_BY_GENRE);
            }
            populate_list_view(gui);
            break;
            
        case ID_VIEW_SORT_TRACK:
            gui->sort_type = SORT_BY_TRACK;
            if (gui->using_filtered_list && gui->current_list) {
                sort_mp3_files(gui->current_list, SORT_BY_TRACK);
            } else {
                sort_mp3_files(&gui->library->all_files, SORT_BY_TRACK);
            }
            populate_list_view(gui);
            break;
            
        // Gestione delle modalità di visualizzazione
        case ID_VIEW_LIST_MODE:
            switch_view_mode(gui, VIEW_MODE_LIST);
            break;
            
        case ID_VIEW_GRID_MODE:
            switch_view_mode(gui, VIEW_MODE_GRID);
            break;
            
        case ID_VIEW_ALBUM_MODE:
            switch_view_mode(gui, VIEW_MODE_ALBUM);
            break;
            
        case ID_VIEW_SHOW_DETAILS:
            // Mostra/Nascondi la vista dei dettagli
            if (IsWindowVisible(gui->hDetailView)) {
                ShowWindow(gui->hDetailView, SW_HIDE);
            } else {
                ShowWindow(gui->hDetailView, SW_SHOW);
                update_details_view(gui);
            }
            resize_controls(hWnd, gui);
            break;
    }
}

// Gestisce le notifiche della ListView
void handle_list_view_notification(HWND hWnd, LPARAM lParam, GUIData* gui) {
    NMHDR* pnmh = (NMHDR*)lParam;
    switch (pnmh->code) {
        case LVN_ITEMCHANGED:
            {
                NMLISTVIEW* pnmlv = (NMLISTVIEW*)lParam;
                if (pnmlv->uChanged & LVIF_STATE) {
                    if (pnmlv->uNewState & LVIS_SELECTED) {
                        gui->selected_item = pnmlv->iItem;
                        // Mostra e aggiorna la vista dei dettagli
                        if (gui->hDetailView) {
                            ShowWindow(gui->hDetailView, SW_SHOW);
                            update_details_view(gui);
                        }
                    }
                }
            }
            break;
            
        case NM_DBLCLK:
            {
                NMITEMACTIVATE* pnmia = (NMITEMACTIVATE*)lParam;
                
                // Se siamo in vista griglia, gestisci il doppio click su un album
                if (gui->view_mode == VIEW_MODE_GRID && pnmia->iItem != -1) {
                    // Ottieni il file rappresentativo dell'album
                    LVITEM lvItem;
                    ZeroMemory(&lvItem, sizeof(LVITEM));
                    lvItem.mask = LVIF_PARAM;
                    lvItem.iItem = pnmia->iItem;
                    
                    if (ListView_GetItem(gui->hListView, &lvItem)) {
                        MP3File* representative_file = (MP3File*)lvItem.lParam;
                        handle_album_selection(gui, representative_file);
                    }
                } else {
                    // Doppio click nella vista lista, avvia la riproduzione
                    play_selected_file(gui);
                }
            }
            break;
    }
}

// Riproduce il file selezionato
void play_selected_file(GUIData* gui) {
    if (!gui || !gui->player || gui->selected_item < 0) return;
    
    // Otteniamo il puntatore al file MP3 dall'item selezionato nella ListView
    LVITEM lvItem;
    ZeroMemory(&lvItem, sizeof(LVITEM));
    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = gui->selected_item;
    
    if (ListView_GetItem(gui->hListView, &lvItem)) {
        MP3File* selected_file = (MP3File*)lvItem.lParam;
        
        if (selected_file) {
            // Puliamo la coda corrente
            clear_queue(gui->player);
            
            // Aggiungiamo tutti i file visualizzati nella ListView alla coda
            int file_count = ListView_GetItemCount(gui->hListView);
            MP3File* file_to_play = NULL;
            
            for (int i = 0; i < file_count; i++) {
                LVITEM item;
                ZeroMemory(&item, sizeof(LVITEM));
                item.mask = LVIF_PARAM;
                item.iItem = i;
                
                if (ListView_GetItem(gui->hListView, &item)) {
                    MP3File* file = (MP3File*)item.lParam;
                    if (file) {
                        add_to_queue(gui->player, file);
                        
                        // Se questo è il file selezionato, teniamo il riferimento
                        if (i == gui->selected_item) {
                            file_to_play = get_current_file(gui->player);
                        }
                    }
                }
            }
            
            // Troviamo il file selezionato nella coda e lo rendiamo corrente
            if (file_to_play) {
                // Ora la coda è popolata e dobbiamo impostare il file corrente
                MP3File* current = gui->player->queue->head;
                while (current) {
                    if (strcmp(current->filepath, selected_file->filepath) == 0) {
                        gui->player->queue->current = current;
                        break;
                    }
                    current = current->next;
                }
            }
            
            // Avviamo la riproduzione
            if (play_current(gui->player)) {
                // Se la riproduzione è partita, avviamo il timer per aggiornare la progress bar
                if (gui->timer_id == 0) {
                    gui->timer_id = SetTimer(gui->hWnd, 1, 500, NULL); // Aggiorna ogni 500ms
                }
                
                // Aggiorniamo l'interfaccia
                update_playback_ui(gui);
            }
        }
    }
}

// Aggiorna l'interfaccia in base allo stato di riproduzione
void update_playback_ui(GUIData* gui) {
    if (!gui || !gui->player) return;
    
    PlaybackState state = get_playback_state(gui->player);
    
    // Aggiorna la status bar
    if (state == PLAYBACK_PLAYING || state == PLAYBACK_PAUSED) {
        // Se c'è un file in riproduzione, mostriamo le informazioni
        MP3File* current = get_current_file(gui->player);
        if (current) {
            char status[256];
            if (state == PLAYBACK_PLAYING) {
                sprintf(status, "In riproduzione: %s - %s", 
                        current->metadata.artist[0] ? current->metadata.artist : "Artista sconosciuto", 
                        current->metadata.title[0] ? current->metadata.title : "Titolo sconosciuto");
            } else {
                sprintf(status, "In pausa: %s - %s", 
                        current->metadata.artist[0] ? current->metadata.artist : "Artista sconosciuto", 
                        current->metadata.title[0] ? current->metadata.title : "Titolo sconosciuto");
            }
            SetWindowText(gui->hStatusBar, status);
            
            // Trova la traccia in riproduzione nella ListView
            int count = ListView_GetItemCount(gui->hListView);
            for (int i = 0; i < count; i++) {
                LVITEM item;
                ZeroMemory(&item, sizeof(LVITEM));
                item.mask = LVIF_PARAM;
                item.iItem = i;
                
                if (ListView_GetItem(gui->hListView, &item)) {
                    MP3File* file = (MP3File*)item.lParam;
                    if (file && strcmp(file->filepath, current->filepath) == 0) {
                        // Se il brano attualmente evidenziato è diverso, aggiorna
                        if (currently_highlighted_item != i) {
                            // Rimuovi evidenziazione precedente se presente
                            if (currently_highlighted_item != -1) {
                                ListView_RedrawItems(gui->hListView, currently_highlighted_item, currently_highlighted_item);
                            }
                            
                            // Memorizza la nuova traccia in riproduzione
                            currently_highlighted_item = i;
                            
                            // Assicura che l'item sia visibile
                            ListView_EnsureVisible(gui->hListView, i, FALSE);
                        }
                        
                        // Ridisegna l'elemento che sarà in grassetto
                        ListView_RedrawItems(gui->hListView, i, i);
                        UpdateWindow(gui->hListView);
                        
                        break;
                    }
                }
            }
        } else {
            if (state == PLAYBACK_PLAYING) {
                SetWindowText(gui->hStatusBar, "In riproduzione");
            } else {
                SetWindowText(gui->hStatusBar, "In pausa");
            }
        }
    } else {
        // Ripristina la visualizzazione del numero di file nella libreria
        update_status_bar(gui);
        
        // Rimuovi l'evidenziazione se precedentemente presente
        if (currently_highlighted_item != -1) {
            int old_item = currently_highlighted_item;
            currently_highlighted_item = -1;
            
            // Ridisegna l'elemento precedentemente evidenziato
            ListView_RedrawItems(gui->hListView, old_item, old_item);
            UpdateWindow(gui->hListView);
        }
    }
}

// Gestisce le notifiche di riproduzione
void handle_playback_notification(HWND hWnd, WPARAM wParam, LPARAM lParam, GUIData* gui) {
    if (!gui || !gui->player) return;
    
    // Gestisce le diverse notifiche
    switch (wParam) {
        case AUDIO_NOTIFY_COMPLETE:
            // La riproduzione è terminata
            // Passiamo al brano successivo o fermiamo la riproduzione in base alla modalità
            if (get_playback_mode(gui->player) == PLAYBACK_MODE_NORMAL) {
                // In modalità normale, se c'è un brano successivo, lo riproduciamo
                if (gui->player->queue->current && gui->player->queue->current->next) {
                    next_track(gui->player);
                    update_playback_ui(gui);
                } else {
                    // Altrimenti fermiamo la riproduzione
                    stop_playback(gui->player);
                    
                    // Fermiamo il timer
                    if (gui->timer_id != 0) {
                        KillTimer(hWnd, gui->timer_id);
                        gui->timer_id = 0;
                    }
                    
                    // Aggiorniamo l'interfaccia
                    update_playback_ui(gui);
                    
                    // Azzeriamo la progress bar
                    SendMessage(gui->hProgressBar, PBM_SETPOS, 0, 0);
                }
            } else {
                // Nelle altre modalità, passiamo al brano successivo
                next_track(gui->player);
                update_playback_ui(gui);
            }
            break;
    }
}

// Gestisce i controlli di riproduzione
void handle_playback_controls(HWND hWnd, int control_id, GUIData* gui) {
    if (!gui || !gui->player) return;
    
    switch (control_id) {
        case ID_PLAY_START:
            // Se c'è un file selezionato, lo riproduciamo
            if (gui->selected_item >= 0) {
                play_selected_file(gui);
            } 
            // Altrimenti, se la riproduzione è in pausa, la riprendiamo
            else if (get_playback_state(gui->player) == PLAYBACK_PAUSED) {
                if (resume_playback(gui->player)) {
                    // Se la riproduzione riprende, avviamo il timer per aggiornare la progress bar
                    if (gui->timer_id == 0) {
                        gui->timer_id = SetTimer(gui->hWnd, 1, 500, NULL); // Aggiorna ogni 500ms
                    }
                    update_playback_ui(gui);
                }
            }
            break;
            
        case ID_PLAY_PAUSE:
            // Se la riproduzione è in corso, la mettiamo in pausa
            if (get_playback_state(gui->player) == PLAYBACK_PLAYING) {
                if (pause_playback(gui->player)) {
                    update_playback_ui(gui);
                }
            } 
            // Se è in pausa, la riprendiamo
            else if (get_playback_state(gui->player) == PLAYBACK_PAUSED) {
                if (resume_playback(gui->player)) {
                    // Se la riproduzione riprende, avviamo il timer per aggiornare la progress bar
                    if (gui->timer_id == 0) {
                        gui->timer_id = SetTimer(gui->hWnd, 1, 500, NULL); // Aggiorna ogni 500ms
                    }
                    update_playback_ui(gui);
                }
            }
            break;
            
        case ID_PLAY_STOP:
            // Fermiamo la riproduzione
            if (stop_playback(gui->player)) {
                // Fermiamo il timer
                if (gui->timer_id != 0) {
                    KillTimer(hWnd, gui->timer_id);
                    gui->timer_id = 0;
                }
                
                // Aggiorniamo l'interfaccia
                update_playback_ui(gui);
                
                // Azzeriamo la progress bar
                SendMessage(gui->hProgressBar, PBM_SETPOS, 0, 0);
                
                // Aggiorniamo la status bar
                SetWindowText(gui->hStatusBar, "Riproduzione fermata");
            }
            break;
            
        case ID_PLAY_NEXT:
            // Passiamo al brano successivo
            if (next_track(gui->player)) {
                update_playback_ui(gui);
            }
            break;
            
        case ID_PLAY_PREV:
            // Passiamo al brano precedente
            if (previous_track(gui->player)) {
                update_playback_ui(gui);
            }
            break;
            
        case ID_PLAY_MODE_NORMAL:
            // Impostazione della modalità di riproduzione normale
            set_playback_mode(gui->player, PLAYBACK_MODE_NORMAL);
            break;
            
        case ID_PLAY_MODE_REPEAT_ONE:
            // Impostazione della modalità di riproduzione con ripetizione del brano corrente
            set_playback_mode(gui->player, PLAYBACK_MODE_REPEAT_ONE);
            break;
            
        case ID_PLAY_MODE_REPEAT_ALL:
            // Impostazione della modalità di riproduzione con ripetizione di tutti i brani
            set_playback_mode(gui->player, PLAYBACK_MODE_REPEAT_ALL);
            break;
            
        case ID_PLAY_MODE_SHUFFLE:
            // Impostazione della modalità di riproduzione casuale
            set_playback_mode(gui->player, PLAYBACK_MODE_SHUFFLE);
            break;
    }
}

// Disegna un elemento della ListView
void draw_list_view_item(HWND hWnd, LPDRAWITEMSTRUCT lpDrawItem) {
    // Se non è un item valido o il controllo non è una ListView, usciamo
    if (lpDrawItem->CtlType != ODT_LISTVIEW || lpDrawItem->itemID == -1) {
        return;
    }
    
    HWND hListView = lpDrawItem->hwndItem;
    int itemIndex = lpDrawItem->itemID;
    HDC hdc = lpDrawItem->hDC;
    RECT rcItem = lpDrawItem->rcItem;
    BOOL isSelected = (lpDrawItem->itemState & ODS_SELECTED);
    BOOL isPlaying = (itemIndex == currently_highlighted_item);
    
    // Crea un font normale e uno in grassetto
    HFONT hFontNormal = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, 
                                 ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    
    HFONT hFontBold = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, 0, 
                               ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    
    // Colore sfondo - sempre bianco di base, blu solo se selezionato
    COLORREF bgColor;
    if (isSelected) {
        bgColor = RGB(51, 153, 255); // Blu chiaro per selezione
    } else if (itemIndex % 2 == 0) {
        bgColor = RGB(245, 245, 250); // Righe alternate leggermente colorate
    } else {
        bgColor = RGB(255, 255, 255); // Bianco normale
    }
    
    // Colore testo
    COLORREF textColor;
    if (isSelected) {
        textColor = RGB(255, 255, 255); // Bianco su sfondo blu
    } else {
        textColor = RGB(0, 0, 0); // Nero standard
    }
    
    // Riempi lo sfondo
    SetBkColor(hdc, bgColor);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcItem, NULL, 0, NULL);
    
    // Imposta il colore del testo e il modo di sfondo
    SetTextColor(hdc, textColor);
    SetBkMode(hdc, TRANSPARENT);
    
    // Seleziona il font appropriato - solo brano in riproduzione in grassetto
    HFONT hOldFont;
    if (isPlaying) {
        hOldFont = (HFONT)SelectObject(hdc, hFontBold);
    } else {
        hOldFont = (HFONT)SelectObject(hdc, hFontNormal);
    }
    
    // Ottieni il testo per ogni colonna e disegnalo
    char buffer[256];
    RECT rcText;
    
    int columnCount = Header_GetItemCount(ListView_GetHeader(hListView));
    
    for (int i = 0; i < columnCount; i++) {
        // Ottieni il rettangolo della colonna
        ListView_GetSubItemRect(hListView, itemIndex, i, LVIR_BOUNDS, &rcText);
        
        // Se è la prima colonna, aggiusta il rettangolo
        if (i == 0) {
            rcText.right = ListView_GetColumnWidth(hListView, 0);
        }
        
        // Ottieni il testo dell'item
        buffer[0] = '\0';
        ListView_GetItemText(hListView, itemIndex, i, buffer, sizeof(buffer));
        
        // Disegna il testo
        rcText.left += 5; // Margine sinistro
        DrawText(hdc, buffer, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
    
    // Disegna il focus
    if (lpDrawItem->itemState & ODS_FOCUS) {
        DrawFocusRect(hdc, &rcItem);
    }
    
    // Ripristina il vecchio font e libera la memoria
    SelectObject(hdc, hOldFont);
    DeleteObject(hFontNormal);
    DeleteObject(hFontBold);
}

// Procedura della finestra principale
LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            // Inizializza la GUIData
            g_gui_data.selected_item = -1;
            g_gui_data.sort_type = SORT_BY_TRACK; // Imposta l'ordinamento predefinito per traccia
            g_gui_data.using_filtered_list = FALSE;
            g_gui_data.current_list = NULL;
            g_gui_data.timer_id = 0;
            
            // Crea i controlli
            create_controls(hWnd, &g_gui_data);
            return 0;
            
        case WM_SIZE:
            // Ridimensiona i controlli
            resize_controls(hWnd, &g_gui_data);
            return 0;
            
        case WM_COMMAND:
            {
                int id = LOWORD(wParam);
                
                // Controlla se è un comando del menu o del toolbar
                if (id >= ID_PLAY_START && id <= ID_PLAY_MODE_SHUFFLE) {
                    // Gestisce i controlli di riproduzione
                    handle_playback_controls(hWnd, id, &g_gui_data);
                    return 0;
                } else {
                    // Gestisce gli altri comandi
                    handle_menu_action(hWnd, wParam, &g_gui_data);
                    return 0;
                }
            }
            
        case WM_DRAWITEM:
            // Gestisce il disegno personalizzato degli elementi della ListView
            if (wParam == LISTVIEW_ID) {
                draw_list_view_item(hWnd, (LPDRAWITEMSTRUCT)lParam);
                return TRUE;
            }
            break;
            
        case WM_NOTIFY:
            // Gestisce le notifiche dei controlli
            if (((LPNMHDR)lParam)->idFrom == LISTVIEW_ID) {
                handle_list_view_notification(hWnd, lParam, &g_gui_data);
            }
            else if (((LPNMHDR)lParam)->idFrom == TABS_ID) {
                // Gestisce le notifiche del controllo Tab
                if (((LPNMHDR)lParam)->code == TCN_SELCHANGE) {
                    // Ottieni l'indice della tab selezionata
                    int tab = TabCtrl_GetCurSel(g_gui_data.hTabs);
                    
                    // Cambia la modalità di visualizzazione in base alla tab
                    switch_view_mode(&g_gui_data, tab);
                }
            }
            return 0;
            
        case WM_HSCROLL:
            // Gestisce lo scroll orizzontale per il controllo del volume
            if ((HWND)lParam == g_gui_data.hVolumeBar) {
                int volume = (int)SendMessage(g_gui_data.hVolumeBar, TBM_GETPOS, 0, 0);
                if (g_gui_data.player) {
                    set_volume(g_gui_data.player, volume);
                }
            }
            return 0;
            
        case WM_TIMER:
            // Aggiorna la barra di avanzamento
            if (wParam == g_gui_data.timer_id) {
                update_progress_bar(&g_gui_data);
            }
            return 0;
            
        case WM_AUDIO_NOTIFY:
            // Gestisce le notifiche di riproduzione
            handle_playback_notification(hWnd, wParam, lParam, &g_gui_data);
            return 0;
            
        case WM_CLOSE:
            // Ferma la riproduzione se è in corso
            if (g_gui_data.player && get_playback_state(g_gui_data.player) != PLAYBACK_STOPPED) {
                stop_playback(g_gui_data.player);
            }
            
            // Ferma il timer se è attivo
            if (g_gui_data.timer_id != 0) {
                KillTimer(hWnd, g_gui_data.timer_id);
                g_gui_data.timer_id = 0;
            }
            
            DestroyWindow(hWnd);
            return 0;
            
        case WM_DESTROY:
            // Libera la memoria del player audio
            if (g_gui_data.player) {
                free_audio_player(g_gui_data.player);
                g_gui_data.player = NULL;
            }
            
            // Libera il bitmap dell'album art
            if (g_gui_data.hAlbumBitmap) {
                DeleteObject(g_gui_data.hAlbumBitmap);
                g_gui_data.hAlbumBitmap = NULL;
            }
            
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// Funzione principale per avviare l'interfaccia grafica
int start_gui(HINSTANCE hInstance, MP3Library* library) {
    // Inizializza i controlli comuni di Windows
    if (!init_gui_controls()) {
        MessageBox(NULL, "Impossibile inizializzare i controlli comuni", "Errore", MB_ICONERROR);
        return 1;
    }
    
    // Inizializza GDI+
    ULONG_PTR gdiplusToken;
    GdiplusStartupInput gdiplusStartupInput;
    gdiplusStartupInput.GdiplusVersion = 1;
    gdiplusStartupInput.DebugEventCallback = NULL;
    gdiplusStartupInput.SuppressBackgroundThread = FALSE;
    gdiplusStartupInput.SuppressExternalCodecs = FALSE;
    
    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Ok) {
        MessageBox(NULL, "Impossibile inizializzare GDI+", "Errore", MB_ICONERROR);
        return 1;
    }
    
    // Crea la finestra principale
    HWND hWnd = create_main_window(hInstance);
    if (!hWnd) {
        return 1;
    }
    
    // Salva il puntatore alla libreria
    g_gui_data.library = library;
    
    // Imposta le dimensioni iniziali dei controlli
    resize_controls(hWnd, &g_gui_data);
    
    // Mostra la finestra
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    
    // Popola la lista con i file MP3 dalla libreria
    populate_list_view(&g_gui_data);
    
    // Imposta il testo della StatusBar iniziale
    SetWindowText(g_gui_data.hStatusBar, "Pronto");
    
    // Loop dei messaggi
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Termina GDI+
    GdiplusShutdown(gdiplusToken);
    
    return (int)msg.wParam;
}

// Crea la finestra di equalizzazione
void create_equalizer_dialog(HWND hParent, GUIData* gui) {
    // Crea la finestra di dialogo
    gui->hEqDialog = CreateWindowEx(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        "STATIC",
        "Equalizzatore",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        0, 0, 400, 300,
        hParent,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    // Posiziona la finestra al centro della finestra principale
    RECT rcParent, rcDialog;
    GetWindowRect(hParent, &rcParent);
    GetWindowRect(gui->hEqDialog, &rcDialog);
    
    int x = rcParent.left + ((rcParent.right - rcParent.left) - (rcDialog.right - rcDialog.left)) / 2;
    int y = rcParent.top + ((rcParent.bottom - rcParent.top) - (rcDialog.bottom - rcDialog.top)) / 2;
    
    SetWindowPos(gui->hEqDialog, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    
    // Crea un contenitore per gli slider
    HWND hContainer = CreateWindowEx(
        0,
        "STATIC",
        "",
        WS_CHILD | WS_VISIBLE | SS_NOTIFY,
        10, 10, 380, 230,
        gui->hEqDialog,
        (HMENU)EQ_CONTAINER_ID,
        GetModuleHandle(NULL),
        NULL
    );
    
    // Crea gli slider per le bande dell'equalizzatore
    int slider_width = 30;
    int slider_spacing = 35;
    int slider_x = 10;
    
    for (int i = 0; i < EQ_BANDS; i++) {
        // Crea l'etichetta per la frequenza
        HWND hLabel = CreateWindowEx(
            0,
            "STATIC",
            eq_labels[i],
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            slider_x, 210, slider_width, 20,
            hContainer,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );
        
        // Crea lo slider verticale
        gui->hEqSliders[i] = CreateWindowEx(
            0,
            TRACKBAR_CLASS,
            "",
            WS_CHILD | WS_VISIBLE | TBS_VERT | TBS_NOTICKS,
            slider_x, 20, slider_width, 180,
            hContainer,
            (HMENU)(ID_EQ_60HZ + i),
            GetModuleHandle(NULL),
            NULL
        );
        
        // Imposta il range dello slider (-10 dB a +10 dB)
        SendMessage(gui->hEqSliders[i], TBM_SETRANGE, TRUE, MAKELPARAM(-10, 10));
        SendMessage(gui->hEqSliders[i], TBM_SETPOS, TRUE, 0);
        
        slider_x += slider_spacing;
    }
    
    // Crea il pulsante "Reset"
    HWND hResetButton = CreateWindowEx(
        0,
        "BUTTON",
        "Reset",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        150, 245, 100, 25,
        gui->hEqDialog,
        (HMENU)ID_EQ_RESET,
        GetModuleHandle(NULL),
        NULL
    );
}

// Procedura della finestra di equalizzazione
LRESULT CALLBACK EqDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    GUIData* gui = (GUIData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    
    switch (uMsg) {
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_EQ_RESET) {
                // Resetta tutti gli slider a 0
                for (int i = 0; i < EQ_BANDS; i++) {
                    SendMessage(gui->hEqSliders[i], TBM_SETPOS, TRUE, 0);
                    // Applica il valore dell'equalizzatore
                    if (gui->player) {
                        set_eq_band(gui->player, eq_freqs[i], 0.0f);
                    }
                }
                return 0;
            }
            break;
            
        case WM_VSCROLL:
            // Gestisce gli slider dell'equalizzatore
            for (int i = 0; i < EQ_BANDS; i++) {
                if ((HWND)lParam == gui->hEqSliders[i]) {
                    // Ottiene il valore corrente
                    int val = (int)SendMessage(gui->hEqSliders[i], TBM_GETPOS, 0, 0);
                    
                    // Applica il valore all'equalizzatore
                    if (gui->player) {
                        set_eq_band(gui->player, eq_freqs[i], (float)val);
                    }
                    
                    break;
                }
            }
            return 0;
            
        case WM_CLOSE:
            ShowWindow(hWnd, SW_HIDE);
            return 0;
    }
    
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// Mostra o nasconde la finestra dell'equalizzatore
void toggle_equalizer(GUIData* gui) {
    if (!gui->hEqDialog) {
        create_equalizer_dialog(gui->hWnd, gui);
        
        // Imposta la procedura della finestra
        SetWindowLongPtr(gui->hEqDialog, GWLP_USERDATA, (LONG_PTR)gui);
        SetWindowLongPtr(gui->hEqDialog, GWLP_WNDPROC, (LONG_PTR)EqDialogProc);
    }
    
    // Mostra o nasconde la finestra
    if (IsWindowVisible(gui->hEqDialog)) {
        ShowWindow(gui->hEqDialog, SW_HIDE);
    } else {
        ShowWindow(gui->hEqDialog, SW_SHOW);
    }
}

// Crea il controllo per l'immagine dell'album
void create_album_art_view(HWND hWnd, GUIData* gui) {
    // Crea il controllo per l'album art
    gui->hAlbumArt = CreateWindowEx(
        0,
        "STATIC",
        "",
        WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE,
        0, 0, 200, 200, // Posizione e dimensioni (verranno aggiornate in resize_controls)
        hWnd,
        (HMENU)ALBUMART_ID,
        GetModuleHandle(NULL),
        NULL
    );
    
    // Inizializza il bitmap a NULL
    gui->hAlbumBitmap = NULL;
}

// Crea un bitmap dall'album art
HBITMAP create_album_art_bitmap(MP3File* file) {
    if (!file) return NULL;
    
    HDC hScreenDC = GetDC(NULL);
    HBITMAP hBitmap = NULL;
    
    // Se abbiamo dati dell'immagine album nel file, li usiamo
    if (file->metadata.album_art && file->metadata.album_art_size > 0) {
        // Inizializza GDI+
        ULONG_PTR gdiplusToken;
        GdiplusStartupInput gdiplusStartupInput;
        gdiplusStartupInput.GdiplusVersion = 1;
        gdiplusStartupInput.DebugEventCallback = NULL;
        gdiplusStartupInput.SuppressBackgroundThread = FALSE;
        gdiplusStartupInput.SuppressExternalCodecs = FALSE;
        
        if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) == Ok) {
            // Crea uno stream di memoria con i dati dell'album art
            IStream* pStream = NULL;
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, file->metadata.album_art_size);
            if (hMem) {
                void* pMem = GlobalLock(hMem);
                if (pMem) {
                    memcpy(pMem, file->metadata.album_art, file->metadata.album_art_size);
                    GlobalUnlock(hMem);
                    
                    if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK) {
                        // Carica l'immagine dallo stream
                        GpBitmap* pBitmap = NULL;
                        if (GdipCreateBitmapFromStream(pStream, &pBitmap) == Ok) {
                            // Ottieni le dimensioni originali
                            UINT origWidth, origHeight;
                            GdipGetImageWidth(pBitmap, &origWidth);
                            GdipGetImageHeight(pBitmap, &origHeight);
                            
                            // Calcola le dimensioni per mantenere le proporzioni
                            float ratio = min((float)200 / origWidth, (float)200 / origHeight);
                            int newWidth = (int)(origWidth * ratio);
                            int newHeight = (int)(origHeight * ratio);
                            int offsetX = (200 - newWidth) / 2;
                            int offsetY = (200 - newHeight) / 2;
                            
                            // Crea una bitmap compatibile con lo schermo
                            hBitmap = CreateCompatibleBitmap(hScreenDC, 200, 200);
                            HDC hMemDC = CreateCompatibleDC(hScreenDC);
                            HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
                            
                            // Riempi lo sfondo
                            HBRUSH hBackgroundBrush = CreateSolidBrush(RGB(50, 50, 50));
                            RECT rect = {0, 0, 200, 200};
                            FillRect(hMemDC, &rect, hBackgroundBrush);
                            DeleteObject(hBackgroundBrush);
                            
                            // Crea una grafica GDI+ dal DC
                            GpGraphics* graphics = NULL;
                            if (GdipCreateFromHDC(hMemDC, &graphics) == Ok) {
                                // Imposta la qualità di rendering
                                GdipSetInterpolationMode(graphics, InterpolationModeHighQualityBicubic);
                                GdipSetSmoothingMode(graphics, SmoothingModeHighQuality);
                                
                                // Disegna l'immagine ridimensionata
                                GdipDrawImageRectI(graphics, pBitmap, offsetX, offsetY, newWidth, newHeight);
                                
                                // Libera la grafica
                                GdipDeleteGraphics(graphics);
                            }
                            
                            // Ripristina e libera risorse
                            SelectObject(hMemDC, hOldBitmap);
                            DeleteDC(hMemDC);
                            
                            // Libera la bitmap GDI+
                            GdipDisposeImage(pBitmap);
                        }
                        // Rilascia lo stream COM
                        pStream->lpVtbl->Release(pStream);
                    }
                } else {
                    GlobalFree(hMem);
                }
            }
            
            // Chiudi GDI+
            GdiplusShutdown(gdiplusToken);
        }
    }
    
    // Se non abbiamo potuto caricare l'immagine, creiamo un placeholder
    if (!hBitmap) {
        hBitmap = CreateCompatibleBitmap(hScreenDC, 200, 200);
        
        // Crea un DC di memoria
        HDC hMemDC = CreateCompatibleDC(hScreenDC);
        
        // Seleziona il bitmap nel DC di memoria
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
        
        // Riempi il bitmap con un colore di sfondo
        HBRUSH hBrush = CreateSolidBrush(RGB(50, 50, 50));
        RECT rect = {0, 0, 200, 200};
        FillRect(hMemDC, &rect, hBrush);
        DeleteObject(hBrush);
        
        // Disegna un cerchio colorato basato sul genere musicale
        int r = 200, g = 200, b = 200;
        
        // Colore basato sul genere
        if (strstr(file->metadata.genre, "Rock")) {
            r = 255; g = 50; b = 50;  // Rosso per Rock
        } else if (strstr(file->metadata.genre, "Pop")) {
            r = 255; g = 200; b = 50; // Giallo per Pop
        } else if (strstr(file->metadata.genre, "Jazz")) {
            r = 50; g = 50; b = 255;  // Blu per Jazz
        } else if (strstr(file->metadata.genre, "Metal")) {
            r = 0; g = 0; b = 0;      // Nero per Metal
        } else if (strstr(file->metadata.genre, "Classical")) {
            r = 255; g = 255; b = 255; // Bianco per Classica
        }
        
        // Crea un pennello per disegnare il cerchio
        hBrush = CreateSolidBrush(RGB(r, g, b));
        
        // Seleziona il pennello nel DC
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, hBrush);
        
        // Disegna un cerchio (ellisse completo)
        Ellipse(hMemDC, 50, 50, 150, 150);
        
        // Ripristina e rilascia le risorse
        SelectObject(hMemDC, hOldBrush);
        DeleteObject(hBrush);
        
        // Disegna il titolo dell'album
        SetBkMode(hMemDC, TRANSPARENT);
        SetTextColor(hMemDC, RGB(255, 255, 255));
        
        // Testo centrato
        rect.top = 160;
        rect.bottom = 180;
        DrawText(hMemDC, file->metadata.album, -1, &rect, DT_CENTER);
        
        // Testo dell'artista
        rect.top = 180;
        rect.bottom = 200;
        DrawText(hMemDC, file->metadata.artist, -1, &rect, DT_CENTER);
        
        // Ripristina il bitmap originale e rilascia i DC
        SelectObject(hMemDC, hOldBitmap);
        DeleteDC(hMemDC);
    }
    
    ReleaseDC(NULL, hScreenDC);
    return hBitmap;
}

// Aggiorna l'immagine dell'album
void update_album_art(GUIData* gui, MP3File* file) {
    // Libera il bitmap precedente se esiste
    free_album_art_bitmap(gui);
    
    // Crea il nuovo bitmap
    gui->hAlbumBitmap = create_album_art_bitmap(file);
    
    // Imposta il bitmap nel controllo statico
    if (gui->hAlbumBitmap) {
        SendMessage(gui->hAlbumArt, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)gui->hAlbumBitmap);
    } else {
        // Se non c'è immagine, svuota il controllo
        SendMessage(gui->hAlbumArt, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);
    }
}

// Libera il bitmap dell'album art
void free_album_art_bitmap(GUIData* gui) {
    if (gui->hAlbumBitmap) {
        DeleteObject(gui->hAlbumBitmap);
        gui->hAlbumBitmap = NULL;
    }
}

// Crea la vista dettagli
void create_detail_view(HWND hWnd, GUIData* gui) {
    // Crea un controllo text per mostrare i dettagli
    gui->hDetailView = CreateWindowEx(
        0,
        "STATIC",
        "",
        WS_CHILD | WS_BORDER | SS_LEFT,
        0, 0, 0, 0, // Posizione e dimensioni (verranno aggiornate in resize_controls)
        hWnd,
        (HMENU)DETAIL_VIEW_ID,
        GetModuleHandle(NULL),
        NULL
    );
    
    // Imposta il font Arial
    HFONT hFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    SendMessage(gui->hDetailView, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Mostra il controllo all'inizio
    ShowWindow(gui->hDetailView, SW_SHOW);
}

// Aggiorna la vista dei dettagli
void update_details_view(GUIData* gui) {
    if (!gui || !gui->hDetailView || gui->selected_item < 0) return;
    
    // Ottieni il file MP3 selezionato
    LVITEM lvItem;
    ZeroMemory(&lvItem, sizeof(LVITEM));
    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = gui->selected_item;
    
    if (ListView_GetItem(gui->hListView, &lvItem)) {
        MP3File* file = (MP3File*)lvItem.lParam;
        
        if (file) {
            // Formatta il testo dei dettagli
            char details[1024];
            sprintf(details, 
                "Titolo: %s\n"
                "Artista: %s\n"
                "Album: %s\n"
                "Anno: %d\n"
                "Genere: %s\n"
                "Traccia: %d\n"
                "Durata: %d:%02d\n"
                "Formato Copertina: %s",
                file->metadata.title[0] ? file->metadata.title : "Sconosciuto",
                file->metadata.artist[0] ? file->metadata.artist : "Sconosciuto",
                file->metadata.album[0] ? file->metadata.album : "Sconosciuto",
                file->metadata.year > 0 ? file->metadata.year : 0,
                file->metadata.genre[0] ? file->metadata.genre : "Sconosciuto",
                file->metadata.track_number > 0 ? file->metadata.track_number : 0,
                file->metadata.duration / 60, file->metadata.duration % 60,
                file->metadata.album_art_format == ALBUM_ART_JPEG ? "JPEG" : 
                  (file->metadata.album_art_format == ALBUM_ART_PNG ? "PNG" : 
                   (file->metadata.album_art_format == ALBUM_ART_OTHER ? "Altro" : "Nessuno")),
                file->filepath
            );
            
            // Imposta il testo nel controllo
            SetWindowText(gui->hDetailView, details);
            
            // Aggiorna l'album art
            update_album_art(gui, file);
        }
    }
}

// Crea i tabs per cambiare visualizzazione
void create_tabs(HWND hWnd, GUIData* gui) {
    // Crea il controllo tab
    gui->hTabs = CreateWindowEx(
        0,
        WC_TABCONTROL,
        "",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        0, 0, 0, 0, // Posizione e dimensioni (verranno aggiornate in resize_controls)
        hWnd,
        (HMENU)TABS_ID,
        GetModuleHandle(NULL),
        NULL
    );
    
    // Aggiungi le tabs
    TCITEM tie;
    ZeroMemory(&tie, sizeof(TCITEM));
    tie.mask = TCIF_TEXT;
    
    tie.pszText = "Lista";
    TabCtrl_InsertItem(gui->hTabs, 0, &tie);
    
    tie.pszText = "Griglia";
    TabCtrl_InsertItem(gui->hTabs, 1, &tie);
    
    tie.pszText = "Album";
    TabCtrl_InsertItem(gui->hTabs, 2, &tie);
}

// Modifica il layout della finestra in base alla modalità di visualizzazione
void switch_view_mode(GUIData* gui, int view_mode) {
    if (!gui) return;
    
    // Salva la modalità di visualizzazione corrente
    gui->view_mode = view_mode;
    
    // Aggiorna la selezione della tab
    if (gui->hTabs) {
        TabCtrl_SetCurSel(gui->hTabs, view_mode);
    }
    
    // Gestisci il cambio di visualizzazione
    switch (view_mode) {
        case VIEW_MODE_LIST:
            // Visualizzazione a lista (default)
            SetWindowLong(gui->hListView, GWL_STYLE, 
                GetWindowLong(gui->hListView, GWL_STYLE) & ~LVS_TYPEMASK | LVS_REPORT);
            
            // Riattiva l'owner draw
            SetWindowLong(gui->hListView, GWL_STYLE, 
                GetWindowLong(gui->hListView, GWL_STYLE) | LVS_OWNERDRAWFIXED);
                
            // Mostra le intestazioni delle colonne
            ListView_SetExtendedListViewStyle(gui->hListView, 
                ListView_GetExtendedListViewStyle(gui->hListView) | 
                LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);
            
            break;
            
        case VIEW_MODE_GRID:
            {
                // Visualizzazione a griglia con copertine
                // Cambia lo stile della lista a icon view e disabilita owner draw
                SetWindowLong(gui->hListView, GWL_STYLE, 
                    (GetWindowLong(gui->hListView, GWL_STYLE) & ~LVS_TYPEMASK & ~LVS_OWNERDRAWFIXED) | LVS_ICON);
                
                // Rimuovi stili estesi non necessari per la vista icone
                ListView_SetExtendedListViewStyle(gui->hListView, 
                    ListView_GetExtendedListViewStyle(gui->hListView) & 
                    ~(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP));
                
                // Imposta la spaziatura tra le icone per la visualizzazione a griglia
                ListView_SetIconSpacing(gui->hListView, 220, 240);  // Larghezza, Altezza
                
                // Prepara le icone di album per tutti gli elementi
                prepare_grid_view_items(gui);
            }
            break;
            
        case VIEW_MODE_ALBUM:
            // Visualizzazione album (gruppo per album)
            MessageBox(gui->hWnd, "Visualizzazione per album non ancora implementata completamente", 
                     "Avviso", MB_OK | MB_ICONINFORMATION);
            // Usa la vista a icone come base
            SetWindowLong(gui->hListView, GWL_STYLE, 
                (GetWindowLong(gui->hListView, GWL_STYLE) & ~LVS_TYPEMASK) | LVS_ICON);
            break;
    }
    
    // Ridisegna la finestra
    InvalidateRect(gui->hListView, NULL, TRUE);
    UpdateWindow(gui->hListView);
    
    // Ridimensiona i controlli
    resize_controls(gui->hWnd, gui);
}

// Struttura per tenere traccia degli album unici
typedef struct AlbumInfo {
    char album[MAX_ALBUM_LENGTH];
    char artist[MAX_ARTIST_LENGTH];
    int year;
    MP3File* representative_file; // Un file rappresentativo per questo album
    int track_count;              // Numero di tracce nell'album
    struct AlbumInfo* next;
} AlbumInfo;

// Prepara gli elementi per la visualizzazione a griglia
void prepare_grid_view_items(GUIData* gui) {
    if (!gui || !gui->hListView) return;
    
    // Cancella tutti gli elementi nella ListView
    ListView_DeleteAllItems(gui->hListView);
    
    // Lista di bitmap per gli elementi
    static HIMAGELIST g_hLargeImageList = NULL;
    
    // Distruggi la vecchia image list se esiste
    if (g_hLargeImageList) {
        ImageList_Destroy(g_hLargeImageList);
        g_hLargeImageList = NULL;
    }
    
    // Crea una nuova image list per le copertine degli album
    g_hLargeImageList = ImageList_Create(200, 200, ILC_COLOR32, 100, 10);
    if (!g_hLargeImageList) return;
    
    // Imposta l'image list per la vista a icone
    ListView_SetImageList(gui->hListView, g_hLargeImageList, LVSIL_NORMAL);
    
    // Ottieni la lista dei file MP3 da visualizzare
    MP3File* list_to_show = gui->current_list ? gui->current_list : gui->library->all_files;
    
    // Crea una lista di album unici
    AlbumInfo* album_list = NULL;
    MP3File* current_file = list_to_show;
    
    while (current_file) {
        // Controlla se l'album è già nella lista
        BOOL album_found = FALSE;
        AlbumInfo* current_album = album_list;
        
        while (current_album) {
            // Confronta per nome album (case-insensitive)
            if (stricmp(current_album->album, current_file->metadata.album) == 0) {
                // Album trovato, incrementa il conteggio delle tracce
                current_album->track_count++;
                album_found = TRUE;
                break;
            }
            current_album = current_album->next;
        }
        
        // Se l'album non è nella lista, aggiungilo
        if (!album_found) {
            AlbumInfo* new_album = (AlbumInfo*)malloc(sizeof(AlbumInfo));
            if (new_album) {
                strncpy(new_album->album, current_file->metadata.album[0] ? 
                         current_file->metadata.album : "Unknown Album", MAX_ALBUM_LENGTH - 1);
                new_album->album[MAX_ALBUM_LENGTH - 1] = '\0';
                
                strncpy(new_album->artist, current_file->metadata.artist[0] ? 
                         current_file->metadata.artist : "Unknown Artist", MAX_ARTIST_LENGTH - 1);
                new_album->artist[MAX_ARTIST_LENGTH - 1] = '\0';
                
                new_album->year = current_file->metadata.year;
                new_album->representative_file = current_file;
                new_album->track_count = 1;
                
                // Aggiunge il nuovo album all'inizio della lista
                new_album->next = album_list;
                album_list = new_album;
            }
        }
        
        current_file = current_file->next;
    }
    
    // Ora aggiungi un elemento alla ListView per ogni album unico
    AlbumInfo* current_album = album_list;
    int index = 0;
    
    while (current_album) {
        // Crea il bitmap dell'album art
        HBITMAP hBitmap = create_album_art_bitmap(current_album->representative_file);
        
        if (hBitmap) {
            // Aggiungi il bitmap all'image list
            int imageIndex = ImageList_Add(g_hLargeImageList, hBitmap, NULL);
            
            // Crea un nuovo elemento nella ListView
            LVITEM lvItem;
            ZeroMemory(&lvItem, sizeof(LVITEM));
            
            lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
            lvItem.iItem = index;
            lvItem.iImage = imageIndex;
            
            // Formatta il testo con album, artista, anno e numero di tracce
            char text[512];
            sprintf(text, "%s\n%s\n%d\n%d tracce", 
                    current_album->album, 
                    current_album->artist,
                    current_album->year > 0 ? current_album->year : 0,
                    current_album->track_count);
            
            lvItem.pszText = text;
            lvItem.lParam = (LPARAM)current_album->representative_file;  // Salva il puntatore al file rappresentativo
            
            // Inserisci l'elemento
            ListView_InsertItem(gui->hListView, &lvItem);
            
            // Libera il bitmap originale (l'image list ne ha fatto una copia)
            DeleteObject(hBitmap);
        }
        
        // Vai al prossimo album
        AlbumInfo* temp = current_album;
        current_album = current_album->next;
        free(temp);  // Libera la memoria dell'AlbumInfo corrente
        
        index++;
    }
}

// Quando un album è stato selezionato nella vista a griglia, seleziona tutte le tracce corrispondenti
void handle_album_selection(GUIData* gui, MP3File* representative_file) {
    if (!gui || !representative_file) return;
    
    // Ottieni il nome dell'album selezionato
    const char* selected_album = representative_file->metadata.album;
    
    // Crea un filtro per l'album selezionato
    MP3Filter filter;
    filter.filter_type = FILTER_BY_ALBUM;
    strncpy(filter.filter_text, selected_album, MAX_FILTER_LENGTH - 1);
    filter.filter_text[MAX_FILTER_LENGTH - 1] = '\0';
    
    // Libera la lista filtrata precedente se esiste
    if (gui->using_filtered_list && gui->current_list) {
        MP3File* current = gui->current_list;
        while (current) {
            MP3File* next = current->next;
            free(current);
            current = next;
        }
        gui->current_list = NULL;
    }
    
    // Crea una nuova lista filtrata con le tracce dell'album
    gui->current_list = filter_mp3_files(gui->library, &filter);
    gui->using_filtered_list = TRUE;
    
    // Ora ordina la lista filtrata per numero di traccia
    if (gui->current_list) {
        sort_mp3_files(&gui->current_list, SORT_BY_TRACK);
    }
    
    // Passa alla vista lista e mostra solo le tracce dell'album
    switch_view_mode(gui, VIEW_MODE_LIST);
    
    // Aggiorna la visualizzazione
    populate_list_view(gui);
    
    // Seleziona il primo elemento se disponibile
    if (ListView_GetItemCount(gui->hListView) > 0) {
        ListView_SetItemState(gui->hListView, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        gui->selected_item = 0;
        
        // Aggiorna anche la vista dettagli e la copertina dell'album
        update_details_view(gui);
        update_album_art(gui, representative_file);
    }
    
    // Aggiorna la barra di stato con informazioni sull'album
    char statusText[256];
    sprintf(statusText, "Album: %s - Artista: %s - %d tracce", 
            selected_album, 
            representative_file->metadata.artist[0] ? representative_file->metadata.artist : "Unknown",
            ListView_GetItemCount(gui->hListView));
    SetWindowText(gui->hStatusBar, statusText);
}