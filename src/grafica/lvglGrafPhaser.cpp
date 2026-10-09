// ============================================================
// lvglGrafPhaser.cpp — PHASER editor
// ============================================================
// Target MCU per LWS: 'M' (Lud-WS-Mod)
// Array: phaser_arr[8 preset][3 params]
// ============================================================
#include "globals.h"
#include "lvglGraf.h"
#include "lvglGraf_internal.h"
#include "lvgl_v8_port.h"
#include "src/preset/preset_ui.h"
#include <math.h>

#define PHASER_PRESETS  8
#define PHASER_PARAMS   3

enum {
    PHS_RATE = 0,
    PHS_DEPTH,
    PHS_FB
};

// Colori per i 3 slider (verde, azzurro, viola)
static const uint32_t phaser_colors[3] = {
    0x00DD00, 0x00AAFF, 0xAA44FF
};

// ============================================================
// Array [8][3]
// ============================================================
uint8_t phaser_arr[PHASER_PRESETS][PHASER_PARAMS] = {
//  RATE  DEPTH  FB
    { 50,   50,   50 },
    { 30,   70,   40 },
    { 70,   30,   60 },
    { 40,   60,   80 },
    { 60,   40,   20 },
    { 20,   80,   50 },
    { 80,   20,   70 },
    { 50,   50,   30 }
};

// ============================================================
// Stato
// ============================================================
static uint8_t phaser_cur_preset = 0;

static lv_obj_t *phaser_dd       = nullptr;
static lv_obj_t *phaser_slider[3] = {nullptr,nullptr,nullptr};
static lv_obj_t *phaser_val_lbl[3]= {nullptr,nullptr,nullptr};

// ============================================================
// Slider callbacks
// ============================================================
struct PhaserCtx { int idx; };
static PhaserCtx phaser_ctx[3];

static void phaser_slider_cb(lv_event_t *e) {
    lv_obj_t *s = lv_event_get_target(e);
    PhaserCtx *ctx = (PhaserCtx*)lv_event_get_user_data(e);
    if (!ctx) return;
    int i = ctx->idx;
    if (i < 0 || i > 2) return;

    int v = lv_slider_get_value(s);
    if (phaser_val_lbl[i] && lv_obj_is_valid(phaser_val_lbl[i]))
        lv_label_set_text_fmt(phaser_val_lbl[i], "%d", v);

    phaser_arr[phaser_cur_preset][i] = (uint8_t)v;
    // TODO: LWS send to MCU 'M'
}

static void phaser_slider_create(lv_obj_t *parent, int i,
                                  int x, int y, int w, int h,
                                  const char *name, uint32_t col) {
    const int LBL_H  = 24;
    const int CONT_H = LBL_H + 6 + h + 6 + LBL_H;

    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_size(c, w, CONT_H);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_set_style_pad_all(c, 0, 0);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);

    // Nome (top)
    lv_obj_t *lbl = lv_label_create(c);
    lv_label_set_text(lbl, name);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 0);

    // Slider (center)
    lv_obj_t *s = lv_slider_create(c);
    lv_obj_set_size(s, w, h);
    lv_obj_align(s, LV_ALIGN_CENTER, 0, 0);
    lv_slider_set_range(s, 0, 100);
    int v = phaser_arr[phaser_cur_preset][i];
    lv_slider_set_value(s, v, LV_ANIM_OFF);

    lv_obj_set_style_bg_img_src(s, &img_slider_track, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_src(s, &img_slider_indicator, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor(s, lv_color_hex(col), LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor_opa(s,
        (sliderColorDepth * 255) / 100, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_src(s, &img_slider_knob, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_width(s, 45, LV_PART_KNOB);
    lv_obj_set_style_height(s, 30, LV_PART_KNOB);

    phaser_slider[i] = s;

    // Valore (bottom)
    lv_obj_t *vlbl = lv_label_create(c);
    lv_obj_set_style_text_color(vlbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(vlbl, &lv_font_montserrat_18, 0);
    lv_label_set_text_fmt(vlbl, "%d", v);
    lv_obj_align(vlbl, LV_ALIGN_BOTTOM_MID, 0, 0);
    phaser_val_lbl[i] = vlbl;

    phaser_ctx[i].idx = i;
    lv_obj_add_event_cb(s, phaser_slider_cb, LV_EVENT_VALUE_CHANGED,
                        &phaser_ctx[i]);
}

// ============================================================
// Applica preset
// ============================================================
static void phaser_apply_preset() {
    for (int i = 0; i < 3; i++) {
        if (!phaser_slider[i] || !lv_obj_is_valid(phaser_slider[i])) continue;
        int v = phaser_arr[phaser_cur_preset][i];
        lv_slider_set_value(phaser_slider[i], v, LV_ANIM_OFF);
        if (phaser_val_lbl[i] && lv_obj_is_valid(phaser_val_lbl[i]))
            lv_label_set_text_fmt(phaser_val_lbl[i], "%d", v);
    }
}

// ============================================================
// Dropdown preset
// ============================================================
static void phaser_dd_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    int sel = lv_dropdown_get_selected(dd);
    if (sel < 0 || sel >= PHASER_PRESETS) return;
    phaser_cur_preset = (uint8_t)sel;
    phaser_apply_preset();
}

// ============================================================
// Reset puntatori
// ============================================================
void phaser_reset_pointers() {
    phaser_dd = nullptr;
    for (int i = 0; i < 3; i++) {
        phaser_slider[i] = nullptr;
        phaser_val_lbl[i] = nullptr;
    }
}

// ============================================================
// Pagina PHASER
// ============================================================
void phaser_page_create(lv_obj_t *parent) {
    // Reset puntatori
    for (int i = 0; i < 3; i++) {
        phaser_slider[i] = nullptr;
        phaser_val_lbl[i] = nullptr;
    }

    // ---- Dropdown "PHS" accanto al titolo ----
    phaser_dd = styled_dropdown(parent, 130, 5, 110, 45,
                                 0xFF6600,
                                 &lv_font_montserrat_16,
                                 "PHS 1\nPHS 2\nPHS 3\nPHS 4\n"
                                 "PHS 5\nPHS 6\nPHS 7\nPHS 8");
    lv_dropdown_set_selected(phaser_dd, phaser_cur_preset);
    lv_obj_add_event_cb(phaser_dd, phaser_dd_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // ---- Frame "PHASER" ----
    const int FR_X = 100, FR_Y = 100, FR_W = 600, FR_H = 250;
    lv_obj_t *frame = lv_obj_create(parent);
    lv_obj_set_size(frame, FR_W, FR_H);
    lv_obj_set_pos(frame, FR_X, FR_Y);
    lv_obj_set_style_bg_opa(frame, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(frame, 1, 0);
    lv_obj_set_style_border_color(frame, lv_color_hex(0x888888), 0);
    lv_obj_set_style_radius(frame, 6, 0);
    lv_obj_set_style_pad_all(frame, 0, 0);
    lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(frame, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_clip_corner(frame, 0, 0);

    lv_obj_t *fl = lv_label_create(frame);
    lv_label_set_text(fl, "PHASER");
    lv_obj_set_style_text_color(fl, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(fl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_bg_color(fl, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(fl, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(fl, 8, 0);
    lv_obj_align(fl, LV_ALIGN_TOP_MID, 0, -20);

    // ---- 3 slider centrati ----
    const char *names[3] = {"RATE", "DEPTH", "FB"};
    const int SL_W = 80, SL_H = 150;
    const int SL_DX = 180;
    const int SL_X0 = 120, SL_Y = 30;

    for (int i = 0; i < 3; i++) {
        phaser_slider_create(frame, i,
                             SL_X0 + i * SL_DX, SL_Y,
                             SL_W, SL_H,
                             names[i], phaser_colors[i]);
    }
    // ---- Bottone Salva (top-right) ----
    mkbtn(parent, 680, 5, 110, 45,
          0xFF4444, "Salva",
          &lv_font_montserrat_16,
          [](lv_event_t *e) {
              (void)e;
              // TODO: salva phaser_arr su SD / invia a MCU 'M'
             // log_add("PHASER salvato", lv_color_hex(0x00FF00));
              toast_show("PHASER salvato!", lv_color_hex(0x00FF00), TOAST_DUR);
          }, 0, 6, 2);
    // ---- Applica preset ----
    phaser_apply_preset();
}