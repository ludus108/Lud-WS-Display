#ifndef LVGLGRAF_INTERNAL_H
#define LVGLGRAF_INTERNAL_H
// ============================================================
// Header interno condiviso tra i .cpp di lvglGraf*.
// Contiene: statics condivise, prototipi cross-file, debug macro.
// ============================================================

#include "globals.h"

// ============================================================
// DIMENSIONI GLOBALI (devono stare PRIMA degli extern che le usano)
// ============================================================
#define DRUM_ROWS            9
#define DRUM_COLS            16

#define SONG_SLOTS           256
#define SONG_SLOTS_PER_PAGE  32
#define SONG_ROWS            4
#define SONG_COLS            8

#define SONG_COL_TIPO        0
#define SONG_COL_NUM         1
#define SONG_COL_KIT         2
#define SONG_COL_REV         3

#define SONG_NAME_LEN        24
#define KIT_NAME_LEN         24

#define DRUM_MIX_COUNT       8

#define VCF_TICK_COUNT       5
#define VCF_SCALE_MAX        6

// --------------------------- DEBUG ---------------------------
#define LGF_DBG 0
#if LGF_DBG
  #define DBG(...)  Serial.printf("[GF] " __VA_ARGS__)
#else
  #define DBG(...)  do {} while (0)
#endif

void chorus_page_create(lv_obj_t *parent);
void chorus_reset_pointers();
void phaser_page_create(lv_obj_t *parent);
void phaser_reset_pointers();
// --------------------------- UI MAP --------------------------
// Indice UI waveform → valore firmware SynthB
// 0 SAW→0, 1 SAW8→4, 2 TRI→3, 3 SQR→2, 4 SINE→1, 5..7 FM1..3→5..7, 8 NOISE→8
extern const uint8_t ui2fw_wave[9];

// ============================================================
// IMMAGINI (definite in images.c)
// ============================================================
extern const lv_img_dsc_t img_micro_meter_audio_track;
extern const lv_img_dsc_t img_micro_meter_audio_indicator;

// ============================================================
// DRUM — SEQUENCER (in lvglGrafDrum.cpp)
// ============================================================
extern lv_obj_t  *drum_cell[DRUM_ROWS][DRUM_COLS];
extern lv_obj_t  *drum_dot [DRUM_ROWS][DRUM_COLS];
extern uint8_t    drum_pattern_data[DRUM_ROWS][DRUM_COLS];
extern lv_obj_t  *drum_cursor;
extern uint8_t    drum_step_counter;
extern bool       drum_playing;
extern lv_timer_t*drum_step_timer;
extern lv_obj_t  *drum_play_btn;
extern lv_obj_t  *drum_play_lbl;
extern int  drum_grid_x, drum_grid_y, drum_grid_w, drum_grid_h, drum_cell_w;

extern lv_obj_t  *drum_row_meter[DRUM_ROWS];
extern lv_obj_t  *drum_ptn_dd;

extern lv_obj_t *drum_sec_btn[4];
extern lv_obj_t *drum_sec_lbl[4];
extern uint8_t   drum_cur_section;   // 0=A 1=B 2=C 3=D

extern lv_obj_t *drum_fl_btn;
extern lv_obj_t *drum_fl_lbl;
extern uint8_t   drum_mode;
extern uint8_t   drum_cur_fill;

void drum_seq_page_create(lv_obj_t *parent, lv_obj_t *title_lbl);

// ============================================================
// DRUM — BPM + SWING (in lvglGrafDrum.cpp)
// ============================================================
extern lv_obj_t *drum_bpm_btn;
extern lv_obj_t *drum_bpm_lbl;
extern lv_obj_t *drum_swing_slider;
extern lv_obj_t *drum_swing_val_lbl;
extern uint16_t  drum_bpm;
extern uint16_t  drum_swing;

void drum_meter_init();
void drum_bpm_swing_create(lv_obj_t *parent);
void drum_close_bpm_window();
uint32_t drum_swing_max_for_bpm(uint16_t bpm);
void drum_bpm_set_from_teensy(uint16_t bpm);
void drum_swing_set_from_teensy(uint16_t swing);

// ============================================================
// DRUM — MIX (8 slider livelli voce)
// ============================================================
extern lv_obj_t *drum_mix_slider [DRUM_MIX_COUNT];
extern lv_obj_t *drum_mix_val_lbl[DRUM_MIX_COUNT];
extern uint8_t   drum_mix_val    [DRUM_MIX_COUNT];

void drum_mix_create(lv_obj_t *parent);

// ============================================================
// DRUM — SONG editor (in lvglGrafSong.cpp)
// ============================================================
extern uint8_t  songArr[16][4][SONG_SLOTS];
extern uint16_t songLen[16];
extern uint8_t  song_cur_song;
extern uint8_t  song_cur_page;
extern int      song_sel_slot;   // -1 = nessuno

extern char song_names[16][SONG_NAME_LEN];

extern lv_obj_t *song_song_dd;
extern lv_obj_t *song_fln_dd;
extern lv_obj_t *song_kit_dd;
extern lv_obj_t *song_rev_dd;
extern lv_obj_t *song_sec_btn[4];
extern lv_obj_t *song_sec_lbl[4];
extern lv_obj_t *song_page_lbl;
extern lv_obj_t *song_play_btn;
extern lv_obj_t *song_play_lbl;
extern lv_obj_t *song_cursor;

void songArr_init_if_needed();
void drum_seqArr_init_if_needed();

extern bool      song_loop;
extern lv_obj_t *song_loop_btn;
extern lv_obj_t *song_loop_lbl;

extern lv_obj_t *song_slot            [SONG_ROWS][SONG_COLS];
extern lv_obj_t *song_slot_lbl        [SONG_ROWS][SONG_COLS];
extern lv_obj_t *song_slot_kit_lbl    [SONG_ROWS][SONG_COLS];
extern lv_obj_t *song_slot_num_lbl    [SONG_ROWS][SONG_COLS];
extern lv_obj_t *song_slot_rev_lbl    [SONG_ROWS][SONG_COLS];

extern int  song_play_slot;
extern int  song_play_step;
int  song_current_section();      // -1 se fill o non playing
int  song_current_pattern();      // -1 se non playing
bool song_current_is_fill();      // true se slot corrente è fill
//-------------------------------------------------------------------------------

extern int drum_cursor_off_x;
void drum_meters_update_from_slot(int slot, uint8_t step);
// DRUM — MIX meters (micro meter per slider DRUM MIX)
extern lv_obj_t *drum_mix_meter[DRUM_MIX_COUNT];
void drum_mix_meters_trigger_from_slot(int slot, uint8_t step);
void drum_mix_meters_reset();

// DRUM — array per accesso cross-file (song → meter)
extern uint8_t drum_seqArr[16][DRUM_ROWS][64];
extern uint8_t drum_fillArr[16][DRUM_ROWS][16];

void song_page_create(lv_obj_t *parent);
void song_stop();
void song_update_bpm(uint16_t bpm);
void song_close_rename_window();
void song_send_to_teensy(uint8_t song);
extern lv_obj_t *drum_play_song_btn;
extern lv_obj_t *drum_play_song_lbl;

void song_play();   // pubblica (era static)
void drum_play_song_btn_cb(lv_event_t *e);
extern bool song_playing;
// DRUM — pagina principale: dropdown SNG + monitor
extern lv_obj_t *drum_song_dd;
extern lv_obj_t *drum_monitor_lbl;
void drum_monitor_update();
void song_build_dd_options(char *buf, size_t bufSize);

void drum_seq_update_bpm(uint16_t bpm);
// ============================================================
// DRUM — REV (in lvglGrafRev.cpp)
// ============================================================
extern lv_obj_t *rev_mode_btn;
extern lv_obj_t *rev_mode_lbl;
extern lv_obj_t *rev_preset_dd;
extern uint8_t   rev_preset;
extern uint8_t   rev_mode;

void rev_page_create(lv_obj_t *parent);

// ============================================================
// DRUM — KIT (in lvglGrafKit.cpp)
// ============================================================
extern lv_obj_t *kit_preset_dd;
extern lv_obj_t *kit_rev_dd;
extern lv_obj_t *kit_voice_dd[9];
extern uint8_t   kit_cur_kit;   // 0..15
extern char      kit_names[16][KIT_NAME_LEN];

void kit_close_rename_window();
void kit_page_create(lv_obj_t *parent);

// ============================================================
// SYNTHB VCF PAGE (in lvglGrafVcfB.cpp)
// ============================================================
extern lv_obj_t *sB_vcf_btn;
extern lv_obj_t *sB_vcf_btn_lbl;
extern uint8_t   sB_vcf_mode;

extern lv_obj_t *sB_sub_btn;
extern lv_obj_t *sB_sub_lbl;
extern uint8_t   sB_filter_submode;
extern uint8_t   sB_wovel_submode;

extern lv_obj_t *sB_filter_type_btn;
extern lv_obj_t *sB_filter_type_lbl;
extern uint8_t   sB_filter_type;

extern lv_obj_t *sB_vcf_arc[3];
extern lv_obj_t *sB_vcf_arc_dot[3];
extern lv_obj_t *sB_vcf_arc_lbl[3];
extern lv_obj_t *sB_vcf_val_lbl[3];
extern lv_obj_t *sB_vcf_top_lbl[3];
extern lv_obj_t *sB_vcf_tick[3][VCF_TICK_COUNT];
extern lv_obj_t *sB_vcf_scale_lbl[3][VCF_SCALE_MAX];
extern uint8_t   sB_vcf_cut[3];

extern lv_obj_t *sB_vcfb_arc[3];
extern lv_obj_t *sB_vcfb_lbl[3];
extern lv_obj_t *sB_vcfb_val_lbl[3];
extern lv_obj_t *sB_vcfb_dot[3];
extern lv_obj_t *sB_vcfb_tick[3][VCF_TICK_COUNT];
extern uint8_t   sB_vcfb_target[3];
extern int       sB_vcfb_last[3];
extern bool      sB_vcfb_crossed[3];

extern lv_obj_t *sB_wov_env_att_dd;
extern lv_obj_t *sB_wov_env_sus_dd;
extern lv_obj_t *sB_wov_env_rel_dd;
extern lv_obj_t *sB_wov_env_att_lbl;
extern lv_obj_t *sB_wov_env_sus_lbl;
extern lv_obj_t *sB_wov_env_rel_lbl;

extern lv_obj_t          *sB_vcf_env_chart;
extern lv_chart_series_t *sB_vcf_env_serie;

void sB_vcf_env_plot_update();
void vcf_page_synthb_create(lv_obj_t *parent);

// ============================================================
// FX / FV-1 (in lvglGrafFx.cpp)
// ============================================================
extern lv_obj_t *pot_size_FV1;
extern lv_obj_t *pot_LF_FV1;
extern lv_obj_t *pot_HF_FV1;
extern lv_obj_t *fx_rev_btn;
extern lv_obj_t *fx_rev_lbl;
extern lv_obj_t *fx_preset_dd;
extern lv_obj_t *fx_save_btn;

void fx_page_create(lv_obj_t *parent);

// ============================================================
// LIFECYCLE (in lvglGrafPages.cpp)
// ============================================================
void reset_ui_pointers();
void page_begin(const char *title);   // title==nullptr → HOME
// ============================================================
// DLY A / DLY B (in lvglGrafDly.cpp)
// ============================================================
void dly_page_create(lv_obj_t *parent, bool isA);
void dly_reset_pointers();
// ============================================================
// VCF A (in lvglGrafVcfA.cpp)
// ============================================================
void vcfA_page_create(lv_obj_t *parent);
void vcfA_reset_pointers();
void vcfA_env_plot_update();
// ============================================================
// Dati FM (definiti in lvglGrafCore.cpp, editabili da FM Edit)
// ============================================================
extern uint8_t fmSetSin[8][3];
extern uint8_t fmSetDiv[8][3];

// ============================================================
// FM Edit (in lvglGrafFmEdit.cpp)
// ============================================================
void fmEdit_page_create(lv_obj_t *parent, bool isA);
void fmEdit_reset_pointers();
void fmEdit_update_plot();       // ridisegna il plotter col preset corrente
#endif