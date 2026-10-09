// ============================================================
// lvglGrafRev.cpp — Riverbero (pagina REV)
// ============================================================
#include "globals.h"
#include "lvglGraf.h"
#include "lvglGraf_internal.h"
#include "lvgl_v8_port.h"
#include "src/preset/preset_ui.h"
#include <math.h>

// ============================================================
// Stato REV
// ============================================================
lv_obj_t *rev_mode_btn  = nullptr;
lv_obj_t *rev_mode_lbl  = nullptr;
uint8_t   rev_mode      = 0;   // 0=SER, 1=PAR

lv_obj_t *rev_preset_dd = nullptr;
uint8_t   rev_preset    = 0;   // 0..15

#define REV_SLIDER_COUNT   8
#define REV_SLIDER_W       60
#define REV_SLIDER_H       200
#define REV_SLIDER_GAP     30
#define REV_SLIDER_Y       100   // centrato verticalmente
#define REV_LBL_H          22

// Valori correnti degli 8 slider (0..20)
static uint8_t   rev_val    [REV_SLIDER_COUNT] = {10,10,10,10,10,10,10,10};
static lv_obj_t *rev_val_lbl[REV_SLIDER_COUNT] = {nullptr};
static lv_obj_t *rev_slider_obj[REV_SLIDER_COUNT] = {nullptr};

static const char *rev_names[REV_SLIDER_COUNT] = {
    "PRED", "SIZE", "DAMP", "CUT1", "RES1", "CUT2", "RES2", "LEV"
};

// ============================================================
// Preset array (9 righe × 16 preset)
//   riga 0 = room size  → SIZE   (slider idx 1)
//   riga 1 = damping    → DAMP   (slider idx 2)
//   riga 2 = cut off    → CUT1   (slider idx 3)
//   riga 3 = res        → RES1   (slider idx 4)
//   riga 4 = pre dly    → PRED   (slider idx 0)
//   riga 5 = cut off2   → CUT2   (slider idx 5)
//   riga 6 = res 2      → RES2   (slider idx 6)
//   riga 7 = filtSet    → SER/PAR toggle
//   riga 8 = lev out    → LEV    (slider idx 7)
// ============================================================
static const int revPresetArr[9][16] = {
    { 2, 10, 15,  6,  5,  5,  5,  5,  2, 10, 15,  6,  5,  5,  5,  5 }, // room size
    { 0,  4, 10, 20,  8,  2, 12, 12,  0,  4, 10, 20,  8,  2, 12, 12 }, // damping
    { 0, 20, 10,  5, 12,  2, 15,  8,  0, 20, 10,  5, 12,  2, 15,  8 }, // cut off
    { 0,  3,  4,  5, 10,  2, 15,  8,  0,  3,  4,  5, 10,  2, 15,  8 }, // res
    { 0,  3,  4,  5, 10,  2, 15,  8,  0,  3,  4,  5, 10,  2, 15,  8 }, // pre dly
    {15,  8,  0, 20, 10,  5, 12,  2, 15,  0, 20, 10,  5, 12,  2,  8 }, // cut off2
    { 8,  0,  3,  4,  5, 10,  2, 15,  0,  3,  4,  5, 10,  2, 15,  8 }, // res 2
    { 0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1 }, // filtSet ser/par
    {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10 }  // lev out
};

// Mappa slider → riga di revPresetArr
static const int rev_slider_row[REV_SLIDER_COUNT] = {
    4,  // PRED  ← pre dly
    0,  // SIZE  ← room size
    1,  // DAMP  ← damping
    2,  // CUT1  ← cut off
    3,  // RES1  ← res
    5,  // CUT2  ← cut off2
    6,  // RES2  ← res 2
    8   // LEV   ← lev out
};

// ============================================================
// Colore per ogni slider
// ============================================================
static uint32_t rev_slider_color(int idx) {
    switch (idx) {
        case 0: return 0x00DD00;  // PRED   verde
        case 1: return 0x00AAFF;  // SIZE   azzurro
        case 2: return 0xAA44FF;  // DAMP   viola
        case 3: return 0xCC0066;  // CUT1   rosso porpora
        case 4: return 0xFFCC33;  // RES1   giallo
        case 5: return 0xCC0066;  // CUT2   rosso porpora
        case 6: return 0xFFCC33;  // RES2   giallo
        case 7: return 0xCC3300;  // LEV    rosso
        default: return 0xFFFFFF;
    }
}

// ============================================================
// Aggiorna tutti gli slider al preset corrente
// ============================================================
static void rev_apply_preset(uint8_t preset) {
    if (preset >= 16) return;
    rev_preset = preset;

    for (int i = 0; i < REV_SLIDER_COUNT; i++) {
        if (!rev_slider_obj[i] || !lv_obj_is_valid(rev_slider_obj[i])) continue;
        int v = revPresetArr[rev_slider_row[i]][preset];
        rev_val[i] = (uint8_t)v;
        lv_slider_set_value(rev_slider_obj[i], v, LV_ANIM_OFF);
        if (rev_val_lbl[i] && lv_obj_is_valid(rev_val_lbl[i])) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", v);
            lv_label_set_text(rev_val_lbl[i], buf);
        }
    }

    // SER / PAR
    rev_mode = (uint8_t)revPresetArr[7][preset];
    if (rev_mode_lbl && lv_obj_is_valid(rev_mode_lbl)) {
        lv_label_set_text(rev_mode_lbl, rev_mode == 0 ? "SER" : "PAR");
        lv_obj_set_style_text_color(rev_mode_lbl,
            lv_color_hex(rev_mode == 0 ? 0x0099FF : 0xFF8800), 0);
    }
    if (rev_mode_btn && lv_obj_is_valid(rev_mode_btn)) {
        lv_obj_set_style_border_color(rev_mode_btn,
            lv_color_hex(rev_mode == 0 ? 0x0099FF : 0xFF8800), 0);
    }
}

// ============================================================
// Callback
// ============================================================
static void rev_slider_cb(lv_event_t *e) {
    lv_obj_t *s = lv_event_get_target(e);
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= REV_SLIDER_COUNT) return;

    int v = lv_slider_get_value(s);
    rev_val[idx] = (uint8_t)v;

    if (rev_val_lbl[idx] && lv_obj_is_valid(rev_val_lbl[idx])) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", v);
        lv_label_set_text(rev_val_lbl[idx], buf);
    }
    // TODO: invio LWS al Teensy (in futuro)
}

static void rev_mode_cb(lv_event_t *e) {
    (void)e;
    rev_mode = (rev_mode == 0) ? 1 : 0;
    if (rev_mode_lbl && lv_obj_is_valid(rev_mode_lbl)) {
        lv_label_set_text(rev_mode_lbl, rev_mode == 0 ? "SER" : "PAR");
        lv_obj_set_style_text_color(rev_mode_lbl,
            lv_color_hex(rev_mode == 0 ? 0x0099FF : 0xFF8800), 0);
    }
    if (rev_mode_btn && lv_obj_is_valid(rev_mode_btn)) {
        lv_obj_set_style_border_color(rev_mode_btn,
            lv_color_hex(rev_mode == 0 ? 0x0099FF : 0xFF8800), 0);
    }
    // TODO: invio LWS al Teensy (in futuro)
}

static void rev_preset_dd_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    int sel = lv_dropdown_get_selected(dd);
    if (sel < 0 || sel >= 16) return;
    rev_apply_preset((uint8_t)sel);
}

// ============================================================
// Salva
// ============================================================
static void rev_save_btn_cb(lv_event_t *e) {
    (void)e;
    // TODO: invio bulk revPresetArr via LWS
    Serial.printf("[REV] send to Teensy (stub) preset=%u mode=%u\n",
                  (unsigned)rev_preset, (unsigned)rev_mode);
    log_add("REV salvato", lv_color_hex(0x00FF00));
    toast_show("REV salvato!", lv_color_hex(0x00FF00), TOAST_DUR);
}

// ============================================================
// Creazione di un singolo slider REV
//   layout verticale:  [nome]  /  [slider]  /  [valore]
// ============================================================
static lv_obj_t* rev_slider_create(lv_obj_t *parent, int idx,
                                    int x, int y, int w, int h) {
    const int CONT_H = h + 2 * REV_LBL_H + 6;

    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_size(c, w, CONT_H);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_set_style_pad_all(c, 0, 0);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);

    // ---- Label nome (top) ----
    lv_obj_t *lbl = lv_label_create(c);
    lv_label_set_text(lbl, rev_names[idx]);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 0);

    // ---- Slider (center) ----
    lv_obj_t *s = lv_slider_create(c);
    lv_obj_set_size(s, w, h);
    lv_obj_align(s, LV_ALIGN_CENTER, 0, 0);
    lv_slider_set_range(s, 0, 20);
    lv_slider_set_value(s, rev_val[idx], LV_ANIM_OFF);

    uint32_t col = rev_slider_color(idx);

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

    // ---- Label valore (bottom) ----
    lv_obj_t *vlbl = lv_label_create(c);
    lv_obj_set_style_text_color(vlbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(vlbl, &lv_font_montserrat_16, 0);
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", (unsigned)rev_val[idx]);
    lv_label_set_text(vlbl, buf);
    lv_obj_align(vlbl, LV_ALIGN_BOTTOM_MID, 0, 0);

    rev_val_lbl[idx]    = vlbl;
    rev_slider_obj[idx] = s;

    lv_obj_add_event_cb(s, rev_slider_cb, LV_EVENT_VALUE_CHANGED,
                        (void*)(uintptr_t)idx);
    return s;
}

// ============================================================
// Pagina REV
// ============================================================
void rev_page_create(lv_obj_t *parent) {
    // ---- Home bottom-left ----
    mkbtn(parent, 5, 415, 55, 45,
          0x9B59B6, LV_SYMBOL_LEFT,
          &lv_font_montserrat_24,
          eb, -3, 6, 2);

    // ---- Dropdown preset REV 1..16 (top, dopo il titolo) ----
    rev_preset_dd = styled_dropdown(parent, 90, 5, 110, 45,
                                    0xAA0000,
                                    &lv_font_montserrat_20,
                                    "REV 1\nREV 2\nREV 3\nREV 4\nREV 5\n"
                                    "REV 6\nREV 7\nREV 8\nREV 9\nREV 10\n"
                                    "REV 11\nREV 12\nREV 13\nREV 14\n"
                                    "REV 15\nREV 16");
    lv_dropdown_set_selected(rev_preset_dd, rev_preset);
    lv_obj_add_event_cb(rev_preset_dd, rev_preset_dd_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);

    // ---- 8 slider in riga centrale ----
    // Larghezza totale = 8*60 + 7*30 = 690; centrata: start_x = (800-690)/2 = 55
    const int start_x = 55;
    for (int i = 0; i < REV_SLIDER_COUNT; i++) {
        int x = start_x + i * (REV_SLIDER_W + REV_SLIDER_GAP);
        rev_slider_create(parent, i, x, REV_SLIDER_Y,
                          REV_SLIDER_W, REV_SLIDER_H);
    }

    // ---- Toggle SER / PAR ----
    //     Rimane manuale: label e colore bordo cambiano runtime
    rev_mode_btn = lv_btn_create(parent);
    lv_obj_set_size(rev_mode_btn, 120, 45);
    lv_obj_set_pos(rev_mode_btn, 340, 415);
    lv_obj_set_style_bg_color(rev_mode_btn, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(rev_mode_btn, lv_color_hex(0x0F3460),
                              LV_STATE_PRESSED);
    lv_obj_set_style_radius(rev_mode_btn, 6, 0);
    lv_obj_set_style_border_width(rev_mode_btn, 3, 0);
    lv_obj_set_style_border_color(rev_mode_btn,
        lv_color_hex(rev_mode == 0 ? 0x0099FF : 0xFF8800), 0);

    rev_mode_lbl = lv_label_create(rev_mode_btn);
    lv_label_set_text(rev_mode_lbl, rev_mode == 0 ? "SER" : "PAR");
    lv_obj_set_style_text_color(rev_mode_lbl,
        lv_color_hex(rev_mode == 0 ? 0x0099FF : 0xFF8800), 0);
    lv_obj_set_style_text_font(rev_mode_lbl, &lv_font_montserrat_24, 0);
    lv_obj_center(rev_mode_lbl);

    lv_obj_add_event_cb(rev_mode_btn, rev_mode_cb, LV_EVENT_CLICKED, NULL);

    // ---- Bottone Salva (bottom-right, stile SONG/SEQ) ----
    mkbtn(parent, 680, 415, 110, 45,
          0xFF4444, "Salva",
          &lv_font_montserrat_16,
          rev_save_btn_cb, 0, 6, 2);

    // ---- Applica il preset corrente ----
    rev_apply_preset(rev_preset);
}