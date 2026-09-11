// Hardware: VIEWE UEDX80480050E_WB_B (ESP32-S3, 800x480)
/**
 * LUD-WS - Display Versione 0.0.10
 * ============================================================
 */
#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <math.h>
#include <lvgl.h>
#include "lvgl_v8_port.h"
#include <driver/ledc.h>
#include <EEPROM.h>
#include <SD.h>

// ========================== INCLUDE GLOBALE ==========================
#include "globals.h"
// ========================== DEFINIZIONI VARIABILI GLOBALI ==========================
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
const char* last_version = "V.0.10";
uint8_t sliderColorDepth = 50;

int presetNumA = 0;
int presetNumB = 0;
int timbrA[MAX_preset][MAX_timbrA];
int timbrB[MAX_preset][MAX_timbrB];
int tempTimbrA[MAX_timbrA];
int tempTimbrB[MAX_timbrB];
char presetNamesA[MAX_preset][MAX_timbrA];
char presetNamesB[MAX_preset][MAX_timbrB];
int preset_visible_count = 8;
lv_obj_t *preset_dropdown_A = NULL;
lv_obj_t *preset_dropdown_B = NULL;
lv_obj_t *preset_label_A = NULL;
lv_obj_t *preset_label_B = NULL;
int pendingPresetA = -1;
int pendingPresetB = -1;
bool blink_state = false;
lv_timer_t *blink_timer = NULL;
lv_obj_t *grid_btns[2][20];
int grid_btn_count[2] = {0, 0};
int grid_selected_idx[2] = {0, 0};

// ========================== DEFINIZIONE WAVESHAPE ==========================
WaveDef WAVE_DEFS[NUM_WAVES] = {
    {"SAW", CAT_WF}, {"SAW8", CAT_WF}, {"TRI", CAT_WF},
    {"SQR", CAT_WF}, {"SINE", CAT_WF}, {"FM 1", CAT_WF},
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
#include "lvglGraf.h"
#include "preset_sd.h"
#include "comunicazioni.h"

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
        for (int i = 0; i < MAX_timbrA; i++) tempTimbrA[i] = timbrA[idx][i];
    }
    if (chi == 1) {
        for (int i = 0; i < MAX_timbrB; i++) tempTimbrB[i] = timbrB[idx][i];
    }
}

// ========================== SETUP ==========================
void setup() {
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, 18, 17);
    EEPROM.begin(EEPROM_SIZE);
    load_bright();
    bool sd_ok = sd_init();

    randomSeed(analogRead(0));
    for (int i=0; i<8; i++) {
        g.pre[i] = random(0,256);
    }
    g.pre_shape_A = random(0, 101);
    g.pre_shape_B = random(0, 101);
    g.shape_crs_A = g.shape_crs_B = false;
    g.shape_last_A = g.shape_last_B = 50;

    g.arc_target = random(0, 101);
    g.arc_value = random(0, 101);
    g.arc_crossed = false;
    g.arc_last = 50;

    // PWM
    ledc_timer_config_t t = {LEDC_LOW_SPEED_MODE, LEDC_TIMER_10_BIT, LEDC_TIMER_0, PWM_FREQ, LEDC_AUTO_CLK};
    ledc_timer_config(&t);
    ledc_channel_config_t c = {PIN_BL, LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_INTR_DISABLE, LEDC_TIMER_0,
                               (uint32_t)map(g.bright,0,100,0,1023), 0};
    ledc_channel_config(&c);

    // Display
    Board *board = new Board();
    board->init();
    assert(board->begin());
    lvgl_port_init(board->getLCD(), board->getTouch());

    // Inizializza nomi e dropdown
     initPresetNamesA();
	 initPresetNamesB();
    update_preset_dropdown_options();

    lvgl_port_lock(-1);
    create_home();
    lvgl_port_unlock();

    // Log e caricamento preset da SD
    if (sd_ok) {
        log_add("SD: OK", lv_color_hex(0x00FF00));
        if (!loadAllFromSD()) {
            log_add("Preset non trovati, creazione default...", lv_color_hex(0xFFAA00));
            initPresetValues();
            saveAllToSD();
            log_add("Preset default creati e salvati", lv_color_hex(0x00FF00));
        } else {
            log_add("Preset caricati da SD", lv_color_hex(0x00FF00));
        }
		update_preset_dropdown_options();
        update_all_targets();
    } else {
        log_add("SD: ERRORE", lv_color_hex(0xFF0000));
    }

    // MCU Discovery
    delay(500);
    log_add("LUD WS avviato", lv_color_hex(0x00FF00));
    char b[16]; 
    snprintf(b, sizeof(b), "Lum: %d%%", g.bright);
    log_add(b, lv_color_hex(0xFFFFFF));
    log_add("MCU Discovery...", lv_color_hex(0xFFFF00));
    log_add("-----------------------------", lv_color_hex(0xFFFFFF));
    delay(500);
    discover_all_mcu();  // Ping a TUTTI gli MCU (una sola volta)
    delay(500);
}

// ========================== LOOP ==========================
void loop() {
    if (Serial1.available() > 0) leggiSer();
    lv_timer_handler();
    delay(5);

    if (g.log_v && !g.err && millis()-g.log_t > LOG_TIMEOUT) log_hide();
    if (g.toast_v && g.toast && millis() > g.toast_t) {
        lv_obj_add_flag(g.toast, LV_OBJ_FLAG_HIDDEN);
        g.toast_v = false;
    } 
}
