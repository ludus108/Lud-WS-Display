// ============================================================
// lvglGrafLfoB.cpp — LFO B editor (SynthB LFO3)
// ============================================================
#include "globals.h"
#include "lvglGraf.h"
#include "lvglGraf_internal.h"
#include "lvgl_v8_port.h"
#include "preset/preset_ui.h"
#include <math.h>

// ============================================================
// Nomi forme d'onda (indice 0..7)
// ============================================================
static const char *lfoB_wave_names[8] = {
    "SINE", "SAW", "RSAW", "SQR",
    "RND", "RND50", "RND25", "RND10"
};

// Colori bottoni wave
#define LFO_WAVE_BG_OFF    0x004488
#define LFO_WAVE_BD_OFF    0x0066AA
#define LFO_WAVE_BG_ON     0x00AA00
#define LFO_WAVE_BD_ON     0x00FF44

// Colori slider
#define LFO_SL_RATE_COL    0x00AAFF   // azzurro
#define LFO_SL_LEV_COL     0xAA44FF   // viola
#define LFO_SL_SPREAD_COL  0xFF8800   // arancione

// Colori freccina
#define LFO_ARROW_ACTIVE   0xFF0000
#define LFO_SL_COL_ACTIVE  0x000000   // non usato (indicator usa i colori sopra)

// ============================================================
// Stato
// ============================================================
static uint8_t lfoB_wave = 0;   // 0..7

static lv_obj_t *lfoB_wave_btn[8]  = {nullptr};

static lv_obj_t *lfoB_slider[3]    = {nullptr};
static lv_obj_t *lfoB_arrow[3]     = {nullptr};
static lv_obj_t *lfoB_val_lbl[3]   = {nullptr};
static int       lfoB_slider_last[3] = {50, 50, 50};
static int       lfoB_slider_tgt[3]  = {50, 50, 50};
static bool      lfoB_slider_crs[3]  = {false, false, false};

// ============================================================
// Aggiorna visuale bottoni wave
// ============================================================
static void lfoB_update_wave_visual() {
    for (int i = 0; i < 8; i++) {
        lv_obj_t *b = lfoB_wave_btn[i];
        if (!b || !lv_obj_is_valid(b)) continue;
        if (i == (int)lfoB_wave) {
            lv_obj_set_style_bg_color(b, lv_color_hex(LFO_WAVE_BG_ON), 0);
            lv_obj_set_style_border_color(b, lv_color_hex(LFO_WAVE_BD_ON), 0);
            lv_obj_set_style_border_width(b, 3, 0);
        } else {
            lv_obj_set_style_bg_color(b, lv_color_hex(LFO_WAVE_BG_OFF), 0);
            lv_obj_set_style_border_color(b, lv_color_hex(LFO_WAVE_BD_OFF), 0);
            lv_obj_set_style_border_width(b, 2, 0);
        }
    }
}

// ============================================================
// Callback bottone wave
// ============================================================
static void lfoB_wave_cb(lv_event_t *e) {
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx > 7) return;
    lfoB_wave = (uint8_t)idx;
    lfoB_update_wave_visual();
    uiSetParamB_U8('b', lfoB_wave);
}

// ============================================================
// Callback slider (freccina + invio LWS)
// ============================================================
static void lfoB_slider_cb(lv_event_t *e) {
    lv_obj_t *s = lv_event_get_target(e);
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx > 2) return;

    int cur    = lv_slider_get_value(s);
    int prev   = lfoB_slider_last[idx];
    int target = lfoB_slider_tgt[idx];

    // Freccina (crossing come in VCA)
    if (!lfoB_slider_crs[idx] && lfoB_arrow[idx] && lv_obj_is_valid(lfoB_arrow[idx])) {
        bool crossed = (prev < target && cur > target) ||
                       (prev > target && cur < target) ||
                       (prev == target && cur != target);
        if (crossed) {
            lfoB_slider_crs[idx] = true;
            lv_obj_add_flag(lfoB_arrow[idx], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(lfoB_arrow[idx], LV_OBJ_FLAG_HIDDEN);
        }
    }
    lfoB_slider_last[idx] = cur;

    // Label valore
    if (lfoB_val_lbl[idx] && lv_obj_is_valid(lfoB_val_lbl[idx])) {
        char buf[12];
        if (idx == 0) {
            // RATE: mostra in µs (o ms se > 1000)
            int rate_us = 350 + (cur * (80000 - 350)) / 255;
            if (rate_us >= 1000)
                snprintf(buf, sizeof(buf), "%dms", rate_us / 1000);
            else
                snprintf(buf, sizeof(buf), "%dus", rate_us);
        } else if (idx == 1) {
            // LEV: 0..1023
            snprintf(buf, sizeof(buf), "%d", cur * 4);
        } else {
            // SPREAD: 0..255
            snprintf(buf, sizeof(buf), "%d", cur);
        }
        lv_label_set_text(lfoB_val_lbl[idx], buf);
    }

    // Invio LWS
    if (idx == 0) {
        int rate_us = 350 + (cur * (80000 - 350)) / 255;
        uiSetParamB_I32('k', rate_us);
    } else if (idx == 1) {
        uiSetParamB_I32('l', cur * 4);
    } else {
        uiSetParamB_U8('c', (uint8_t)cur);
    }
}

// ============================================================
// Crea bottone wave
// ============================================================
static lv_obj_t* lfoB_create_wave_btn(lv_obj_t *parent, int idx,
                                       int x, int y, int w, int h) {
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_size(b, w, h);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_style_bg_color(b, lv_color_hex(LFO_WAVE_BG_OFF), 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x0055AA), LV_STATE_PRESSED);
    lv_obj_set_style_radius(b, 6, 0);
    lv_obj_set_style_border_width(b, 2, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(LFO_WAVE_BD_OFF), 0);

    lv_obj_t *lbl = lv_label_create(b);
    lv_label_set_text(lbl, lfoB_wave_names[idx]);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl);

    lv_obj_add_event_cb(b, lfoB_wave_cb, LV_EVENT_CLICKED,
                        (void*)(uintptr_t)idx);
    return b;
}

// ============================================================
// Crea slider verticale con freccina
// ============================================================
static void lfoB_create_slider(lv_obj_t *parent, int idx,
                                int x, int y, int w, int h,
                                const char *name, uint32_t col,
                                int init_val) {
    // Container verticale
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_size(c, w, 242);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_set_style_pad_all(c, 0, 0);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(c, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    // Slider
    lv_obj_t *s = lv_slider_create(c);
    lv_obj_set_size(s, w, h);
    lv_obj_align(s, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_slider_set_range(s, 0, 255);
    lv_slider_set_value(s, init_val, LV_ANIM_OFF);

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

    // Nome (top)
    lv_obj_t *lbl = lv_label_create(c);
    lv_label_set_text(lbl, name);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFA500), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 0);

    // Valore (bottom)
    lv_obj_t *vlbl = lv_label_create(c);
    lv_obj_set_style_text_color(vlbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(vlbl, &lv_font_montserrat_14, 0);
    // Valore iniziale
    char buf[12];
    if (idx == 0) {
        int rate_us = 350 + (init_val * (80000 - 350)) / 255;
        if (rate_us >= 1000) snprintf(buf, sizeof(buf), "%dms", rate_us/1000);
        else                 snprintf(buf, sizeof(buf), "%dus", rate_us);
    } else if (idx == 1) {
        snprintf(buf, sizeof(buf), "%d", init_val * 4);
    } else {
        snprintf(buf, sizeof(buf), "%d", init_val);
    }
    lv_label_set_text(vlbl, buf);
    lv_obj_align(vlbl, LV_ALIGN_BOTTOM_MID, 0, 0);
    lfoB_val_lbl[idx] = vlbl;

    // Freccina a sinistra
    int arrow_y = map(lfoB_slider_tgt[idx], 0, 255, y + 242 - 13, y + 30);
    lv_obj_t *a = lv_label_create(parent);
    lv_label_set_text(a, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(a, lv_color_hex(LFO_ARROW_ACTIVE), 0);
    lv_obj_set_style_text_font(a, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(a, x - 18, arrow_y);
    lfoB_arrow[idx] = a;

    lfoB_slider[idx] = s;
    lfoB_slider_last[idx] = init_val;

    lv_obj_add_event_cb(s, lfoB_slider_cb, LV_EVENT_VALUE_CHANGED,
                        (void*)(uintptr_t)idx);
}

// ============================================================
// Reset puntatori
// ============================================================
void lfoB_reset_pointers() {
    for (int i = 0; i < 8; i++) lfoB_wave_btn[i] = nullptr;
    for (int i = 0; i < 3; i++) {
        lfoB_slider[i]    = nullptr;
        lfoB_arrow[i]     = nullptr;
        lfoB_val_lbl[i]   = nullptr;
        lfoB_slider_last[i] = 50;
        lfoB_slider_crs[i]  = false;
    }
}

// ============================================================
// Pagina LFO B
// ============================================================
void lfoB_page_create(lv_obj_t *parent) {
    // Reset puntatori (sicurezza)
    for (int i = 0; i < 8; i++) lfoB_wave_btn[i] = nullptr;
    for (int i = 0; i < 3; i++) {
        lfoB_slider[i]  = nullptr;
        lfoB_arrow[i]   = nullptr;
        lfoB_val_lbl[i] = nullptr;
        lfoB_slider_crs[i] = false;
    }

    // ============================================================
    // 8 bottoni wave (4×2, x=10)
    //   larghezza 95, gap 5 → 4*95 + 3*5 = 395 → x da 10 a 405
    //   righe: y=60 e y=125, altezza 55
    // ============================================================
    const int BTN_W = 95;
    const int BTN_H = 55;
    const int BTN_GX = 100;   // pitch orizzontale (95 + 5)
    const int BTN_GY = 65;    // pitch verticale (55 + 10)
    const int BTN_X0 = 10;
    const int BTN_Y0 = 60;

    for (int i = 0; i < 8; i++) {
        int row = i / 4;
        int col = i % 4;
        int x = BTN_X0 + col * BTN_GX;
        int y = BTN_Y0 + row * BTN_GY;
        lfoB_wave_btn[i] = lfoB_create_wave_btn(parent, i, x, y,
                                                 BTN_W, BTN_H);
    }
    lfoB_update_wave_visual();

    // ============================================================
    // 3 slider verticali (RATE, LEV, SPREAD)
    //   x da 430, larghezza 60, gap 40
    //   x = [430, 530, 630], y=60, h=200
    // ============================================================
    const int SL_W  = 60;
    const int SL_H  = 200;
    const int SL_Y  = 60;
    const int SL_X0 = 430;
    const int SL_DX = 100;

    lfoB_create_slider(parent, 0, SL_X0 + 0*SL_DX, SL_Y, SL_W, SL_H,
                       "RATE", LFO_SL_RATE_COL, 50);
    lfoB_create_slider(parent, 1, SL_X0 + 1*SL_DX, SL_Y, SL_W, SL_H,
                       "LEV", LFO_SL_LEV_COL, 50);
    lfoB_create_slider(parent, 2, SL_X0 + 2*SL_DX, SL_Y, SL_W, SL_H,
                       "SPREAD", LFO_SL_SPREAD_COL, 50);

    // Home bottom-left (già creato dal chiamante? No, lo creo io)
   // home_btn(parent, -2);
}