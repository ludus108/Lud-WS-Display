// ============================================================
// lvglGrafFx.cpp — FX / FV-1
// ============================================================
#include "globals.h"
#include "lvglGraf.h"
#include "lvglGraf_internal.h"
#include "lvgl_v8_port.h"
#include "src/preset/preset_ui.h"
#include <math.h>

// ============================================================
// FX / FV-1
// ============================================================
lv_obj_t *pot_size_FV1 = nullptr;
lv_obj_t *pot_LF_FV1   = nullptr;
lv_obj_t *pot_HF_FV1   = nullptr;
lv_obj_t *fx_rev_btn   = nullptr;
lv_obj_t *fx_rev_lbl   = nullptr;
lv_obj_t *fx_preset_dd = nullptr;
lv_obj_t *fx_save_btn  = nullptr;

// ============================================================
// Callbacks
// ============================================================
// fx_rev_btn: label (Rev1/Rev2) e bg_color cambiano runtime.
// Rimane manuale (mkbtn non espone la label per modifiche post-create).
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

// ============================================================
// Pagina FX
// ============================================================
void fx_page_create(lv_obj_t *parent) {
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

    lv_obj_t *fr_lbl = lv_label_create(frame);
    lv_label_set_text(fr_lbl, "FV-1");
    lv_obj_set_style_text_color(fr_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(fr_lbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_bg_color(fr_lbl, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(fr_lbl, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(fr_lbl, 6, 0);
    lv_obj_align(fr_lbl, LV_ALIGN_TOP_MID, 0, -22);

    // ---- Dropdown preset P1..P8 ----
    fx_preset_dd = styled_dropdown(frame, 8, 12, 65, 45,
                                   0x666666,
                                   &lv_font_montserrat_16,
                                   "P1\nP2\nP3\nP4\nP5\nP6\nP7\nP8");
    lv_dropdown_set_selected(fx_preset_dd, 0);

    // ---- Toggle Rev1 / Rev2 (label e colore cambiano runtime) ----
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

    // ---- Bottone Salva ----
    fx_save_btn = mkbtn(frame, 165, 12, 85, 45,
                        0xFF4444, "Salva",
                        &lv_font_montserrat_18,
                        fx_save_cb, 0, 6, 2);

    // ---- 3 slider verticali: SIZE / LF / HF ----
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