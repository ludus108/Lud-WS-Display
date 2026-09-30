#ifndef LVGLGRAF_INTERNAL_H
#define LVGLGRAF_INTERNAL_H
// ============================================================
// Header interno condiviso tra i 3 .cpp di lvglGraf*.
// Contiene: statics condivise, prototipi cross-file, debug macro.
// ============================================================

#include "globals.h"

// --------------------------- DEBUG ---------------------------
#define LGF_DBG 0
#if LGF_DBG
  #define DBG(...)  Serial.printf("[GF] " __VA_ARGS__)
#else
  #define DBG(...)  do {} while (0)
#endif

// --------------------------- UI MAP --------------------------
// Indice UI waveform → valore firmware SynthB
// 0 SAW→0, 1 SAW8→4, 2 TRI→3, 3 SQR→2, 4 SINE→1, 5..7 FM1..3→5..7, 8 NOISE→8
extern const uint8_t ui2fw_wave[9];

// ============================================================
// DRUM SEQUENCER (definito in lvglGrafModules.cpp)
// ============================================================
#define DRUM_ROWS 9
#define DRUM_COLS 16
// DRUM — SONG editor
extern lv_obj_t *song_song_dd;
extern lv_obj_t *song_fln_dd;
extern lv_obj_t *song_sec_btn[4];
extern lv_obj_t *song_sec_lbl[4];
extern lv_obj_t *song_page_lbl;
extern uint8_t   song_cur_song;
extern uint8_t   song_cur_page;
extern int       song_sel_slot;   // -1 = nessuno; altrimenti indice globale 0..255
extern lv_obj_t *song_kit_dd;
void song_page_create(lv_obj_t *parent);
extern lv_obj_t *drum_fl_btn;
extern lv_obj_t *drum_fl_lbl;
extern uint8_t   drum_mode;
extern uint8_t   drum_cur_fill;
// DRUM — REV page
extern lv_obj_t *rev_mode_btn;
extern lv_obj_t *rev_mode_lbl;
extern lv_obj_t *rev_preset_dd;
extern uint8_t   rev_preset;
extern uint8_t   rev_mode;

void rev_page_create(lv_obj_t *parent);
// DRUM — KIT page
extern lv_obj_t *kit_preset_dd;
extern lv_obj_t *kit_rev_dd;
extern lv_obj_t *kit_voice_dd[9];
extern uint8_t   kit_cur_kit;   // 0..10
#define KIT_NAME_LEN 24
extern char kit_names[16][KIT_NAME_LEN];
void kit_close_rename_window();
void kit_page_create(lv_obj_t *parent);

// DRUM — SONG: layout griglia + array slot (visibili cross-file)
#define SONG_SLOTS           256
#define SONG_SLOTS_PER_PAGE  32
#define SONG_ROWS            4
#define SONG_COLS            8

extern lv_obj_t *song_slot    [SONG_ROWS][SONG_COLS];
extern lv_obj_t *song_slot_lbl[SONG_ROWS][SONG_COLS];
extern lv_obj_t *song_slot_kit_lbl[SONG_ROWS][SONG_COLS];
extern lv_obj_t *song_slot_num_lbl[SONG_ROWS][SONG_COLS];
// Immagini micro meter (definite in images.c)
extern const lv_img_dsc_t img_micro_meter_audio_track;
extern const lv_img_dsc_t img_micro_meter_audio_indicator;
// DRUM — micro meter per riga + dropdown pattern
extern lv_obj_t *drum_row_meter[DRUM_ROWS];
extern lv_obj_t *drum_ptn_dd;
// DRUM — sezione A/B/C/D (pendente al boundary sul Teensy)
extern lv_obj_t *drum_sec_btn[4];
extern lv_obj_t *drum_sec_lbl[4];
extern uint8_t   drum_cur_section;   // 0=A 1=B 2=C 3=D
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
// DRUM — sezione A/B/C/D (pendente al boundary sul Teensy)
extern lv_obj_t *drum_sec_btn[4];
extern lv_obj_t *drum_sec_lbl[4];
// drum_cur_section resta file-static in Modules: persiste in RAM
// tra una pagina e l'altra, non serve esporlo.
// ============================================================
// SYNTHB VCF PAGE (definito in lvglGrafModules.cpp)
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

#define VCF_TICK_COUNT  5
#define VCF_SCALE_MAX   6

extern lv_obj_t *sB_vcf_arc[3];
extern lv_obj_t *sB_vcf_arc_dot[3];
extern lv_obj_t *sB_vcf_arc_lbl[3];
extern lv_obj_t *sB_vcf_val_lbl[3];
extern lv_obj_t *sB_vcf_top_lbl[3];
extern lv_obj_t *sB_vcf_tick[3][VCF_TICK_COUNT];
extern lv_obj_t *sB_vcf_scale_lbl[3][VCF_SCALE_MAX];
extern uint8_t   sB_vcf_cut[3];
// 3 arc addizionali sotto F1/F2/F3: RES, EnvA, EnvV
// (hanno tick radiali + label centrale + label valore sotto, NO pallino/target)
// 3 arc addizionali sotto F1/F2/F3: RES, EnvA, EnvV
// (hanno tick radiali + label centrale + label valore sotto + pallino target)
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

// Plotter ADSR della pagina VCF SynthB (sopra il frame ENV vir)
extern lv_obj_t          *sB_vcf_env_chart;
extern lv_chart_series_t *sB_vcf_env_serie;
void sB_vcf_env_plot_update();   // chiamata da eslider (Pages)

// ============================================================
// FX / FV-1 (definito in lvglGrafModules.cpp)
// ============================================================
extern lv_obj_t *pot_size_FV1;
extern lv_obj_t *pot_LF_FV1;
extern lv_obj_t *pot_HF_FV1;
extern lv_obj_t *fx_rev_btn;
extern lv_obj_t *fx_rev_lbl;
extern lv_obj_t *fx_preset_dd;
extern lv_obj_t *fx_save_btn;

// ============================================================
// PROTOTIPI cross-file (Modules)
// ============================================================
void drum_seq_page_create(lv_obj_t *parent, lv_obj_t *title_lbl);
void vcf_page_synthb_create(lv_obj_t *parent);
void fx_page_create(lv_obj_t *parent);

// ============================================================
// PROTOTIPI lifecycle (Pages)
// ============================================================
void reset_ui_pointers();
void page_begin(const char *title);   // title==nullptr → HOME

#endif