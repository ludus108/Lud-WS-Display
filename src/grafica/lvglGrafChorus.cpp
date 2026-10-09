// ============================================================
// lvglGrafChorus.cpp — CHORUS editor
// ============================================================
// Target MCU per LWS: 'M' (Lud-WS-Mod)
// Array: chorus_arr[8 preset][12 params]
// ============================================================
#include "globals.h"
#include "lvglGraf.h"
#include "lvglGraf_internal.h"
#include "lvgl_v8_port.h"
#include "src/preset/preset_ui.h"
#include <math.h>

#define CHORUS_PRESETS   8
#define CHORUS_PARAMS    12

enum {
    CRS_MODE = 0,       // 0=SOL, 1=EQ
    CRS_OFFSET,
    CRS_LFO_SLOW_TIME,
    CRS_LFO_SLOW_GAIN,
    CRS_LFO_FAST_TIME,
    CRS_LFO_FAST_GAIN,
    CRS_EQ_220,
    CRS_EQ_510,
    CRS_EQ_730,
    CRS_EQ_1000,
    CRS_EQ_3500,
    CRS_EQ_6000
};

#define CRS_SOL_COLOR     0x00AA00
#define CRS_EQ_MODE_COLOR 0xFF6600
#define CRS_OFFSET_ACTIVE 0xAA44FF   // viola vivo
#define CRS_OFFSET_PASSED 0x442266   // viola spento
#define CRS_LBL_ACTIVE    0xFFA500   // arancione (come VCA)
#define CRS_LBL_PASSED    0xFFFFFF   // bianco (come VCA)

static const uint32_t chorus_eq_colors[6] = {
    0xFF0000, 0xFF8800, 0xFFDD00, 0x88DD00, 0x00DDAA, 0x00AAFF
};

// ============================================================
// Array [8][12]
// ============================================================
uint8_t chorus_arr[CHORUS_PRESETS][CHORUS_PARAMS] = {
//  MOD  OFF  SLT  SLG  FST  FSG  E1   E2   E3   E4   E5   E6
    { 0,  50,  30,  60,  70,  40,  50,  50,  50,  50,  50,  50 },
    { 1,  50,  35,  65,  75,  45,  50,  50,  50,  50,  50,  50 },
    { 0,  50,  40,  70,  80,  50,  50,  50,  50,  50,  50,  50 },
    { 1,  50,  45,  75,  85,  55,  50,  50,  50,  50,  50,  50 },
    { 0,  50,  30,  60,  70,  40,  50,  50,  50,  50,  50,  50 },
    { 1,  50,  35,  65,  75,  45,  50,  50,  50,  50,  50,  50 },
    { 0,  50,  40,  70,  80,  50,  50,  50,  50,  50,  50,  50 },
    { 1,  50,  45,  75,  85,  55,  50,  50,  50,  50,  50,  50 }
};

// ============================================================
// Stato
// ============================================================
static uint8_t chorus_cur_preset = 0;

static lv_obj_t *chorus_dd       = nullptr;
static lv_obj_t *chorus_mode_btn = nullptr;
static lv_obj_t *chorus_mode_lbl = nullptr;

static lv_obj_t *chorus_arc[4]     = {nullptr,nullptr,nullptr,nullptr};
static lv_obj_t *chorus_arc_lbl[4] = {nullptr,nullptr,nullptr,nullptr};
static lv_obj_t *chorus_arc_val[4] = {nullptr,nullptr,nullptr,nullptr};
static int       chorus_arc_last[4]= {0,0,0,0};

static lv_obj_t *chorus_eq_slider[6]  = {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};
static lv_obj_t *chorus_eq_val_lbl[6] = {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};

static lv_obj_t *chorus_offset_slider   = nullptr;
static lv_obj_t *chorus_offset_val_lbl  = nullptr;
static lv_obj_t *chorus_offset_name_lbl = nullptr;   // NUOVO
static lv_obj_t *chorus_offset_arrow    = nullptr;
static int       chorus_offset_last     = 50;
static int       chorus_offset_target   = 50;
static bool      chorus_offset_crossed  = false;      // NUOVO

// ============================================================
// Mode button SOL/EQ + visibilità EQ
// ============================================================
static void chorus_update_mode_visual() {
    int v = chorus_arr[chorus_cur_preset][CRS_MODE];
    v = (v != 0) ? 1 : 0;
    if (chorus_mode_lbl && lv_obj_is_valid(chorus_mode_lbl))
        lv_label_set_text(chorus_mode_lbl, v == 0 ? "SOL" : "EQ");
    if (chorus_mode_btn && lv_obj_is_valid(chorus_mode_btn))
        lv_obj_set_style_border_color(chorus_mode_btn,
            lv_color_hex(v == 0 ? CRS_SOL_COLOR : CRS_EQ_MODE_COLOR), 0);
}

// Nasconde/mostra indicator + knob + touch dei 6 slider EQ.
//   SOL (mode=0) → nascosti, no touch
//   EQ  (mode=1) → visibili, touch attivo
static void chorus_update_eq_visibility() {
    int mode = chorus_arr[chorus_cur_preset][CRS_MODE];
    bool eq_on = (mode != 0);

    for (int i = 0; i < 6; i++) {
        lv_obj_t *s = chorus_eq_slider[i];
        if (!s || !lv_obj_is_valid(s)) continue;

        if (eq_on) {
            // Indicator
            lv_obj_set_style_bg_img_src(s, &img_slider_indicator,
                                        LV_PART_INDICATOR);
            lv_obj_set_style_bg_img_recolor(s,
                lv_color_hex(chorus_eq_colors[i]), LV_PART_INDICATOR);
            lv_obj_set_style_bg_img_recolor_opa(s,
                (sliderColorDepth * 255) / 100, LV_PART_INDICATOR);
            // Knob
            lv_obj_set_style_bg_img_src(s, &img_slider_knob, LV_PART_KNOB);
            lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_KNOB);
            lv_obj_set_style_width (s, 40, LV_PART_KNOB);
            lv_obj_set_style_height(s, 20, LV_PART_KNOB);
            // Touch
            lv_obj_add_flag(s, LV_OBJ_FLAG_CLICKABLE);
        } else {
            // Indicator off
            lv_obj_set_style_bg_img_src(s, NULL, LV_PART_INDICATOR);
            // Knob off
            lv_obj_set_style_bg_img_src(s, NULL, LV_PART_KNOB);
            lv_obj_set_style_width (s, 0, LV_PART_KNOB);
            lv_obj_set_style_height(s, 0, LV_PART_KNOB);
            // No touch
            lv_obj_clear_flag(s, LV_OBJ_FLAG_CLICKABLE);
        }
    }
}

static void chorus_mode_cb(lv_event_t *e) {
    (void)e;
    uint8_t &v = chorus_arr[chorus_cur_preset][CRS_MODE];
    v = (v == 0) ? 1 : 0;
    chorus_update_mode_visual();
    chorus_update_eq_visibility();
    // TODO: LWS send to MCU 'M'
}

// ============================================================
// Arc (senza pallino target)
// ============================================================
struct ChorusArcCtx { int idx; };
static ChorusArcCtx chorus_arc_ctx[4];

static void chorus_arc_cb(lv_event_t *e) {
    lv_obj_t *arc = lv_event_get_target(e);
    ChorusArcCtx *ctx = (ChorusArcCtx*)lv_event_get_user_data(e);
    if (!ctx) return;
    int i = ctx->idx;
    if (i < 0 || i > 3) return;

    int cur = lv_arc_get_value(arc);
    if (chorus_arc_val[i] && lv_obj_is_valid(chorus_arc_val[i])) {
        lv_label_set_text_fmt(chorus_arc_val[i], "%d", cur);
        if (chorus_arc[i] && lv_obj_is_valid(chorus_arc[i])) {
            lv_obj_update_layout(chorus_arc_val[i]);
            lv_obj_align_to(chorus_arc_val[i], chorus_arc[i],
                            LV_ALIGN_OUT_BOTTOM_MID, 0, -8);
        }
    }
    chorus_arc_last[i] = cur;
    chorus_arr[chorus_cur_preset][CRS_LFO_SLOW_TIME + i] = (uint8_t)cur;
    // TODO: LWS send to MCU 'M'
}

static void chorus_arc_create(lv_obj_t *parent, int i,
                               int x, int y, int w, int h,
                               const char *name) {
    if (i < 0 || i > 3) return;

    int param = CRS_LFO_SLOW_TIME + i;
    int initVal = chorus_arr[chorus_cur_preset][param];
    if (initVal < 0) initVal = 0;
    if (initVal > 100) initVal = 100;

    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, w, h);
    lv_obj_set_pos(arc, x, y);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, initVal);

    lv_obj_set_style_arc_img_src(arc, &img_arc_bg,    LV_PART_MAIN);
    lv_obj_set_style_arc_img_src(arc, &img_arc_indic, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color  (arc, lv_color_hex(0x00AAFF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa     (arc, LV_OPA_TRANSP,  LV_PART_KNOB);
    chorus_arc[i] = arc;

    lv_obj_t *plabel = lv_label_create(parent);
    lv_label_set_text(plabel, name);
    lv_obj_set_style_text_color(plabel, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_style_text_font (plabel, &lv_font_montserrat_14, 0);
    lv_obj_align_to(plabel, arc, LV_ALIGN_CENTER, 0, 0);
    chorus_arc_lbl[i] = plabel;

    lv_obj_t *val_lbl = lv_label_create(parent);
    lv_obj_set_style_text_color(val_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font (val_lbl, &lv_font_montserrat_18, 0);
    lv_label_set_text_fmt(val_lbl, "%d", initVal);
    lv_obj_align_to(val_lbl, arc, LV_ALIGN_OUT_BOTTOM_MID, 0, -8);
    chorus_arc_val[i] = val_lbl;

    chorus_arc_last[i] = initVal;

    chorus_arc_ctx[i].idx = i;
    lv_obj_add_event_cb(arc, chorus_arc_cb, LV_EVENT_VALUE_CHANGED,
                        &chorus_arc_ctx[i]);
}

// ============================================================
// Slider EQ (senza freccina, compatto)
// ============================================================
struct ChorusEqCtx { int idx; };
static ChorusEqCtx chorus_eq_ctx[6];

static void chorus_eq_cb(lv_event_t *e) {
    lv_obj_t *s = lv_event_get_target(e);
    ChorusEqCtx *ctx = (ChorusEqCtx*)lv_event_get_user_data(e);
    if (!ctx) return;
    int i = ctx->idx;
    if (i < 0 || i > 5) return;

    int v = lv_slider_get_value(s);
    if (chorus_eq_val_lbl[i] && lv_obj_is_valid(chorus_eq_val_lbl[i]))
        lv_label_set_text_fmt(chorus_eq_val_lbl[i], "%d", v);

    chorus_arr[chorus_cur_preset][CRS_EQ_220 + i] = (uint8_t)v;
    // TODO: LWS send to MCU 'M'
}

static void chorus_eq_create(lv_obj_t *parent, int i,
                              int x, int y, int w, int h,
                              const char *name, uint32_t col) {
    const int LBL_H = 20;
    const int CONT_H = LBL_H + 4 + h + 4 + LBL_H;

    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_size(c, w, CONT_H);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_set_style_pad_all(c, 0, 0);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(c);
    lv_label_set_text(lbl, name);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *s = lv_slider_create(c);
    lv_obj_set_size(s, w, h);
    lv_obj_align(s, LV_ALIGN_CENTER, 0, 0);
    lv_slider_set_range(s, 0, 100);
    int initVal = chorus_arr[chorus_cur_preset][CRS_EQ_220 + i];
    if (initVal > 100) initVal = 100;
    lv_slider_set_value(s, initVal, LV_ANIM_OFF);

    lv_obj_set_style_bg_img_src(s, &img_slider_track, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_src(s, &img_slider_indicator, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor(s, lv_color_hex(col), LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor_opa(s,
        (sliderColorDepth * 255) / 100, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_src(s, &img_slider_knob, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_width(s, 40, LV_PART_KNOB);
    lv_obj_set_style_height(s, 20, LV_PART_KNOB);

    chorus_eq_slider[i] = s;

    lv_obj_t *vlbl = lv_label_create(c);
    lv_obj_set_style_text_color(vlbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(vlbl, &lv_font_montserrat_14, 0);
    lv_label_set_text_fmt(vlbl, "%d", initVal);
    lv_obj_align(vlbl, LV_ALIGN_BOTTOM_MID, 0, 0);
    chorus_eq_val_lbl[i] = vlbl;

    chorus_eq_ctx[i].idx = i;
    lv_obj_add_event_cb(s, chorus_eq_cb, LV_EVENT_VALUE_CHANGED,
                        &chorus_eq_ctx[i]);
}

// ============================================================
// Offset slider (con freccina, comportamento come VCA)
// ============================================================
static void chorus_offset_cb(lv_event_t *e) {
    lv_obj_t *s = lv_event_get_target(e);
    int cur = lv_slider_get_value(s);

    if (chorus_offset_val_lbl && lv_obj_is_valid(chorus_offset_val_lbl))
        lv_label_set_text_fmt(chorus_offset_val_lbl, "%d", cur);

    // Logica freccina identica a eslider() di VCA
    if (chorus_offset_arrow && lv_obj_is_valid(chorus_offset_arrow)) {
        if (!chorus_offset_crossed) {
            int prev   = chorus_offset_last;
            int target = chorus_offset_target;
            bool crossed = (prev < target && cur > target) ||
                           (prev > target && cur < target) ||
                           (prev == target && cur != target);
            if (crossed) {
                chorus_offset_crossed = true;
                // 1. nascondi freccina
                lv_obj_add_flag(chorus_offset_arrow, LV_OBJ_FLAG_HIDDEN);
                // 2. label nome → bianca
                if (chorus_offset_name_lbl &&
                    lv_obj_is_valid(chorus_offset_name_lbl))
                    lv_obj_set_style_text_color(chorus_offset_name_lbl,
                        lv_color_hex(CRS_LBL_PASSED), 0);
                // 3. colore slider → passed
                if (chorus_offset_slider &&
                    lv_obj_is_valid(chorus_offset_slider))
                    lv_obj_set_style_bg_img_recolor(chorus_offset_slider,
                        lv_color_hex(CRS_OFFSET_PASSED), LV_PART_INDICATOR);
            } else {
                // freccina visibile
                lv_obj_clear_flag(chorus_offset_arrow, LV_OBJ_FLAG_HIDDEN);
                // label nome → arancione
                if (chorus_offset_name_lbl &&
                    lv_obj_is_valid(chorus_offset_name_lbl))
                    lv_obj_set_style_text_color(chorus_offset_name_lbl,
                        lv_color_hex(CRS_LBL_ACTIVE), 0);
                // colore slider → active
                if (chorus_offset_slider &&
                    lv_obj_is_valid(chorus_offset_slider))
                    lv_obj_set_style_bg_img_recolor(chorus_offset_slider,
                        lv_color_hex(CRS_OFFSET_ACTIVE), LV_PART_INDICATOR);
            }
        }
    }

    chorus_offset_last = cur;

    chorus_arr[chorus_cur_preset][CRS_OFFSET] = (uint8_t)cur;
    // TODO: LWS send to MCU 'M'
}

static void chorus_offset_create(lv_obj_t *parent,
                                  int x, int y, int w, int h) {
    const int LBL_H  = 20;
    const int CONT_H = LBL_H + 4 + h + 4 + LBL_H;

    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_size(c, w, CONT_H);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_set_style_pad_all(c, 0, 0);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(c);
    lv_label_set_text(lbl, "Offset");
    lv_obj_set_style_text_color(lbl, lv_color_hex(CRS_LBL_ACTIVE), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 0);
    chorus_offset_name_lbl = lbl;

    lv_obj_t *s = lv_slider_create(c);
    lv_obj_set_size(s, w, h);
    lv_obj_align(s, LV_ALIGN_CENTER, 0, 0);
    lv_slider_set_range(s, 0, 100);
    int initVal = chorus_arr[chorus_cur_preset][CRS_OFFSET];
    if (initVal > 100) initVal = 100;
    lv_slider_set_value(s, initVal, LV_ANIM_OFF);

    lv_obj_set_style_bg_img_src(s, &img_slider_track, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_src(s, &img_slider_indicator, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor(s, lv_color_hex(CRS_OFFSET_ACTIVE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor_opa(s,
        (sliderColorDepth * 255) / 100, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_src(s, &img_slider_knob, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_width(s, 45, LV_PART_KNOB);
    lv_obj_set_style_height(s, 30, LV_PART_KNOB);

    chorus_offset_slider = s;

    lv_obj_t *vlbl = lv_label_create(c);
    lv_obj_set_style_text_color(vlbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(vlbl, &lv_font_montserrat_14, 0);
    lv_label_set_text_fmt(vlbl, "%d", initVal);
    lv_obj_align(vlbl, LV_ALIGN_BOTTOM_MID, 0, 0);
    chorus_offset_val_lbl = vlbl;

    // Freccina a sinistra dello slider, posizionata sul target
    chorus_offset_target = 50;
    int yf = map(chorus_offset_target, 0, 100,
                 y + LBL_H + 4 + h - 13,
                 y + LBL_H + 4 + 0);
    lv_obj_t *a = lv_label_create(parent);
    lv_label_set_text(a, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(a, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_text_font(a, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(a, x - 18, yf);
    chorus_offset_arrow  = a;
    chorus_offset_last   = initVal;
    chorus_offset_crossed = false;

    lv_obj_add_event_cb(s, chorus_offset_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

// ============================================================
// Applica preset
// ============================================================
static void chorus_apply_preset() {
    // Arc
    for (int i = 0; i < 4; i++) {
        if (!chorus_arc[i] || !lv_obj_is_valid(chorus_arc[i])) continue;
        int param = CRS_LFO_SLOW_TIME + i;
        int v = chorus_arr[chorus_cur_preset][param];
        if (v > 100) v = 100;
        lv_arc_set_value(chorus_arc[i], v);
        chorus_arc_last[i] = v;
        if (chorus_arc_val[i] && lv_obj_is_valid(chorus_arc_val[i]))
            lv_label_set_text_fmt(chorus_arc_val[i], "%d", v);
    }

    // EQ slider
    for (int i = 0; i < 6; i++) {
        if (!chorus_eq_slider[i] || !lv_obj_is_valid(chorus_eq_slider[i])) continue;
        int v = chorus_arr[chorus_cur_preset][CRS_EQ_220 + i];
        if (v > 100) v = 100;
        lv_slider_set_value(chorus_eq_slider[i], v, LV_ANIM_OFF);
        if (chorus_eq_val_lbl[i] && lv_obj_is_valid(chorus_eq_val_lbl[i]))
            lv_label_set_text_fmt(chorus_eq_val_lbl[i], "%d", v);
    }

    // Offset: reset freccina + colori
    if (chorus_offset_slider && lv_obj_is_valid(chorus_offset_slider)) {
        int v = chorus_arr[chorus_cur_preset][CRS_OFFSET];
        if (v > 100) v = 100;
        lv_slider_set_value(chorus_offset_slider, v, LV_ANIM_OFF);
        chorus_offset_last    = v;
        chorus_offset_crossed = false;

        if (chorus_offset_val_lbl && lv_obj_is_valid(chorus_offset_val_lbl))
            lv_label_set_text_fmt(chorus_offset_val_lbl, "%d", v);

        if (chorus_offset_arrow && lv_obj_is_valid(chorus_offset_arrow))
            lv_obj_clear_flag(chorus_offset_arrow, LV_OBJ_FLAG_HIDDEN);

        if (chorus_offset_name_lbl && lv_obj_is_valid(chorus_offset_name_lbl))
            lv_obj_set_style_text_color(chorus_offset_name_lbl,
                lv_color_hex(CRS_LBL_ACTIVE), 0);

        lv_obj_set_style_bg_img_recolor(chorus_offset_slider,
            lv_color_hex(CRS_OFFSET_ACTIVE), LV_PART_INDICATOR);
    }

    // Mode + visibilità EQ
    chorus_update_mode_visual();
    chorus_update_eq_visibility();
}

// ============================================================
// Dropdown preset
// ============================================================
static void chorus_dd_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    int sel = lv_dropdown_get_selected(dd);
    if (sel < 0 || sel >= CHORUS_PRESETS) return;
    chorus_cur_preset = (uint8_t)sel;
    chorus_apply_preset();
}

// ============================================================
// Reset puntatori
// ============================================================
void chorus_reset_pointers() {
    chorus_dd = nullptr;
    chorus_mode_btn = nullptr;
    chorus_mode_lbl = nullptr;

    for (int i = 0; i < 4; i++) {
        chorus_arc[i] = nullptr;
        chorus_arc_lbl[i] = nullptr;
        chorus_arc_val[i] = nullptr;
        chorus_arc_last[i] = 0;
    }
    for (int i = 0; i < 6; i++) {
        chorus_eq_slider[i] = nullptr;
        chorus_eq_val_lbl[i] = nullptr;
    }
    chorus_offset_slider   = nullptr;
    chorus_offset_val_lbl  = nullptr;
    chorus_offset_name_lbl = nullptr;
    chorus_offset_arrow    = nullptr;
    chorus_offset_crossed  = false;
}

// ============================================================
// Pagina CHORUS
// ============================================================
void chorus_page_create(lv_obj_t *parent) {
    // Reset puntatori (sicurezza)
    for (int i = 0; i < 4; i++) {
        chorus_arc[i] = nullptr; chorus_arc_lbl[i] = nullptr; chorus_arc_val[i] = nullptr;
    }
    for (int i = 0; i < 6; i++) {
        chorus_eq_slider[i] = nullptr; chorus_eq_val_lbl[i] = nullptr;
    }
    chorus_offset_slider   = nullptr;
    chorus_offset_val_lbl  = nullptr;
    chorus_offset_name_lbl = nullptr;
    chorus_offset_arrow    = nullptr;
    chorus_offset_crossed  = false;
    chorus_mode_btn = nullptr; chorus_mode_lbl = nullptr;

    // ---- Dropdown "CRS" accanto al titolo ----
    chorus_dd = styled_dropdown(parent, 130, 5, 110, 45,
                                 0x00AA00,
                                 &lv_font_montserrat_16,
                                 "CRS 1\nCRS 2\nCRS 3\nCRS 4\n"
                                 "CRS 5\nCRS 6\nCRS 7\nCRS 8");
    lv_dropdown_set_selected(chorus_dd, chorus_cur_preset);
    lv_obj_add_event_cb(chorus_dd, chorus_dd_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // ---- Mode button SOL/EQ ----
    chorus_mode_btn = mkbtn(parent, 20, 100, 100, 100,
                             0x00AA00, "SOL",
                             &lv_font_montserrat_24,
                             chorus_mode_cb, 0, 8, 3);
    chorus_mode_lbl = lv_obj_get_child(chorus_mode_btn, 0);

    // ---- Frame EQ (6 slider) ----
    const int EQ_X = 140, EQ_Y = 65, EQ_W = 490, EQ_H = 200;
    lv_obj_t *eq_frame = lv_obj_create(parent);
    lv_obj_set_size(eq_frame, EQ_W, EQ_H);
    lv_obj_set_pos(eq_frame, EQ_X, EQ_Y);
    lv_obj_set_style_bg_opa(eq_frame, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(eq_frame, 1, 0);
    lv_obj_set_style_border_color(eq_frame, lv_color_hex(0x888888), 0);
    lv_obj_set_style_radius(eq_frame, 6, 0);
    lv_obj_set_style_pad_all(eq_frame, 0, 0);
    lv_obj_clear_flag(eq_frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(eq_frame, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_clip_corner(eq_frame, 0, 0);

    lv_obj_t *eq_lbl = lv_label_create(eq_frame);
    lv_label_set_text(eq_lbl, "EQ");
    lv_obj_set_style_text_color(eq_lbl, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(eq_lbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_bg_color(eq_lbl, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(eq_lbl, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(eq_lbl, 8, 0);
    lv_obj_align(eq_lbl, LV_ALIGN_TOP_MID, 0, -20);

    const char *eq_names[6] = {"220","510","730","1000","3500","6000"};
    const int EQ_SL_W = 60, EQ_SL_H = 140;
    const int EQ_X0 = 15, EQ_DX = 80;
    const int EQ_SL_Y = 10;

    for (int i = 0; i < 6; i++) {
        chorus_eq_create(eq_frame, i,
                         EQ_X0 + i * EQ_DX, EQ_SL_Y,
                         EQ_SL_W, EQ_SL_H,
                         eq_names[i], chorus_eq_colors[i]);
    }

    // ---- Offset slider (fuori dal frame, stessa riga) ----
    chorus_offset_create(parent, 660, EQ_Y, 60, EQ_H - 60);

    // ---- Frame LFO (4 arc) in basso, a destra di home ----
    //   COMPATTATO ORIZZONTALMENTE: W 680→500, ARC_DX 170→110
    const int LFO_X = 110, LFO_Y = 320, LFO_W = 500, LFO_H = 115;
    lv_obj_t *lfo_frame = lv_obj_create(parent);
    lv_obj_set_size(lfo_frame, LFO_W, LFO_H);
    lv_obj_set_pos(lfo_frame, LFO_X, LFO_Y);
    lv_obj_set_style_bg_opa(lfo_frame, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(lfo_frame, 1, 0);
    lv_obj_set_style_border_color(lfo_frame, lv_color_hex(0x888888), 0);
    lv_obj_set_style_radius(lfo_frame, 6, 0);
    lv_obj_set_style_pad_all(lfo_frame, 0, 0);
    lv_obj_clear_flag(lfo_frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(lfo_frame, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_clip_corner(lfo_frame, 0, 0);

    lv_obj_t *lfo_lbl = lv_label_create(lfo_frame);
    lv_label_set_text(lfo_lbl, "LFO");
    lv_obj_set_style_text_color(lfo_lbl, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(lfo_lbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_bg_color(lfo_lbl, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(lfo_lbl, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(lfo_lbl, 8, 0);
    lv_obj_align(lfo_lbl, LV_ALIGN_TOP_MID, 0, -20);

    const char *lfo_names[4] = {"SlowTime","SlowGain","FastTime","FastGain"};
    const int ARC_W = 80, ARC_H = 80;
    const int ARC_Y = 20;
    const int ARC_X0 = 45, ARC_DX = 110;   // NUOVO (era 170)
    for (int i = 0; i < 4; i++) {
        chorus_arc_create(lfo_frame, i,
                          ARC_X0 + i * ARC_DX, ARC_Y,
                          ARC_W, ARC_H, lfo_names[i]);
    }

    // ---- Bottone Salva (top-right) ----
    mkbtn(parent, 680, 5, 110, 45,
          0xFF4444, "Salva",
          &lv_font_montserrat_16,
          [](lv_event_t *e) {
              (void)e;
              log_add("CHORUS salvato", lv_color_hex(0x00FF00));
              toast_show("CHORUS salvato!", lv_color_hex(0x00FF00), TOAST_DUR);
          }, 0, 6, 2);

    // ---- Applica preset ----
    chorus_apply_preset();
}