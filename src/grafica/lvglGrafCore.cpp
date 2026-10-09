// ============================================================
// lvglGrafCore.cpp — Widget riutilizzabili + utility grafiche
// ============================================================
#include "globals.h"
#include "lvglGraf.h"
#include "lvglGraf_internal.h"
#include <Arduino.h>
#include <math.h>
#include "lvgl_v8_port.h"
#include "src/preset/preset_ui.h"

#define ENV_POINTS     60


// Valori provvisori (futuro: editabili + SD)
// Ratio effettivo = fmSetSin / fmSetDiv
// Ispirati a FM1/FM2/FM3 del firmware (ratio 3, 7, 4+11, 11, ecc.)
// ============================================================
// Dati FM (modificabili da FM Edit; futuro: SD)
// ============================================================
uint8_t fmSetSin[8][3] = {
    {  3,  5,  7 },
    {  7,  9, 11 },
    {  4, 11, 13 },
    {  2,  7, 11 },
    {  5,  8, 13 },
    {  3,  7, 13 },
    {  3,  5, 11 },
    {  1,  3,  5 }
};

uint8_t fmSetDiv[8][3] = {
    { 1, 1, 1 },
    { 1, 1, 1 },
    { 1, 1, 1 },
    { 1, 1, 1 },
    { 1, 1, 1 },
    { 2, 2, 2 },
    { 1, 1, 1 },
    { 1, 1, 1 }
};
const uint8_t ui2fw_wave[9] = {0, 4, 3, 2, 1, 5, 6, 7, 8};
// Colore del bordo per categoria waveform (WF/FM/AM)
static uint32_t cat_color(int cat) {
    switch (cat) {
        case CAT_FM: return 0xFF8800;
        case CAT_AM: return 0xAA44FF;
        default:     return 0x00AAFF;   // CAT_WF
    }
}
// ============================================================
// 1) WIDGET DI BASE
// ============================================================
void btn(lv_obj_t *p, const char *l, int x, int y, int w, int h, lv_color_t c, int id) {
    lv_obj_t *b = lv_btn_create(p);
    lv_obj_set_size(b, w, h); lv_obj_set_pos(b, x, y);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_radius(b, 8, 0);
    lv_obj_set_style_border_width(b, 3, 0);
    lv_obj_set_style_border_color(b, c, 0);
    lv_obj_t *lb = lv_label_create(b);
    lv_label_set_text(lb, l);
    lv_obj_set_style_text_color(lb, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lb, &lv_font_montserrat_18, 0);
    lv_obj_center(lb);
    lv_obj_add_event_cb(b, eb, LV_EVENT_CLICKED, (void*)(uintptr_t)id);
}

static lv_obj_t* home_btn_at(lv_obj_t *p, int id, int x, int y) {
    lv_obj_t *b = lv_btn_create(p);
    lv_obj_set_size(b, 90, 90); lv_obj_set_pos(b, x, y);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_radius(b, 8, 0);
    lv_obj_set_style_border_width(b, 3, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(0x9B59B6), 0);

    lv_obj_t *lbl = lv_label_create(b);
    lv_label_set_text(lbl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_32, 0);
    lv_obj_center(lbl);

    lv_obj_add_event_cb(b, eb, LV_EVENT_CLICKED, (void*)(uintptr_t)id);
    return b;
}

void home_btn(lv_obj_t *p, int id) {
    home_btn_at(p, id, 10, 360);
}

// ============================================================
// 2) SLIDER VERTICALE + H_SLIDER (DCO)
// ============================================================
void slider(lv_obj_t *p, int x, int y, const char *l, int idx,
            lv_color_t active_color, lv_color_t passed_color) {
    lv_obj_t *c = lv_obj_create(p);
    lv_obj_set_size(c, 48, 242); lv_obj_set_pos(c, x, y);
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(c, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_clip_corner(c, 0, 0);

    lv_obj_t *s = lv_slider_create(c);
    lv_obj_set_size(s, 48, 212);
    lv_obj_align(s, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_slider_set_range(s, 0, 255);
    lv_slider_set_value(s, 127, LV_ANIM_OFF);
    slider_objs[idx] = s;
    slider_base_x[idx] = x;
    slider_base_y[idx] = y;

    lv_obj_set_style_bg_img_src(s, &img_slider_track, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_src(s, &img_slider_indicator, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor(s, active_color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor_opa(s, (sliderColorDepth * 255) / 100, LV_PART_INDICATOR);

    lv_obj_set_style_bg_img_src(s, &img_slider_knob, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_width(s, 45, LV_PART_KNOB);
    lv_obj_set_style_height(s, 30, LV_PART_KNOB);

    lv_obj_t *lb = lv_label_create(c);
    lv_label_set_text(lb, l);
    lv_obj_set_style_text_color(lb, lv_color_hex(0xFFA500), 0);
    lv_obj_set_style_text_font(lb, &lv_font_montserrat_18, 0);
    lv_obj_align_to(lb, s, LV_ALIGN_OUT_TOP_MID, 0, -10);
    sd[idx] = {lb, idx};
    lv_obj_add_event_cb(s, eslider, LV_EVENT_VALUE_CHANGED, &sd[idx]);

       int yf = map(g.pre[idx], 0, 255, (y+242)-13, y+30);
    lv_obj_t *a = lv_label_create(p);
    lv_label_set_text(a, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(a, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_text_font(a, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(a, x-18, yf);
    arr[idx] = a;
    crs[idx] = false;
    last[idx] = 127;
}

// ============================================================
// Slider "plain" — identico a slider() ma SENZA freccina/arrow.
// Usato per gli slider ADSR su pagine dove il valore è controllato
// solo dal touch (nessun pot esterno).
// ============================================================
void slider_plain(lv_obj_t *p, int x, int y, const char *l, int idx,
                  lv_color_t active_color, lv_color_t passed_color) {
    (void)passed_color;   // non usato senza meccanismo di crossing

    lv_obj_t *c = lv_obj_create(p);
    lv_obj_set_size(c, 48, 242); lv_obj_set_pos(c, x, y);
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(c, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_clip_corner(c, 0, 0);

    lv_obj_t *s = lv_slider_create(c);
    lv_obj_set_size(s, 48, 212);
    lv_obj_align(s, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_slider_set_range(s, 0, 255);
    lv_slider_set_value(s, 127, LV_ANIM_OFF);
    slider_objs[idx] = s;
    slider_base_x[idx] = x;
    slider_base_y[idx] = y;

    lv_obj_set_style_bg_img_src(s, &img_slider_track, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_src(s, &img_slider_indicator, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor(s, active_color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor_opa(s, (sliderColorDepth * 255) / 100, LV_PART_INDICATOR);

    lv_obj_set_style_bg_img_src(s, &img_slider_knob, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_width(s, 45, LV_PART_KNOB);
    lv_obj_set_style_height(s, 30, LV_PART_KNOB);

    lv_obj_t *lb = lv_label_create(c);
    lv_label_set_text(lb, l);
    lv_obj_set_style_text_color(lb, lv_color_hex(0xFFA500), 0);
    lv_obj_set_style_text_font(lb, &lv_font_montserrat_18, 0);
    lv_obj_align_to(lb, s, LV_ALIGN_OUT_TOP_MID, 0, -10);
    sd[idx] = {lb, idx};
    lv_obj_add_event_cb(s, eslider, LV_EVENT_VALUE_CHANGED, &sd[idx]);

    // Nessuna freccina
    arr[idx] = nullptr;
    crs[idx] = false;
    last[idx] = 127;
}

void h_slider(lv_obj_t *p, int id) {
    const char *title = (id == SRC_A) ? "DCO A" : "DCO B";
    lv_obj_t *title_lbl = lv_label_create(p);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(title_lbl, 20, 5);

    int idx = id - 1;
    ShapeData *d = &shape_data[idx];

    // ============================================================
    // PLOTTER (in alto a sinistra)
    // ============================================================
    const int PLOT_X = 10, PLOT_Y = 45;
    const int PLOT_W = 220, PLOT_H = 105;

    lv_obj_t *chart = lv_chart_create(p);
    lv_obj_set_size(chart, PLOT_W, PLOT_H);
    lv_obj_set_pos(chart, PLOT_X, PLOT_Y);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_point_count(chart, WAVE_POINTS);
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(chart, 1, 0);
    lv_obj_set_style_border_color(chart, lv_color_hex(0x666666), 0);
    lv_chart_set_div_line_count(chart, 0, 0);
    lv_chart_series_t *serie = lv_chart_add_series(chart, lv_color_hex(0xFFFF00),
                                                   LV_CHART_AXIS_PRIMARY_Y);
    d->chart = chart;
    d->serie = serie;
    update_plotter_by_wave(id);

    // ============================================================
    // SLIDER ORIZZONTALE (sotto il plotter, nessuna label SHAPE)
    // ============================================================
    const int SL_X = PLOT_X + 40;
    const int SL_Y = PLOT_Y + PLOT_H + 5;
    const int SL_W = 212;
    const int SL_H = 54;

    lv_obj_t *s = lv_slider_create(p);
    lv_obj_set_size(s, SL_W, SL_H);
    lv_obj_set_pos(s, SL_X, SL_Y);
    lv_slider_set_range(s, 0, 100);
    lv_obj_set_style_pad_all(s, 0, 0);
    lv_obj_add_flag(s, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_radius(s, 0, 0);
    lv_obj_set_style_radius(s, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s, 0, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s, 0, LV_PART_KNOB);

    uint8_t init_shape = (id == SRC_A) ? g.shape_a : g.shape_b;
    lv_slider_set_value(s, init_shape, LV_ANIM_OFF);
    if (id == SRC_A) g.shape_last_A = init_shape;
    else             g.shape_last_B = init_shape;

    lv_obj_set_style_bg_img_src(s, &img_slider_oriz_track, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_src(s, &img_slider_oriz_indicator, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor(s, lv_color_hex(COLOR_SHAPE_SLIDER_ACTIVE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor_opa(s, (sliderColorDepth * 255) / 100, LV_PART_INDICATOR);

    lv_obj_set_style_bg_img_src(s, &img_slider_oriz_knob, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_width(s, 0, LV_PART_KNOB);
    lv_obj_set_style_outline_width(s, 0, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(s, 0, LV_PART_KNOB);
    lv_obj_set_style_width(s, 39, LV_PART_KNOB);
    lv_obj_set_style_height(s, 45, LV_PART_KNOB);

    if (id == SRC_A) g.shape_slider_A = s;
    else             g.shape_slider_B = s;

    // Label A / B ai lati dello slider
    const char *letter = (id == SRC_A) ? "A" : "B";
    lv_obj_t *label_A = lv_label_create(p);
    lv_label_set_text(label_A, letter);
    lv_obj_set_style_text_color(label_A, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_A, &lv_font_montserrat_18, 0);
    lv_obj_align_to(label_A, s, LV_ALIGN_OUT_LEFT_MID, -15, 0);

    lv_obj_t *label_B = lv_label_create(p);
    lv_label_set_text(label_B, "B");
    lv_obj_set_style_text_color(label_B, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_B, &lv_font_montserrat_18, 0);
    lv_obj_align_to(label_B, s, LV_ALIGN_OUT_RIGHT_MID, 15, 0);

    d->id = id;
    lv_obj_add_event_cb(s, ehslider, LV_EVENT_VALUE_CHANGED, d);

    // Freccina di target
      uint8_t target = (id == SRC_A) ? g.pre_shape_A : g.pre_shape_B;
    int arrow_x = SL_X + (int)((target / 100.0f) * SL_W) - 8;
    int arrow_y = SL_Y + SL_H + 5;
    lv_obj_t *arrow = lv_label_create(p);
    lv_label_set_text(arrow, LV_SYMBOL_UP);
    lv_obj_set_style_text_color(arrow, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_text_font(arrow, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(arrow, arrow_x, arrow_y);
    if (id == SRC_A) g.shape_arrow_A = arrow;
    else             g.shape_arrow_B = arrow;
	

    bool crs_flag = (id == SRC_A) ? g.shape_crs_A : g.shape_crs_B;
    if (crs_flag) {
        lv_obj_add_flag(arrow, LV_OBJ_FLAG_HIDDEN);
        update_shape_slider_color(id, false);
    } else {
        lv_obj_clear_flag(arrow, LV_OBJ_FLAG_HIDDEN);
        update_shape_slider_color(id, true);
    }

    // ============================================================
    // GRIGLIA WAVE SHAPE — 4 colonne, senza label "Wave Shape"
    // ============================================================
    const int GRID_X = 10, GRID_Y = 245;
    const int GRID_W = 305, GRID_H = 100;

    lv_obj_t *grid_container = lv_obj_create(p);
    lv_obj_set_size(grid_container, GRID_W, GRID_H);
    lv_obj_set_pos(grid_container, GRID_X, GRID_Y);
    lv_obj_set_style_bg_opa(grid_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid_container, 0, 0);
    lv_obj_clear_flag(grid_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(grid_container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid_container, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(grid_container, 5, 0);
    lv_obj_set_style_pad_row(grid_container, 5, 0);
    lv_obj_set_style_pad_column(grid_container, 5, 0);

    d->list = grid_container;
    d->current_cat = CAT_WF;
    d->list_items = WF_ITEMS;
    populate_grid(grid_container, WF_ITEMS, 0, id);

    // ============================================================
    // BOTTONE UNICO WF/FM/AM (in basso, a destra di home)
    // ============================================================
    lv_obj_t *cat_btn = lv_btn_create(p);
    lv_obj_set_size(cat_btn, 90, 90);
    lv_obj_set_pos(cat_btn, 110, 360);
    lv_obj_set_style_bg_color(cat_btn, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(cat_btn, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_radius(cat_btn, 8, 0);
    lv_obj_set_style_border_width(cat_btn, 3, 0);
    lv_obj_set_style_border_color(cat_btn, lv_color_hex(0x00AAFF), 0);

    lv_obj_t *cat_lbl = lv_label_create(cat_btn);
    lv_label_set_text(cat_lbl, "WF");
    lv_obj_set_style_text_color(cat_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(cat_lbl, &lv_font_montserrat_24, 0);
    lv_obj_center(cat_lbl);

    d->cat_btn     = cat_btn;
    d->cat_btn_lbl = cat_lbl;
    lv_obj_add_event_cb(cat_btn, ecat_cycle, LV_EVENT_CLICKED, (void*)(uintptr_t)id);

    // ============================================================
    // BOTTONE "Edit" (visibile solo quando cat=FM)
    // ============================================================
    lv_obj_t *edit_btn = lv_btn_create(p);
    lv_obj_set_size(edit_btn, 90, 90);
    lv_obj_set_pos(edit_btn, 210, 360);
    lv_obj_set_style_bg_color(edit_btn, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(edit_btn, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_radius(edit_btn, 8, 0);
    lv_obj_set_style_border_width(edit_btn, 3, 0);
    lv_obj_set_style_border_color(edit_btn, lv_color_hex(0xFF2222), 0);

    lv_obj_t *edit_lbl = lv_label_create(edit_btn);
    lv_label_set_text(edit_lbl, "Edit");
    lv_obj_set_style_text_color(edit_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(edit_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(edit_lbl);

    d->edit_btn = edit_btn;
    lv_obj_add_flag(edit_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(edit_btn, fm_edit_btn_cb, LV_EVENT_CLICKED, NULL);

}
// ============================================================
// 3) PLOTTER / GRIGLIA / WAVEFORM
// ============================================================

void grid_btn_click(lv_event_t *e) {
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    int synth_id = id >> 8;
    int btn_idx = id & 0xFF;
    int idx = (synth_id == SRC_A) ? 0 : 1;

    int cat = shape_data[idx].current_cat;
    uint32_t sel_color = cat_color(cat);

    // Deseleziona precedente: border sottile grigio
    if (grid_selected_idx[idx] >= 0 && grid_selected_idx[idx] < grid_btn_count[idx]) {
        lv_obj_set_style_border_width(grid_btns[idx][grid_selected_idx[idx]], 1, 0);
        lv_obj_set_style_border_color(grid_btns[idx][grid_selected_idx[idx]],
                                      lv_color_hex(0x666666), 0);
    }
    // Seleziona nuovo: border spesso colorato, bg invariato
    lv_obj_set_style_border_width(grid_btns[idx][btn_idx], 2, 0);
    lv_obj_set_style_border_color(grid_btns[idx][btn_idx],
                                  lv_color_hex(sel_color), 0);
    grid_selected_idx[idx] = btn_idx;

    if (synth_id == SRC_A) {
        int global_idx;
        switch (cat) {
            case CAT_WF: global_idx = btn_idx; break;
            case CAT_FM: global_idx = 9 + btn_idx; break;
            case CAT_AM: global_idx = 17 + btn_idx; break;
            default:     global_idx = btn_idx; break;
        }
        g.wa = global_idx;
    } else {
        uint8_t fwWave;
        if (cat == CAT_WF) fwWave = (btn_idx < 9) ? ui2fw_wave[btn_idx] : 0;
        else               fwWave = (uint8_t)btn_idx;
        g.wb = btn_idx;
        uiSetParamB_U8('w', fwWave);
        uiSetParamB_U8('m', (uint8_t)cat);
    }
    update_plotter_by_wave(synth_id);
}

void populate_grid(lv_obj_t *container, const char *items, int selected_idx, int synth_id) {
    lv_obj_clean(container);
    int idx = (synth_id == SRC_A) ? 0 : 1;
    int count = 0;
    const char *start = items;
    const char *end;

    while (*start && count < 20) {
        end = start;
        while (*end && *end != '\n') end++;
        int len = end - start;
        if (len > 0) {
            char text[32];
            strncpy(text, start, len);
            text[len] = '\0';
            lv_obj_t *btn = lv_btn_create(container);
            lv_obj_set_size(btn, 70, 40);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x1A1A2E), 0);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
            lv_obj_set_style_border_width(btn, 1, 0);
            lv_obj_set_style_border_color(btn, lv_color_hex(0x666666), 0);
            lv_obj_set_style_radius(btn, 4, 0);
            lv_obj_t *label = lv_label_create(btn);
            lv_label_set_text(label, text);
            lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
            lv_obj_center(label);
            grid_btns[idx][count] = btn;
            lv_obj_add_event_cb(btn, grid_btn_click, LV_EVENT_CLICKED,
                (void*)(uintptr_t)(count | (synth_id << 8)));
            count++;
        }
        if (*end == '\n') start = end + 1;
        else break;
    }
    grid_btn_count[idx] = count;
    grid_selected_idx[idx] = selected_idx;

    // Selezionato: bg invariato, bordo spesso col colore della categoria
    if (selected_idx >= 0 && selected_idx < count) {
        uint32_t sel_color = cat_color(shape_data[idx].current_cat);
        lv_obj_set_style_border_width(grid_btns[idx][selected_idx], 2, 0);
        lv_obj_set_style_border_color(grid_btns[idx][selected_idx],
                                      lv_color_hex(sel_color), 0);
    }
}

// ============================================================
// Plotter waveform — funzioni helper
// ============================================================

// Dente di sega ↔ triangolo (morphing via shape 0..100)
static void plot_saw_wave(int *out, int n, int cycles, int shape) {
    float pos = shape / 100.0f;
    float p = constrain(1.0f - pos, 0.001f, 0.999f);
    for (int i = 0; i < n; i++) {
        float phase = (float)i / n * cycles;
        float frac  = phase - floorf(phase);
        float y = (frac <= p) ? 100.0f * frac / p
                              : 100.0f * (1.0f - frac) / (1.0f - p);
        out[i] = (int)y;
    }
}

// Triangolo ↔ dente di sega (0 = tri, 100 = saw)
static void plot_tri_wave(int *out, int n, int shape) {
    float pos = shape / 100.0f;
    for (int i = 0; i < n; i++) {
        float phase = (float)i / n * 2.0f;
        float frac  = phase - floorf(phase);
        float tri   = (frac < 0.5f) ? (frac * 2.0f) : (2.0f - frac * 2.0f);
        float saw   = frac;
        float y     = tri * (1.0f - pos) + saw * pos;
        out[i] = (int)(y * 100.0f);
    }
}

// Square con PWM (shape 1..99 → duty)
static void plot_sqr_wave(int *out, int n, int shape) {
    uint8_t duty = constrain(shape, 1, 99);
    float   d    = duty / 100.0f;
    for (int i = 0; i < n; i++) {
        float phase = (float)i / n * 2.0f;
        float frac  = phase - floorf(phase);
        out[i] = (frac < d) ? 100 : 0;
    }
}

// Seno puro (2 cicli)
static void plot_sine_wave(int *out, int n, int shape) {
    (void)shape;
    for (int i = 0; i < n; i++) {
        float phase = (float)i / n * 2.0f;
        float y     = sinf(phase * PI * 2.0f);
        out[i] = (int)((y * 0.5f + 0.5f) * 100.0f);
    }
}

// Rumore pseudocasuale deterministico (hold di 3 campioni)
static void plot_noise_wave(int *out, int n, int shape) {
    (void)shape;
    uint32_t seed = 0x12345u;
    int hold = 0;
    int val  = 50;
    for (int i = 0; i < n; i++) {
        if (hold <= 0) {
            seed = seed * 1103515245u + 12345u;
            val  = (int)((seed >> 16) % 101);
            hold = 3;
        }
        out[i] = val;
        hold--;
    }
}

// FM: carrier sin(φ) con modulatore sin(ratio·φ)
//   shape → indice FM (0..maxIdx)
//   variant 0..7 → ratio e maxIdx diversi
static void plot_fm_wave(int *out, int n, int shape, int variant) {
    static const float ratio_arr[8] = {
        1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 1.5f, 2.5f, 3.5f
    };
    static const float maxIdx[8] = {
        4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 5.0f, 6.0f, 7.0f
    };

    if (variant < 0) variant = 0;
    if (variant > 7) variant = 7;

    float ratio = ratio_arr[variant];
    float index = (shape / 100.0f) * maxIdx[variant];

    for (int i = 0; i < n; i++) {
        float phase = (float)i / n * 2.0f;
        float mod   = sinf(phase * PI * 2.0f * ratio);
        float y     = sinf(phase * PI * 2.0f + index * mod);
        out[i] = (int)((y * 0.5f + 0.5f) * 100.0f);
    }
}

// AM: carrier sin(4φ) · (1 + depth·sin(fm·φ)) / (1 + depth)
//   shape → depth (0..1)
//   variant 0..7 → frequenza modulante
static void plot_am_wave(int *out, int n, int shape, int variant) {
    static const float am_freq[8] = {
        1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f, 10.0f
    };

    if (variant < 0) variant = 0;
    if (variant > 7) variant = 7;

    float fm    = am_freq[variant];
    float depth = shape / 100.0f;

    for (int i = 0; i < n; i++) {
        float phase   = (float)i / n * 2.0f;
        float carrier = sinf(phase * PI * 2.0f * 4.0f);
        float mod     = sinf(phase * PI * 2.0f * fm);
        float y       = carrier * (1.0f + depth * mod) / (1.0f + depth);
        out[i] = (int)((y * 0.5f + 0.5f) * 100.0f);
    }
}

// ============================================================
// Plotter principale — dispatch per categoria + nome
// ============================================================
// ============================================================
// Plotter waveform — replica del firmware SynthB
// ============================================================

// ---- Wavetable base (replica di wavetable_setup) ----
static void plot_gen_base(int32_t *wt, int cat, int wave_index) {
    const float SAMPLE_LEV = 551.0f;
    const float PIx2       = 2.0f * M_PI;

    if (cat == CAT_WF) {
        int fw = ui2fw_wave[wave_index];   // 0..8
        switch (fw) {
            case 0: // SAW
                for (int i = 0; i < 256; i++)
                    wt[i] = i * 4 - (int)SAMPLE_LEV + 1;
                break;
            case 1: // SINE
                for (int i = 0; i < 256; i++)
                    wt[i] = (int)(sinf(PIx2 * i / 256.0f) * SAMPLE_LEV);
                break;
            case 2: // SQR
                for (int i = 0; i < 128; i++) {
                    wt[i]       =  (int)SAMPLE_LEV;
                    wt[i + 128] = -(int)SAMPLE_LEV;
                }
                break;
            case 3: // TRI
                for (int i = 0; i < 128; i++) {
                    wt[i]       = i * 8 - (int)SAMPLE_LEV;
                    wt[i + 128] = (int)SAMPLE_LEV - i * 8;
                }
                break;
            case 4: // OCT-SAW
                for (int i = 0; i < 128; i++) {
                    wt[i]       = i * 4 - ((int)SAMPLE_LEV + 1) + i * 2;
                    wt[i + 128] = i * 2 - (((int)SAMPLE_LEV + 1) / 2) + i * 4;
                }
                break;
            case 5: // FM1
                for (int i = 0; i < 256; i++)
                    wt[i] = (int)(sinf(PIx2*i/256.0f +
                                sinf(PIx2*3.0f*i/256.0f)) * SAMPLE_LEV);
                break;
            case 6: // FM2
                for (int i = 0; i < 256; i++)
                    wt[i] = (int)(sinf(PIx2*i/256.0f +
                                sinf(PIx2*7.0f*i/256.0f)) * SAMPLE_LEV);
                break;
            case 7: // FM3
                for (int i = 0; i < 256; i++)
                    wt[i] = (int)(sinf(PIx2*i/256.0f +
                                sinf(PIx2*4.0f*i/256.0f +
                                sinf(PIx2*11.0f*i/256.0f))) * SAMPLE_LEV);
                break;
            case 8: { // NOISE (deterministico)
                uint32_t seed = 0x12345u;
                for (int i = 0; i < 256; i++) {
                    seed = seed * 1103515245u + 12345u;
                    wt[i] = (int)((seed >> 16) % 510) + 511 - (int)SAMPLE_LEV;
                }
                break;
            }
            default:
                for (int i = 0; i < 256; i++) wt[i] = 0;
                break;
        }
    }
    else if (cat == CAT_AM) {
        int fw = wave_index - 17;   // 0..7
        switch (fw) {
            case 0: for (int i = 0; i < 256; i++)
                        wt[i] = (int)(sinf(PIx2*i/256.0f) * SAMPLE_LEV); break;
            case 1: for (int i = 0; i < 256; i++)
                        wt[i] = (int)(sinf(PIx2*i/256.0f +
                                    sinf(PIx2*3.0f*i/256.0f)) * SAMPLE_LEV); break;
            case 2: for (int i = 0; i < 256; i++)
                        wt[i] = (int)(sinf(PIx2*i/256.0f +
                                    sinf(PIx2*5.0f*i/256.0f)) * SAMPLE_LEV); break;
            case 3: for (int i = 0; i < 256; i++)
                        wt[i] = (int)(sinf(PIx2*i/256.0f +
                                    sinf(PIx2*4.0f*i/256.0f +
                                    sinf(PIx2*11.0f*i/256.0f))) * SAMPLE_LEV); break;
            case 4: for (int i = 0; i < 256; i++)
                        wt[i] = (int)(sinf(PIx2*i/256.0f +
                                    sinf(PIx2*1.28f*i/256.0f)) * SAMPLE_LEV); break;
            case 5: for (int i = 0; i < 256; i++)
                        wt[i] = (int)(sinf(PIx2*i/256.0f +
                                    sinf(PIx2*3.19f*i/256.0f)) * SAMPLE_LEV); break;
            case 6: for (int i = 0; i < 256; i++)
                        wt[i] = (int)(sinf(PIx2*i/256.0f +
                                    sinf(PIx2*2.3f*i/256.0f +
                                    sinf(PIx2*7.3f*i/256.0f))) * SAMPLE_LEV); break;
            case 7: for (int i = 0; i < 256; i++)
                        wt[i] = (int)(sinf(PIx2*i/256.0f +
                                    sinf(PIx2*6.3f*i/256.0f +
                                    sinf(PIx2*11.3f*i/256.0f))) * SAMPLE_LEV); break;
            default: for (int i = 0; i < 256; i++) wt[i] = 0; break;
        }
    }
    else {
        // FM: base zero (viene riempita dal loop FM)
        for (int i = 0; i < 256; i++) wt[i] = 0;
    }
}

// ---- Wavefold (replica del loop1 case mode=0) ----
static void plot_apply_wavefold(const int32_t *in, int32_t *out,
                                 int fw_wave, float mod) {
    if (fw_wave == 2) {
        // SQR: PWM (mod = offset 0..250)
        int modInt = (int)mod;
        if (modInt < 0)   modInt = 0;
        if (modInt > 250) modInt = 250;
        for (int i = 0; i < 128 + modInt; i++) out[i] =  511;
        for (int i = 128 + modInt; i < 256; i++) out[i] = -511;
        return;
    }
    // Wavefold generico
    for (int i = 0; i < 256; i++) {
        float m = (float)in[i] * mod;
        int32_t v;
        if      (m >  511 && m <  1535) v =  1024 - (int)m;
        else if (m < -512 && m > -1536) v = -1023 - (int)m;
        else if (m < -1535)             v =  2048 + (int)m;
        else if (m >  1534)             v =  (int)m - 2047;
        else                            v =  (int)m;
        out[i] = v;
    }
}

// ---- FM (replica del loop1 case mode=1) ----
static void plot_apply_fm(int32_t *out, int mm, int fw_case, int fmSel) {
    const float PIx2 = 2.0f * M_PI;
    float mm_f = (float)mm;

    // Ratio float (evita divisione intera)
    float r0 = (float)fmSetSin[fmSel][0] / (float)fmSetDiv[fmSel][0];
    float r1 = (float)fmSetSin[fmSel][1] / (float)fmSetDiv[fmSel][1];
    float r2 = (float)fmSetSin[fmSel][2] / (float)fmSetDiv[fmSel][2];

    for (int i = 0; i < 256; i++) {
        float v = 0.0f;
        switch (fw_case) {
            case 0: case 2: case 3: case 6: case 7:
                v = sinf(PIx2*i/256
                    + mm_f/128 * sinf(PIx2*r0*i/256
                    + mm_f/128 * sinf(PIx2*r1*i/256
                    + mm_f/128 * sinf(PIx2*r2*i/256)))) * 511;
                break;
            case 1: case 4: case 5:
                v = (sinf(PIx2*i/256 + mm_f/128*sinf(PIx2*r0*i/256))
                   + sinf(PIx2*r1*i/256
                   + mm_f/128*sinf(PIx2*r2*i/256))) * 250;
                break;
            default:
                v = sinf(PIx2*i/256) * 511;
                break;
        }
        out[i] = (int)v;
    }
}

// ---- Plotter principale (dispatch per categoria) ----
void update_wave_plot(lv_obj_t *chart, lv_chart_series_t *serie,
                      uint8_t shape_val, int wave_index) {
    if (!chart || !serie) return;
    if (wave_index < 0 || wave_index >= NUM_WAVES) return;

    int cat = WAVE_DEFS[wave_index].category;

    // Colore serie in base alla categoria
    uint32_t col = 0xFFFF00;                    // WF → giallo
    if      (cat == CAT_FM) col = 0xFF8800;     // FM → arancio
    else if (cat == CAT_AM) col = 0xAA44FF;     // AM → viola
    lv_chart_set_series_color(chart, serie, lv_color_hex(col));

    static int32_t wt_base[256];
    static int32_t wt_mod [256];

    // 1. Wavetable base
    plot_gen_base(wt_base, cat, wave_index);

    // 2. tmpmod equivalente: shape 0..100 → 0..1023
    int tmpmod = (shape_val * 1023) / 100;

    // 3. Elaborazione per categoria
    if (cat == CAT_WF) {
        int fw_wave = ui2fw_wave[wave_index];
        float mod;
        if (fw_wave != 2)  mod = (float)tmpmod * 0.0036f + 0.90f;
        else               mod = (float)(tmpmod >> 3);
        plot_apply_wavefold(wt_base, wt_mod, fw_wave, mod);
    }
     
	     else if (cat == CAT_FM) {
        int fmSel   = wave_index - 9;      // 0..7 → sceglie il set FM
        int fw_case = 0;                    // topologia: 3-op FM (default)
        // Indice FM: il firmware usa 0..1 (mm/128 con mm≤127).
        // Moltiplichiamo per FM_PLOT_SCALE per rendere visibile
        // la distorsione nel plotter (didattico, non fedele al bit).
        #define FM_PLOT_SCALE 12
        int mm = (tmpmod >> 3) * FM_PLOT_SCALE;
        plot_apply_fm(wt_mod, mm, fw_case, fmSel);
    }
	
	else if (cat == CAT_AM) {
        // Nel firmware il mod controlla la velocità della modulazione.
        // Qui mostriamo l'ampiezza istantanea in funzione di shape.
        int am_k = (int)((shape_val * 63) / 100);   // 0..63
        float sinVal = sinf(2.0f * M_PI * am_k / 63.0f);
        for (int i = 0; i < 256; i++)
            wt_mod[i] = (int)(wt_base[i] * sinVal);
    }
    else {
        // fallback
        for (int i = 0; i < 256; i++) wt_mod[i] = wt_base[i];
    }

    // 4. Disegna — mappa 256 punti in WAVE_POINTS
    //    Range wavefold: circa -1024..1024 → normalizza in 0..100
    for (int i = 0; i < WAVE_POINTS; i++) {
        int src_i = (i * 256) / WAVE_POINTS;
        int32_t v = wt_mod[src_i];
        // Normalizza da -550..550 a 0..100 (clamp per wavefold estremi)
        int y = (int)(((int64_t)v + 550) * 100 / 1100);
        if (y < 0)   y = 0;
        if (y > 100) y = 100;
        lv_chart_set_next_value(chart, serie, y);
    }
    lv_chart_refresh(chart);
}

void update_plotter_by_wave(int synth_id) {
    if (synth_id != SRC_A && synth_id != SRC_B) return;
    int idx = (synth_id == SRC_A) ? 0 : 1;
    ShapeData *d = &shape_data[idx];
    if (!d->chart || !d->serie) return;
    int wave_idx = (synth_id == SRC_A) ? g.wa : g.wb;
    uint8_t shape_val = (synth_id == SRC_A) ? g.shape_a : g.shape_b;
    update_wave_plot(d->chart, d->serie, shape_val, wave_idx);
}

// ============================================================
// 4) LED E COLORI
// ============================================================
void update_leds(lv_obj_t **leds, int active_cat) {
    for (int i = 0; i < 3; i++) {
        lv_obj_set_style_bg_color(leds[i],
            (i == active_cat) ? lv_color_hex(0x00FF00) : lv_color_hex(0x333333), 0);
    }
}

void update_slider_color(int idx, bool active) {
    if (idx < 0 || idx > 7) return;
    lv_obj_t *slider = slider_objs[idx];
    if (!slider) return;
    lv_color_t active_color, passed_color;
    if (strcmp(g.synth, "SYNTH A") == 0) {
        active_color = lv_color_hex(COLOR_SLIDER_SYNTH_A_ACTIVE);
        passed_color = lv_color_hex(COLOR_SLIDER_SYNTH_A_PASSED);
    } else {
        active_color = lv_color_hex(COLOR_SLIDER_SYNTH_B_ACTIVE);
        passed_color = lv_color_hex(COLOR_SLIDER_SYNTH_B_PASSED);
    }
    lv_color_t target = active ? active_color : passed_color;
    lv_obj_set_style_bg_img_recolor(slider, target, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor_opa(slider, (sliderColorDepth * 255) / 100, LV_PART_INDICATOR);
}

void update_shape_slider_color(int synth_id, bool active) {
    lv_obj_t *slider = (synth_id == SRC_A) ? g.shape_slider_A : g.shape_slider_B;
    if (!slider) return;
    lv_color_t target = active ? lv_color_hex(COLOR_SHAPE_SLIDER_ACTIVE)
                               : lv_color_hex(COLOR_SHAPE_SLIDER_PASSED);
    lv_obj_set_style_bg_img_recolor(slider, target, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor_opa(slider, (sliderColorDepth * 255) / 100, LV_PART_INDICATOR);
}

// ============================================================
// 5) ARC + POT CONTAINER
// ============================================================
void earc_changed(lv_event_t *e) {
    lv_obj_t *arc = lv_event_get_target(e);
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= 6) return;

    int cur    = lv_arc_get_value(arc);
    int prev   = g.arc_last[idx];
    int target = g.arc_target[idx];
    g.arc_value[idx] = cur;

    if (g.arc_label_value[idx] && lv_obj_is_valid(g.arc_label_value[idx]))
        lv_label_set_text_fmt(g.arc_label_value[idx], "%d", cur);

    if (g.arc_arrow[idx] && g.arc_obj[idx] &&
        lv_obj_is_valid(g.arc_arrow[idx]) && lv_obj_is_valid(g.arc_obj[idx])) {
        if (!g.arc_crossed[idx]) {
            bool crossed = (prev < target && cur >= target) ||
                           (prev > target && cur <= target) ||
                           (prev == target && cur != target);
            if (crossed) {
                g.arc_crossed[idx] = true;
                lv_obj_add_flag(g.arc_arrow[idx], LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_style_arc_color(g.arc_obj[idx],
                    lv_color_hex(COLOR_ARC_PASSED), LV_PART_INDICATOR);
            } else {
                lv_obj_clear_flag(g.arc_arrow[idx], LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_style_arc_color(g.arc_obj[idx],
                    lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
            }
        }
    }
    g.arc_last[idx] = cur;
}

void arc_with_image(lv_obj_t *parent, int idx, int x, int y, int w, int h, const char *pname) {
    if (idx < 0 || idx >= 6) return;

    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, w, h);
    lv_obj_set_pos(arc, x, y);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, g.arc_value[idx]);

    lv_obj_set_style_arc_img_src(arc, &img_arc_bg,    LV_PART_MAIN);
    lv_obj_set_style_arc_img_src(arc, &img_arc_indic, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color  (arc, lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa     (arc, LV_OPA_TRANSP,  LV_PART_KNOB);
    g.arc_obj[idx] = arc;

    lv_obj_t *plabel = lv_label_create(parent);
    lv_label_set_text(plabel, pname);
    lv_obj_set_style_text_color(plabel, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_style_text_font (plabel, &lv_font_montserrat_20, 0);
    lv_obj_align_to(plabel, arc, LV_ALIGN_CENTER, 0, 0);
    g.arc_label_p[idx] = plabel;

    lv_obj_t *val_label = lv_label_create(parent);
    lv_obj_set_style_text_color(val_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font (val_label, &lv_font_montserrat_20, 0);
    lv_label_set_text_fmt(val_label, "%d", g.arc_value[idx]);
    lv_obj_align_to(val_label, arc, LV_ALIGN_BOTTOM_MID, 0, 0);
    g.arc_label_value[idx] = val_label;

    float angle = 135.0f + (g.arc_target[idx] * 2.7f);
    if (angle >= 360.0f) angle -= 360.0f;
    if (angle <  0.0f)   angle += 360.0f;

    int radius   = (w / 2) - 2 + 10;
    int center_x = x + w / 2;
    int center_y = y + h / 2;
    float rad = angle * PI / 180.0f;
    int dot_x = center_x + (int)(radius * cosf(rad)) - 4;
    int dot_y = center_y + (int)(radius * sinf(rad)) - 4;

    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_pos(dot, dot_x, dot_y);
    g.arc_arrow[idx] = dot;

    if (g.arc_value[idx] > g.arc_target[idx]) {
        g.arc_crossed[idx] = true;
        lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_arc_color(arc, lv_color_hex(COLOR_ARC_PASSED), LV_PART_INDICATOR);
    } else {
        g.arc_crossed[idx] = false;
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_arc_color(arc, lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
    }
    lv_obj_add_event_cb(arc, earc_changed, LV_EVENT_VALUE_CHANGED, (void*)(uintptr_t)idx);
}

lv_obj_t* create_pot_container(lv_obj_t *parent, int x, int y) {
    const int arc_w   = 80;
    const int arc_h   = 80;
    const int gap     = 25;
    const int pad_top = 20;

    const int cont_w = 6 * arc_w + 5 * gap;
    const int cont_h = pad_top + arc_h;

    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, cont_w, cont_h);
    lv_obj_set_pos(cont, x, y);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    static const char *pnames[6] = {"P1","P2","P3","P4","P5","P6"};
    for (int i = 0; i < 6; i++) {
        int px = i * (arc_w + gap);
        arc_with_image(cont, i, px, pad_top, arc_w, arc_h, pnames[i]);
    }
    pot_container = cont;
    return cont;
}

// ============================================================
// 6) TIMELINE
// ============================================================
static void format_time(uint32_t ms, char *buf, size_t buflen) {
    uint32_t total_s = ms / 1000;
    uint32_t m = total_s / 60;
    uint32_t s = total_s % 60;
    if (m > 99) m = 99;
    snprintf(buf, buflen, "%02u:%02u", (unsigned)m, (unsigned)s);
}

// ============================================================
// Timeline controls — 2 bottoni: Rewind + Play/Stop toggle
// ============================================================
static void tl_init_cb(lv_event_t *e) {
    (void)e;
    // Rewind: reset del tempo a 0 (funziona anche durante il play)
    timeline_demo_ms = 0;
    update_timeline(0, 60000);
}

static void tl_play_cb(lv_event_t *e) {
    (void)e;
    timeline_playing = !timeline_playing;

    lv_obj_t *lbl = nullptr;
    if (timeline_btn_play && lv_obj_is_valid(timeline_btn_play))
        lbl = lv_obj_get_child(timeline_btn_play, 0);

    if (timeline_playing) {
        if (lbl) lv_label_set_text(lbl, LV_SYMBOL_STOP);
        if (timeline_btn_play && lv_obj_is_valid(timeline_btn_play))
            lv_obj_set_style_border_color(timeline_btn_play,
                                          lv_color_hex(0x888888), 0);
    } else {
        if (lbl) lv_label_set_text(lbl, LV_SYMBOL_PLAY);
        if (timeline_btn_play && lv_obj_is_valid(timeline_btn_play))
            lv_obj_set_style_border_color(timeline_btn_play,
                                          lv_color_hex(0x00FF00), 0);
    }
}

lv_obj_t* create_timeline_controls(lv_obj_t *parent, int x, int y) {
    const int btn_w = 100;
    const int btn_h = 80;
    const int gap   = 10;

    // ---- Rewind to start (⏮) ----
    timeline_btn_init = mkbtn(parent, x, y, btn_w, btn_h,
                              0x00AAFF,
                              LV_SYMBOL_PREV,
                              &lv_font_montserrat_32,
                              tl_init_cb, 0, 8, 2);

    // ---- Play/Stop toggle (▶ / ■) ----
    timeline_btn_play = mkbtn(parent, x + btn_w + gap, y, btn_w, btn_h,
                              0x00FF00,
                              LV_SYMBOL_PLAY,
                              &lv_font_montserrat_32,
                              tl_play_cb, 0, 8, 2);

    return timeline_btn_init;
}

lv_obj_t* create_timeline(lv_obj_t *parent, int x, int y, int w, int h) {
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, w, h);
    lv_obj_set_pos(cont, x, y);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    const int time_w = 70;
    const int bar_x  = time_w + 10;
    const int bar_w  = w - 2 * (time_w + 10);
    const int bar_h  = 8;
    const int bar_y  = (h - bar_h) / 2;

    timeline_label_cur = lv_label_create(cont);
    lv_label_set_text(timeline_label_cur, "00:00");
    lv_obj_set_style_text_color(timeline_label_cur, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(timeline_label_cur, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(timeline_label_cur, 0, (h - 22) / 2);

    timeline_bar_bg = lv_obj_create(cont);
    lv_obj_set_size(timeline_bar_bg, bar_w, bar_h);
    lv_obj_set_pos(timeline_bar_bg, bar_x, bar_y);
    lv_obj_set_style_bg_color(timeline_bar_bg, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(timeline_bar_bg, 0, 0);
    lv_obj_set_style_radius(timeline_bar_bg, bar_h / 2, 0);
    lv_obj_set_style_pad_all(timeline_bar_bg, 0, 0);
    lv_obj_clear_flag(timeline_bar_bg, LV_OBJ_FLAG_SCROLLABLE);

    timeline_bar_fill = lv_obj_create(cont);
    lv_obj_set_size(timeline_bar_fill, 0, bar_h);
    lv_obj_set_pos(timeline_bar_fill, bar_x, bar_y);
    lv_obj_set_style_bg_color(timeline_bar_fill, lv_color_hex(0x00AAFF), 0);
    lv_obj_set_style_border_width(timeline_bar_fill, 0, 0);
    lv_obj_set_style_radius(timeline_bar_fill, bar_h / 2, 0);
    lv_obj_set_style_pad_all(timeline_bar_fill, 0, 0);
    lv_obj_clear_flag(timeline_bar_fill, LV_OBJ_FLAG_SCROLLABLE);

    timeline_cursor = lv_obj_create(cont);
    lv_obj_set_size(timeline_cursor, 4, h - 16);
    lv_obj_set_pos(timeline_cursor, bar_x - 2, 8);
    lv_obj_set_style_bg_color(timeline_cursor, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_style_border_width(timeline_cursor, 0, 0);
    lv_obj_set_style_radius(timeline_cursor, 2, 0);
    lv_obj_clear_flag(timeline_cursor, LV_OBJ_FLAG_SCROLLABLE);

    timeline_label_tot = lv_label_create(cont);
    lv_label_set_text(timeline_label_tot, "00:00");
    lv_obj_set_style_text_color(timeline_label_tot, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(timeline_label_tot, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(timeline_label_tot, w - time_w, (h - 22) / 2);

    timeline_bar_w = bar_w;
    timeline_obj   = cont;

    update_timeline(timeline_cur_ms, timeline_total_ms);
    return cont;
}

void update_timeline(uint32_t cur_ms, uint32_t tot_ms) {
    timeline_cur_ms   = cur_ms;
    timeline_total_ms = tot_ms;

    char buf[16];
    if (timeline_label_cur && lv_obj_is_valid(timeline_label_cur)) {
        format_time(cur_ms, buf, sizeof(buf));
        lv_label_set_text(timeline_label_cur, buf);
    }
    if (timeline_label_tot && lv_obj_is_valid(timeline_label_tot)) {
        format_time(tot_ms, buf, sizeof(buf));
        lv_label_set_text(timeline_label_tot, buf);
    }

    float pct = 0.0f;
    if (tot_ms > 0) pct = (float)cur_ms / (float)tot_ms;
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 1.0f) pct = 1.0f;

    if (timeline_bar_fill && lv_obj_is_valid(timeline_bar_fill)) {
        int fill_w = (int)(timeline_bar_w * pct);
        if (fill_w < 1 && pct > 0.0f) fill_w = 1;
        lv_obj_set_width(timeline_bar_fill, fill_w);
    }
    if (timeline_cursor && lv_obj_is_valid(timeline_cursor) &&
        timeline_bar_bg && lv_obj_is_valid(timeline_bar_bg)) {
        int bg_x = lv_obj_get_x(timeline_bar_bg);
        int cx   = bg_x + (int)(timeline_bar_w * pct) - 2;
        lv_obj_set_x(timeline_cursor, cx);
    }
}

// ============================================================
// 7) SUBMENU (DCO/VCF/MOD/VCA/DLY)
// ============================================================

void submenu(lv_obj_t *p) {
    lv_obj_add_flag(p, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_clip_corner(p, 0, 0);

    home_btn_at(p, -1, 30, 360);

    // Suffisso in base al synth attivo
    const char *sfx = (g.synth && strcmp(g.synth, "SYNTH B") == 0) ? " B" : " A";

    const char *names[] = {"DCO","VCF","MOD","VCA","DLY"};
    lv_color_t cols[] = {
        lv_color_hex(0x00FF00),   // DCO
        lv_color_hex(0xCC3300),   // VCF
        lv_color_hex(0x0099FF),   // MOD
        lv_color_hex(0xFFCC33),   // VCA
        lv_color_hex(0xBB88FF)    // DLY
    };
    char label[16];
    for (int i = 0; i < 5; i++) {
        snprintf(label, sizeof(label), "%s%s", names[i], sfx);
        btn(p, label, 135 + i * 130, 360, 115, 90, cols[i], 10 + i);
    }
	  // ---- CHORUS (Synth A) / PHASER (Synth B) sopra DLY ----
    bool isA = (g.synth && strcmp(g.synth, "SYNTH A") == 0);
    if (isA) {
        btn(p, "CHORUS", 655, 260, 115, 90,
            lv_color_hex(0x006600), 35);
    } else {
        btn(p, "PHASER", 655, 260, 115, 90,
            lv_color_hex(0xFF6600), 36);
    }
}

// ============================================================
// 8) METER
// ============================================================
lv_obj_t* meter(lv_obj_t *p, int x, int y, int w, int h, const char *label, bool inv) {
    lv_obj_t *c = lv_obj_create(p);
    lv_obj_set_size(c, w, h); lv_obj_set_pos(c, x, y);
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_set_style_radius(c, 0, 0);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    if (inv) {
        lv_obj_set_style_transform_angle(c, 1800, 0);
        lv_obj_set_style_transform_pivot_x(c, w/2, 0);
        lv_obj_set_style_transform_pivot_y(c, h/2, 0);
    }

    lv_obj_t *s = lv_slider_create(c);
    lv_obj_set_size(s, w, h);
    lv_obj_align(s, LV_ALIGN_CENTER, 0, 0);
    lv_slider_set_range(s, 0, 100);
    lv_slider_set_value(s, 0, LV_ANIM_OFF);
    lv_obj_set_style_radius(s, 0, 0);
    lv_obj_set_style_radius(s, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s, 0, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s, 0, LV_PART_KNOB);

    const lv_img_dsc_t *tr = inv ? &img_meter_comp_track : &img_meter_audio_track;
    lv_obj_set_style_bg_img_src(s, tr, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, LV_PART_MAIN);
    const lv_img_dsc_t *ind = inv ? &img_meter_comp_indicator : &img_meter_audio_indicator;
    lv_obj_set_style_bg_img_src(s, ind, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, LV_PART_INDICATOR);

    lv_obj_set_style_bg_img_src(s, NULL, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(s, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_width(s, 0, LV_PART_KNOB);
    lv_obj_set_style_outline_width(s, 0, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(s, 0, LV_PART_KNOB);
    lv_obj_clear_flag(s, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s, LV_OBJ_FLAG_PRESS_LOCK);

    lv_obj_t *lb = lv_label_create(p);
    lv_label_set_text(lb, label);
    lv_obj_set_style_text_color(lb, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(lb, &lv_font_montserrat_14, 0);
    lv_obj_align_to(lb, c, LV_ALIGN_OUT_TOP_MID, 0, -4);
    return s;
}

void update_meters() {
    if (!timeline_playing) return;
    static float ph = 0;
    ph += 0.1f;
    if (ph > 6.28f) ph -= 6.28f;
    float vL = constrain(0.5f + 0.5f*sinf(ph) + 0.05f*((float)random(0,100)/100.0f - 0.5f), 0, 1);
    float vR = constrain(0.5f + 0.5f*cosf(ph*0.7f) + 0.05f*((float)random(0,100)/100.0f - 0.5f), 0, 1);
    float vC = constrain((vL+vR)*0.5f * (1.0f - 0.3f*(vL+vR)*0.5f), 0, 1);
    float dk = 0.95f;
    pkL = max(vL, pkL*dk); pkR = max(vR, pkR*dk); pkC = max(vC, pkC*dk);
    if (mL) lv_slider_set_value(mL, (int)(pkL*100), LV_ANIM_OFF);
    if (mR) lv_slider_set_value(mR, (int)(pkR*100), LV_ANIM_OFF);
    if (mC) lv_slider_set_value(mC, (int)(pkC*100), LV_ANIM_OFF);
}

void stop_meters() {
    if (mt) { lv_timer_del(mt); mt=0; }
    mL=mR=mC=0; pkL=pkR=pkC=0;
}

// ============================================================
// 9) LOG / TOAST
// ============================================================
void log_show() { if (g.log_f) { lv_obj_clear_flag(g.log_f, LV_OBJ_FLAG_HIDDEN); g.log_v = 1; } }
void log_hide() { if (g.log_f) { lv_obj_add_flag(g.log_f, LV_OBJ_FLAG_HIDDEN); g.log_v = 0; } }

void log_add(const char *msg, lv_color_t c) {
    if (!g.log_c) return;
    lv_obj_t *l = lv_label_create(g.log_c);
    lv_label_set_text(l, msg);
    lv_obj_set_style_text_color(l, c, 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_24, 0);
    lv_obj_set_width(l, lv_obj_get_width(g.log_c) - 10);
    lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    lv_obj_scroll_to_view(l, LV_ANIM_OFF);
    g.log_t = millis();
    log_show();
    if (c.full == lv_color_hex(0xFF0000).full) g.err = 1;
}

void toast_show(const char *msg, lv_color_t c, uint32_t dur) {
    if (!g.toast) {
        if (!g.page) return;
        g.toast = lv_label_create(g.page);
        lv_obj_set_style_text_font(g.toast, &lv_font_montserrat_24, 0);
        lv_obj_set_style_bg_color(g.toast, lv_color_hex(0x000000), 0);
        lv_obj_set_style_pad_all(g.toast, 10, 0);
        lv_obj_set_style_radius(g.toast, 5, 0);
        lv_obj_set_style_border_width(g.toast, 2, 0);
        lv_obj_align(g.toast, LV_ALIGN_TOP_MID, 0, 80);
        lv_obj_set_width(g.toast, 400);
        lv_label_set_long_mode(g.toast, LV_LABEL_LONG_WRAP);
    }
    if (!g.toast) return;
    lv_label_set_text(g.toast, msg);
    lv_obj_set_style_text_color(g.toast, c, 0);
    lv_obj_set_style_border_color(g.toast, c, 0);
    lv_obj_clear_flag(g.toast, LV_OBJ_FLAG_HIDDEN);
    g.toast_v = 1;
    g.toast_t = millis() + dur;
}

void create_log_widget(lv_obj_t *parent, int x, int y, int w, int h) {
    g.log_f = lv_obj_create(parent);
    lv_obj_set_size(g.log_f, w, h);
    lv_obj_set_pos(g.log_f, x, y);
    lv_obj_set_style_bg_color(g.log_f, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_border_width(g.log_f, 2, 0);
    lv_obj_set_style_border_color(g.log_f, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(g.log_f, 4, 0);
    lv_obj_clear_flag(g.log_f, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(g.log_f, LV_OBJ_FLAG_HIDDEN);

    g.log_c = lv_obj_create(g.log_f);
    lv_obj_set_size(g.log_c, w - 20, h - 20);
    lv_obj_set_pos(g.log_c, 10, 10);
    lv_obj_set_style_bg_color(g.log_c, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_border_width(g.log_c, 0, 0);
    lv_obj_set_style_pad_all(g.log_c, 5, 0);
    lv_obj_set_style_pad_row(g.log_c, 3, 0);
    lv_obj_set_flex_flow(g.log_c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g.log_c, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(g.log_c, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(g.log_c, LV_OBJ_FLAG_SCROLLABLE);
}

// ============================================================
// 10) EEPROM BRIGHTNESS
// ============================================================
void set_bright(uint8_t v) {
    g.bright = constrain(v, 0, 100);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, map(g.bright, 0, 100, 0, 1023));
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void load_bright() {
    uint8_t v = EEPROM.read(EEPROM_ADDR);
    g.bright = (v <= 100) ? v : 100;
    set_bright(g.bright);
}

void save_bright() {
    EEPROM.write(EEPROM_ADDR, g.bright);
    EEPROM.commit();
    char b[20]; snprintf(b, 20, "Lum: %d%%", g.bright);
    log_add(b, lv_color_hex(0x00FF00));
    toast_show(b, lv_color_hex(0x00FF00), TOAST_DUR);
}

// ============================================================
// 11) RESET FRECCINE
// ============================================================
void reset_arrows() {
    for (int i = 0; i < MAX_SLIDERS; i++) {
        crs[i] = 0;
        if (arr[i] && lv_obj_is_valid(arr[i])) {
            lv_obj_clear_flag(arr[i], LV_OBJ_FLAG_HIDDEN);
            if (sd[i].label && lv_obj_is_valid(sd[i].label))
                lv_obj_set_style_text_color(sd[i].label, lv_color_hex(0xFFA500), 0);
        } else arr[i] = 0;
        if (slider_objs[i] && lv_obj_is_valid(slider_objs[i]))
            update_slider_color(i, true);
        else slider_objs[i] = 0;
    }

    g.shape_crs_A = false;
    g.shape_crs_B = false;
    if (g.shape_arrow_A && lv_obj_is_valid(g.shape_arrow_A)) {
        lv_obj_clear_flag(g.shape_arrow_A, LV_OBJ_FLAG_HIDDEN);
        if (g.shape_slider_A && lv_obj_is_valid(g.shape_slider_A))
            update_shape_slider_color(SRC_A, true);
    } else { g.shape_arrow_A = 0; g.shape_slider_A = 0; }
    if (g.shape_arrow_B && lv_obj_is_valid(g.shape_arrow_B)) {
        lv_obj_clear_flag(g.shape_arrow_B, LV_OBJ_FLAG_HIDDEN);
        if (g.shape_slider_B && lv_obj_is_valid(g.shape_slider_B))
            update_shape_slider_color(SRC_B, true);
    } else { g.shape_arrow_B = 0; g.shape_slider_B = 0; }

    for (int i = 0; i < MAX_ARCS; i++) {
        g.arc_crossed[i] = false;
        if (g.arc_obj[i] && lv_obj_is_valid(g.arc_obj[i])) {
            if (g.arc_arrow[i] && lv_obj_is_valid(g.arc_arrow[i])) {
                lv_obj_clear_flag(g.arc_arrow[i], LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_style_arc_color(g.arc_obj[i],
                    lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
            } else g.arc_arrow[i] = NULL;
        } else {
            g.arc_obj[i]   = NULL;
            g.arc_arrow[i] = NULL;
        }
    }
extern void chorus_offset_reset_arrow();   
    log_add("Freccine ripristinate", lv_color_hex(0x66AAFF));
    toast_show("Freccine ripristinate!", lv_color_hex(0x66AAFF), TOAST_DUR);
}

// ============================================================
// 12) ENVELOPE PLOTTER
// ============================================================
lv_obj_t* create_env_plot(lv_obj_t *parent, int x, int y, int w, int h, bool isA) {
    lv_obj_t *chart = lv_chart_create(parent);
    lv_obj_set_size(chart, w, h);
    lv_obj_set_pos(chart, x, y);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_point_count(chart, ENV_POINTS);
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(chart, 1, 0);
    lv_obj_set_style_border_color(chart, lv_color_hex(0x666666), 0);
    lv_obj_set_style_radius(chart, 4, 0);
    lv_chart_set_div_line_count(chart, 0, 0);
    lv_obj_clear_flag(chart, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(chart, LV_OBJ_FLAG_CLICKABLE);

    lv_chart_series_t *serie = lv_chart_add_series(chart,
                                lv_color_hex(0xFFFF00), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_series_t *serie_tgt = lv_chart_add_series(chart,
                                lv_color_hex(0xFF4444), LV_CHART_AXIS_PRIMARY_Y);

    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "ENV");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lbl, x, y - 18);

    if (isA) { env_chart_A = chart; env_serie_A = serie; env_serie_tgt_A = serie_tgt; }
    else     { env_chart_B = chart; env_serie_B = serie; env_serie_tgt_B = serie_tgt; }
    return chart;
}

static void compute_env_points(int vA, int vD, int vS, int vR, int *out, int n) {
    float tA = (vA + 4) / 259.0f;
    float tD = (vD + 4) / 259.0f;
    float tR = (vR + 4) / 259.0f;
    float tS = 0.4f;
    float total = tA + tD + tS + tR;
    float sLevel = vS / 255.0f;

    for (int i = 0; i < n; i++) {
        float x     = (float)i / (n - 1);
        float t_pos = x * total;
        float y     = 0.0f;

        if (t_pos < tA) {
            y = (tA > 0.001f) ? (t_pos / tA) : 1.0f;
        } else if (t_pos < tA + tD) {
            float d = t_pos - tA;
            y = 1.0f - (1.0f - sLevel) * (d / tD);
        } else if (t_pos < tA + tD + tS) {
            y = sLevel;
        } else {
            float r = t_pos - tA - tD - tS;
            y = sLevel * (1.0f - r / tR);
        }

        int yi = (int)(y * 100.0f);
        if (yi < 0)   yi = 0;
        if (yi > 100) yi = 100;
        out[i] = yi;
    }
}

void update_env_plot(bool isA) {
    lv_obj_t          *chart     = isA ? env_chart_A     : env_chart_B;
    lv_chart_series_t *serie     = isA ? env_serie_A     : env_serie_B;
    lv_chart_series_t *serie_tgt = isA ? env_serie_tgt_A : env_serie_tgt_B;
    if (!chart || !lv_obj_is_valid(chart) || !serie) return;

    int cA = 0, cD = 0, cS = 0, cR = 0;
    if (slider_objs[4] && lv_obj_is_valid(slider_objs[4])) cA = lv_slider_get_value(slider_objs[4]);
    if (slider_objs[5] && lv_obj_is_valid(slider_objs[5])) cD = lv_slider_get_value(slider_objs[5]);
    if (slider_objs[6] && lv_obj_is_valid(slider_objs[6])) cS = lv_slider_get_value(slider_objs[6]);
    if (slider_objs[7] && lv_obj_is_valid(slider_objs[7])) cR = lv_slider_get_value(slider_objs[7]);

    int tA = g.pre[4];
    int tD = g.pre[5];
    int tS = g.pre[6];
    int tR = g.pre[7];

    bool all_crossed = crs[4] && crs[5] && crs[6] && crs[7];

    int pts_cur[ENV_POINTS];
    compute_env_points(cA, cD, cS, cR, pts_cur, ENV_POINTS);
    for (int i = 0; i < ENV_POINTS; i++)
        lv_chart_set_next_value(chart, serie, pts_cur[i]);

    if (serie_tgt) {
        if (all_crossed) {
            lv_chart_hide_series(chart, serie_tgt, true);
            lv_chart_set_series_color(chart, serie, lv_color_hex(0xFFFFFF));
        } else {
            lv_chart_hide_series(chart, serie_tgt, false);
            lv_chart_set_series_color(chart, serie, lv_color_hex(0xFFFF00));
            int pts_tgt[ENV_POINTS];
            compute_env_points(tA, tD, tS, tR, pts_tgt, ENV_POINTS);
            for (int i = 0; i < ENV_POINTS; i++)
                lv_chart_set_next_value(chart, serie_tgt, pts_tgt[i]);
        }
    }
    lv_chart_refresh(chart);
}

// ============================================================
// Disegna un envelope ADSR su chart già esistente.
// (usato dal plotter VCF-B; l'ENV_POINTS è quello locale al file)
// ============================================================
void env_plot_draw(lv_obj_t *chart, lv_chart_series_t *serie,
                   int vA, int vD, int vS, int vR) {
    if (!chart || !lv_obj_is_valid(chart) || !serie) return;
    int pts[ENV_POINTS];
    compute_env_points(vA, vD, vS, vR, pts, ENV_POINTS);
    for (int i = 0; i < ENV_POINTS; i++) {
        lv_chart_set_next_value(chart, serie, pts[i]);
    }
    lv_chart_refresh(chart);
}

// ============================================================
// 13) KEYBOARD
// ============================================================
lv_obj_t* create_keyboard(lv_obj_t *parent, int x, int y, int w, int h) {
    static const int white_notes[KB_WHITE_KEYS] = {
        0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26, 28, 30, 31
    };
    static const int black_notes[KB_BLACK_KEYS]    = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25, 27, 29};
    static const int black_after_white[KB_BLACK_KEYS] = {0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14, 15, 16};

    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, w, h);
    lv_obj_set_pos(cont, x, y);
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(cont, 1, 0);
    lv_obj_set_style_border_color(cont, lv_color_hex(0x666666), 0);
    lv_obj_set_style_radius(cont, 2, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_CLICKABLE);

    const int bk_w = 11;
    const int bk_h = (h * 60) / 100;

    for (int i = 0; i < KB_WHITE_KEYS; i++) {
        int x0 = (i * w) / KB_WHITE_KEYS;
        int x1 = ((i + 1) * w) / KB_WHITE_KEYS;
        lv_obj_t *k = lv_obj_create(cont);
        lv_obj_set_size(k, x1 - x0, h);
        lv_obj_set_pos(k, x0, 0);
        lv_obj_set_style_radius(k, 0, 0);
        lv_obj_set_style_border_width(k, 0, 0);
        lv_obj_set_style_pad_all(k, 0, 0);
        lv_obj_clear_flag(k, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(k, LV_OBJ_FLAG_CLICKABLE);
        kb_white[i]      = k;
        kb_white_note[i] = white_notes[i];
    }
    for (int i = 0; i < KB_BLACK_KEYS; i++) {
        int wi       = black_after_white[i];
        int boundary = ((wi + 1) * w) / KB_WHITE_KEYS;
        int bx       = boundary - bk_w / 2;
        lv_obj_t *k = lv_obj_create(cont);
        lv_obj_set_size(k, bk_w, bk_h);
        lv_obj_set_pos(k, bx, 0);
        lv_obj_set_style_radius(k, 0, 0);
        lv_obj_set_style_border_width(k, 0, 0);
        lv_obj_set_style_pad_all(k, 0, 0);
        lv_obj_clear_flag(k, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(k, LV_OBJ_FLAG_CLICKABLE);
        kb_black[i]      = k;
        kb_black_note[i] = black_notes[i];
    }

    keyboard_obj = cont;
    update_keyboard_colors();
    return cont;
}



void update_keyboard_colors() {
    lv_color_t w_yellow = lv_color_hex(0xFFFFAA);
    lv_color_t w_blue   = lv_color_hex(0xAADDFF);
    lv_color_t b_yellow = lv_color_hex(0x886600);
    lv_color_t b_blue   = lv_color_hex(0x224466);
	
	   for (int i = 0; i < KB_WHITE_KEYS; i++) {
        if (!kb_white[i] || !lv_obj_is_valid(kb_white[i])) continue;
        lv_color_t c = (kb_white_note[i] <= midi_split) ? w_yellow : w_blue;
        lv_obj_set_style_bg_color(kb_white[i], c, 0);
        lv_obj_set_style_bg_opa(kb_white[i], LV_OPA_COVER, 0);
    }
    for (int i = 0; i < KB_BLACK_KEYS; i++) {
        if (!kb_black[i] || !lv_obj_is_valid(kb_black[i])) continue;
        lv_color_t c = (kb_black_note[i] <= midi_split) ? b_yellow : b_blue;
        lv_obj_set_style_bg_color(kb_black[i], c, 0);
        lv_obj_set_style_bg_opa(kb_black[i], LV_OPA_COVER, 0);
    }
}

// ============================================================
// 14) HELPER WIDGET — bottone e dropdown standard
// ============================================================

lv_obj_t* mkbtn(lv_obj_t *parent,
                int x, int y, int w, int h,
                uint32_t border_color,
                const char *text,
                const lv_font_t *font,
                lv_event_cb_t cb,
                intptr_t user_data,
                int radius,
                int border_width)
{
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_size(b, w, h);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_radius(b, radius, 0);
    lv_obj_set_style_border_width(b, border_width, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(border_color), 0);

    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_center(l);

    if (cb) {
        lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED,
                            (void*)user_data);
    }
    return b;
}

lv_obj_t* styled_dropdown(lv_obj_t *parent,
                          int x, int y, int w, int h,
                          uint32_t border_color,
                          const lv_font_t *font,
                          const char *options)
{
    lv_obj_t *dd = lv_dropdown_create(parent);
    lv_obj_set_size(dd, w, h);
    lv_obj_set_pos(dd, x, y);
    lv_obj_set_style_bg_color(dd, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(dd, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(dd, 2, 0);
    lv_obj_set_style_border_color(dd, lv_color_hex(border_color), 0);
    lv_obj_set_style_radius(dd, 6, 0);
    lv_obj_set_style_text_color(dd, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(dd, font, 0);
    lv_obj_set_style_pad_left(dd, 6, 0);
    lv_obj_set_style_pad_right(dd, 4, 0);

    lv_dropdown_set_options(dd, options);
    lv_dropdown_set_symbol(dd, NULL);

    lv_obj_t *list = lv_dropdown_get_list(dd);
    if (list) {
        lv_obj_set_style_text_font(list, &lv_font_montserrat_16, 0);
        lv_obj_set_style_bg_color(list, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_text_color(list, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_max_height(list, 380, 0);
    }
    return dd;
}

// ============================================================
// 15) RENAME MODAL — finestra unica riusabile
// ============================================================
static lv_obj_t    *g_rename_win  = nullptr;
static lv_obj_t    *g_rename_ta   = nullptr;
static RenameApplyCb g_rename_cb  = nullptr;
static void        *g_rename_ud   = nullptr;

static void rename_modal_confirm_cb(lv_event_t *e) {
    (void)e;
    if (g_rename_ta && lv_obj_is_valid(g_rename_ta) && g_rename_cb) {
        const char *name = lv_textarea_get_text(g_rename_ta);
        if (name && strlen(name) > 0) {
            g_rename_cb(name, g_rename_ud);
        }
    }
    close_rename_modal();
}

static void rename_modal_cancel_cb(lv_event_t *e) {
    (void)e;
    close_rename_modal();
}

void close_rename_modal() {
    if (g_rename_win) {
        lv_obj_del(g_rename_win);
        g_rename_win = nullptr;
        g_rename_ta  = nullptr;
        g_rename_cb  = nullptr;
        g_rename_ud  = nullptr;
    }
}

bool rename_modal_is_open() {
    return g_rename_win != nullptr;
}

void open_rename_modal(const char *title,
                       const char *initial_text,
                       int max_len,
                       uint32_t border_color,
                       RenameApplyCb on_apply,
                       void *user_data)
{
    if (g_rename_win) return;   // già aperta

    g_rename_cb = on_apply;
    g_rename_ud = user_data;

    g_rename_win = lv_win_create(lv_scr_act(), 0);
    lv_obj_set_size(g_rename_win, 580, 360);
    lv_obj_set_pos(g_rename_win, 110, 15);
    lv_obj_set_style_bg_color(g_rename_win, lv_color_hex(0x111111), 0);
    lv_obj_set_style_border_color(g_rename_win, lv_color_hex(border_color), 0);
    lv_obj_set_style_border_width(g_rename_win, 2, 0);
    lv_obj_set_style_radius(g_rename_win, 8, 0);

    lv_obj_t *client = lv_win_get_content(g_rename_win);
    lv_obj_set_style_pad_all(client, 10, 0);
    lv_obj_set_style_bg_color(client, lv_color_hex(0x000000), 0);
    lv_obj_set_flex_flow(client, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(client, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(client, 10, 0);

    if (title) {
        lv_obj_t *lbl = lv_label_create(client);
        lv_label_set_text(lbl, title);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
    }

    g_rename_ta = lv_textarea_create(client);
    lv_obj_set_size(g_rename_ta, 460, 45);
    lv_obj_set_style_bg_color(g_rename_ta, lv_color_hex(0x222222), 0);
    lv_obj_set_style_text_color(g_rename_ta, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(g_rename_ta, 1, 0);
    lv_obj_set_style_border_color(g_rename_ta, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(g_rename_ta, &lv_font_montserrat_18, 0);
    lv_textarea_set_text(g_rename_ta, initial_text ? initial_text : "");
    lv_textarea_set_max_length(g_rename_ta, max_len);
    lv_textarea_set_one_line(g_rename_ta, true);

    lv_obj_t *kb = lv_keyboard_create(client);
    lv_obj_set_size(kb, 550, 200);
    lv_obj_set_style_text_font(kb, &lv_font_montserrat_18, 0);
    lv_keyboard_set_textarea(kb, g_rename_ta);
    lv_obj_set_style_bg_color(kb, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_color(kb, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_color(kb, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_color(kb, lv_color_hex(0x444444), LV_STATE_PRESSED);
    lv_obj_set_style_text_color(kb, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    lv_obj_t *btn_cont = lv_obj_create(client);
    lv_obj_set_size(btn_cont, 460, 50);
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
    lv_obj_add_event_cb(ok, rename_modal_confirm_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cancel = lv_btn_create(btn_cont);
    lv_obj_set_size(cancel, 120, 40);
    lv_obj_set_style_bg_color(cancel, lv_color_hex(0x6B1A1A), 0);
    lv_obj_t *cn_l = lv_label_create(cancel);
    lv_label_set_text(cn_l, "Annulla");
    lv_obj_set_style_text_color(cn_l, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(cn_l);
    lv_obj_add_event_cb(cancel, rename_modal_cancel_cb, LV_EVENT_CLICKED, NULL);
}	

 
