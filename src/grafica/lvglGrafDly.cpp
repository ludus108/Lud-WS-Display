// ============================================================
// lvglGrafDly.cpp — DLY A / DLY B editor
// ============================================================
#include "globals.h"
#include "lvglGraf.h"
#include "lvglGraf_internal.h"
#include "lvgl_v8_port.h"
#include "src/preset/preset_ui.h"
#include <math.h>

// ============================================================
// Costanti
// ============================================================
#define DLY_ARC_PARAMS     8    // parametri rappresentati da arc
#define DLY_PARAM_COUNT    10   // 8 arc + 2 bottoni (VCF mode, PRE/POST)
#define DLY_PRESET_COUNT   16

// Indici parametri in dly*_arr
enum {
    DLY_TIME = 0,
    DLY_FB,
    DLY_CUT,
    DLY_RES,
    DLY_VCFLFO_DEPTH,
    DLY_VCFLFO_RATE,
    DLY_LFO_DEPTH,
    DLY_LFO_RATE,
    DLY_VCF_MODE,     // 0=LP, 1=BP
    DLY_PREPOST       // 0=PRE, 1=POST
};

// Colori bottoni
#define DLY_LP_COLOR     0x0088FF
#define DLY_BP_COLOR     0xFF2222
#define DLY_PRE_COLOR    0x00AAFF
#define DLY_POST_COLOR   0xFF8800

// Frame "DLY" (top): TIME, FB, VCFLFO_depth, VCFLFO_rate
static const int   dly_frameDly_idx[4]  = { DLY_TIME, DLY_FB, DLY_VCFLFO_DEPTH, DLY_VCFLFO_RATE };
static const char *dly_frameDly_name[4] = { "TIME", "FB", "LFO", "RATE" };

// Frame "VCF" (bottom): CUT, RES, LFO_depth, LFO_rate
static const int   dly_frameVcf_idx[4]  = { DLY_CUT, DLY_RES, DLY_LFO_DEPTH, DLY_LFO_RATE };
static const char *dly_frameVcf_name[4] = { "CUT", "RES", "LFO", "RATE" };

// ============================================================
// Array preset (10 parametri × 16 preset)
// ============================================================
uint8_t dlyA_arr[DLY_PARAM_COUNT][DLY_PRESET_COUNT] = {
    { 50, 30, 70, 20, 60, 40, 80, 25, 55, 35, 65, 45, 75, 15, 85,  5 }, // TIME
    { 30, 50, 20, 70, 40, 60, 25, 75, 35, 55, 15, 65, 45, 80, 10, 90 }, // FB
    { 80, 70, 90, 60, 85, 75, 95, 55, 82, 72, 92, 62, 87, 77, 97, 52 }, // CUT
    { 20, 30, 10, 40, 25, 35, 15, 45, 22, 32, 12, 42, 27, 37, 17, 47 }, // RES
    { 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50 }, // VCF LFO depth
    { 40, 45, 35, 50, 42, 47, 37, 52, 44, 49, 39, 54, 46, 51, 41, 56 }, // VCF LFO rate
    { 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50 }, // LFO depth
    { 40, 45, 35, 50, 42, 47, 37, 52, 44, 49, 39, 54, 46, 51, 41, 56 }, // LFO rate
    {  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1 }, // VCF mode  LP/BP
    {  0,  0,  1,  1,  0,  0,  1,  1,  0,  0,  1,  1,  0,  0,  1,  1 }  // PRE/POST
};

uint8_t dlyB_arr[DLY_PARAM_COUNT][DLY_PRESET_COUNT] = {
    { 50, 30, 70, 20, 60, 40, 80, 25, 55, 35, 65, 45, 75, 15, 85,  5 },
    { 30, 50, 20, 70, 40, 60, 25, 75, 35, 55, 15, 65, 45, 80, 10, 90 },
    { 80, 70, 90, 60, 85, 75, 95, 55, 82, 72, 92, 62, 87, 77, 97, 52 },
    { 20, 30, 10, 40, 25, 35, 15, 45, 22, 32, 12, 42, 27, 37, 17, 47 },
    { 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50 },
    { 40, 45, 35, 50, 42, 47, 37, 52, 44, 49, 39, 54, 46, 51, 41, 56 },
    { 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50 },
    { 40, 45, 35, 50, 42, 47, 37, 52, 44, 49, 39, 54, 46, 51, 41, 56 },
    {  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1 },
    {  0,  0,  1,  1,  0,  0,  1,  1,  0,  0,  1,  1,  0,  0,  1,  1 }
};

// ============================================================
// Stato runtime
// ============================================================
static bool    dly_is_A       = true;
static uint8_t dly_cur_preset = 0;

static lv_obj_t *dly_dd            = nullptr;
static lv_obj_t *dly_arc           [DLY_ARC_PARAMS] = {nullptr};
static lv_obj_t *dly_arc_dot       [DLY_ARC_PARAMS] = {nullptr};
static lv_obj_t *dly_arc_lbl       [DLY_ARC_PARAMS] = {nullptr};
static lv_obj_t *dly_arc_val_lbl   [DLY_ARC_PARAMS] = {nullptr};
static int       dly_arc_last      [DLY_ARC_PARAMS] = {0};
static uint8_t   dly_arc_target    [DLY_ARC_PARAMS] = {50,50,50,50,50,50,50,50};

static lv_obj_t *dly_vcfmode_btn   = nullptr;
static lv_obj_t *dly_vcfmode_lbl   = nullptr;
static lv_obj_t *dly_prepost_btn   = nullptr;
static lv_obj_t *dly_prepost_lbl   = nullptr;

// ============================================================
// Accesso array
// ============================================================
static inline uint8_t* dly_get_arr_flat() {
    return dly_is_A ? &dlyA_arr[0][0] : &dlyB_arr[0][0];
}
static inline uint8_t& dly_arr_at(uint8_t *arr, int param, int preset) {
    return arr[param * DLY_PRESET_COUNT + preset];
}

// ============================================================
// Aggiorna visuale bottoni (mode + prepost)
// ============================================================
static void dly_update_vcfmode_visual() {
    uint8_t *arr = dly_get_arr_flat();
    int v = dly_arr_at(arr, DLY_VCF_MODE, dly_cur_preset);
    v = (v != 0) ? 1 : 0;

    if (dly_vcfmode_lbl && lv_obj_is_valid(dly_vcfmode_lbl))
        lv_label_set_text(dly_vcfmode_lbl, v == 0 ? "LP" : "BP");
    if (dly_vcfmode_btn && lv_obj_is_valid(dly_vcfmode_btn))
        lv_obj_set_style_border_color(dly_vcfmode_btn,
            lv_color_hex(v == 0 ? DLY_LP_COLOR : DLY_BP_COLOR), 0);
}

static void dly_update_prepost_visual() {
    uint8_t *arr = dly_get_arr_flat();
    int v = dly_arr_at(arr, DLY_PREPOST, dly_cur_preset);
    v = (v != 0) ? 1 : 0;

    if (dly_prepost_lbl && lv_obj_is_valid(dly_prepost_lbl))
        lv_label_set_text(dly_prepost_lbl, v == 0 ? "PRE" : "POST");
    if (dly_prepost_btn && lv_obj_is_valid(dly_prepost_btn))
        lv_obj_set_style_border_color(dly_prepost_btn,
            lv_color_hex(v == 0 ? DLY_PRE_COLOR : DLY_POST_COLOR), 0);
}

// ============================================================
// Callback bottoni
// ============================================================
static void dly_vcfmode_cb(lv_event_t *e) {
    (void)e;
    uint8_t *arr = dly_get_arr_flat();
    uint8_t &v = dly_arr_at(arr, DLY_VCF_MODE, dly_cur_preset);
    v = (v == 0) ? 1 : 0;
    dly_update_vcfmode_visual();
}

static void dly_prepost_cb(lv_event_t *e) {
    (void)e;
    uint8_t *arr = dly_get_arr_flat();
    uint8_t &v = dly_arr_at(arr, DLY_PREPOST, dly_cur_preset);
    v = (v == 0) ? 1 : 0;
    dly_update_prepost_visual();
}

// ============================================================
// Callback arc
// ============================================================
struct DlyArcCtx { int param_idx; };
static DlyArcCtx dly_arc_ctx[DLY_ARC_PARAMS];

static void dly_arc_cb(lv_event_t *e) {
    lv_obj_t *arc = lv_event_get_target(e);
    DlyArcCtx *ctx = (DlyArcCtx*)lv_event_get_user_data(e);
    if (!ctx) return;
    int p = ctx->param_idx;
    if (p < 0 || p >= DLY_ARC_PARAMS) return;

    int cur    = lv_arc_get_value(arc);
    int prev   = dly_arc_last[p];
    int target = dly_arc_target[p];

    if (dly_arc_val_lbl[p] && lv_obj_is_valid(dly_arc_val_lbl[p])) {
        lv_label_set_text_fmt(dly_arc_val_lbl[p], "%d", cur);
        if (dly_arc[p] && lv_obj_is_valid(dly_arc[p])) {
            lv_obj_update_layout(dly_arc_val_lbl[p]);
            lv_obj_align_to(dly_arc_val_lbl[p], dly_arc[p],
                            LV_ALIGN_OUT_BOTTOM_MID, 0, -8);
        }
    }

    if (dly_arc_dot[p] && lv_obj_is_valid(dly_arc_dot[p])) {
        bool crossed = (prev < target && cur >= target) ||
                       (prev > target && cur <= target) ||
                       (prev == target && cur != target);
        if (crossed) {
            lv_obj_add_flag(dly_arc_dot[p], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_arc_color(arc,
                lv_color_hex(COLOR_ARC_PASSED), LV_PART_INDICATOR);
        } else {
            lv_obj_clear_flag(dly_arc_dot[p], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_arc_color(arc,
                lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
        }
    }
    dly_arc_last[p] = cur;

    uint8_t *arr = dly_get_arr_flat();
    dly_arr_at(arr, p, dly_cur_preset) = (uint8_t)cur;
}

// ============================================================
// Creazione arc
// ============================================================
static void dly_arc_create(lv_obj_t *parent, int param_idx,
                            int x, int y, int w, int h,
                            const char *pname) {
    if (param_idx < 0 || param_idx >= DLY_ARC_PARAMS) return;

    uint8_t *arr = dly_get_arr_flat();
    int initVal = dly_arr_at(arr, param_idx, dly_cur_preset);
    if (initVal < 0)   initVal = 0;
    if (initVal > 100) initVal = 100;

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
    dly_arc[param_idx] = arc;

    lv_obj_t *plabel = lv_label_create(parent);
    lv_label_set_text(plabel, pname);
    lv_obj_set_style_text_color(plabel, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_style_text_font (plabel, &lv_font_montserrat_20, 0);
    lv_obj_align_to(plabel, arc, LV_ALIGN_CENTER, 0, 0);
    dly_arc_lbl[param_idx] = plabel;

    lv_obj_t *val_lbl = lv_label_create(parent);
    lv_obj_set_style_text_color(val_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font (val_lbl, &lv_font_montserrat_20, 0);
    lv_label_set_text_fmt(val_lbl, "%d", initVal);
    lv_obj_align_to(val_lbl, arc, LV_ALIGN_OUT_BOTTOM_MID, 0, -8);
    dly_arc_val_lbl[param_idx] = val_lbl;

    float angle = 135.0f + (dly_arc_target[param_idx] * 2.7f);
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
    dly_arc_dot[param_idx] = dot;

    if (initVal > dly_arc_target[param_idx]) {
        lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_arc_color(arc,
            lv_color_hex(COLOR_ARC_PASSED), LV_PART_INDICATOR);
    } else {
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_arc_color(arc,
            lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
    }

    dly_arc_last[param_idx] = initVal;

    dly_arc_ctx[param_idx].param_idx = param_idx;
    lv_obj_add_event_cb(arc, dly_arc_cb, LV_EVENT_VALUE_CHANGED,
                        &dly_arc_ctx[param_idx]);
}

// ============================================================
// Applica preset corrente
// ============================================================
static void dly_apply_preset() {
    uint8_t *arr = dly_get_arr_flat();

    for (int p = 0; p < DLY_ARC_PARAMS; p++) {
        if (!dly_arc[p] || !lv_obj_is_valid(dly_arc[p])) continue;
        int v = dly_arr_at(arr, p, dly_cur_preset);
        if (v > 100) v = 100;

        lv_arc_set_value(dly_arc[p], v);
        dly_arc_last[p] = v;

        if (dly_arc_val_lbl[p] && lv_obj_is_valid(dly_arc_val_lbl[p])) {
            lv_label_set_text_fmt(dly_arc_val_lbl[p], "%d", v);
        }

        if (dly_arc_dot[p] && lv_obj_is_valid(dly_arc_dot[p])) {
            if (v >= dly_arc_target[p]) {
                lv_obj_add_flag(dly_arc_dot[p], LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_style_arc_color(dly_arc[p],
                    lv_color_hex(COLOR_ARC_PASSED), LV_PART_INDICATOR);
            } else {
                lv_obj_clear_flag(dly_arc_dot[p], LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_style_arc_color(dly_arc[p],
                    lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
            }
        }
    }

    dly_update_vcfmode_visual();
    dly_update_prepost_visual();
}

// ============================================================
// Dropdown preset
// ============================================================
static void dly_dd_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    int sel = lv_dropdown_get_selected(dd);
    if (sel < 0 || sel >= DLY_PRESET_COUNT) return;
    dly_cur_preset = (uint8_t)sel;
    dly_apply_preset();
}

// ============================================================
// Reset puntatori
// ============================================================
void dly_reset_pointers() {
    dly_dd = nullptr;
    for (int p = 0; p < DLY_ARC_PARAMS; p++) {
        dly_arc[p]         = nullptr;
        dly_arc_dot[p]     = nullptr;
        dly_arc_lbl[p]     = nullptr;
        dly_arc_val_lbl[p] = nullptr;
        dly_arc_last[p]    = 0;
    }
    dly_vcfmode_btn  = nullptr;
    dly_vcfmode_lbl  = nullptr;
    dly_prepost_btn  = nullptr;
    dly_prepost_lbl  = nullptr;
}

// ============================================================
// Pagina DLY
// ============================================================
void dly_page_create(lv_obj_t *parent, bool isA) {
    dly_is_A = isA;

    // ---- Dropdown preset ----
    static const char *dd_opts_A =
        "DLY A 1\nDLY A 2\nDLY A 3\nDLY A 4\nDLY A 5\nDLY A 6\nDLY A 7\nDLY A 8\n"
        "DLY A 9\nDLY A 10\nDLY A 11\nDLY A 12\nDLY A 13\nDLY A 14\nDLY A 15\nDLY A 16";
    static const char *dd_opts_B =
        "DLY B 1\nDLY B 2\nDLY B 3\nDLY B 4\nDLY B 5\nDLY B 6\nDLY B 7\nDLY B 8\n"
        "DLY B 9\nDLY B 10\nDLY B 11\nDLY B 12\nDLY B 13\nDLY B 14\nDLY B 15\nDLY B 16";

    dly_dd = styled_dropdown(parent, 10, 5, 130, 45,
                             0xFF8800,
                             &lv_font_montserrat_16,
                             isA ? dd_opts_A : dd_opts_B);
    lv_dropdown_set_selected(dly_dd, dly_cur_preset);
    lv_obj_add_event_cb(dly_dd, dly_dd_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Reset puntatori (sicurezza)
    for (int p = 0; p < DLY_ARC_PARAMS; p++) {
        dly_arc[p]         = nullptr;
        dly_arc_dot[p]     = nullptr;
        dly_arc_lbl[p]     = nullptr;
        dly_arc_val_lbl[p] = nullptr;
    }

    // ---- Geometria ----
    const int FR_X = 40;
    const int FR_W = 720;
    const int FR_H = 150;
    const int FR1_Y = 65;
    const int FR2_Y = 230;

    // 5 elementi per frame (1 bottone + 4 arc), equidistanti
    //   Totale elementi = 5 * 80 = 400
    //   Gap (6 spazi uguali): (720 - 400) / 6 = 53.33
    const int ELEM_W  = 80;
    const int ELEM_H  = 80;
    const int ELEM_X0 = 53;
    const int ELEM_DX = 133;   // 80 + 53
    const int ELEM_Y  = 30;    // offset verticale nel frame

    // ========================================
    // Frame "DLY" (top)
    // ========================================
    lv_obj_t *frame1 = lv_obj_create(parent);
    lv_obj_set_size(frame1, FR_W, FR_H);
    lv_obj_set_pos(frame1, FR_X, FR1_Y);
    lv_obj_set_style_bg_opa(frame1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(frame1, 1, 0);
    lv_obj_set_style_border_color(frame1, lv_color_hex(0x888888), 0);
    lv_obj_set_style_radius(frame1, 6, 0);
    lv_obj_set_style_pad_all(frame1, 0, 0);
    lv_obj_clear_flag(frame1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(frame1, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_clip_corner(frame1, 0, 0);

    lv_obj_t *l1 = lv_label_create(frame1);
    lv_label_set_text(l1, "DLY");
    lv_obj_set_style_text_color(l1, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(l1, &lv_font_montserrat_18, 0);
    lv_obj_set_style_bg_color(l1, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(l1, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(l1, 8, 0);
    lv_obj_align(l1, LV_ALIGN_TOP_MID, 0, -20);

    // Bottone PRE/POST (elemento 0)
    dly_prepost_btn = mkbtn(frame1, ELEM_X0, ELEM_Y, ELEM_W, ELEM_H,
                             DLY_PRE_COLOR, "PRE",
                             &lv_font_montserrat_20,
                             dly_prepost_cb, 0, 6, 3);
    dly_prepost_lbl = lv_obj_get_child(dly_prepost_btn, 0);

    // 4 arc (elementi 1..4)
    for (int i = 0; i < 4; i++) {
        int px = ELEM_X0 + (i + 1) * ELEM_DX;
        dly_arc_create(frame1, dly_frameDly_idx[i],
                       px, ELEM_Y, ELEM_W, ELEM_H,
                       dly_frameDly_name[i]);
    }

    // ========================================
    // Frame "VCF" (bottom)
    // ========================================
    lv_obj_t *frame2 = lv_obj_create(parent);
    lv_obj_set_size(frame2, FR_W, FR_H);
    lv_obj_set_pos(frame2, FR_X, FR2_Y);
    lv_obj_set_style_bg_opa(frame2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(frame2, 1, 0);
    lv_obj_set_style_border_color(frame2, lv_color_hex(0x888888), 0);
    lv_obj_set_style_radius(frame2, 6, 0);
    lv_obj_set_style_pad_all(frame2, 0, 0);
    lv_obj_clear_flag(frame2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(frame2, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_clip_corner(frame2, 0, 0);

    lv_obj_t *l2 = lv_label_create(frame2);
    lv_label_set_text(l2, "VCF");
    lv_obj_set_style_text_color(l2, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(l2, &lv_font_montserrat_18, 0);
    lv_obj_set_style_bg_color(l2, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(l2, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(l2, 8, 0);
    lv_obj_align(l2, LV_ALIGN_TOP_MID, 0, -20);

    // Bottone LP/BP (elemento 0)
    dly_vcfmode_btn = mkbtn(frame2, ELEM_X0, ELEM_Y, ELEM_W, ELEM_H,
                             DLY_LP_COLOR, "LP",
                             &lv_font_montserrat_20,
                             dly_vcfmode_cb, 0, 6, 3);
    dly_vcfmode_lbl = lv_obj_get_child(dly_vcfmode_btn, 0);

    // 4 arc (elementi 1..4)
    for (int i = 0; i < 4; i++) {
        int px = ELEM_X0 + (i + 1) * ELEM_DX;
        dly_arc_create(frame2, dly_frameVcf_idx[i],
                       px, ELEM_Y, ELEM_W, ELEM_H,
                       dly_frameVcf_name[i]);
    }

    // Applica preset corrente (aggiorna arc + bottoni)
    dly_apply_preset();
}