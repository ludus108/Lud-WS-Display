// ============================================================
// lvglGrafDrum.cpp — DRUM sequencer (editing pattern)
// ============================================================
#include "globals.h"
#include "lvglGraf.h"
#include "lvglGraf_internal.h"
#include "lvgl_v8_port.h"
#include "src/preset/preset_ui.h"
#include <math.h>

// ============================================================
// DRUM SEQUENCER
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
    "BD","SD","HH","OH","H2","Cp","P1","P2","P3"
};
static const uint32_t drum_row_colors[DRUM_ROWS] = {
    0xAA0044, 0xFF8800, 0xFFFF00, 0xFFFF00, 0xAAAA00,
    0x00FF00, 0x00AAFF, 0x00AAFF, 0x00AAFF
};

#define DRUM_PATTERNS       16
#define DRUM_STEPS          64
#define DRUM_SECTION_STEPS  16

#define DRUM_METER_DECAY_MS  500.0f
#define DRUM_METER_HOLD_MS   100.0f
#define DRUM_METER_TICK_MS   25

uint8_t    drum_seqArr[DRUM_PATTERNS][DRUM_ROWS][DRUM_STEPS] = {};
lv_obj_t  *drum_row_meter[DRUM_ROWS] = {nullptr};
lv_obj_t  *drum_ptn_dd = nullptr;

uint8_t   drum_mode      = 0;
uint8_t   drum_cur_fill  = 0;
uint8_t   drum_fillArr[DRUM_PATTERNS][DRUM_ROWS][DRUM_SECTION_STEPS] = {};

lv_obj_t *drum_fl_btn = nullptr;
lv_obj_t *drum_fl_lbl = nullptr;

lv_obj_t *drum_sec_btn[4] = {nullptr,nullptr,nullptr,nullptr};
lv_obj_t *drum_sec_lbl[4] = {nullptr,nullptr,nullptr,nullptr};
uint8_t   drum_cur_section = 0;

#define DRUM_SEC_ACTIVE_BORDER  0xFFCC33
#define DRUM_SEC_IDLE_BORDER    0x666666
#define DRUM_SEC_BG             0x1A1A2E
#define DRUM_SEC_BG_PRESSED     0x0F3460
#define DRUM_FL_ACTIVE_BORDER   0x00DD00

lv_obj_t *drum_mix_meter[DRUM_MIX_COUNT] = {nullptr};
static float drum_mix_meter_value[DRUM_MIX_COUNT] = {0.0f};
static int   drum_mix_meter_hold [DRUM_MIX_COUNT] = {0};

static int          drum_meter_hold [DRUM_ROWS] = {0};
static int          drum_meter_hold_ticks = 0;
static float        drum_meter_value [DRUM_ROWS] = {0.0f};
static lv_timer_t  *drum_meter_timer = nullptr;
static float        drum_meter_alpha = 0.0f;

static uint8_t  drum_cur_pattern        = 0;
static bool     drum_seqArr_initialized = false;
static int      drum_cursor_off_x       = 1;

static void drum_dd_update_options();

// ------------------------------------------------------------
// Meter envelope
// ------------------------------------------------------------
static void drum_meter_compute_alpha() {
    drum_meter_alpha = powf(0.01f, (float)DRUM_METER_TICK_MS / DRUM_METER_DECAY_MS);
    drum_meter_hold_ticks = (int)(DRUM_METER_HOLD_MS / DRUM_METER_TICK_MS);
    if (drum_meter_hold_ticks < 1) drum_meter_hold_ticks = 1;
}

static void drum_meter_tick_cb(lv_timer_t *t) {
    (void)t;
    for (int r = 0; r < DRUM_ROWS; r++) {
        if (!drum_row_meter[r] || !lv_obj_is_valid(drum_row_meter[r])) continue;

        if (drum_meter_hold[r] > 0) {
            drum_meter_hold[r]--;
            continue;
        }

        float v = drum_meter_value[r];
        if (v <= 0.0f) continue;

        v *= drum_meter_alpha;
        if (v < 0.5f) v = 0.0f;
        drum_meter_value[r] = v;
        lv_slider_set_value(drum_row_meter[r], (int)v, LV_ANIM_OFF);
    }

    // ---- Meter DRUM MIX ----
    for (int i = 0; i < DRUM_MIX_COUNT; i++) {
        if (!drum_mix_meter[i] || !lv_obj_is_valid(drum_mix_meter[i])) continue;

        if (drum_mix_meter_hold[i] > 0) {
            drum_mix_meter_hold[i]--;
            continue;
        }

        float v = drum_mix_meter_value[i];
        if (v <= 0.0f) continue;

        v *= drum_meter_alpha;
        if (v < 0.5f) v = 0.0f;
        drum_mix_meter_value[i] = v;
        lv_slider_set_value(drum_mix_meter[i], (int)v, LV_ANIM_OFF);
    }
}

static void drum_meter_create(lv_obj_t *parent, int row,
                              int x, int y, int w, int h) {
    lv_obj_t *s = lv_slider_create(parent);
    lv_obj_set_size(s, w, h);
    lv_obj_set_pos(s, x, y);
    lv_slider_set_range(s, 0, 100);
    lv_slider_set_value(s, 0, LV_ANIM_OFF);

    lv_obj_set_style_radius(s, 0, 0);
    lv_obj_set_style_radius(s, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s, 0, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s, 0, LV_PART_KNOB);

    lv_obj_set_style_pad_all(s, 0, 0);
    lv_obj_set_style_pad_all(s, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s, 0, LV_PART_INDICATOR);
    lv_obj_set_style_pad_all(s, 0, LV_PART_KNOB);

    lv_obj_set_style_width (s, 0, LV_PART_KNOB);
    lv_obj_set_style_height(s, 0, LV_PART_KNOB);

    lv_obj_set_style_bg_img_src(s, &img_micro_meter_audio_track, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(s, &img_micro_meter_audio_indicator, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, LV_PART_INDICATOR);

    lv_obj_set_style_bg_img_src(s, NULL, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_width(s, 0, LV_PART_KNOB);
    lv_obj_set_style_outline_width(s, 0, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(s, 0, LV_PART_KNOB);

    lv_obj_clear_flag(s, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s, LV_OBJ_FLAG_SCROLLABLE);

    drum_row_meter[row] = s;
}

static void drum_meters_update(uint8_t step) {
    for (int r = 0; r < DRUM_ROWS; r++) {
        uint8_t v = drum_pattern_data[r][step];
        if (v > 0) {
            drum_meter_value[r] = (v * 100.0f) / 3.0f;
            drum_meter_hold[r]  = drum_meter_hold_ticks;
        }
    }
}

static void drum_meters_reset() {
    for (int r = 0; r < DRUM_ROWS; r++) {
        drum_meter_value[r] = 0.0f;
        drum_meter_hold[r]  = 0;
        if (drum_row_meter[r] && lv_obj_is_valid(drum_row_meter[r]))
            lv_slider_set_value(drum_row_meter[r], 0, LV_ANIM_OFF);
    }
}

// ------------------------------------------------------------
// Vista <-> array attivo
// ------------------------------------------------------------
static uint8_t* drum_cell_ptr(int row, int col) {
    if (drum_mode == 0) {
        return &drum_seqArr[drum_cur_pattern][row]
                          [drum_cur_section * DRUM_SECTION_STEPS + col];
    } else {
        return &drum_fillArr[drum_cur_fill][row][col];
    }
}

static void drum_view_load_from_seqArr() {
    for (int r = 0; r < DRUM_ROWS; r++)
        for (int c = 0; c < DRUM_COLS; c++)
            drum_pattern_data[r][c] = *drum_cell_ptr(r, c);
}

static void drum_view_save_to_seqArr() {
    for (int r = 0; r < DRUM_ROWS; r++)
        for (int c = 0; c < DRUM_COLS; c++)
            *drum_cell_ptr(r, c) = drum_pattern_data[r][c];
}

static void drum_seqArr_randomize() {
    for (int p = 0; p < DRUM_PATTERNS; p++)
        for (int r = 0; r < DRUM_ROWS; r++)
            for (int s = 0; s < DRUM_STEPS; s++) {
                uint32_t rnd = (uint32_t)random(0, 100);
                uint8_t v;
                if      (rnd < 65) v = 0;
                else if (rnd < 77) v = 1;
                else if (rnd < 89) v = 2;
                else               v = 3;
                drum_seqArr[p][r][s] = v;
            }
    Serial.println("[DRUM] seqArr randomizzato");
}

static void drum_fillArr_randomize() {
    for (int f = 0; f < DRUM_PATTERNS; f++)
        for (int r = 0; r < DRUM_ROWS; r++)
            for (int s = 0; s < DRUM_SECTION_STEPS; s++) {
                uint32_t rnd = (uint32_t)random(0, 100);
                uint8_t v;
                if      (rnd < 60) v = 0;
                else if (rnd < 78) v = 1;
                else if (rnd < 92) v = 2;
                else               v = 3;
                drum_fillArr[f][r][s] = v;
            }
    Serial.println("[DRUM] fillArr randomizzato");
}

 void drum_seqArr_init_if_needed() {
    if (drum_seqArr_initialized) return;
    drum_seqArr_randomize();
    drum_fillArr_randomize();
    drum_seqArr_initialized = true;
}

// ------------------------------------------------------------
// Long-press
// ------------------------------------------------------------
static lv_timer_t *drum_lp_timer = nullptr;
static int         drum_lp_row   = -1;
static int         drum_lp_col   = -1;
static bool        drum_lp_fired = false;

static void drum_lp_timer_cb(lv_timer_t *t) {
    (void)t;
    drum_lp_timer = nullptr;
    if (drum_lp_row >= 0 && drum_lp_col >= 0) {
        drum_pattern_data[drum_lp_row][drum_lp_col] = 0;
        *drum_cell_ptr(drum_lp_row, drum_lp_col) = 0;
        extern void drum_update_cell_visual(int row, int col);
        drum_update_cell_visual(drum_lp_row, drum_lp_col);
        drum_lp_fired = true;
    }
}

static void drum_start_longpress(int row, int col) {
    if (drum_lp_timer) {
        lv_timer_del(drum_lp_timer);
        drum_lp_timer = nullptr;
    }
    drum_lp_row   = row;
    drum_lp_col   = col;
    drum_lp_fired = false;
    drum_lp_timer = lv_timer_create(drum_lp_timer_cb, 1000, NULL);
    lv_timer_set_repeat_count(drum_lp_timer, 1);
}

static void drum_cancel_longpress() {
    if (drum_lp_timer) {
        lv_timer_del(drum_lp_timer);
        drum_lp_timer = nullptr;
    }
    drum_lp_row = -1;
    drum_lp_col = -1;
}

// ------------------------------------------------------------
// Update visuale cella
// ------------------------------------------------------------
void drum_update_cell_visual(int row, int col) {
    if (row < 0 || row >= DRUM_ROWS) return;
    if (col < 0 || col >= DRUM_COLS) return;

    lv_obj_t *rect = drum_dot[row][col];
    if (!rect || !lv_obj_is_valid(rect)) return;

    uint8_t v = drum_pattern_data[row][col];
    if (v == 0) {
        lv_obj_add_flag(rect, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_t *cell = drum_cell[row][col];
    if (!cell || !lv_obj_is_valid(cell)) return;

    int content_h = lv_obj_get_content_height(cell);
    int h_max = content_h - 4;
    if (h_max < 3) h_max = 3;

    int h = (h_max * (int)v) / 3;
    if (h < 2) h = 2;

    lv_obj_set_height(rect, h);
    lv_obj_set_style_bg_color(rect, lv_color_hex(drum_row_colors[row]), 0);
    lv_obj_clear_flag(rect, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(rect, LV_ALIGN_BOTTOM_MID, 0, -2);
}

// ------------------------------------------------------------
// Event handler cella
// ------------------------------------------------------------
static void drum_cell_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    int id  = (int)(uintptr_t)lv_event_get_user_data(e);
    int row = (id >> 8) & 0xFF;
    int col =  id       & 0xFF;
    if (row >= DRUM_ROWS || col >= DRUM_COLS) return;

    if (code == LV_EVENT_PRESSED) {
        drum_start_longpress(row, col);
        return;
    }
    if (code == LV_EVENT_PRESS_LOST) {
        drum_cancel_longpress();
        drum_lp_fired = false;
        return;
    }
    if (code == LV_EVENT_CLICKED) {
        if (drum_lp_fired) {
            drum_lp_fired = false;
            drum_lp_row = -1;
            drum_lp_col = -1;
            return;
        }
        drum_cancel_longpress();
        uint8_t cur  = drum_pattern_data[row][col];
        uint8_t next = (cur >= 3) ? 0 : (cur + 1);
        drum_pattern_data[row][col] = next;
        *drum_cell_ptr(row, col) = next;
        drum_update_cell_visual(row, col);
        Serial.printf("[DRUM] %s [%d][%d] -> %d\n",
                      drum_mode == 0 ? "PTRN" : "FILL", row, col, next);
        return;
    }
}

// ------------------------------------------------------------
// Griglia
// ------------------------------------------------------------
static void drum_grid_create(lv_obj_t *parent) {
    const int X0      = 10;
    const int Y0      = 0;
    const int X1      = 770;
    const int LABEL_W = 30;

    const int METER_W = 22;
    const int METER_H = 24;
    const int METER_GAP = 4;

    int meterX = X0 + LABEL_W;
    int gridX  = meterX + METER_W + METER_GAP;
    int gridW  = X1 - gridX;

    int cellW = gridW / DRUM_COLS;
    int cellH = cellW;
    int gridH = cellH * DRUM_ROWS;

    int gap   = 2;
    int cw    = cellW - gap;
    int ch    = cellH - gap;

    int rect_w = cw - 22;
    if (rect_w < 4) rect_w = 4;

    for (int r = 0; r < DRUM_ROWS; r++) {
        lv_obj_t *lbl = lv_label_create(parent);
        lv_label_set_text(lbl, drum_row_names[r]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(drum_row_colors[r]), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(lbl, X0, Y0 + r * cellH + (cellH - 16) / 2);

        drum_meter_create(parent, r,
                          meterX,
                          Y0 + r * cellH + (cellH - METER_H) / 2,
                          METER_W,
                          METER_H);

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

            void *ud = (void*)(uintptr_t)((r << 8) | c);
            lv_obj_add_event_cb(cell, drum_cell_event_cb, LV_EVENT_PRESSED,    ud);
            lv_obj_add_event_cb(cell, drum_cell_event_cb, LV_EVENT_PRESS_LOST, ud);
            lv_obj_add_event_cb(cell, drum_cell_event_cb, LV_EVENT_CLICKED,    ud);

            drum_cell[r][c] = cell;

            lv_obj_t *rect = lv_obj_create(cell);
            lv_obj_set_size(rect, rect_w, 4);
            lv_obj_set_style_radius(rect, 1, 0);
            lv_obj_set_style_bg_color(rect, lv_color_hex(drum_row_colors[r]), 0);
            lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(rect, 0, 0);
            lv_obj_set_style_pad_all(rect, 0, 0);
            lv_obj_clear_flag(rect, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(rect, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_align(rect, LV_ALIGN_CENTER, 0, 0);
            lv_obj_add_flag(rect, LV_OBJ_FLAG_HIDDEN);
            drum_dot[r][c] = rect;
        }
    }

    // ---- Linee orizzontali tra le righe ----
    for (int r = 1; r < DRUM_ROWS; r++) {
        int ly = Y0 + r * cellH - gap;
        lv_obj_t *hline = lv_obj_create(parent);
        lv_obj_set_size(hline, gridW, gap);
        lv_obj_set_pos(hline, gridX, ly);
        lv_obj_set_style_bg_color(hline, lv_color_hex(0x666666), 0);
        lv_obj_set_style_bg_opa(hline, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(hline, 0, 0);
        lv_obj_set_style_radius(hline, 0, 0);
        lv_obj_set_style_pad_all(hline, 0, 0);
        lv_obj_clear_flag(hline, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(hline, LV_OBJ_FLAG_CLICKABLE);
    }

    // ---- Separatori quarti ----
    struct SepDef { int colAfter; uint32_t color; const char *label; };
    static const SepDef seps[4] = {
        { 0, 0x886600, "1" }, { 4, 0xCCCCCC, "2" },
        { 8, 0x886600, "3" }, {12, 0xCCCCCC, "4" },
    };
    for (int i = 0; i < 4; i++) {
        int sx = gridX + seps[i].colAfter * cellW;
        int sy = Y0;
        int sh = gridH;

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
        lv_obj_set_pos(lbl, sx + 3, sy + 2);
    }

    drum_grid_x = gridX;
    drum_grid_y = Y0;
    drum_grid_w = gridW;
    drum_grid_h = gridH;
    drum_cell_w = cellW;

    drum_cursor_off_x = gap / 2;

    drum_cursor = lv_obj_create(parent);
    lv_obj_set_size(drum_cursor, cw, gridH);
    lv_obj_set_pos(drum_cursor, gridX + drum_cursor_off_x, Y0);
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
    for (int r = 0; r < DRUM_ROWS; r++)
        for (int c = 0; c < DRUM_COLS; c++)
            drum_update_cell_visual(r, c);
}

void drum_seq_update_bpm(uint16_t bpm) {
    if (!drum_step_timer) return;
    if (bpm < 60)  bpm = 60;
    if (bpm > 250) bpm = 250;
    uint32_t period = (60000 / (uint32_t)bpm) / 4;
    if (period < 5) period = 5;
    lv_timer_set_period(drum_step_timer, period);
}

static void drum_step_timer_cb(lv_timer_t *t) {
    (void)t;
    if (!drum_playing)  return;
    if (!drum_cursor)   return;
    drum_step_counter = (drum_step_counter + 1) & 0x0F;
    if (lv_obj_is_valid(drum_cursor)) {
        lv_obj_set_x(drum_cursor,
            drum_grid_x + drum_cursor_off_x
            + drum_step_counter * drum_cell_w);
    }
    drum_meters_update(drum_step_counter);
}

static void drum_play_btn_cb(lv_event_t *e) {
    (void)e;
    drum_playing = !drum_playing;

    if (drum_playing) {
		        drum_seq_update_bpm(drum_bpm);
        drum_step_counter = 0;
        if (drum_cursor && lv_obj_is_valid(drum_cursor)) {
            lv_obj_set_x(drum_cursor, drum_grid_x + drum_cursor_off_x);
            lv_obj_clear_flag(drum_cursor, LV_OBJ_FLAG_HIDDEN);
        }
        drum_meters_reset();
        if (drum_play_btn && lv_obj_is_valid(drum_play_btn))
            lv_obj_set_style_border_color(drum_play_btn, lv_color_hex(0x888888), 0);
        if (drum_play_lbl && lv_obj_is_valid(drum_play_lbl))
            lv_label_set_text(drum_play_lbl, "Stop");
    } else {
        drum_step_counter = 0;
        if (drum_cursor && lv_obj_is_valid(drum_cursor))
            lv_obj_add_flag(drum_cursor, LV_OBJ_FLAG_HIDDEN);
        drum_meters_reset();
        if (drum_play_btn && lv_obj_is_valid(drum_play_btn))
            lv_obj_set_style_border_color(drum_play_btn, lv_color_hex(0x00FF00), 0);
        if (drum_play_lbl && lv_obj_is_valid(drum_play_lbl))
            lv_label_set_text(drum_play_lbl, "Play");
    }
}

// ------------------------------------------------------------
// Sezioni A/B/C/D
// ------------------------------------------------------------
static void drum_sec_apply_visual() {
    bool fill_mode = (drum_mode == 1);

    for (int i = 0; i < 4; i++) {
        if (!drum_sec_btn[i] || !lv_obj_is_valid(drum_sec_btn[i])) continue;
        bool on = (!fill_mode && i == drum_cur_section);
        lv_obj_set_style_border_width(drum_sec_btn[i], on ? 3 : 2, 0);
        lv_obj_set_style_border_color(drum_sec_btn[i],
            lv_color_hex(on ? DRUM_SEC_ACTIVE_BORDER : DRUM_SEC_IDLE_BORDER), 0);
        if (drum_sec_lbl[i] && lv_obj_is_valid(drum_sec_lbl[i])) {
            lv_obj_set_style_text_color(drum_sec_lbl[i],
                lv_color_hex(on ? DRUM_SEC_ACTIVE_BORDER : 0xFFFFFF), 0);
        }
    }

    if (drum_fl_btn && lv_obj_is_valid(drum_fl_btn)) {
        lv_obj_set_style_border_width(drum_fl_btn, fill_mode ? 3 : 2, 0);
        lv_obj_set_style_border_color(drum_fl_btn,
            lv_color_hex(fill_mode ? DRUM_FL_ACTIVE_BORDER : DRUM_SEC_IDLE_BORDER), 0);
        if (drum_fl_lbl && lv_obj_is_valid(drum_fl_lbl)) {
            lv_obj_set_style_text_color(drum_fl_lbl,
                lv_color_hex(fill_mode ? DRUM_FL_ACTIVE_BORDER : 0xFFFFFF), 0);
        }
    }
}

static void drum_sec_btn_cb(lv_event_t *e) {
    int sec = (int)(uintptr_t)lv_event_get_user_data(e);
    if (sec < 0 || sec > 3) return;

    bool was_fill = (drum_mode == 1);
    if (!was_fill && sec == drum_cur_section) return;

    drum_view_save_to_seqArr();

    drum_mode = 0;
    drum_cur_section = (uint8_t)sec;

    drum_sec_apply_visual();
    drum_dd_update_options();

    drum_view_load_from_seqArr();

    for (int r = 0; r < DRUM_ROWS; r++)
        for (int c = 0; c < DRUM_COLS; c++)
            drum_update_cell_visual(r, c);

    send_param_update(ID_TEENSY, 'o', drum_cur_section);
}

static void drum_sec_create(lv_obj_t *parent) {
    static const char *names[4] = {"A","B","C","D"};
    const int W = 55, H = 45, GAP = 5;
    const int Y  = 400;
    const int X0 = 175;

    for (int i = 0; i < 4; i++) {
        lv_obj_t *b = lv_btn_create(parent);
        lv_obj_set_size(b, W, H);
        lv_obj_set_pos(b, X0 + i * (W + GAP), Y);
        lv_obj_set_style_bg_color(b, lv_color_hex(DRUM_SEC_BG), 0);
        lv_obj_set_style_bg_color(b, lv_color_hex(DRUM_SEC_BG_PRESSED),
                                  LV_STATE_PRESSED);
        lv_obj_set_style_radius(b, 6, 0);
        lv_obj_set_style_border_width(b, 2, 0);
        lv_obj_set_style_border_color(b, lv_color_hex(DRUM_SEC_IDLE_BORDER), 0);

        lv_obj_t *l = lv_label_create(b);
        lv_label_set_text(l, names[i]);
        lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_24, 0);
        lv_obj_center(l);

        drum_sec_btn[i] = b;
        drum_sec_lbl[i] = l;
        lv_obj_add_event_cb(b, drum_sec_btn_cb, LV_EVENT_CLICKED,
                            (void*)(uintptr_t)i);
    }
    drum_sec_apply_visual();
}

static void drum_fl_btn_cb(lv_event_t *e) {
    (void)e;
    if (drum_mode == 1) return;

    drum_view_save_to_seqArr();

    drum_mode = 1;

    drum_sec_apply_visual();
    drum_dd_update_options();

    drum_view_load_from_seqArr();

    for (int r = 0; r < DRUM_ROWS; r++)
        for (int c = 0; c < DRUM_COLS; c++)
            drum_update_cell_visual(r, c);
}

static void drum_fl_btn_create(lv_obj_t *parent) {
    drum_fl_btn = lv_btn_create(parent);
    lv_obj_set_size(drum_fl_btn, 55, 45);
    lv_obj_set_pos(drum_fl_btn, 415, 400);
    lv_obj_set_style_bg_color(drum_fl_btn, lv_color_hex(DRUM_SEC_BG), 0);
    lv_obj_set_style_bg_color(drum_fl_btn, lv_color_hex(DRUM_SEC_BG_PRESSED),
                              LV_STATE_PRESSED);
    lv_obj_set_style_radius(drum_fl_btn, 6, 0);
    lv_obj_set_style_border_width(drum_fl_btn, 2, 0);
    lv_obj_set_style_border_color(drum_fl_btn, lv_color_hex(DRUM_SEC_IDLE_BORDER), 0);

    drum_fl_lbl = lv_label_create(drum_fl_btn);
    lv_label_set_text(drum_fl_lbl, "FL");
    lv_obj_set_style_text_color(drum_fl_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(drum_fl_lbl, &lv_font_montserrat_24, 0);
    lv_obj_center(drum_fl_lbl);

    lv_obj_add_event_cb(drum_fl_btn, drum_fl_btn_cb, LV_EVENT_CLICKED, NULL);
}

// ------------------------------------------------------------
// Dropdown PTN/FLN
// ------------------------------------------------------------
static void drum_ptn_dd_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    int sel = lv_dropdown_get_selected(dd);
    if (sel < 0 || sel >= DRUM_PATTERNS) return;

    if (drum_mode == 0) {
        if (sel == drum_cur_pattern) return;
        drum_view_save_to_seqArr();
        drum_cur_pattern = (uint8_t)sel;
        drum_view_load_from_seqArr();
        send_param_update(ID_TEENSY, 'N', drum_cur_pattern);
    } else {
        if (sel == drum_cur_fill) return;
        drum_view_save_to_seqArr();
        drum_cur_fill = (uint8_t)sel;
        drum_view_load_from_seqArr();
        send_param_update(ID_TEENSY, 'i', drum_cur_fill);
    }

    for (int r = 0; r < DRUM_ROWS; r++)
        for (int c = 0; c < DRUM_COLS; c++)
            drum_update_cell_visual(r, c);
}

static void drum_dd_update_options() {
    if (!drum_ptn_dd || !lv_obj_is_valid(drum_ptn_dd)) return;

    if (drum_mode == 0) {
        lv_dropdown_set_options(drum_ptn_dd,
            "PTN 1\nPTN 2\nPTN 3\nPTN 4\nPTN 5\nPTN 6\nPTN 7\nPTN 8\n"
            "PTN 9\nPTN 10\nPTN 11\nPTN 12\nPTN 13\nPTN 14\nPTN 15\nPTN 16");
        lv_dropdown_set_selected(drum_ptn_dd, drum_cur_pattern);
        lv_dropdown_set_symbol(drum_ptn_dd, NULL);
    } else {
        lv_dropdown_set_options(drum_ptn_dd,
            "FLN 1\nFLN 2\nFLN 3\nFLN 4\nFLN 5\nFLN 6\nFLN 7\nFLN 8\n"
            "FLN 9\nFLN 10\nFLN 11\nFLN 12\nFLN 13\nFLN 14\nFLN 15\nFLN 16");
        lv_dropdown_set_selected(drum_ptn_dd, drum_cur_fill);
    }
}

static void drum_ptn_dd_create(lv_obj_t *parent) {
    drum_ptn_dd = lv_dropdown_create(parent);
    lv_obj_set_size(drum_ptn_dd, 95, 45);
    lv_obj_set_pos(drum_ptn_dd, 70, 400);
    lv_obj_set_style_bg_color(drum_ptn_dd, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(drum_ptn_dd, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(drum_ptn_dd, 2, 0);
    lv_obj_set_style_border_color(drum_ptn_dd, lv_color_hex(0x00AAFF), 0);
    lv_obj_set_style_radius(drum_ptn_dd, 6, 0);
    lv_obj_set_style_text_color(drum_ptn_dd, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(drum_ptn_dd, &lv_font_montserrat_24, 0);
    lv_obj_set_style_pad_left(drum_ptn_dd, 6, 0);
    lv_obj_set_style_pad_right(drum_ptn_dd, 4, 0);

    drum_dd_update_options();

    lv_obj_t *list = lv_dropdown_get_list(drum_ptn_dd);
    if (list) {
        lv_obj_set_style_text_font(list, &lv_font_montserrat_16, 0);
        lv_obj_set_style_bg_color(list, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_text_color(list, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_max_height(list, 380, 0);
    }

    lv_obj_add_event_cb(drum_ptn_dd, drum_ptn_dd_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

// ------------------------------------------------------------
// Pagina SEQ
// ------------------------------------------------------------
void drum_seq_page_create(lv_obj_t *parent, lv_obj_t *title_lbl) {
	
	
    (void)title_lbl;

    lv_obj_t *home = lv_btn_create(parent);
    lv_obj_set_size(home, 55, 45);
    lv_obj_set_pos(home, 5, 400);
    lv_obj_set_style_bg_color(home, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(home, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_radius(home, 6, 0);
    lv_obj_set_style_border_width(home, 2, 0);
    lv_obj_set_style_border_color(home, lv_color_hex(0x9B59B6), 0);
    lv_obj_t *home_lbl = lv_label_create(home);
    lv_label_set_text(home_lbl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(home_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(home_lbl, &lv_font_montserrat_24, 0);
    lv_obj_center(home_lbl);
    lv_obj_add_event_cb(home, eb, LV_EVENT_CLICKED, (void*)(uintptr_t)-3);

    drum_ptn_dd_create(parent);
    drum_sec_create(parent);
    drum_fl_btn_create(parent);

    drum_play_btn = lv_btn_create(parent);
    lv_obj_set_size(drum_play_btn, 55, 45);
    lv_obj_set_pos(drum_play_btn, 480, 400);
    lv_obj_set_style_bg_color(drum_play_btn, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(drum_play_btn, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_radius(drum_play_btn, 6, 0);
    lv_obj_set_style_border_width(drum_play_btn, 2, 0);
    lv_obj_set_style_border_color(drum_play_btn, lv_color_hex(0x00FF00), 0);

    drum_play_lbl = lv_label_create(drum_play_btn);
    lv_label_set_text(drum_play_lbl, "Play");
    lv_obj_set_style_text_color(drum_play_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(drum_play_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(drum_play_lbl);
    lv_obj_add_event_cb(drum_play_btn, drum_play_btn_cb, LV_EVENT_CLICKED, NULL);

       if (!drum_step_timer) {
        uint32_t period = (60000 / (uint32_t)drum_bpm) / 4;
        if (period < 5) period = 5;
        drum_step_timer = lv_timer_create(drum_step_timer_cb, period, NULL);
    } else {
        drum_seq_update_bpm(drum_bpm);
    }

    lv_obj_t *save_btn = lv_btn_create(parent);
    lv_obj_set_size(save_btn, 70, 45);
    lv_obj_set_pos(save_btn, 705, 400);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0xAA0000), 0);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0xDD2222), LV_STATE_PRESSED);
    lv_obj_set_style_radius(save_btn, 6, 0);
    lv_obj_set_style_border_width(save_btn, 2, 0);
    lv_obj_set_style_border_color(save_btn, lv_color_hex(0xFF4444), 0);
    lv_obj_t *save_lbl = lv_label_create(save_btn);
    lv_label_set_text(save_lbl, "Salva");
    lv_obj_set_style_text_color(save_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(save_lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(save_lbl);
    lv_obj_add_event_cb(save_btn, eb, LV_EVENT_CLICKED, (void*)(uintptr_t)31);

    for (int r = 0; r < DRUM_ROWS; r++) drum_row_meter[r] = nullptr;

    drum_seqArr_init_if_needed();
    drum_view_load_from_seqArr();

    drum_grid_create(parent);
    drum_grid_update();

    if (drum_meter_alpha == 0.0f) drum_meter_compute_alpha();
    drum_meters_reset();
    if (!drum_meter_timer)
        drum_meter_timer = lv_timer_create(drum_meter_tick_cb,
                                           DRUM_METER_TICK_MS, NULL);

    drum_playing      = false;
    drum_step_counter = 0;
}



// ============================================================
// BPM + SWING (pagina DRUM principale)
// ============================================================
lv_obj_t *drum_bpm_btn      = nullptr;
lv_obj_t *drum_bpm_lbl      = nullptr;
lv_obj_t *drum_swing_slider = nullptr;
lv_obj_t *drum_swing_val_lbl= nullptr;
uint16_t  drum_bpm   = 120;   // 60..250
uint16_t  drum_swing = 0;     // 0..swingMax

// Finestra popup BPM (tastiera numerica)
static lv_obj_t *drum_bpm_win = nullptr;
static lv_obj_t *drum_bpm_ta  = nullptr;

// ------------------------------------------------------------
// Calcolo swingMax per il BPM dato (stessa formula del Teensy)
// ------------------------------------------------------------
uint32_t drum_swing_max_for_bpm(uint16_t bpm) {
    if (bpm < 60)  bpm = 60;
    if (bpm > 250) bpm = 250;
    uint32_t preSeqTime = (60000 / bpm) / 4;
    if (preSeqTime < 4) return 0;
    return (preSeqTime / 2) - 2;
}

static void drum_swing_update_label() {
    if (!drum_swing_val_lbl || !lv_obj_is_valid(drum_swing_val_lbl)) return;

    uint32_t sMax = drum_swing_max_for_bpm(drum_bpm);
    if (sMax == 0) {
        lv_label_set_text(drum_swing_val_lbl, "0 %");
        return;
    }

    uint32_t pct = ((uint32_t)drum_swing * 100) / sMax;
    if (pct > 100) pct = 100;

    char buf[8];
    snprintf(buf, sizeof(buf), "%u %%", (unsigned)pct);
    lv_label_set_text(drum_swing_val_lbl, buf);
}

// ------------------------------------------------------------
// Aggiorna range+valore dello slider in base al BPM corrente
// ------------------------------------------------------------

// Ricalcola drum_swing per mantenere la STESSA PERCENTUALE
// quando cambia swingMax (es. dopo un cambio BPM).
static void drum_swing_scale_to_new_max(uint32_t oldMax, uint32_t newMax) {
    if (oldMax == 0 || newMax == 0) {
        drum_swing = 0;
    } else {
        uint32_t pct = ((uint32_t)drum_swing * 100) / oldMax;
        if (pct > 100) pct = 100;
        drum_swing = (uint16_t)((pct * newMax + 50) / 100);   // arrotonda
        if (drum_swing > newMax) drum_swing = (uint16_t)newMax;
    }

    if (drum_swing_slider && lv_obj_is_valid(drum_swing_slider)) {
        lv_slider_set_range(drum_swing_slider, 0, (int32_t)newMax);
        lv_slider_set_value(drum_swing_slider, drum_swing, LV_ANIM_OFF);
    }
    drum_swing_update_label();
}


// ------------------------------------------------------------
// Echo dal Teensy
// ------------------------------------------------------------

void drum_bpm_set_from_teensy(uint16_t bpm) {
    if (bpm < 60)  bpm = 60;
    if (bpm > 250) bpm = 250;

    uint32_t oldMax = drum_swing_max_for_bpm(drum_bpm);
    if (oldMax > 255) oldMax = 255;

    drum_bpm = bpm;

    if (drum_bpm_lbl && lv_obj_is_valid(drum_bpm_lbl)) {
        char buf[16];
        snprintf(buf, sizeof(buf), "BPM %u", (unsigned)drum_bpm);
        lv_label_set_text(drum_bpm_lbl, buf);
    }

    uint32_t newMax = drum_swing_max_for_bpm(drum_bpm);
    if (newMax > 255) newMax = 255;

    drum_swing_scale_to_new_max(oldMax, newMax);

    // Aggiorna velocità PTN e SONG
    drum_seq_update_bpm(drum_bpm);
    song_update_bpm(drum_bpm);
}

void drum_swing_set_from_teensy(uint16_t swing) {
    uint32_t sMax = drum_swing_max_for_bpm(drum_bpm);
    if (swing > sMax) swing = (uint16_t)sMax;
    drum_swing = swing;

    if (drum_swing_slider && lv_obj_is_valid(drum_swing_slider)) {
		  lv_slider_set_range(drum_swing_slider, 0, (int32_t)sMax);
        lv_slider_set_value(drum_swing_slider, drum_swing, LV_ANIM_OFF);
    }
     drum_swing_update_label();
}

// ------------------------------------------------------------
// Slider SWING: drag → aggiorna valore + invia 'W' al Teensy
// ------------------------------------------------------------
static void drum_swing_cb(lv_event_t *e) {
    lv_obj_t *s = lv_event_get_target(e);
    int v = lv_slider_get_value(s);
    drum_swing = (uint16_t)v;
    drum_swing_update_label();	
    send_param_update(ID_TEENSY, 'W', (uint8_t)v);
}

// ------------------------------------------------------------
// Popup tastiera numerica per BPM
// ------------------------------------------------------------
void drum_close_bpm_window() {
    if (drum_bpm_win) {
        lv_obj_del(drum_bpm_win);
        drum_bpm_win = nullptr;
        drum_bpm_ta  = nullptr;
    }
}

static void drum_bpm_confirm_cb(lv_event_t *e) {
    (void)e;
    if (!drum_bpm_ta) return;
    const char *txt = lv_textarea_get_text(drum_bpm_ta);
    int val = atoi(txt);
    if (val < 60)  val = 60;
    if (val > 250) val = 250;

    uint32_t oldMax = drum_swing_max_for_bpm(drum_bpm);
    if (oldMax > 255) oldMax = 255;

    drum_bpm = (uint16_t)val;

    if (drum_bpm_lbl && lv_obj_is_valid(drum_bpm_lbl)) {
        char buf[16];
        snprintf(buf, sizeof(buf), "BPM %u", (unsigned)drum_bpm);
        lv_label_set_text(drum_bpm_lbl, buf);
    }

    uint32_t newMax = drum_swing_max_for_bpm(drum_bpm);
    if (newMax > 255) newMax = 255;

    drum_swing_scale_to_new_max(oldMax, newMax);

    // Aggiorna velocità PTN e SONG
    drum_seq_update_bpm(drum_bpm);
    song_update_bpm(drum_bpm);

    // Invia al Teensy
    send_param_update(ID_TEENSY, 'b', (uint8_t)drum_bpm);
    send_param_update(ID_TEENSY, 'W', (uint8_t)drum_swing);

    drum_close_bpm_window();
}

static void drum_bpm_cancel_cb(lv_event_t *e) {
    (void)e;
    drum_close_bpm_window();
}

static void drum_bpm_btn_cb(lv_event_t *e) {
    (void)e;
    if (drum_bpm_win) return;

    drum_bpm_win = lv_win_create(lv_scr_act(), 0);
    lv_obj_set_size(drum_bpm_win, 420, 380);
    lv_obj_set_pos(drum_bpm_win, 190, 50);
    lv_obj_set_style_bg_color(drum_bpm_win, lv_color_hex(0x111111), 0);
    lv_obj_set_style_border_color(drum_bpm_win, lv_color_hex(0x00AAFF), 0);
    lv_obj_set_style_border_width(drum_bpm_win, 2, 0);
    lv_obj_set_style_radius(drum_bpm_win, 8, 0);

    lv_obj_t *client = lv_win_get_content(drum_bpm_win);
    lv_obj_set_style_pad_all(client, 10, 0);
    lv_obj_set_style_bg_color(client, lv_color_hex(0x000000), 0);
    lv_obj_set_flex_flow(client, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(client, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(client, 10, 0);

    lv_obj_t *lbl = lv_label_create(client);
    lv_label_set_text(lbl, "BPM (60..250)");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);

    drum_bpm_ta = lv_textarea_create(client);
    lv_obj_set_size(drum_bpm_ta, 380, 50);
    lv_obj_set_style_bg_color(drum_bpm_ta, lv_color_hex(0x222222), 0);
    lv_obj_set_style_text_color(drum_bpm_ta, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(drum_bpm_ta, 1, 0);
    lv_obj_set_style_border_color(drum_bpm_ta, lv_color_hex(0x00AAFF), 0);
    lv_obj_set_style_text_font(drum_bpm_ta, &lv_font_montserrat_24, 0);
    lv_textarea_set_one_line(drum_bpm_ta, true);
    lv_textarea_set_max_length(drum_bpm_ta, 3);
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", (unsigned)drum_bpm);
    lv_textarea_set_text(drum_bpm_ta, buf);

    lv_obj_t *kb = lv_keyboard_create(client);
    lv_obj_set_size(kb, 380, 200);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
    lv_keyboard_set_textarea(kb, drum_bpm_ta);
    lv_obj_set_style_text_font(kb, &lv_font_montserrat_18, 0);
    lv_obj_set_style_bg_color(kb, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_color(kb, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_color(kb, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_color(kb, lv_color_hex(0x444444), LV_STATE_PRESSED);
    lv_obj_set_style_text_color(kb, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    lv_obj_t *btn_cont = lv_obj_create(client);
    lv_obj_set_size(btn_cont, 380, 50);
    lv_obj_set_style_bg_opa(btn_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_cont, 0, 0);
    lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(btn_cont, 20, 0);

    lv_obj_t *ok = lv_btn_create(btn_cont);
    lv_obj_set_size(ok, 120, 40);
    lv_obj_set_style_bg_color(ok, lv_color_hex(0x1A6B4A), 0);
    lv_obj_t *ok_l = lv_label_create(ok);
    lv_label_set_text(ok_l, "OK");
    lv_obj_set_style_text_color(ok_l, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(ok_l);
    lv_obj_add_event_cb(ok, drum_bpm_confirm_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cancel = lv_btn_create(btn_cont);
    lv_obj_set_size(cancel, 120, 40);
    lv_obj_set_style_bg_color(cancel, lv_color_hex(0x6B1A1A), 0);
    lv_obj_t *cn_l = lv_label_create(cancel);
    lv_label_set_text(cn_l, "Annulla");
    lv_obj_set_style_text_color(cn_l, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(cn_l);
    lv_obj_add_event_cb(cancel, drum_bpm_cancel_cb, LV_EVENT_CLICKED, NULL);
}

// ------------------------------------------------------------
// Creazione dei widget BPM + SWING (chiamato da create_page DRUM)
// ------------------------------------------------------------
void drum_bpm_swing_create(lv_obj_t *parent) {
    // ---- Bottone BPM (top-right) ----
    drum_bpm_btn = lv_btn_create(parent);
    lv_obj_set_size(drum_bpm_btn, 140, 50);
    lv_obj_set_pos(drum_bpm_btn, 640, 10);
    lv_obj_set_style_bg_color(drum_bpm_btn, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(drum_bpm_btn, lv_color_hex(0x0F3460),
                              LV_STATE_PRESSED);
    lv_obj_set_style_radius(drum_bpm_btn, 6, 0);
    lv_obj_set_style_border_width(drum_bpm_btn, 2, 0);
    lv_obj_set_style_border_color(drum_bpm_btn, lv_color_hex(0x00AAFF), 0);

    drum_bpm_lbl = lv_label_create(drum_bpm_btn);
    char buf[16];
    snprintf(buf, sizeof(buf), "BPM %u", (unsigned)drum_bpm);
    lv_label_set_text(drum_bpm_lbl, buf);
    lv_obj_set_style_text_color(drum_bpm_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(drum_bpm_lbl, &lv_font_montserrat_24, 0);
    lv_obj_center(drum_bpm_lbl);
    lv_obj_add_event_cb(drum_bpm_btn, drum_bpm_btn_cb,
                        LV_EVENT_CLICKED, NULL);

    // ---- Slider SWING (sotto BPM) ----
    // Container verticale: nome top + slider + valore bottom
    const int SL_W = 60;
    const int SL_H = 200;
    const int SL_X = 723;   // 800 - 10 margin - 60 = 730
    const int SL_Y = 90;

    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_size(c, SL_W, SL_H + 50);
    lv_obj_set_pos(c, SL_X, SL_Y);
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_set_style_pad_all(c, 0, 0);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);

    // Label nome "SWING" (top)
    lv_obj_t *lbl = lv_label_create(c);
    lv_label_set_text(lbl, "SWING");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 0);

    // Slider (center) — stile immagini come in REV
    lv_obj_t *s = lv_slider_create(c);
    lv_obj_set_size(s, SL_W, SL_H);
    lv_obj_align(s, LV_ALIGN_CENTER, 0, 6);
    uint32_t sMax = drum_swing_max_for_bpm(drum_bpm);
    if (sMax > 255) sMax = 255;
    lv_slider_set_range(s, 0, (int32_t)sMax);
    lv_slider_set_value(s, drum_swing, LV_ANIM_OFF);

    lv_obj_set_style_bg_img_src(s, &img_slider_track, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_src(s, &img_slider_indicator, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor(s, lv_color_hex(0x00AAFF),
                                    LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor_opa(s,
        (sliderColorDepth * 255) / 100, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_src(s, &img_slider_knob, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_width(s, 45, LV_PART_KNOB);
    lv_obj_set_style_height(s, 30, LV_PART_KNOB);

    drum_swing_slider = s;

    // Label valore (bottom)
       drum_swing_val_lbl = lv_label_create(c);
    lv_obj_set_style_text_color(drum_swing_val_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(drum_swing_val_lbl, &lv_font_montserrat_16, 0);
    lv_obj_align(drum_swing_val_lbl, LV_ALIGN_BOTTOM_MID, 0, 0);
    drum_swing_update_label();

    lv_obj_add_event_cb(s, drum_swing_cb, LV_EVENT_VALUE_CHANGED, NULL);
}


// ============================================================
// MIX — 8 slider livelli voce (in riga con SWING)
// ============================================================
lv_obj_t *drum_mix_slider [DRUM_MIX_COUNT] = {nullptr};
lv_obj_t *drum_mix_val_lbl[DRUM_MIX_COUNT] = {nullptr};
uint8_t   drum_mix_val    [DRUM_MIX_COUNT] = {14,14,14,14,14,14,14,14};

// Nomi voce (in ordine come in kitLevArr)
static const char *drum_mix_names[DRUM_MIX_COUNT] = {
    "BD", "SD", "HH", "HH2", "CLAP", "P1", "P2", "P3"
};

// Colori dei nomi riga in PTN (per BD, SD, HH, H2, Cp, P1, P2, P3)
static const uint32_t drum_mix_colors[DRUM_MIX_COUNT] = {
    0xAA0044,  // BD
    0xFF8800,  // SD
    0xFFFF00,  // HH
    0xAAAA00,  // HH2  (H2 in PTN)
    0x00FF00,  // CLAP (Cp in PTN)
    0x00AAFF,  // PERC1
    0x00AAFF,  // PERC2
    0x00AAFF   // PERC3
};

// Voice index 1-based per comando LWS 'V' (mappa ai seqArr idx)
//   BD=0->V1, SD=1->V2, HH=2->V3, HH2=4->V5, CLAP=5->V6,
//   PERC1=6->V7, PERC2=7->V8, PERC3=8->V9
static const uint8_t drum_mix_voice[DRUM_MIX_COUNT] = {
    1, 2, 3, 5, 6, 7, 8, 9
};

static void drum_mix_slider_cb(lv_event_t *e) {
    lv_obj_t *s = lv_event_get_target(e);
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= DRUM_MIX_COUNT) return;

    int v = lv_slider_get_value(s);
    drum_mix_val[idx] = (uint8_t)v;

    if (drum_mix_val_lbl[idx] && lv_obj_is_valid(drum_mix_val_lbl[idx])) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", v);
        lv_label_set_text(drum_mix_val_lbl[idx], buf);
    }

    // LWS: prima seleziona la voce, poi imposta il livello
    send_param_update(ID_TEENSY, 'V', drum_mix_voice[idx]);
    send_param_update(ID_TEENSY, 'L', (uint8_t)v);
}

// Trigger dei meter MIX da uno slot song
void drum_mix_meters_trigger_from_slot(int slot, uint8_t step) {
    if (slot < 0 || slot >= (int)songLen[song_cur_song]) return;
    if (step >= 16) return;

    uint8_t tipo = songArr[song_cur_song][SONG_COL_TIPO][slot];
    uint8_t num  = songArr[song_cur_song][SONG_COL_NUM][slot];

    uint8_t vals[DRUM_MIX_COUNT] = {0};

    if (tipo == 4) {
        // Fill
        for (int r = 0; r < DRUM_MIX_COUNT; r++) {
            int seqRow;
            switch (r) {
                case 0: seqRow = 0; break;   // BD
                case 1: seqRow = 1; break;   // SD
                case 2:                       // HH = max(HH, OH)
                {
                    uint8_t hh = drum_fillArr[num][2][step];
                    uint8_t oh = drum_fillArr[num][3][step];
                    vals[r] = (hh > oh) ? hh : oh;
                    continue;
                }
                case 3: seqRow = 4; break;   // HH2
                case 4: seqRow = 5; break;   // CLAP
                case 5: seqRow = 6; break;   // P1
                case 6: seqRow = 7; break;   // P2
                case 7: seqRow = 8; break;   // P3
                default: seqRow = 0; break;
            }
            vals[r] = drum_fillArr[num][seqRow][step];
        }
    } else {
        // Sezione A/B/C/D del pattern
        uint8_t sec = tipo;   // 0..3
        uint8_t s = sec * 16 + step;
        for (int r = 0; r < DRUM_MIX_COUNT; r++) {
            int seqRow;
            switch (r) {
                case 0: seqRow = 0; break;
                case 1: seqRow = 1; break;
                case 2:
                {
                    uint8_t hh = drum_seqArr[num][2][s];
                    uint8_t oh = drum_seqArr[num][3][s];
                    vals[r] = (hh > oh) ? hh : oh;
                    continue;
                }
                case 3: seqRow = 4; break;
                case 4: seqRow = 5; break;
                case 5: seqRow = 6; break;
                case 6: seqRow = 7; break;
                case 7: seqRow = 8; break;
                default: seqRow = 0; break;
            }
            vals[r] = drum_seqArr[num][seqRow][s];
        }
    }

    for (int r = 0; r < DRUM_MIX_COUNT; r++) {
        if (vals[r] > 0) {
            drum_mix_meter_value[r] = (vals[r] * 100.0f) / 3.0f;
            drum_mix_meter_hold[r]  = drum_meter_hold_ticks;
        }
    }
}

void drum_mix_meters_reset() {
    for (int r = 0; r < DRUM_MIX_COUNT; r++) {
        drum_mix_meter_value[r] = 0.0f;
        drum_mix_meter_hold[r]  = 0;
        if (drum_mix_meter[r] && lv_obj_is_valid(drum_mix_meter[r]))
            lv_slider_set_value(drum_mix_meter[r], 0, LV_ANIM_OFF);
    }
}


static lv_obj_t* drum_mix_slider_create(lv_obj_t *parent, int idx,
                                        int x, int y, int w, int h) {
    const int METER_H = 24;
    const int LBL_H   = 22;
    const int GAP     = 4;
    const int CONT_H  = METER_H + GAP + h + GAP + LBL_H;

    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_size(c, w, CONT_H);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_set_style_pad_all(c, 0, 0);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);

    // ---- Micro meter (top) ----
    lv_obj_t *mt = lv_slider_create(c);
    lv_obj_set_size(mt, 22, METER_H);
    lv_obj_align(mt, LV_ALIGN_TOP_MID, 0, 0);
    lv_slider_set_range(mt, 0, 100);
    lv_slider_set_value(mt, 0, LV_ANIM_OFF);
    lv_obj_set_style_radius(mt, 0, 0);
    lv_obj_set_style_radius(mt, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(mt, 0, LV_PART_INDICATOR);
    lv_obj_set_style_radius(mt, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_all(mt, 0, 0);
    lv_obj_set_style_pad_all(mt, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(mt, 0, LV_PART_INDICATOR);
    lv_obj_set_style_pad_all(mt, 0, LV_PART_KNOB);
    lv_obj_set_style_width(mt, 0, LV_PART_KNOB);
    lv_obj_set_style_height(mt, 0, LV_PART_KNOB);
    lv_obj_set_style_bg_img_src(mt, &img_micro_meter_audio_track, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(mt, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(mt, &img_micro_meter_audio_indicator, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(mt, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_src(mt, NULL, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(mt, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_width(mt, 0, LV_PART_KNOB);
    lv_obj_set_style_outline_width(mt, 0, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(mt, 0, LV_PART_KNOB);
    lv_obj_clear_flag(mt, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(mt, LV_OBJ_FLAG_SCROLLABLE);

    drum_mix_meter[idx] = mt;

    // ---- Slider (center) ----
    lv_obj_t *s = lv_slider_create(c);
    lv_obj_set_size(s, w, h);
    lv_obj_align_to(s, mt, LV_ALIGN_OUT_BOTTOM_MID, 0, GAP);
    lv_slider_set_range(s, 0, 20);
    lv_slider_set_value(s, drum_mix_val[idx], LV_ANIM_OFF);

    lv_obj_set_style_bg_img_src(s, &img_slider_track, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_src(s, &img_slider_indicator, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor(s,
        lv_color_hex(drum_mix_colors[idx]), LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor_opa(s,
        (sliderColorDepth * 255) / 100, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_src(s, &img_slider_knob, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_width(s, 45, LV_PART_KNOB);
    lv_obj_set_style_height(s, 30, LV_PART_KNOB);

    drum_mix_slider[idx] = s;

    // ---- Label nome (bottom) ----
    lv_obj_t *lbl = lv_label_create(c);
    lv_label_set_text(lbl, drum_mix_names[idx]);
    lv_obj_set_style_text_color(lbl,
        lv_color_hex(drum_mix_colors[idx]), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_add_event_cb(s, drum_mix_slider_cb, LV_EVENT_VALUE_CHANGED,
                        (void*)(uintptr_t)idx);
    return s;
}


void drum_mix_create(lv_obj_t *parent) {
    // ---- Frame DRUM MIX (occupa tutto lo spazio disponibile) ----
    //   Sinistra: 10px margine
    //   Destra:   fino a SWING (x=730), quindi 720 = limite frame
    //   Top:      y=70 (sotto monitor e BPM)
    //   Bottom:   y=355 (5px sopra la bottom row a y=360)
	
	    for (int i = 0; i < DRUM_MIX_COUNT; i++) drum_mix_meter[i] = nullptr;
		
    const int FR_X = 10;
    const int FR_Y = 70;
    const int FR_W = 710;
    const int FR_H = 285;

    lv_obj_t *frame = lv_obj_create(parent);
    lv_obj_set_size(frame, FR_W, FR_H);
    lv_obj_set_pos(frame, FR_X, FR_Y);
    lv_obj_set_style_bg_opa(frame, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(frame, 1, 0);
    lv_obj_set_style_border_color(frame, lv_color_hex(0x888888), 0);
    lv_obj_set_style_radius(frame, 6, 0);
    lv_obj_set_style_pad_all(frame, 10, 0);
    lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(frame, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_clip_corner(frame, 0, 0);

    // Label "DRUM MIX" sopra il bordo
    lv_obj_t *fr_lbl = lv_label_create(frame);
    lv_label_set_text(fr_lbl, "DRUM MIX");
    lv_obj_set_style_text_color(fr_lbl, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(fr_lbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_bg_color(fr_lbl, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(fr_lbl, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(fr_lbl, 8, 0);
    lv_obj_align(fr_lbl, LV_ALIGN_TOP_MID, 0, -22);

    // ---- 8 slider equidistanti ----
    //   Content area (dopo pad=10): 690 x 265
    //   8 slider da 60px + 7 gap da 30px = 690 (fit perfetto)
       const int SL_W = 60;
    const int SL_H = 180;   // era 200 (ridotto per far stare il meter sopra)
    const int N    = DRUM_MIX_COUNT;
    const int INNER_W = FR_W - 20;
    const int GAP = (INNER_W - N * SL_W) / (N - 1);
    const int START_X = 0;
    const int CONT_H = 24 + 4 + SL_H + 4 + 22;   // meter + slider + nome = 234
    const int START_Y = (FR_H - 20 - CONT_H) / 2 - 1;

    for (int i = 0; i < N; i++) {
        int x = START_X + i * (SL_W + GAP);
        drum_mix_slider_create(frame, i, x, START_Y, SL_W, SL_H);
    }
	    drum_mix_meters_reset();
}
//fine drum mix crate

