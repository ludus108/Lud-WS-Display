// Hardware: VIEWE UEDX80480050E_WB_B (ESP32-S3, 800x480)
//FQBN: esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi
// ============================================================
// LUD-WS-Display — Versione 0.16
// ============================================================
// MODIFICHE SESSIONE 2026-09-30
//
// REFACTORING
//  - lvglGrafModules.cpp diviso in 4 file:
//      lvglGrafDrum.cpp   (DRUM/SEQ editing)
//      lvglGrafSong.cpp   (SONG editor)
//      lvglGrafVcfB.cpp   (VCF SynthB)
//      lvglGrafFx.cpp     (FX / FV-1)
//    + lvglGrafRev.cpp   (REV — nuovo)
//    + lvglGrafKit.cpp   (KIT — nuovo)
//    Il vecchio lvglGrafModules.cpp è .bak
//
// DRUM / SEQ (editing pattern)
//  - Griglia spostata a y=0, bottoni spostati in basso (y=400)
//  - Bottone "SEQ" rinominato "PTN"
//  - Bottone "FL" verde (modalità FILL): griglia mostra fillArr
//  - Dropdown PTN 1..16 / FLN 1..16 dinamico (no freccia)
//  - Sezioni A/B/C/D con highlight
//  - Celle quadrate, rettangolino 22px più stretto, centrato
//  - Micro meter 22×24 con envelope (HOLD 100ms + DECAY 500ms)
//  - Linee orizzontali grigio, verticali giallo (col 0, 8) / grigio (col 4, 12)
//
// DRUM / SONG (editor song)
//  - Array songArr[16][3][256] (tipo/num/kit) + songLen[16]
//  - Griglia 4×8 slot con paginazione (32 slot/pagina, 8 pagine)
//  - Slot: label KIT n (top, 14pt) / A|B|C|D|FLN n (center) / num (bottom)
//  - Dropdown SNG 1..16, KIT (NO KIT + 16 nomi), FLN 1..16, A/B/C/D
//  - Paginazione ◄ ► + label N/M, +/− per add/remove slot (min 1)
//  - Kit su slot 1 permanente; modificabile solo su altri slot
//  - Linee orizzontali grigio, verticale giallo tra col 4 e 5
//
// DRUM / REV (riverbero)
//  - 8 slider senza freccina, colorati per gruppo:
//      PRED verde, SIZE azzurro, DAMP viola,
//      CUT1/CUT2 rosso porpora, RES1/RES2 giallo, LEV rosso
//  - Label nome top + valore bottom
//  - Toggle SER/PAR in basso
//  - Dropdown REV 1..16 collegato a revPresetArr[9][16]
//  - Cambio preset → aggiorna slider + SER/PAR
//
// DRUM / KIT (editor kit)
//  - Dropdown KIT (16 item "n Nome") + dropdown REV 1..16
//  - 9 dropdown voci (BD/SD/HH/OH/H2/CLAP/PERC1/PERC2/PERC3)
//    ciascuno elenca i set disponibili per la voce (estratti da lista.h)
//  - Cambio KIT → aggiorna i 9+1 dropdown da kitArr[10][16]
//  - Bottone RINOMINA: finestra modale con textarea + tastiera
//  - kit_names[16][24] editabile in RAM (persistenza SD: TODO)
//
// DRUM / (pagina principale)
//  - Layout: PTN+SONG centrati, KIT (arancione) + REV (rosso scuro) a destra
//  - Sotto-pagine: KIT, REV
//
// FIX
//  - Driver esp32 reinstallato (risolto link error objs.a)
//  - Dropdown senza freccia: lv_dropdown_set_symbol(dd, NULL)
//  - songLen da uint8_t a uint16_t
//
// TODO PRIORITARI
//  - Sync array (seqArr/fillArr/songArr) Display <-> Teensy via LWS
//  - Mute toggle (cmd 'M', 6 bitmask) in DRUM/SEQ
//  - BPM + Swing in DRUM/SEQ (cmd 'b' 'W')
//  - Pagina PLAY: play mode pattern/song, trigger fill, sezioni live
//  - Persistenza kit_names su SD (/drum/kit_names.txt)
//  - Mappatura LWS RES/EnvA/EnvV (VCF-B)
//  - Python generator preset SynthB aggiornato
// ============================================================
#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <math.h>
#include <lvgl.h>
#include "lvgl_v8_port.h"
#include <driver/ledc.h>
#include <EEPROM.h>
#include <SD.h>

// ======== INCLUDE GLOBALE ================
#include "globals.h"
#define LWS_BAUD    1000000UL      // 1 Mbps — bus LWS
// ============= DEFINIZIONI VARIABILI GLOBALI =======
const char* last_version = "V 0.0.16";
struct GlobalData g;
lv_obj_t *arr[8] = {0};
lv_obj_t *slider_objs[8] = {0};
bool crs[8] = {0};
int last[8] = {0};
int slider_base_x[8] = {0};
int slider_base_y[8] = {0};
struct SliderData sd[8];
struct ShapeData shape_data[2];
lv_obj_t *mL=0, *mR=0, *mC=0;
lv_timer_t *mt=0;
float pkL=0, pkR=0, pkC=0;
uint8_t sliderColorDepth = 50;

int presetNumA = 0;
int presetNumB = 0;

int preset_visible_count = 8;
lv_obj_t *preset_dropdown_A = NULL;
lv_obj_t *preset_dropdown_B = NULL;
lv_obj_t *preset_label_A = NULL;
lv_obj_t *preset_label_B = NULL;
lv_obj_t *pot_container = nullptr;
int pendingPresetA = -1;
int pendingPresetB = -1;
bool blink_state = false;
lv_timer_t *blink_timer = NULL;
lv_obj_t *grid_btns[2][20];
int grid_btn_count[2] = {0, 0};
int grid_selected_idx[2] = {0, 0};
lv_obj_t *timeline_obj       = nullptr;
lv_obj_t *timeline_bar_bg    = nullptr;
lv_obj_t *timeline_bar_fill  = nullptr;
lv_obj_t *timeline_cursor    = nullptr;
lv_obj_t *timeline_label_cur = nullptr;
lv_obj_t *timeline_label_tot = nullptr;
uint32_t  timeline_cur_ms    = 0;
uint32_t  timeline_total_ms  = 0;
int       timeline_bar_w     = 0;
lv_timer_t *tdt             = nullptr;
uint32_t    timeline_demo_ms = 0;
lv_obj_t   *timeline_btn_init = nullptr;
lv_obj_t   *timeline_btn_stop = nullptr;
lv_obj_t   *timeline_btn_play = nullptr;
bool        timeline_playing  = false;
lv_obj_t          *env_chart_A = nullptr;
lv_obj_t          *env_chart_B = nullptr;
lv_chart_series_t *env_serie_A = nullptr;
lv_chart_series_t *env_serie_B = nullptr;
lv_chart_series_t *env_serie_tgt_A = nullptr;
lv_chart_series_t *env_serie_tgt_B = nullptr;
int midi_channel_A = 1;
int midi_channel_B = 2;
int midi_channel_D = 3;
int midi_split = 0;
lv_obj_t *kb_white[KB_WHITE_KEYS] = {0};
lv_obj_t *kb_black[KB_BLACK_KEYS] = {0};
int       kb_white_note[KB_WHITE_KEYS];
int       kb_black_note[KB_BLACK_KEYS];
lv_obj_t *keyboard_obj = nullptr;
lv_obj_t *drum_pattern_label = nullptr;
volatile bool    preset_ack_received = false;
volatile uint8_t preset_ack_status   = 0;
// ========================== DEFINIZIONE WAVESHAPE ==========================
WaveDef WAVE_DEFS[NUM_WAVES] = {
    {"SAW", CAT_WF},  {"SAW8", CAT_WF}, {"TRI", CAT_WF},
    {"SQR", CAT_WF},  {"SINE", CAT_WF}, {"FM 1", CAT_WF},
    {"FM 2", CAT_WF}, {"FM 3", CAT_WF}, {"NOISE", CAT_WF},
    {"FM 1", CAT_FM}, {"FM 2", CAT_FM}, {"FM 3", CAT_FM},
    {"FM 4", CAT_FM}, {"FM 5", CAT_FM}, {"FM 6", CAT_FM},
    {"FM 7", CAT_FM}, {"FM 8", CAT_FM},
    {"AM 1", CAT_AM}, {"AM 2", CAT_AM}, {"AM 3", CAT_AM},
    {"AM 4", CAT_AM}, {"AM 5", CAT_AM}, {"AM 6", CAT_AM},
    {"AM 7", CAT_AM}, {"AM 8", CAT_AM}
};

// ========================== PROTOTIPI ==========================
bool sd_init();

// ========================== INCLUDE HEADER ==========================
#include "src/grafica/lvglGraf.h"
#include "comunicazioni.h"
#include "src/preset/preset_sd.h"           // (già dentro globals.h, se hai fatto la patch)
#include "src/preset/preset_cache.h"
#include "src/preset/preset_transfer.h"     // contiene sendBlobToVoice, loadPresetToVoice, ecc.
#include "src/preset/preset_ui.h"
// =========================================================================
// PRESET — Wrapper per la UI
// =========================================================================

bool requestPresetLoad(int synth, int presetId) {
    if (synth == 0) {
        // SynthA: carica su tutte le voci (Poly). Se sei in MultiMono,
        // chiama loadPresetSynthA_MultiMono(map) al posto di questa.
        return loadPresetSynthA_Poly((uint8_t)presetId);
    } else {
        return loadPresetSynthB((uint8_t)presetId);
    }
}

bool requestPresetSave(int synth, int presetId) {
    if (synth == 0) {
        // Salva la cache della voce 0 (la UI del Display rappresenta
        // una sola voce alla volta in modalità Poly).
        return savePresetA(0, (uint8_t)presetId);
    } else {
        return savePresetB((uint8_t)presetId);
    }
}

void requestRenameApply(int synth, int presetId, const char *newName) {
    if (newName == nullptr) return;
    if (synth == 0) {
        strncpy(nome_presetA[presetId], newName, PRESET_NAME_LEN - 1);
        nome_presetA[presetId][PRESET_NAME_LEN - 1] = '\0';
        savePresetNamesToSD(0);
    } else {
        strncpy(nome_presetB[presetId], newName, PRESET_NAME_LEN - 1);
        nome_presetB[presetId][PRESET_NAME_LEN - 1] = '\0';
        savePresetNamesToSD(1);
    }
}
// ========================== SD CARD ==========================
bool sd_init() {
    SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS)) return false;
    File f = SD.open("/test.txt", FILE_WRITE);
    if (!f) return false;
    f.println("LUD-WS SD OK");
    f.close();
    return true;
}

// ========================== SEL PRESET ==========================
void selPreset(byte chi, int idx) {
    if (chi == 0) {
        // SynthA: carica preset idx su tutte le voci (Poly)
        loadPresetSynthA_Poly((uint8_t)idx);
    } else if (chi == 1) {
        // SynthB
        loadPresetSynthB((uint8_t)idx);
    }
}
void reset_GT911(){
		pinMode(18, OUTPUT);   // GT911 INT
digitalWrite(18, LOW); // seleziona indirizzo 0x5D (o HIGH per 0x14)
pinMode(38, OUTPUT);   // GT911 RST
digitalWrite(38, LOW);
delay(50);
digitalWrite(38, HIGH);
delay(100);
}
// ========================== SETUP ==========================
void setup() {
    Serial.begin(115200);
reset_GT911();

    Serial1.begin(LWS_BAUD, SERIAL_8N1, 18, 17);
    EEPROM.begin(EEPROM_SIZE);
  load_all_settings();
    bool sd_ok = sd_init();

    randomSeed(analogRead(0));
    for (int i = 0; i < 8; i++) {
        g.pre[i] = random(0, 256);
    }
    g.pre_shape_A = random(0, 101);
    g.pre_shape_B = random(0, 101);
    g.shape_crs_A = g.shape_crs_B = false;
    g.shape_last_A = g.shape_last_B = 50;

    // Inizializza i 6 arc/pot (Synth A / MOD)
    for (int i = 0; i < 6; i++) {
        g.arc_target[i]      = random(0, 101);
        g.arc_value[i]       = random(0, 101);
        g.arc_crossed[i]     = false;
        g.arc_last[i]        = 50;
        g.arc_obj[i]         = NULL;
        g.arc_arrow[i]       = NULL;
        g.arc_label_value[i] = NULL;
        g.arc_label_p[i]     = NULL;
    }

    // PWM backlight
    ledc_timer_config_t t = {LEDC_LOW_SPEED_MODE, LEDC_TIMER_10_BIT, LEDC_TIMER_0,
                             PWM_FREQ, LEDC_AUTO_CLK};
    ledc_timer_config(&t);
    ledc_channel_config_t c = {PIN_BL, LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0,
                               LEDC_INTR_DISABLE, LEDC_TIMER_0,
                               (uint32_t)map(g.bright, 0, 100, 0, 1023), 0};
    ledc_channel_config(&c);

    // Display + LVGL
    Board *board = new Board();
    board->init();
    assert(board->begin());
    lvgl_port_init(board->getLCD(), board->getTouch());

    // Preset names + dropdown options
    initPresetNamesA();
    initPresetNamesB();
    update_preset_dropdown_options();

    lvgl_port_lock(-1);
    create_home();
    lvgl_port_unlock();

    // Log + caricamento preset da SD
       if (sd_ok) {
        log_add("SD: OK", lv_color_hex(0x00FF00));

        // Inizializza struttura cartelle + nomi preset (30 per A e B)
        init_sd();

        // Cache locale vuota all'avvio
        presetCacheClearAll();

        log_add("Preset pronti", lv_color_hex(0x00FF00));
    } else {
        log_add("SD: ERRORE", lv_color_hex(0xFF0000));
    }
// Chiedi al Teensy il pattern drum corrente
send_param_update(ID_TEENSY, 'Q', 0);
    // Boot messages
    log_add("LUD WS avviato", lv_color_hex(0x00FF00));
    char b[16];
    snprintf(b, sizeof(b), "Lum: %d%%", g.bright);
    log_add(b, lv_color_hex(0xFFFFFF));
    log_add("MCU Discovery...", lv_color_hex(0xFFFF00));
    log_add("-----------------------------", lv_color_hex(0xFFFFFF));

    // Discovery NON bloccante: parte e prosegue nel loop()
    discover_all_mcu_start();
}

// ========================== LOOP ==========================
void loop() {
    // RX LWSv1 dal Router
    if (Serial1.available() > 0) leggiSer();

    // State machine discovery (un ping per ciclo, early-exit se tutti rispondono)
    discover_all_mcu_poll();

    // UI
    lv_timer_handler();
    delay(5);

   // Auto-hide del log: dopo LOG_TIMEOUT (2000 ms), anche per gli errori
    // Non nasconde durante la discovery, per non perdere messaggi utili.
    if (g.log_v && !discovery_active() && millis() - g.log_t > LOG_TIMEOUT) {
        log_hide();
        g.err = 0;   // reset del flag, così il prossimo errore può auto-nascondersi
    }

    // Auto-hide del toast
    if (g.toast_v && g.toast && millis() > g.toast_t) {
        lv_obj_add_flag(g.toast, LV_OBJ_FLAG_HIDDEN);
        g.toast_v = false;
    }
}