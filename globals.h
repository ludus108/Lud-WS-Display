#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include "lvgl_v8_port.h"
#include <EEPROM.h>   // per EEPROM

// ========================== NAMESPACE ==========================
using namespace esp_panel::drivers;
using namespace esp_panel::board;

// ========================== COSTANTI ==========================
#define PIN_BL           2
#define PWM_FREQ       5000
#define SD_CS          10
#define SD_MOSI        11
#define SD_CLK         12
#define SD_MISO        13
#define EEPROM_SIZE     4
#define EEPROM_ADDR     0
#define LOG_TIMEOUT  2000
#define TOAST_DUR    2000
#define WAVE_POINTS  100

// ========================== COLORI ==========================
#define COLOR_SLIDER_SYNTH_A_ACTIVE   0xFFFF00
#define COLOR_SLIDER_SYNTH_A_PASSED   0x00FF00
#define COLOR_SLIDER_SYNTH_B_ACTIVE   0xFF0000
#define COLOR_SLIDER_SYNTH_B_PASSED   0x440000
#define COLOR_SHAPE_SLIDER_ACTIVE     0xFFFF00
#define COLOR_SHAPE_SLIDER_PASSED     0x888800
#define COLOR_ARC_ACTIVE              0xFFFF00
#define COLOR_ARC_PASSED              0x888800

// ========================== WAVESHAPE ==========================
#define SRC_A 1
#define SRC_B 2
#define NUM_WAVES 26
#define WF_SAW_INDEX 0
#define WF_S8W_INDEX 1
#define WF_SQU_INDEX 3
#define WF_SIN_INDEX 4

enum WaveCategory { CAT_WF = 0, CAT_FM = 1, CAT_AM = 2 };

struct WaveDef { const char* name; int category; };
extern struct WaveDef WAVE_DEFS[NUM_WAVES];

#define WF_ITEMS "SAW\nSAW8\nTRI\nSQR\nSINE\nFM 1\nFM 2\nFM3\nNOISE"
#define FM_ITEMS "FM 0\nFM 1\nFM 2\nFM 3\nFM 4\nFM 5\nFM 6\nFM 7"
#define AM_ITEMS "AM 1\nAM 2\nAM 3\nAM 4\nAM 5\nAM 6\nAM 7\nAM 8"

// ========================== PRESET (INDIPENDENTI) ==========================
#define MAX_timbrA 23
#define MAX_timbrB 26
#define MAX_preset 16

enum paramA {
  wave_mode_A, wave_A, shape_A, shape_lev_A, shape_rate_A, lfo_pitch_lev_A,
  cutOff_A, res_A, vcf_lfo_A, vcf_env_A, vcf_ana_env_A,
  ana_ATTACK_A, ana_DECAY_A, ana_SUSTAIN_A, ana_RELEASE_A,
  vir_ATTACK_A, vir_DECAY_A, vir_SUSTAIN_A, vir_RELEASE_A,
  lfo_wave_A, lfo_rate_A, vca_vir_env_A, vca_lfo_A
};
enum paramB {
  wave_mode_B, wave_B, shape_B, shape_lev_B, shape_rate_B, lfo_pitch_lev_B,
  vcf_mode_B, cutOff_1_B, cutOff_2_B, cutOff_3_B, res_B,
  vcf_lfo_B, vcf_env_B, vcf_Bna_env_B,
  ana_BTTACK_B, ana_DECAY_B, ana_SUSTAIN_B, ana_RELEASE_B,
  vir_BTTACK_B, vir_DECAY_B, vir_SUSTAIN_B, vir_RELEASE_B,
  lfo_wave_B, lfo_rate_B, vca_vir_env_B, vca_lfo_B
};

// ========================== STRUTTURE GLOBALI ==========================
struct GlobalData {
    lv_obj_t *page, *log_f, *log_c, *toast;
    const char *synth;
    uint8_t bright, wa, wb, shape_a, shape_b, pre[8];
    uint32_t log_t, toast_t;
    bool log_v, toast_v, err, play;
    uint8_t pre_shape_A, pre_shape_B;
    bool shape_crs_A, shape_crs_B;
    int shape_last_A, shape_last_B;
    lv_obj_t *shape_arrow_A, *shape_arrow_B;
    lv_obj_t *shape_slider_A, *shape_slider_B;
    uint8_t arc_value, arc_target;
    bool arc_crossed;
    int arc_last;
    lv_obj_t *arc_obj, *arc_arrow, *arc_label_value, *arc_label_p1;
};
extern struct GlobalData g;

struct SliderData { lv_obj_t *label; int idx; };
extern struct SliderData sd[8];

struct ShapeData {
    lv_obj_t *label;
    int id;
    lv_obj_t *chart;
    lv_chart_series_t *serie;
    lv_obj_t *list;
    lv_obj_t *leds[3];
    int current_cat;
    const char *list_items;
};
extern struct ShapeData shape_data[2];

// ========================== OGGETTI LVGL ==========================
extern lv_obj_t *arr[8];
extern lv_obj_t *slider_objs[8];
extern bool crs[8];
extern int last[8];
extern int slider_base_x[8];
extern int slider_base_y[8];

extern lv_obj_t *mL, *mR, *mC;
extern lv_timer_t *mt;
extern float pkL, pkR, pkC;
extern const char* last_version;
extern uint8_t sliderColorDepth;

// ========================== PRESET ARRAY ==========================
extern int presetNumA, presetNumB;
extern int timbrA[MAX_preset][MAX_timbrA];
extern int timbrB[MAX_preset][MAX_timbrB];
extern int tempTimbrA[MAX_timbrA];
extern int tempTimbrB[MAX_timbrB];

// ========================== NOMI PRESET ==========================
extern char presetNamesA[MAX_preset][MAX_timbrA];
extern char presetNamesB[MAX_preset][MAX_timbrB];

// ========================== VARIABILI DROPDOWN E RENAME ==========================
extern int preset_visible_count;
extern lv_obj_t *preset_dropdown_A;
extern lv_obj_t *preset_dropdown_B;
extern lv_obj_t *preset_label_A;
extern lv_obj_t *preset_label_B;
extern int pendingPresetA;
extern int pendingPresetB;
extern bool blink_state;
extern lv_timer_t *blink_timer;

// ========================== VARIABILI GRIGLIA ==========================
extern lv_obj_t *grid_btns[2][20];
extern int grid_btn_count[2];
extern int grid_selected_idx[2];

// ========================== IMMAGINI (dichiarazioni esterne) ==========================
extern const lv_img_dsc_t img_slider_track;
extern const lv_img_dsc_t img_slider_indicator;
extern const lv_img_dsc_t img_slider_knob;
extern const lv_img_dsc_t freccina;
extern const lv_img_dsc_t freccina_oriz;
extern const lv_img_dsc_t img_meter_audio_track;
extern const lv_img_dsc_t img_meter_audio_indicator;
extern const lv_img_dsc_t img_meter_comp_track;
extern const lv_img_dsc_t img_meter_comp_indicator;
extern const lv_img_dsc_t img_slider_oriz_track;
extern const lv_img_dsc_t img_slider_oriz_indicator;
extern const lv_img_dsc_t img_slider_oriz_knob;
extern const lv_img_dsc_t img_arc_bg;
extern const lv_img_dsc_t img_arc_indic;
extern const lv_img_dsc_t home;

// ========================== PROTOTIPI FUNZIONI ==========================
void selPreset(byte chi, int idx);
void initPresetNamesA();
void initPresetNamesB();
void update_preset_labels();
void update_preset_dropdown(lv_obj_t *dd, int *numPtr);
void apply_pending_preset(int synth_id);
void update_preset_dropdown_options();
lv_obj_t* create_preset_selector(lv_obj_t *parent, int x, int y, const char *label_text, int *numPtr, char (*names)[32]);
void close_rename_window();
void open_rename_window(int preset_idx);
void update_slider_parameter(int idx, int value);
void update_slider_target(int idx);
void update_all_targets();
void init_sd();
void blink_timer_cb(lv_timer_t *timer);
void toast_show(const char *msg, lv_color_t c, uint32_t dur = TOAST_DUR);
void log_add(const char *msg, lv_color_t c);
void log_hide();
void log_show();
void save_bright();
void load_bright();
void set_bright(uint8_t v);
void create_home();
void create_page(const char *title);
void stop_meters();
void reset_arrows();
void wave_shape(int id, int src);
void h_slider(lv_obj_t *p, int id);
void submenu(lv_obj_t *p);
void btn(lv_obj_t *p, const char *l, int x, int y, int w, int h, lv_color_t c, int id);
void home_btn(lv_obj_t *p, int id);
void slider(lv_obj_t *p, int x, int y, const char *l, int idx,
            lv_color_t active_color, lv_color_t passed_color);
lv_obj_t* meter(lv_obj_t *p, int x, int y, int w, int h, const char *label, bool inv);
void update_meters();
void update_plotter_by_wave(int synth_id);
void populate_grid(lv_obj_t *container, const char *items, int selected_idx, int synth_id);
void update_wave_plot(lv_obj_t *chart, lv_chart_series_t *serie, uint8_t shape_val, int wave_index);
void update_leds(lv_obj_t **leds, int active_cat);
void update_slider_color(int idx, bool active);
void update_shape_slider_color(int synth_id, bool active);
void arc_with_image(lv_obj_t *parent, int x, int y, int w, int h);
void grid_btn_click(lv_event_t *e);
void earc_changed(lv_event_t *e);
void resetPingStatus();
void sendPing(char targetMCU);
void leggiSer();

// Event handler (definiti in lvglGrafFunc.cpp)
void eb(lv_event_t *e);
void ebright(lv_event_t *e);
void eslider(lv_event_t *e);
void ecat_btn(lv_event_t *e);
void ehslider(lv_event_t *e);
void elist(lv_event_t *e);
void rename_btn_click(lv_event_t *e);
void init_sd_btn_click(lv_event_t *e);

#endif