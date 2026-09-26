// ============================================================
// lvglGrafModules.cpp — Sottosistemi specializzati:
//   - Drum sequencer
//   - SynthB VCF (Filter/Wovel)
//   - FX / FV-1
// ============================================================
#include "globals.h"
#include "lvglGraf.h"
#include "lvglGraf_internal.h"
#include "lvgl_v8_port.h"
#include "src/preset/preset_ui.h"
#include <math.h>

// ============================================================
// ============ 1) DRUM SEQUENCER =============================
// ============================================================
lv_obj_t  *drum_cell[DRUM_ROWS][DRUM_COLS] = {};
lv_obj_t  *drum_dot [DRUM_ROWS][DRUM_COLS] = {};
uint8_t    drum_pattern_data[DRUM_ROWS][DRUM_COLS] = {};
lv_obj_t  *drum_cursor       = nullptr;
uint8_t    drum_step_counter = 0;
bool       drum_playing      = false;
lv_timer_t*drum_step_timer   = nullptr;
lv_obj_t  *drum_play_btn     = nullptr;
lv_obj_t  *drum_play_lbl     = nullptr;
int        drum_grid_x = 0, drum_grid_y = 0, drum_grid_w = 0,
           drum_grid_h = 0, drum_cell_w = 0;

static const char *drum_row_names[DRUM_ROWS] = {
    "BD","SD","OH","HH","H2","Cp","P1","P2","P3"
};
static const uint32_t drum_row_colors[DRUM_ROWS] = {
    0xAA0044, 0xFF8800, 0xFFFF00, 0xFFFF00, 0xAAAA00,
    0x00FF00, 0x00AAFF, 0x00AAFF, 0x00AAFF
};

#define SD_COLOR_V1 0xAA5500
#define SD_COLOR_V2 0xFF8800
#define SD_COLOR_V3 0xFFCC66
#define H2_COLOR_V1 0x666600
#define H2_COLOR_V2 0xAAAA00
#define H2_COLOR_V3 0xDDDD44

static inline bool drum_row_is_4state(int row) {
    return (row == 1 || row == 4);
}

static uint32_t drum_cell_color(int row, uint8_t value) {
    if (row == 1) {
        switch (value) {
            case 1: return SD_COLOR_V1;
            case 2: return SD_COLOR_V2;
            case 3: return SD_COLOR_V3;
        }
    }
    if (row == 4) {
        switch (value) {
            case 1: return H2_COLOR_V1;
            case 2: return H2_COLOR_V2;
            case 3: return H2_COLOR_V3;
        }
    }
    return drum_row_colors[row];
}

static void drum_grid_create(lv_obj_t *parent) {
    const int X0      = 10;
    const int Y0      = 55;
    const int Y1      = 355;
    const int X1      = 790;
    const int LABEL_W = 40;

    int gridX = X0 + LABEL_W + 5;
    int gridW = X1 - gridX;
    int gridH = Y1 - Y0;

    int cellW = gridW / DRUM_COLS;
    int cellH = gridH / DRUM_ROWS;
    int gap   = 2;
    int cw    = cellW - gap;
    int ch    = cellH - gap;
    int dsize = ((cw < ch) ? cw : ch) * 80 / 100;

    for (int r = 0; r < DRUM_ROWS; r++) {
        lv_obj_t *lbl = lv_label_create(parent);
        lv_label_set_text(lbl, drum_row_names[r]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(drum_row_colors[r]), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(lbl, X0, Y0 + r * cellH + (cellH - 16) / 2);

        for (int c = 0; c < DRUM_COLS; c++) {
            int x = gridX + c * cellW;
            int y = Y0 + r * cellH;

            lv_obj_t *cell = lv_obj_create(parent);
            lv_obj_set_size(cell, cw, ch);
            lv_obj_set_pos(cell, x, y);
            lv_obj_set_style_bg_color(cell, lv_color_hex(0x1A1A2E), 0);
            lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(cell, 1, 0);
            lv_obj_set_style_border_color(cell, lv_color_hex(0x444444), 0);
            lv_obj_set_style_radius(cell, 2, 0);
            lv_obj_set_style_pad_all(cell, 0, 0);
            lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_bg_color(cell, lv_color_hex(0x2A2A4E), LV_STATE_PRESSED);
            lv_obj_set_style_border_color(cell, lv_color_hex(0x8888FF), LV_STATE_PRESSED);

            lv_obj_add_event_cb(cell, [](lv_event_t *ev){
                int id  = (int)(uintptr_t)lv_event_get_user_data(ev);
                int row = (id >> 8) & 0xFF;
                int col =  id       & 0xFF;
                if (row >= DRUM_ROWS || col >= DRUM_COLS) return;

                uint8_t cur = drum_pattern_data[row][col];
                uint8_t next;
                if (drum_row_is_4state(row)) next = (cur >= 3) ? 0 : (cur + 1);
                else                         next = cur ? 0 : 1;
                drum_pattern_data[row][col] = next;

                if (drum_dot[row][col] && lv_obj_is_valid(drum_dot[row][col])) {
                    if (next > 0) {
                        lv_obj_set_style_bg_color(drum_dot[row][col],
                            lv_color_hex(drum_cell_color(row, next)), 0);
                        lv_obj_clear_flag(drum_dot[row][col], LV_OBJ_FLAG_HIDDEN);
                    } else {
                        lv_obj_add_flag(drum_dot[row][col], LV_OBJ_FLAG_HIDDEN);
                    }
                }
                Serial.printf("[DRUM] cell [%d][%d] -> %d\n", row, col, next);
            }, LV_EVENT_CLICKED, (void*)(uintptr_t)((r << 8) | c));

            drum_cell[r][c] = cell;

            lv_obj_t *dot = lv_obj_create(cell);
            lv_obj_set_size(dot, dsize, dsize);
            lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_color(dot, lv_color_hex(drum_row_colors[r]), 0);
            lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(dot, 0, 0);
            lv_obj_set_style_pad_all(dot, 0, 0);
            lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_center(dot);
            lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
            drum_dot[r][c] = dot;
        }
    }

    struct SepDef { int colAfter; uint32_t color; const char *label; };
    static const SepDef seps[4] = {
        { 0, 0xFFFFFF, "1" }, { 4, 0xCCCCCC, "2" },
        { 8, 0xFFFFFF, "3" }, {12, 0xCCCCCC, "4" },
    };
    for (int i = 0; i < 4; i++) {
        int sx = gridX + seps[i].colAfter * cellW;
        int sy = Y0 - 10;
        int sh = gridH + 20;

        lv_obj_t *line = lv_obj_create(parent);
        lv_obj_set_size(line, 1, sh);
        lv_obj_set_pos(line, sx, sy);
        lv_obj_set_style_bg_color(line, lv_color_hex(seps[i].color), 0);
        lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(line, 0, 0);
        lv_obj_set_style_radius(line, 0, 0);
        lv_obj_set_style_pad_all(line, 0, 0);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *lbl = lv_label_create(parent);
        lv_label_set_text(lbl, seps[i].label);
        lv_obj_set_style_text_color(lbl, lv_color_hex(seps[i].color), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
        lv_obj_align_to(lbl, line, LV_ALIGN_OUT_TOP_MID, 0, -2);
    }

    drum_grid_x = gridX;
    drum_grid_y = Y0;
    drum_grid_w = gridW;
    drum_grid_h = gridH;
    drum_cell_w = cellW;

    drum_cursor = lv_obj_create(parent);
    lv_obj_set_size(drum_cursor, cellW, gridH + 20);
    lv_obj_set_pos(drum_cursor, gridX, Y0 - 10);
    lv_obj_set_style_bg_opa(drum_cursor, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(drum_cursor, 1, 0);
    lv_obj_set_style_border_color(drum_cursor, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_radius(drum_cursor, 0, 0);
    lv_obj_set_style_pad_all(drum_cursor, 0, 0);
    lv_obj_clear_flag(drum_cursor, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(drum_cursor, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(drum_cursor, LV_OBJ_FLAG_HIDDEN);
}

static void drum_grid_update() {
    for (int r = 0; r < DRUM_ROWS; r++) {
        for (int c = 0; c < DRUM_COLS; c++) {
            if (!drum_dot[r][c] || !lv_obj_is_valid(drum_dot[r][c])) continue;
            uint8_t v = drum_pattern_data[r][c];
            if (v > 0) {
                lv_obj_set_style_bg_color(drum_dot[r][c],
                    lv_color_hex(drum_cell_color(r, v)), 0);
                lv_obj_clear_flag(drum_dot[r][c], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(drum_dot[r][c], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

static void drum_step_timer_cb(lv_timer_t *t) {
    (void)t;
    if (!drum_playing)  return;
    if (!drum_cursor)   return;
    drum_step_counter = (drum_step_counter + 1) & 0x0F;
    if (lv_obj_is_valid(drum_cursor)) {
        lv_obj_set_x(drum_cursor, drum_grid_x + drum_step_counter * drum_cell_w);
    }
}

static void drum_play_btn_cb(lv_event_t *e) {
    (void)e;
    drum_playing = !drum_playing;

    if (drum_playing) {
        drum_step_counter = 0;
        if (drum_cursor && lv_obj_is_valid(drum_cursor)) {
            lv_obj_set_x(drum_cursor, drum_grid_x);
            lv_obj_clear_flag(drum_cursor, LV_OBJ_FLAG_HIDDEN);
        }
        if (drum_play_btn && lv_obj_is_valid(drum_play_btn))
            lv_obj_set_style_border_color(drum_play_btn, lv_color_hex(0x888888), 0);
        if (drum_play_lbl && lv_obj_is_valid(drum_play_lbl))
            lv_label_set_text(drum_play_lbl, "Stop");
    } else {
        drum_step_counter = 0;
        if (drum_cursor && lv_obj_is_valid(drum_cursor))
            lv_obj_add_flag(drum_cursor, LV_OBJ_FLAG_HIDDEN);
        if (drum_play_btn && lv_obj_is_valid(drum_play_btn))
            lv_obj_set_style_border_color(drum_play_btn, lv_color_hex(0x00FF00), 0);
        if (drum_play_lbl && lv_obj_is_valid(drum_play_lbl))
            lv_label_set_text(drum_play_lbl, "Play");
    }
}

// Pagina SEQ completa (label PTN + salva + griglia + play + back)
void drum_seq_page_create(lv_obj_t *parent, lv_obj_t *title_lbl) {
    // Label PTN
    drum_pattern_label = lv_label_create(parent);
    lv_label_set_text(drum_pattern_label, "PTN --");
    lv_obj_set_style_text_color(drum_pattern_label, lv_color_hex(0x00FFFF), 0);
    lv_obj_set_style_text_font(drum_pattern_label, &lv_font_montserrat_32, 0);
    if (title_lbl) lv_obj_align_to(drum_pattern_label, title_lbl,
                                   LV_ALIGN_OUT_RIGHT_MID, 30, 0);
    else           lv_obj_set_pos(drum_pattern_label, 130, 3);

    // Bottone Salva
    lv_obj_t *save_btn = lv_btn_create(parent);
    lv_obj_set_size(save_btn, 120, 45);
    lv_obj_set_pos(save_btn, 660, 3);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0xAA0000), 0);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0xDD2222), LV_STATE_PRESSED);
    lv_obj_set_style_radius(save_btn, 6, 0);
    lv_obj_set_style_border_width(save_btn, 2, 0);
    lv_obj_set_style_border_color(save_btn, lv_color_hex(0xFF4444), 0);
    lv_obj_t *save_lbl = lv_label_create(save_btn);
    lv_label_set_text(save_lbl, "Salva");
    lv_obj_set_style_text_color(save_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(save_lbl, &lv_font_montserrat_18, 0);
    lv_obj_center(save_lbl);
    lv_obj_add_event_cb(save_btn, eb, LV_EVENT_CLICKED, (void*)(uintptr_t)31);

    // Griglia
    drum_grid_create(parent);
    drum_grid_update();

    drum_playing      = false;
    drum_step_counter = 0;

    // Play/Stop
    drum_play_btn = lv_btn_create(parent);
    lv_obj_set_size(drum_play_btn, 90, 90);
    lv_obj_set_pos(drum_play_btn, 580, 360);
    lv_obj_set_style_bg_color(drum_play_btn, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(drum_play_btn, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_radius(drum_play_btn, 8, 0);
    lv_obj_set_style_border_width(drum_play_btn, 3, 0);
    lv_obj_set_style_border_color(drum_play_btn, lv_color_hex(0x00FF00), 0);

    drum_play_lbl = lv_label_create(drum_play_btn);
    lv_label_set_text(drum_play_lbl, "Play");
    lv_obj_set_style_text_color(drum_play_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(drum_play_lbl, &lv_font_montserrat_18, 0);
    lv_obj_center(drum_play_lbl);
    lv_obj_add_event_cb(drum_play_btn, drum_play_btn_cb, LV_EVENT_CLICKED, NULL);

    if (!drum_step_timer)
        drum_step_timer = lv_timer_create(drum_step_timer_cb, 400, NULL);

    // Back DRUM
    lv_obj_t *back_btn = lv_btn_create(parent);
    lv_obj_set_size(back_btn, 90, 90);
    lv_obj_set_pos(back_btn, 680, 360);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_radius(back_btn, 8, 0);
    lv_obj_set_style_border_width(back_btn, 3, 0);
    lv_obj_set_style_border_color(back_btn, lv_color_hex(0x9B59B6), 0);
    lv_obj_t *back_img = lv_img_create(back_btn);
    lv_img_set_src(back_img, &home);
    lv_obj_center(back_img);
    lv_obj_add_event_cb(back_btn, eb, LV_EVENT_CLICKED, (void*)(uintptr_t)-3);
}

// ============================================================
// ============ 2) SYNTHB VCF PAGE ============================
// ============================================================
lv_obj_t *sB_vcf_btn         = nullptr;
lv_obj_t *sB_vcf_btn_lbl     = nullptr;
uint8_t   sB_vcf_mode        = 0;   // 0=Filter, 1=Wovel

lv_obj_t *sB_sub_btn         = nullptr;
lv_obj_t *sB_sub_lbl         = nullptr;
uint8_t   sB_filter_submode  = 0;   // 0=UNI, 1=SLV, 2=FRE
uint8_t   sB_wovel_submode   = 0;   // 0=POT, 1=ENV, 2=RND

lv_obj_t *sB_filter_type_btn = nullptr;
lv_obj_t *sB_filter_type_lbl = nullptr;
uint8_t   sB_filter_type     = 0;   // 0=LP, 1=BP

lv_obj_t *sB_vcf_arc[3]      = {nullptr,nullptr,nullptr};
lv_obj_t *sB_vcf_arc_dot[3]  = {nullptr,nullptr,nullptr};
lv_obj_t *sB_vcf_arc_lbl[3]  = {nullptr,nullptr,nullptr};
lv_obj_t *sB_vcf_val_lbl[3]  = {nullptr,nullptr,nullptr};
lv_obj_t *sB_vcf_top_lbl[3]  = {nullptr,nullptr,nullptr};
lv_obj_t *sB_vcf_tick[3][VCF_TICK_COUNT]       = {};
lv_obj_t *sB_vcf_scale_lbl[3][VCF_SCALE_MAX]   = {};
lv_obj_t *sB_vcfb_arc[3]     = {nullptr,nullptr,nullptr};
lv_obj_t *sB_vcfb_lbl[3]     = {nullptr,nullptr,nullptr};
lv_obj_t *sB_vcfb_val_lbl[3] = {nullptr,nullptr,nullptr};
lv_obj_t *sB_vcfb_dot[3]     = {nullptr,nullptr,nullptr};
lv_obj_t *sB_vcfb_tick[3][VCF_TICK_COUNT] = {};
uint8_t   sB_vcfb_target[3]  = {50, 50, 50};
int       sB_vcfb_last[3]    = {0, 0, 0};
bool      sB_vcfb_crossed[3] = {false, false, false};

uint8_t   sB_vcf_cut[3]      = {180, 180, 180};
uint8_t   sB_vcf_target[3]   = {50, 50, 50};
int       sB_vcf_last[3]     = {50, 50, 50};
bool      sB_vcf_crossed[3]  = {false, false, false};

lv_obj_t *sB_wov_env_att_dd  = nullptr;
lv_obj_t *sB_wov_env_sus_dd  = nullptr;
lv_obj_t *sB_wov_env_rel_dd  = nullptr;
lv_obj_t *sB_wov_env_att_lbl = nullptr;
lv_obj_t *sB_wov_env_sus_lbl = nullptr;
lv_obj_t *sB_wov_env_rel_lbl = nullptr;
lv_obj_t          *sB_vcf_env_chart = nullptr;
lv_chart_series_t *sB_vcf_env_serie = nullptr;

#define VCF_MODE_COLOR_FILTER   0x0A5A2A
#define VCF_MODE_COLOR_WOVEL    0xFF8800
#define FILTER_TYPE_LP_COLOR    0x0088FF
#define FILTER_TYPE_BP_COLOR    0xFF2222

#define SUB_FILTER_COLOR        0x44DD44
#define SUB_WOVEL_COLOR         0xFFDD33
#define SUB_BG_DARK             0x1A1A2E
#define SUB_BG_DARK_PRESSED     0x0F3460

#define VCF_DISABLED_LBL_COLOR  0x555555   // grigio scuro

static const char *SUB_LABELS_FILTER[3] = {"UNI","SLV","FRE"};
static const char *SUB_LABELS_WOVEL[3]  = {"POT","ENV","RND"};

// ------------------------------------------------------------
// Prototipi interni
// ------------------------------------------------------------
static void sB_vcf_apply_state();
static void sB_vcf_arc_reset_default(int i);
static void sB_vcf_disable_arc(int i);
static void sB_vcf_hide_arc(int i);
static void sB_vcf_set_top_label(int i, const char *txt);
static void sB_vcf_show_scale_letter(int i, int t, const char *txt);
static void sB_vcf_show_env_dropdowns(bool show);

// ------------------------------------------------------------
// Aggiorna testo+colore del bottone 3-stati (UNI/SLV/FRE / POT/ENV/RND)
// ------------------------------------------------------------
static void sB_update_sub_buttons() {
    if (!sB_sub_btn || !lv_obj_is_valid(sB_sub_btn)) return;
    if (!sB_sub_lbl || !lv_obj_is_valid(sB_sub_lbl)) return;

    bool isFilter = (sB_vcf_mode == 0);
    uint8_t idx = isFilter ? sB_filter_submode : sB_wovel_submode;
    if (idx > 2) idx = 2;

    const char * const *labels = isFilter ? SUB_LABELS_FILTER : SUB_LABELS_WOVEL;
    uint32_t color = isFilter ? SUB_FILTER_COLOR : SUB_WOVEL_COLOR;

    lv_label_set_text(sB_sub_lbl, labels[idx]);
    lv_obj_set_style_text_color(sB_sub_lbl, lv_color_hex(color), 0);
    lv_obj_set_style_border_color(sB_sub_btn, lv_color_hex(color), 0);
}

// ------------------------------------------------------------
// Callback: SUBMODE cicla 0→1→2→0 e riapplica lo stato
// ------------------------------------------------------------
static void sB_sub_btn_cb(lv_event_t *e) {
    (void)e;
    if (sB_vcf_mode == 0) {
        sB_filter_submode = (uint8_t)((sB_filter_submode + 1) % 3);
        uiSetParamB_U8('C', sB_filter_submode);
    } else {
        sB_wovel_submode = (uint8_t)((sB_wovel_submode + 1) % 3);
        uiSetParamB_U8('K', sB_wovel_submode);
    }
    sB_update_sub_buttons();
    sB_vcf_apply_state();
}

// ------------------------------------------------------------
// Callback: FLT↔WOV
// ------------------------------------------------------------
static void vcf_mode_btn_cb(lv_event_t *e) {
    (void)e;
    sB_vcf_mode = (sB_vcf_mode == 0) ? 1 : 0;

    if (sB_vcf_btn && lv_obj_is_valid(sB_vcf_btn)) {
        lv_obj_set_style_border_color(sB_vcf_btn,
            lv_color_hex(sB_vcf_mode == 0 ? VCF_MODE_COLOR_FILTER
                                          : VCF_MODE_COLOR_WOVEL), 0);
    }
    if (sB_vcf_btn_lbl && lv_obj_is_valid(sB_vcf_btn_lbl)) {
        lv_label_set_text(sB_vcf_btn_lbl, sB_vcf_mode == 0 ? "FLT" : "WOV");
    }
    sB_update_sub_buttons();
    sB_vcf_apply_state();

    uiSetParamB_U8('B', sB_vcf_mode);
    if (sB_vcf_mode == 0) uiSetParamB_U8('C', sB_filter_submode);
    else                  uiSetParamB_U8('K', sB_wovel_submode);
}

// ------------------------------------------------------------
// Callback: LP↔BP
// ------------------------------------------------------------
static void sB_filter_type_cb(lv_event_t *e) {
    (void)e;
    sB_filter_type = (sB_filter_type == 0) ? 1 : 0;

    if (sB_filter_type_btn && lv_obj_is_valid(sB_filter_type_btn)) {
        lv_obj_set_style_border_color(sB_filter_type_btn,
            lv_color_hex(sB_filter_type == 0 ? FILTER_TYPE_LP_COLOR
                                             : FILTER_TYPE_BP_COLOR), 0);
    }
    if (sB_filter_type_lbl && lv_obj_is_valid(sB_filter_type_lbl)) {
        lv_label_set_text(sB_filter_type_lbl, sB_filter_type == 0 ? "LP" : "BP");
    }
    uiSetParamB_U8('e', sB_filter_type);
}

// ------------------------------------------------------------
// Callback dei dropdown ENV (ATT/SUS/REL)
// sel 0..4 → mappa 0..255 per SynthB
// ------------------------------------------------------------
static void wov_env_dd_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    int sel = lv_dropdown_get_selected(dd);
    uint8_t val = (uint8_t)map(sel, 0, 4, 0, 255);

    char key = 0;
    if      (dd == sB_wov_env_att_dd) key = 'V';   // wov_vowel_A
    else if (dd == sB_wov_env_sus_dd) key = 'Z';   // wov_vowel_B
    else if (dd == sB_wov_env_rel_dd) key = 'L';   // wov_env_vowel_C
    if (key) uiSetParamB_U8(key, val);
}

// ------------------------------------------------------------
// Arc VCF: drag → label valore + crossing pallino + invio
// ------------------------------------------------------------
static void sB_vcf_arc_cb(lv_event_t *e) {
    lv_obj_t *arc = lv_event_get_target(e);
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx > 2) return;

    int cur    = lv_arc_get_value(arc);
    int prev   = sB_vcf_last[idx];
    int target = sB_vcf_target[idx];

    if (sB_vcf_val_lbl[idx] && lv_obj_is_valid(sB_vcf_val_lbl[idx])) {
        lv_label_set_text_fmt(sB_vcf_val_lbl[idx], "%d", cur);
    }

    if (!sB_vcf_crossed[idx] &&
        sB_vcf_arc_dot[idx] && lv_obj_is_valid(sB_vcf_arc_dot[idx])) {

        bool crossed = (prev < target && cur >= target) ||
                       (prev > target && cur <= target) ||
                       (prev == target && cur != target);

        if (crossed) {
            sB_vcf_crossed[idx] = true;
            lv_obj_add_flag(sB_vcf_arc_dot[idx], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_arc_color(arc,
                lv_color_hex(COLOR_ARC_PASSED), LV_PART_INDICATOR);
        } else {
            lv_obj_clear_flag(sB_vcf_arc_dot[idx], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_arc_color(arc,
                lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
        }
    }
    sB_vcf_last[idx] = cur;

    sB_vcf_cut[idx] = (uint8_t)map(cur, 0, 100, 0, 255);
    char key = '1' + (char)idx;
    uiSetParamB_U8(key, sB_vcf_cut[idx]);
}

// ------------------------------------------------------------
// Crea un arc VCF: arc + label centrale + label valore +
// pallino target + 5 tick + label top + 6 lettere scala (nascoste)
// ------------------------------------------------------------
static void sB_vcf_arc_create(lv_obj_t *parent, int idx,
                              int x, int y, int w, int h,
                              const char *pname) {
    if (idx < 0 || idx > 2) return;

    int initVal = map(sB_vcf_cut[idx], 0, 255, 0, 100);
    int cx = x + w / 2;
    int cy = y + h / 2;

    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, w, h);
    lv_obj_set_pos(arc, x, y);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, initVal);

    lv_obj_set_style_arc_img_src(arc, &img_arc_bg,    LV_PART_MAIN);
    lv_obj_set_style_arc_img_src(arc, &img_arc_indic, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color  (arc, lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa     (arc, LV_OPA_TRANSP,  LV_PART_KNOB);
    sB_vcf_arc[idx] = arc;

    // Label centrale "F1/2/3"
    lv_obj_t *plabel = lv_label_create(parent);
    lv_label_set_text(plabel, pname);
    lv_obj_set_style_text_color(plabel, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_style_text_font (plabel, &lv_font_montserrat_20, 0);
    lv_obj_align_to(plabel, arc, LV_ALIGN_CENTER, 0, 0);
    sB_vcf_arc_lbl[idx] = plabel;

       // ---- Label valore sotto l'arc (10px più in alto) ----
    lv_obj_t *val_lbl = lv_label_create(parent);
    lv_obj_set_style_text_color(val_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font (val_lbl, &lv_font_montserrat_20, 0);
    lv_label_set_text_fmt(val_lbl, "%d", initVal);
    lv_obj_align_to(val_lbl, arc, LV_ALIGN_OUT_BOTTOM_MID, 0, -8);  // era +2
    sB_vcf_val_lbl[idx] = val_lbl;

    // Pallino rosso al valore di TARGET
    float angle = 135.0f + (sB_vcf_target[idx] * 2.7f);
    if (angle >= 360.0f) angle -= 360.0f;
    if (angle <  0.0f)   angle += 360.0f;
    int radius = (w / 2) - 2 + 10;
    float rad  = angle * PI / 180.0f;
    int dx = cx + (int)(radius * cosf(rad)) - 4;
    int dy = cy + (int)(radius * sinf(rad)) - 4;

    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_pos(dot, dx, dy);
    sB_vcf_arc_dot[idx] = dot;

        // ---- Top label: subito sopra il tick radiale superiore (t=2, 270°) ----
    lv_obj_t *top = lv_label_create(parent);
    lv_label_set_text(top, "");
    lv_obj_set_style_text_color(top, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(top, &lv_font_montserrat_16, 0);
    lv_obj_align_to(top, arc, LV_ALIGN_OUT_TOP_MID, 0, -17);   // tick tip a -15 → 2px sopra
    lv_obj_add_flag(top, LV_OBJ_FLAG_HIDDEN);
    sB_vcf_top_lbl[idx] = top;

    // ---- 5 tick radiali grigi ----
    const int r_out = w / 2;
    for (int t = 0; t < VCF_TICK_COUNT; t++) {
        float deg = 135.0f + t * 67.5f;
        if (deg >= 360.0f) deg -= 360.0f;
        float trad = deg * PI / 180.0f;
        float cosv = cosf(trad);
        float sinv = sinf(trad);
        int px = cx + (int)(r_out * cosv);
        int py = cy + (int)(r_out * sinv);

        lv_obj_t *tick = lv_obj_create(parent);
        lv_obj_set_size(tick, 1, 15);
        lv_obj_set_style_bg_color(tick, lv_color_hex(0x888888), 0);
        lv_obj_set_style_bg_opa(tick, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(tick, 0, 0);
        lv_obj_set_style_radius(tick, 0, 0);
        lv_obj_set_style_pad_all(tick, 0, 0);
        lv_obj_clear_flag(tick, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(tick, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_set_style_transform_pivot_x(tick, 0, 0);
        lv_obj_set_style_transform_pivot_y(tick, 15, 0);
        float alpha = 90.0f + deg;
        while (alpha >= 360.0f) alpha -= 360.0f;
        while (alpha <    0.0f) alpha += 360.0f;
        lv_obj_set_style_transform_angle(tick, (int16_t)(alpha * 10.0f), 0);
        lv_obj_set_pos(tick, px, py - 15);
        sB_vcf_tick[idx][t] = tick;
    }

    // ---- 6 lettere scala attorno all'arc (WOV/POT) ----
    // Angoli: 135, 189, 243, 297, 351, 45 (spaziatura 54° = 270/5)
    const int r_scale = r_out + 14;
    for (int t = 0; t < VCF_SCALE_MAX; t++) {
        float deg = 135.0f + t * 54.0f;
        if (deg >= 360.0f) deg -= 360.0f;
        float trad = deg * PI / 180.0f;
        int sx = cx + (int)(r_scale * cosf(trad));
        int sy = cy + (int)(r_scale * sinf(trad));

        lv_obj_t *lbl = lv_label_create(parent);
        lv_label_set_text(lbl, "");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_set_pos(lbl, sx - 5, sy - 9);
        lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
        sB_vcf_scale_lbl[idx][t] = lbl;
    }

    // Stato iniziale pallino
    sB_vcf_crossed[idx] = false;
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
    sB_vcf_last[idx] = initVal;

    lv_obj_add_event_cb(arc, sB_vcf_arc_cb,
                        LV_EVENT_VALUE_CHANGED, (void*)(uintptr_t)idx);
}

// ------------------------------------------------------------
// Reset di un arc al suo stato di default (visibile, attivo)
// ------------------------------------------------------------
static void sB_vcf_arc_reset_default(int i) {
    if (i < 0 || i > 2) return;
    lv_obj_t *a = sB_vcf_arc[i];
    if (!a || !lv_obj_is_valid(a)) return;

    lv_obj_clear_flag(a, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(a, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_opa(a, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(a,
        lv_color_hex(sB_vcf_crossed[i] ? COLOR_ARC_PASSED : COLOR_ARC_ACTIVE),
        LV_PART_INDICATOR);

    if (sB_vcf_arc_lbl[i] && lv_obj_is_valid(sB_vcf_arc_lbl[i])) {
        lv_obj_clear_flag(sB_vcf_arc_lbl[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(sB_vcf_arc_lbl[i], lv_color_hex(0xFFAA00), 0);
    }
    if (sB_vcf_val_lbl[i] && lv_obj_is_valid(sB_vcf_val_lbl[i])) {
        lv_obj_clear_flag(sB_vcf_val_lbl[i], LV_OBJ_FLAG_HIDDEN);
    }
    if (sB_vcf_arc_dot[i] && lv_obj_is_valid(sB_vcf_arc_dot[i])) {
        if (!sB_vcf_crossed[i])
            lv_obj_clear_flag(sB_vcf_arc_dot[i], LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(sB_vcf_arc_dot[i], LV_OBJ_FLAG_HIDDEN);
    }
    for (int t = 0; t < VCF_TICK_COUNT; t++) {
        if (sB_vcf_tick[i][t] && lv_obj_is_valid(sB_vcf_tick[i][t]))
            lv_obj_clear_flag(sB_vcf_tick[i][t], LV_OBJ_FLAG_HIDDEN);
    }
    if (sB_vcf_top_lbl[i] && lv_obj_is_valid(sB_vcf_top_lbl[i]))
        lv_obj_add_flag(sB_vcf_top_lbl[i], LV_OBJ_FLAG_HIDDEN);
    for (int t = 0; t < VCF_SCALE_MAX; t++) {
        if (sB_vcf_scale_lbl[i][t] && lv_obj_is_valid(sB_vcf_scale_lbl[i][t]))
            lv_obj_add_flag(sB_vcf_scale_lbl[i][t], LV_OBJ_FLAG_HIDDEN);
    }
}

// ------------------------------------------------------------
// Arc disabilitato: niente touch, niente indicator, label grigia
// ------------------------------------------------------------
static void sB_vcf_disable_arc(int i) {
    if (i < 0 || i > 2) return;
    lv_obj_t *a = sB_vcf_arc[i];
    if (!a || !lv_obj_is_valid(a)) return;

    lv_obj_set_style_arc_opa(a, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_clear_flag(a, LV_OBJ_FLAG_CLICKABLE);

    if (sB_vcf_arc_lbl[i] && lv_obj_is_valid(sB_vcf_arc_lbl[i]))
        lv_obj_set_style_text_color(sB_vcf_arc_lbl[i],
                                    lv_color_hex(VCF_DISABLED_LBL_COLOR), 0);

    if (sB_vcf_arc_dot[i] && lv_obj_is_valid(sB_vcf_arc_dot[i]))
        lv_obj_add_flag(sB_vcf_arc_dot[i], LV_OBJ_FLAG_HIDDEN);

    // Value label sparisce con l'indicatore
    if (sB_vcf_val_lbl[i] && lv_obj_is_valid(sB_vcf_val_lbl[i]))
        lv_obj_add_flag(sB_vcf_val_lbl[i], LV_OBJ_FLAG_HIDDEN);
}
// ------------------------------------------------------------
// Arc interamente nascosto (ENV: F1/F2 spariscono con label e linee)
// ------------------------------------------------------------
static void sB_vcf_hide_arc(int i) {
    if (i < 0 || i > 2) return;
    lv_obj_t *objs[] = {
        sB_vcf_arc[i], sB_vcf_arc_lbl[i], sB_vcf_val_lbl[i],
        sB_vcf_arc_dot[i], sB_vcf_top_lbl[i]
    };
    for (size_t k = 0; k < sizeof(objs)/sizeof(objs[0]); k++) {
        if (objs[k] && lv_obj_is_valid(objs[k]))
            lv_obj_add_flag(objs[k], LV_OBJ_FLAG_HIDDEN);
    }
    for (int t = 0; t < VCF_TICK_COUNT; t++)
        if (sB_vcf_tick[i][t] && lv_obj_is_valid(sB_vcf_tick[i][t]))
            lv_obj_add_flag(sB_vcf_tick[i][t], LV_OBJ_FLAG_HIDDEN);
    for (int t = 0; t < VCF_SCALE_MAX; t++)
        if (sB_vcf_scale_lbl[i][t] && lv_obj_is_valid(sB_vcf_scale_lbl[i][t]))
            lv_obj_add_flag(sB_vcf_scale_lbl[i][t], LV_OBJ_FLAG_HIDDEN);
}

// ------------------------------------------------------------
// Top label: testo bianco sopra il tick superiore. NULL = nascosta
// ------------------------------------------------------------
static void sB_vcf_set_top_label(int i, const char *txt) {
    if (i < 0 || i > 2) return;
    if (!sB_vcf_top_lbl[i] || !lv_obj_is_valid(sB_vcf_top_lbl[i])) return;
    if (txt == nullptr) {
        lv_obj_add_flag(sB_vcf_top_lbl[i], LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_label_set_text(sB_vcf_top_lbl[i], txt);
    lv_obj_set_style_text_color(sB_vcf_top_lbl[i], lv_color_hex(0xFFFFFF), 0);
    lv_obj_clear_flag(sB_vcf_top_lbl[i], LV_OBJ_FLAG_HIDDEN);
}

static void sB_vcf_show_scale_letter(int i, int t, const char *txt) {
    if (i < 0 || i > 2 || t < 0 || t >= VCF_SCALE_MAX) return;
    lv_obj_t *lbl = sB_vcf_scale_lbl[i][t];
    if (!lbl || !lv_obj_is_valid(lbl)) return;
    lv_label_set_text(lbl, txt);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_clear_flag(lbl, LV_OBJ_FLAG_HIDDEN);
}

static void sB_vcf_show_env_dropdowns(bool show) {
    lv_obj_t *dds[3]  = {sB_wov_env_att_dd,  sB_wov_env_sus_dd,  sB_wov_env_rel_dd};
    lv_obj_t *lbls[3] = {sB_wov_env_att_lbl, sB_wov_env_sus_lbl, sB_wov_env_rel_lbl};
    for (int i = 0; i < 3; i++) {
        if (dds[i] && lv_obj_is_valid(dds[i])) {
            if (show) lv_obj_clear_flag(dds[i], LV_OBJ_FLAG_HIDDEN);
            else      lv_obj_add_flag  (dds[i], LV_OBJ_FLAG_HIDDEN);
        }
        if (lbls[i] && lv_obj_is_valid(lbls[i])) {
            if (show) lv_obj_clear_flag(lbls[i], LV_OBJ_FLAG_HIDDEN);
            else      lv_obj_add_flag  (lbls[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}
// ------------------------------------------------------------
// Reset di un arc vcfb al suo stato di default (visibile, attivo)
// ------------------------------------------------------------
static void sB_vcfb_reset_default(int i) {
    if (i < 0 || i > 2) return;
    lv_obj_t *a = sB_vcfb_arc[i];
    if (!a || !lv_obj_is_valid(a)) return;

    lv_obj_clear_flag(a, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(a, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_opa(a, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(a,
        lv_color_hex(sB_vcfb_crossed[i] ? COLOR_ARC_PASSED : COLOR_ARC_ACTIVE),
        LV_PART_INDICATOR);

    if (sB_vcfb_lbl[i] && lv_obj_is_valid(sB_vcfb_lbl[i])) {
        lv_obj_clear_flag(sB_vcfb_lbl[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(sB_vcfb_lbl[i], lv_color_hex(0xFFAA00), 0);
    }
    if (sB_vcfb_val_lbl[i] && lv_obj_is_valid(sB_vcfb_val_lbl[i]))
        lv_obj_clear_flag(sB_vcfb_val_lbl[i], LV_OBJ_FLAG_HIDDEN);
    if (sB_vcfb_dot[i] && lv_obj_is_valid(sB_vcfb_dot[i])) {
        if (!sB_vcfb_crossed[i])
            lv_obj_clear_flag(sB_vcfb_dot[i], LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(sB_vcfb_dot[i], LV_OBJ_FLAG_HIDDEN);
    }
    for (int t = 0; t < VCF_TICK_COUNT; t++) {
        if (sB_vcfb_tick[i][t] && lv_obj_is_valid(sB_vcfb_tick[i][t]))
            lv_obj_clear_flag(sB_vcfb_tick[i][t], LV_OBJ_FLAG_HIDDEN);
    }
}

// ------------------------------------------------------------
// Arc vcfb disabilitato: niente touch, niente indicator,
// label grigio scuro, pallino e value label nascosti
// ------------------------------------------------------------
static void sB_vcfb_disable_arc(int i) {
    if (i < 0 || i > 2) return;
    lv_obj_t *a = sB_vcfb_arc[i];
    if (!a || !lv_obj_is_valid(a)) return;

    lv_obj_set_style_arc_opa(a, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_clear_flag(a, LV_OBJ_FLAG_CLICKABLE);

    if (sB_vcfb_lbl[i] && lv_obj_is_valid(sB_vcfb_lbl[i]))
        lv_obj_set_style_text_color(sB_vcfb_lbl[i],
                                    lv_color_hex(VCF_DISABLED_LBL_COLOR), 0);

    if (sB_vcfb_dot[i] && lv_obj_is_valid(sB_vcfb_dot[i]))
        lv_obj_add_flag(sB_vcfb_dot[i], LV_OBJ_FLAG_HIDDEN);

    if (sB_vcfb_val_lbl[i] && lv_obj_is_valid(sB_vcfb_val_lbl[i]))
        lv_obj_add_flag(sB_vcfb_val_lbl[i], LV_OBJ_FLAG_HIDDEN);
}
// ------------------------------------------------------------
// STATO GLOBALE: applica le 6 combinazioni
//   FLT/UNI : F1=CUT, F2=DET, F3 disabilitato
//   FLT/SLV : F1=CUT, F2=INT, F3=INT
//   FLT/FRE : F1=CUT, F2=CUT, F3=CUT
//   WOV/POT : F1 con lettere A E I O U A, F2=FORM, F3 disabilitato
//   WOV/ENV : F1,F2 nascosti + 3 dropdown, F3=TIME
//   WOV/RND : F1,F2 disabilitati (grigi), F3=TIME
// ------------------------------------------------------------
static void sB_vcf_apply_state() {
    bool isFilter = (sB_vcf_mode == 0);
    uint8_t sub = isFilter ? sB_filter_submode : sB_wovel_submode;

    // 1) Reset di tutti gli arc allo stato di default
    for (int i = 0; i < 3; i++) sB_vcf_arc_reset_default(i);
    for (int i = 0; i < 3; i++) sB_vcfb_reset_default(i);

    // 2) Nascondi i dropdown ENV (default)
    sB_vcf_show_env_dropdowns(false);

    // 3) Override per modo VCF (F1/F2/F3)
    if (isFilter) {
        const char *top[3] = {nullptr, nullptr, nullptr};
        if (sub == 0) {          // UNI
            top[0] = "CUT"; top[1] = "DET";
            sB_vcf_disable_arc(2);
        } else if (sub == 1) {   // SLV
            top[0] = "CUT"; top[1] = "INT"; top[2] = "INT";
        } else {                 // FRE
            top[0] = "CUT"; top[1] = "CUT"; top[2] = "CUT";
        }
        for (int i = 0; i < 3; i++) sB_vcf_set_top_label(i, top[i]);
    } else {
        if (sub == 0) {          // POT
            const char *letters[6] = {"A","E","I","O","U","A"};
            for (int t = 0; t < 6; t++) sB_vcf_show_scale_letter(0, t, letters[t]);
            sB_vcf_set_top_label(1, "FORM");
            sB_vcf_disable_arc(2);
        } else if (sub == 1) {   // ENV
            sB_vcf_hide_arc(0);
            sB_vcf_hide_arc(1);
            sB_vcf_show_env_dropdowns(true);
            sB_vcf_set_top_label(2, "TIME");
        } else {                 // RND
            sB_vcf_disable_arc(0);
            sB_vcf_disable_arc(1);
            sB_vcf_set_top_label(2, "TIME");
        }
    }

    // 4) Override per i vcfb: in WOV, EnvA (idx 1) è disabilitato
    if (!isFilter) {
        sB_vcfb_disable_arc(1);
    }
}
// ------------------------------------------------------------
// Crea i 3 dropdown ENV (nascosti di default)
// Regione F1+F2: x = arc_x[0] .. arc_x[1]+80  = 35..250 (215px)
// ------------------------------------------------------------
static void sB_vcf_env_dropdowns_create(lv_obj_t *parent,
                                        const int arc_x[3], int arc_y) {
    const int dd_w   = 60;
    const int dd_h   = 45;
    const int dd_gap = 10;
    const int dd_y   = arc_y + 20;
    const int start_x = arc_x[0] + 7;   // ≈ 42

    const char *lbl_txt[3] = {"ATT", "SUS", "REL"};
    lv_obj_t **dd_ptrs[3]  = {&sB_wov_env_att_dd,  &sB_wov_env_sus_dd,  &sB_wov_env_rel_dd};
    lv_obj_t **lbl_ptrs[3] = {&sB_wov_env_att_lbl, &sB_wov_env_sus_lbl, &sB_wov_env_rel_lbl};

    for (int i = 0; i < 3; i++) {
        int x = start_x + i * (dd_w + dd_gap);

        lv_obj_t *lbl = lv_label_create(parent);
        lv_label_set_text(lbl, lbl_txt[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_set_pos(lbl, x + dd_w/2 - 15, dd_y - 22);
        lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
        *lbl_ptrs[i] = lbl;

        lv_obj_t *dd = lv_dropdown_create(parent);
        lv_obj_set_size(dd, dd_w, dd_h);
        lv_obj_set_pos(dd, x, dd_y);
        lv_obj_set_style_bg_color(dd, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_border_width(dd, 2, 0);
        lv_obj_set_style_border_color(dd, lv_color_hex(0x666666), 0);
        lv_obj_set_style_text_color(dd, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(dd, &lv_font_montserrat_16, 0);
        lv_obj_set_style_pad_left(dd, 4, 0);
        lv_obj_set_style_pad_right(dd, 2, 0);
        lv_dropdown_set_options(dd, "A\nE\nI\nO\nU");
        lv_dropdown_set_selected(dd, 0);
        lv_obj_add_flag(dd, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_event_cb(dd, wov_env_dd_cb, LV_EVENT_VALUE_CHANGED, NULL);
        *dd_ptrs[i] = dd;
    }
}

// ------------------------------------------------------------
// Plotter ADSR VCF-B: sopra il frame ENV vir, 2px di gap.
// Frame: (470,160) 280x290  → top = 160.
// Plotter: (470,58) 280x100 → bottom = 158 (2px gap).
// Linee blu 0x0088FF, senza label.
// ------------------------------------------------------------
static void sB_vcf_env_plot_create(lv_obj_t *parent) {
    const int PX = 470, PY = 58, PW = 280, PH = 100;

    sB_vcf_env_chart = lv_chart_create(parent);
    lv_obj_set_size(sB_vcf_env_chart, PW, PH);
    lv_obj_set_pos(sB_vcf_env_chart, PX, PY);
    lv_chart_set_type(sB_vcf_env_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(sB_vcf_env_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_point_count(sB_vcf_env_chart, 60);
    lv_obj_set_style_bg_color(sB_vcf_env_chart, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(sB_vcf_env_chart, 1, 0);
    lv_obj_set_style_border_color(sB_vcf_env_chart, lv_color_hex(0x666666), 0);
    lv_obj_set_style_radius(sB_vcf_env_chart, 4, 0);
    lv_chart_set_div_line_count(sB_vcf_env_chart, 0, 0);
    lv_obj_clear_flag(sB_vcf_env_chart, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(sB_vcf_env_chart, LV_OBJ_FLAG_CLICKABLE);

    sB_vcf_env_serie = lv_chart_add_series(sB_vcf_env_chart,
                                           lv_color_hex(0x0088FF),
                                           LV_CHART_AXIS_PRIMARY_Y);

    sB_vcf_env_plot_update();
}

// Chiamata da eslider (idx 0..3) e dopo la create
void sB_vcf_env_plot_update() {
    if (!sB_vcf_env_chart || !lv_obj_is_valid(sB_vcf_env_chart)) return;
    if (!sB_vcf_env_serie) return;

    int cA = (slider_objs[0] && lv_obj_is_valid(slider_objs[0]))
             ? lv_slider_get_value(slider_objs[0]) : 0;
    int cD = (slider_objs[1] && lv_obj_is_valid(slider_objs[1]))
             ? lv_slider_get_value(slider_objs[1]) : 0;
    int cS = (slider_objs[2] && lv_obj_is_valid(slider_objs[2]))
             ? lv_slider_get_value(slider_objs[2]) : 0;
    int cR = (slider_objs[3] && lv_obj_is_valid(slider_objs[3]))
             ? lv_slider_get_value(slider_objs[3]) : 0;

    env_plot_draw(sB_vcf_env_chart, sB_vcf_env_serie, cA, cD, cS, cR);
}
// ------------------------------------------------------------
// Callback arc addizionali (RES, EnvA, EnvV):
//   aggiorna label valore + gestisce crossing del pallino rosso
//   (stessa logica di F1/F2/F3).
// ------------------------------------------------------------
static void sB_vcfb_arc_cb(lv_event_t *e) {
    lv_obj_t *arc = lv_event_get_target(e);
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx > 2) return;

    int cur    = lv_arc_get_value(arc);
    int prev   = sB_vcfb_last[idx];
    int target = sB_vcfb_target[idx];

    if (sB_vcfb_val_lbl[idx] && lv_obj_is_valid(sB_vcfb_val_lbl[idx])) {
        lv_label_set_text_fmt(sB_vcfb_val_lbl[idx], "%d", cur);
    }

    if (!sB_vcfb_crossed[idx] &&
        sB_vcfb_dot[idx] && lv_obj_is_valid(sB_vcfb_dot[idx])) {

        bool crossed = (prev < target && cur >= target) ||
                       (prev > target && cur <= target) ||
                       (prev == target && cur != target);

        if (crossed) {
            sB_vcfb_crossed[idx] = true;
            lv_obj_add_flag(sB_vcfb_dot[idx], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_arc_color(arc,
                lv_color_hex(COLOR_ARC_PASSED), LV_PART_INDICATOR);
        } else {
            lv_obj_clear_flag(sB_vcfb_dot[idx], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_arc_color(arc,
                lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
        }
    }
    sB_vcfb_last[idx] = cur;

    // TODO: mappatura LWS per RES/EnvA/EnvV
}
// ------------------------------------------------------------
// Crea un arc addizionale (RES/EnvA/EnvV):
//   arc + label centrale + label valore sotto + 5 tick radiali.
// Nessun pallino/target/top-label.
// ------------------------------------------------------------

// ------------------------------------------------------------
// Crea un arc addizionale (RES/EnvA/EnvV):
//   arc + label centrale + label valore sotto + pallino target
//   + 5 tick radiali.
// ------------------------------------------------------------
static void sB_vcfb_arc_create(lv_obj_t *parent, int idx,
                               int x, int y, int w, int h,
                               const char *pname) {
    if (idx < 0 || idx > 2) return;

    int cx = x + w / 2;
    int cy = y + h / 2;
    const int initVal = 0;

    // ---- Arc ----
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, w, h);
    lv_obj_set_pos(arc, x, y);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, initVal);

    lv_obj_set_style_arc_img_src(arc, &img_arc_bg,    LV_PART_MAIN);
    lv_obj_set_style_arc_img_src(arc, &img_arc_indic, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color  (arc, lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa     (arc, LV_OPA_TRANSP,  LV_PART_KNOB);
    sB_vcfb_arc[idx] = arc;

    // ---- Label centrale ----
    lv_obj_t *plabel = lv_label_create(parent);
    lv_label_set_text(plabel, pname);
    lv_obj_set_style_text_color(plabel, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_style_text_font (plabel, &lv_font_montserrat_20, 0);
    lv_obj_align_to(plabel, arc, LV_ALIGN_CENTER, 0, 0);
    sB_vcfb_lbl[idx] = plabel;

    // ---- Label valore sotto l'arc ----
    lv_obj_t *val_lbl = lv_label_create(parent);
    lv_obj_set_style_text_color(val_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font (val_lbl, &lv_font_montserrat_20, 0);
    lv_label_set_text_fmt(val_lbl, "%d", initVal);
    lv_obj_align_to(val_lbl, arc, LV_ALIGN_OUT_BOTTOM_MID, 0, -8);
    sB_vcfb_val_lbl[idx] = val_lbl;

    // ---- Pallino rosso al valore di TARGET ----
    float angle = 135.0f + (sB_vcfb_target[idx] * 2.7f);
    if (angle >= 360.0f) angle -= 360.0f;
    if (angle <  0.0f)   angle += 360.0f;
    int radius = (w / 2) - 2 + 10;
    float rad  = angle * PI / 180.0f;
    int dx = cx + (int)(radius * cosf(rad)) - 4;
    int dy = cy + (int)(radius * sinf(rad)) - 4;

    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_pos(dot, dx, dy);
    sB_vcfb_dot[idx] = dot;

    sB_vcfb_crossed[idx] = false;
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_HIDDEN);

    // ---- 5 tick radiali grigi ----
    const int r_out = w / 2;
    for (int t = 0; t < VCF_TICK_COUNT; t++) {
        float deg = 135.0f + t * 67.5f;
        if (deg >= 360.0f) deg -= 360.0f;
        float trad = deg * PI / 180.0f;
        float cosv = cosf(trad);
        float sinv = sinf(trad);
        int px = cx + (int)(r_out * cosv);
        int py = cy + (int)(r_out * sinv);

        lv_obj_t *tick = lv_obj_create(parent);
        lv_obj_set_size(tick, 1, 15);
        lv_obj_set_style_bg_color(tick, lv_color_hex(0x888888), 0);
        lv_obj_set_style_bg_opa(tick, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(tick, 0, 0);
        lv_obj_set_style_radius(tick, 0, 0);
        lv_obj_set_style_pad_all(tick, 0, 0);
        lv_obj_clear_flag(tick, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(tick, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_set_style_transform_pivot_x(tick, 0, 0);
        lv_obj_set_style_transform_pivot_y(tick, 15, 0);
        float alpha = 90.0f + deg;
        while (alpha >= 360.0f) alpha -= 360.0f;
        while (alpha <    0.0f) alpha += 360.0f;
        lv_obj_set_style_transform_angle(tick, (int16_t)(alpha * 10.0f), 0);
        lv_obj_set_pos(tick, px, py - 15);
        sB_vcfb_tick[idx][t] = tick;
    }

    sB_vcfb_last[idx] = initVal;

    lv_obj_add_event_cb(arc, sB_vcfb_arc_cb,
                        LV_EVENT_VALUE_CHANGED, (void*)(uintptr_t)idx);
}

// ------------------------------------------------------------
// LAYOUT pagina VCF SynthB:
//
//   Row 1 (y=58):   [F1]   [F2]   [F3]      [plotter ADSR]
//                   35     170    305         (470,58) 280x100
//
//   Row 2 (y=160):  (frame ENV vir 280x290 con A D S R plain)
//
//   Bottom (y=360): [Home] [LP/BP] [FLT/WOV] [SUBMODE]
//                    10     110      205       300
// ------------------------------------------------------------
void vcf_page_synthb_create(lv_obj_t *parent) {
    const int ARC_Y  = 80;     // era 58 → +15px più in basso
    const int ARC_Y2 = 216;    // metà dello spazio tra F1/F2/F3 e i bottoni
    const int BTN_W  = 90;
    const int BTN_H  = 90;
    const int BTN_Y  = 360;
    const int BTN1_X = 110;
    const int BTN2_X = 205;
    const int BTN3_X = 300;

    // ---- 1) LP/BP ----
    sB_filter_type_btn = lv_btn_create(parent);
    lv_obj_set_size(sB_filter_type_btn, BTN_W, BTN_H);
    lv_obj_set_pos(sB_filter_type_btn, BTN1_X, BTN_Y);
    lv_obj_set_style_bg_color(sB_filter_type_btn, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(sB_filter_type_btn, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_radius(sB_filter_type_btn, 8, 0);
    lv_obj_set_style_border_width(sB_filter_type_btn, 3, 0);
    lv_obj_set_style_border_color(sB_filter_type_btn,
        lv_color_hex(sB_filter_type == 0 ? FILTER_TYPE_LP_COLOR
                                         : FILTER_TYPE_BP_COLOR), 0);
    sB_filter_type_lbl = lv_label_create(sB_filter_type_btn);
    lv_label_set_text(sB_filter_type_lbl, sB_filter_type == 0 ? "LP" : "BP");
    lv_obj_set_style_text_color(sB_filter_type_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(sB_filter_type_lbl, &lv_font_montserrat_24, 0);
    lv_obj_center(sB_filter_type_lbl);
    lv_obj_add_event_cb(sB_filter_type_btn, sB_filter_type_cb, LV_EVENT_CLICKED, NULL);

    // ---- 2) FLT/WOV ----
    sB_vcf_btn = lv_btn_create(parent);
    lv_obj_set_size(sB_vcf_btn, BTN_W, BTN_H);
    lv_obj_set_pos(sB_vcf_btn, BTN2_X, BTN_Y);
    lv_obj_set_style_bg_color(sB_vcf_btn, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(sB_vcf_btn, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_radius(sB_vcf_btn, 8, 0);
    lv_obj_set_style_border_width(sB_vcf_btn, 3, 0);
    lv_obj_set_style_border_color(sB_vcf_btn,
        lv_color_hex(sB_vcf_mode == 0 ? VCF_MODE_COLOR_FILTER
                                      : VCF_MODE_COLOR_WOVEL), 0);
    sB_vcf_btn_lbl = lv_label_create(sB_vcf_btn);
    lv_label_set_text(sB_vcf_btn_lbl, sB_vcf_mode == 0 ? "FLT" : "WOV");
    lv_obj_set_style_text_color(sB_vcf_btn_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(sB_vcf_btn_lbl, &lv_font_montserrat_24, 0);
    lv_obj_center(sB_vcf_btn_lbl);
    lv_obj_add_event_cb(sB_vcf_btn, vcf_mode_btn_cb, LV_EVENT_CLICKED, NULL);

    // ---- 3) SUBMODE 3-stati ----
    sB_sub_btn = lv_btn_create(parent);
    lv_obj_set_size(sB_sub_btn, BTN_W, BTN_H);
    lv_obj_set_pos(sB_sub_btn, BTN3_X, BTN_Y);
    lv_obj_set_style_bg_color(sB_sub_btn, lv_color_hex(SUB_BG_DARK), 0);
    lv_obj_set_style_bg_color(sB_sub_btn, lv_color_hex(SUB_BG_DARK_PRESSED), LV_STATE_PRESSED);
    lv_obj_set_style_radius(sB_sub_btn, 8, 0);
    lv_obj_set_style_border_width(sB_sub_btn, 3, 0);
    sB_sub_lbl = lv_label_create(sB_sub_btn);
    lv_label_set_text(sB_sub_lbl, "---");
    lv_obj_set_style_text_color(sB_sub_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(sB_sub_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(sB_sub_lbl);
    lv_obj_add_event_cb(sB_sub_btn, sB_sub_btn_cb, LV_EVENT_CLICKED, NULL);
    sB_update_sub_buttons();

    // ---- 4) 3 arc F1/F2/F3 (y=73) ----
    static const char *vcf_names[3] = {"F1", "F2", "F3"};
    const int arc_x[3] = { 35, 170, 305 };
    for (int i = 0; i < 3; i++) {
        sB_vcf_arc_create(parent, i, arc_x[i], ARC_Y, 80, 80, vcf_names[i]);
    }

    // ---- 5) 3 arc addizionali: RES / EnvA / EnvV (y=216) ----
    static const char *vcfb_names[3] = {"RES", "EnvA", "EnvV"};
    for (int i = 0; i < 3; i++) {
        sB_vcfb_arc_create(parent, i, arc_x[i], ARC_Y2, 80, 80, vcfb_names[i]);
    }

    // ---- 6) Dropdown ENV (nascosti, sopra F1+F2) ----
    sB_vcf_env_dropdowns_create(parent, arc_x, ARC_Y);

    // ---- 7) Stato iniziale ----
    sB_vcf_apply_state();

    // ---- 8) Plotter ADSR sopra il frame ENV vir ----
    sB_vcf_env_plot_create(parent);
}
// ============ 3) FX / FV-1 ==================================
// ============================================================
lv_obj_t *pot_size_FV1 = nullptr;
lv_obj_t *pot_LF_FV1   = nullptr;
lv_obj_t *pot_HF_FV1   = nullptr;
lv_obj_t *fx_rev_btn   = nullptr;
lv_obj_t *fx_rev_lbl   = nullptr;
lv_obj_t *fx_preset_dd = nullptr;
lv_obj_t *fx_save_btn  = nullptr;

static void fx_rev_btn_cb(lv_event_t *e) {
    (void)e;
    if (!fx_rev_lbl || !lv_obj_is_valid(fx_rev_lbl)) return;
    const char *cur = lv_label_get_text(fx_rev_lbl);
    if (cur && strcmp(cur, "Rev1") == 0) {
        lv_label_set_text(fx_rev_lbl, "Rev2");
        if (fx_rev_btn && lv_obj_is_valid(fx_rev_btn))
            lv_obj_set_style_bg_color(fx_rev_btn, lv_color_hex(0x66CCFF), 0);
    } else {
        lv_label_set_text(fx_rev_lbl, "Rev1");
        if (fx_rev_btn && lv_obj_is_valid(fx_rev_btn))
            lv_obj_set_style_bg_color(fx_rev_btn, lv_color_hex(0x0099FF), 0);
    }
}

static void fx_save_cb(lv_event_t *e) {
    (void)e;
    log_add("FX salvato", lv_color_hex(0x00FF00));
    toast_show("FX salvato!", lv_color_hex(0x00FF00), TOAST_DUR);
}

void fx_page_create(lv_obj_t *parent) {
    // Placeholder a sinistra
    lv_obj_t *lb = lv_label_create(parent);
    lv_label_set_text(lb, "FX (work in progress)");
    lv_obj_set_style_text_font(lb, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lb, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_pos(lb, 60, 200);

    const int FR_X = 470, FR_Y = 55;
    const int FR_W = 258, FR_H = 310;

    lv_obj_t *frame = lv_obj_create(parent);
    lv_obj_set_size(frame, FR_W, FR_H);
    lv_obj_set_pos(frame, FR_X, FR_Y);
    lv_obj_set_style_bg_opa(frame, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(frame, 1, 0);
    lv_obj_set_style_border_color(frame, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(frame, 6, 0);
    lv_obj_set_style_pad_all(frame, 0, 0);
    lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(frame, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_clip_corner(frame, 0, 0);

    // Label "FV-1" sopra il bordo
    lv_obj_t *fr_lbl = lv_label_create(frame);
    lv_label_set_text(fr_lbl, "FV-1");
    lv_obj_set_style_text_color(fr_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(fr_lbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_bg_color(fr_lbl, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(fr_lbl, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(fr_lbl, 6, 0);
    lv_obj_align(fr_lbl, LV_ALIGN_TOP_MID, 0, -22);

    // Dropdown preset (3 char)
    fx_preset_dd = lv_dropdown_create(frame);
    lv_obj_set_size(fx_preset_dd, 65, 45);
    lv_obj_set_pos(fx_preset_dd, 8, 12);
    lv_obj_set_style_bg_color(fx_preset_dd, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(fx_preset_dd, 2, 0);
    lv_obj_set_style_border_color(fx_preset_dd, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_color(fx_preset_dd, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(fx_preset_dd, &lv_font_montserrat_16, 0);
    lv_obj_set_style_pad_left(fx_preset_dd, 4, 0);
    lv_obj_set_style_pad_right(fx_preset_dd, 2, 0);
    lv_dropdown_set_options(fx_preset_dd, "P1\nP2\nP3\nP4\nP5\nP6\nP7\nP8");
    lv_dropdown_set_selected(fx_preset_dd, 0);

    // Rev toggle (azzurro ↔ azzurro chiaro)
    fx_rev_btn = lv_btn_create(frame);
    lv_obj_set_size(fx_rev_btn, 80, 45);
    lv_obj_set_pos(fx_rev_btn, 79, 12);
    lv_obj_set_style_bg_color(fx_rev_btn, lv_color_hex(0x0099FF), 0);
    lv_obj_set_style_radius(fx_rev_btn, 6, 0);
    lv_obj_set_style_border_width(fx_rev_btn, 2, 0);
    lv_obj_set_style_border_color(fx_rev_btn, lv_color_hex(0xCCEEFF), 0);
    fx_rev_lbl = lv_label_create(fx_rev_btn);
    lv_label_set_text(fx_rev_lbl, "Rev1");
    lv_obj_set_style_text_color(fx_rev_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(fx_rev_lbl, &lv_font_montserrat_18, 0);
    lv_obj_center(fx_rev_lbl);
    lv_obj_add_event_cb(fx_rev_btn, fx_rev_btn_cb, LV_EVENT_CLICKED, NULL);

    // Salva (rosso)
    fx_save_btn = lv_btn_create(frame);
    lv_obj_set_size(fx_save_btn, 85, 45);
    lv_obj_set_pos(fx_save_btn, 165, 12);
    lv_obj_set_style_bg_color(fx_save_btn, lv_color_hex(0xAA0000), 0);
    lv_obj_set_style_bg_color(fx_save_btn, lv_color_hex(0xDD2222), LV_STATE_PRESSED);
    lv_obj_set_style_radius(fx_save_btn, 6, 0);
    lv_obj_set_style_border_width(fx_save_btn, 2, 0);
    lv_obj_set_style_border_color(fx_save_btn, lv_color_hex(0xFF4444), 0);
    lv_obj_t *save_lbl = lv_label_create(fx_save_btn);
    lv_label_set_text(save_lbl, "Salva");
    lv_obj_set_style_text_color(save_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(save_lbl, &lv_font_montserrat_18, 0);
    lv_obj_center(save_lbl);
    lv_obj_add_event_cb(fx_save_btn, fx_save_cb, LV_EVENT_CLICKED, NULL);

    // 3 slider verticali: SIZE, LF, HF (label centrate sopra)
    const char *sl_labels[3] = {"SIZE", "LF", "HF"};
    lv_obj_t **sl_ptrs[3]    = {&pot_size_FV1, &pot_LF_FV1, &pot_HF_FV1};
    const int sl_w    = 48;
    const int sl_h    = 190;
    const int sl_gap  = 38;
    const int total_w = 3 * sl_w + 2 * sl_gap;
    const int sl_start_x = (FR_W - total_w) / 2;
    const int sl_y    = 105;

    for (int i = 0; i < 3; i++) {
        int sx = sl_start_x + i * (sl_w + sl_gap);

        lv_obj_t *c = lv_obj_create(frame);
        lv_obj_set_size(c, sl_w, sl_h);
        lv_obj_set_pos(c, sx, sl_y);
        lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(c, 0, 0);
        lv_obj_set_style_pad_all(c, 0, 0);
        lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *lbl = lv_label_create(frame);
        lv_label_set_text(lbl, sl_labels[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFA500), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
        lv_obj_align_to(lbl, c, LV_ALIGN_OUT_TOP_MID, 0, -6);

        lv_obj_t *s = lv_slider_create(c);
        lv_obj_set_size(s, sl_w, sl_h);
        lv_obj_align(s, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_slider_set_range(s, 0, 255);
        lv_slider_set_value(s, 127, LV_ANIM_OFF);

        lv_obj_set_style_bg_img_src(s, &img_slider_track, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_INDICATOR);
        lv_obj_set_style_bg_img_src(s, &img_slider_indicator, LV_PART_INDICATOR);
        lv_obj_set_style_bg_img_recolor(s, lv_color_hex(0xFFAA00), LV_PART_INDICATOR);
        lv_obj_set_style_bg_img_recolor_opa(s, (sliderColorDepth * 255) / 100, LV_PART_INDICATOR);
        lv_obj_set_style_bg_img_src(s, &img_slider_knob, LV_PART_KNOB);
        lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_KNOB);
        lv_obj_set_style_width(s, 45, LV_PART_KNOB);
        lv_obj_set_style_height(s, 30, LV_PART_KNOB);

        *sl_ptrs[i] = s;
    }
}