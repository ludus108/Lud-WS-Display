// ============================================================
// lvglGrafFunc.cpp - Definizioni di tutte le funzioni LVGL
// ============================================================
#include "globals.h"
#include "lvglGraf.h"
#include "preset_sd.h"
#include <Arduino.h>
#include <math.h>
#include "lvgl_v8_port.h"   // per lvgl_port_lock/unlock

// ========================== VARIABILI STATICHE LOCALI ==========================
 lv_obj_t *rename_win = NULL;
 lv_obj_t *rename_ta = NULL;
 int rename_preset_idx = 0;
 lv_obj_t *confirm_win = NULL;

// ========================== WIDGET DI BASE ==========================
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

void home_btn(lv_obj_t *p, int id) {
    lv_obj_t *b = lv_btn_create(p);
    lv_obj_set_size(b, 90, 90); lv_obj_set_pos(b, 680, 360);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_radius(b, 8, 0);
    lv_obj_set_style_border_width(b, 3, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(0x9B59B6), 0);
    lv_obj_t *img = lv_img_create(b);
    lv_img_set_src(img, &home);
    lv_obj_center(img);
    lv_obj_add_event_cb(b, eb, LV_EVENT_CLICKED, (void*)(uintptr_t)id);
}

// ========================== SLIDER VERTICALE ==========================
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
    lv_obj_t *a = lv_img_create(p);
    lv_img_set_src(a, &freccina);
    lv_obj_set_pos(a, x-18, yf);
    arr[idx] = a;
    crs[idx] = false;
    last[idx] = 127;
}

// ========================== SLIDER ORIZZONTALE SHAPE ==========================
void h_slider(lv_obj_t *p, int id) {
    const char *title = (id == SRC_A) ? "DCO A" : "DCO B";
    lv_obj_t *title_lbl = lv_label_create(p);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(title_lbl, 20, 5);

    const char *cat_names[] = {"WF", "FM", "AM"};
    lv_color_t cat_colors[] = {lv_color_hex(0x00AAFF), lv_color_hex(0xFF8800), lv_color_hex(0xAA44FF)};
    int cat_btn_w = 80, cat_btn_h = 90, cat_gap = 15, cat_start_x = 10, cat_y = 44;
    for (int i = 0; i < 3; i++) {
        lv_obj_t *led = lv_obj_create(p);
        lv_obj_set_size(led, 12, 12);
        lv_obj_set_style_radius(led, 6, 0);
        lv_obj_set_style_bg_color(led, lv_color_hex(0x333333), 0);
        lv_obj_set_style_border_width(led, 0, 0);
        int led_x = cat_start_x + i * (cat_btn_w + cat_gap) + cat_btn_w/2 - 6;
        lv_obj_set_pos(led, led_x, 30);
        shape_data[id-1].leds[i] = led;

        lv_obj_t *cat_btn = lv_btn_create(p);
        lv_obj_set_size(cat_btn, cat_btn_w, cat_btn_h);
        lv_obj_set_pos(cat_btn, cat_start_x + i * (cat_btn_w + cat_gap), cat_y);
        lv_obj_set_style_bg_color(cat_btn, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_bg_color(cat_btn, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
        lv_obj_set_style_radius(cat_btn, 8, 0);
        lv_obj_set_style_border_width(cat_btn, 2, 0);
        lv_obj_set_style_border_color(cat_btn, cat_colors[i], 0);
        lv_obj_t *cat_label = lv_label_create(cat_btn);
        lv_label_set_text(cat_label, cat_names[i]);
        lv_obj_set_style_text_color(cat_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(cat_label, &lv_font_montserrat_24, 0);
        lv_obj_center(cat_label);
        lv_obj_add_event_cb(cat_btn, ecat_btn, LV_EVENT_CLICKED, (void*)(uintptr_t)(id * 10 + i));
    }

    int list_y = cat_y + cat_btn_h + 20;
    lv_obj_t *grid_container = lv_obj_create(p);
    lv_obj_set_size(grid_container, 320, 140);
    lv_obj_set_pos(grid_container, 10, list_y);
    lv_obj_set_style_bg_opa(grid_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid_container, 0, 0);
    lv_obj_clear_flag(grid_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(grid_container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(grid_container, 5, 0);
    lv_obj_set_style_pad_row(grid_container, 5, 0);
    lv_obj_set_style_pad_column(grid_container, 5, 0);

    lv_obj_t *lb = lv_label_create(p);
    lv_label_set_text(lb, "Wave Shape");
    lv_obj_set_style_text_color(lb, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lb, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(lb, 10, list_y - 20);

    shape_data[id-1].list = grid_container;
    shape_data[id-1].current_cat = CAT_WF;
    shape_data[id-1].list_items = WF_ITEMS;
    update_leds(shape_data[id-1].leds, CAT_WF);
    populate_grid(grid_container, WF_ITEMS, 0, id);

    int plotter_x = 300, plotter_y = 60;
    lv_obj_t *chart = lv_chart_create(p);
    lv_obj_set_size(chart, 250, 130);
    lv_obj_set_pos(chart, plotter_x, plotter_y);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_point_count(chart, WAVE_POINTS);
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(chart, 1, 0);
    lv_obj_set_style_border_color(chart, lv_color_hex(0x666666), 0);
    lv_chart_set_div_line_count(chart, 0, 0);
    lv_chart_series_t *serie = lv_chart_add_series(chart, lv_color_hex(0xFFFF00), LV_CHART_AXIS_PRIMARY_Y);
    shape_data[id-1].chart = chart;
    shape_data[id-1].serie = serie;
    update_plotter_by_wave(id);

    int slider_x = plotter_x + 15, slider_y = plotter_y + 160;
    lv_obj_t *s = lv_slider_create(p);
    lv_obj_set_size(s, 212, 54);
    lv_obj_set_pos(s, slider_x, slider_y);
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
    else g.shape_last_B = init_shape;

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
    else g.shape_slider_B = s;

    lv_obj_t *shape_label = lv_label_create(p);
    lv_label_set_text(shape_label, "SHAPE");
    lv_obj_set_style_text_color(shape_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(shape_label, &lv_font_montserrat_18, 0);
    lv_obj_align_to(shape_label, s, LV_ALIGN_OUT_TOP_MID, 0, -10);

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

    shape_data[id-1].id = id;
    lv_obj_add_event_cb(s, ehslider, LV_EVENT_VALUE_CHANGED, &shape_data[id-1]);

    uint8_t target = (id == SRC_A) ? g.pre_shape_A : g.pre_shape_B;
    int arrow_x = slider_x + (int)((target / 100.0f) * 212) - 12;
    int arrow_y = slider_y + 54 + 5;
    lv_obj_t *arrow = lv_img_create(p);
    lv_img_set_src(arrow, &freccina_oriz);
    lv_obj_set_pos(arrow, arrow_x, arrow_y);
    if (id == SRC_A) g.shape_arrow_A = arrow;
    else g.shape_arrow_B = arrow;

    bool crs_flag = (id == SRC_A) ? g.shape_crs_A : g.shape_crs_B;
    if (crs_flag) {
        lv_obj_add_flag(arrow, LV_OBJ_FLAG_HIDDEN);
        update_shape_slider_color(id, false);
    } else {
        lv_obj_clear_flag(arrow, LV_OBJ_FLAG_HIDDEN);
        update_shape_slider_color(id, true);
    }
}

// ========================== PLOTTER E GRIGLIA ==========================
void grid_btn_click(lv_event_t *e) {
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    int synth_id = id >> 8;
    int btn_idx = id & 0xFF;
    int idx = (synth_id == SRC_A) ? 0 : 1;

    if (grid_selected_idx[idx] >= 0 && grid_selected_idx[idx] < grid_btn_count[idx]) {
        lv_obj_set_style_bg_color(grid_btns[idx][grid_selected_idx[idx]], lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_border_color(grid_btns[idx][grid_selected_idx[idx]], lv_color_hex(0x666666), 0);
    }
    lv_obj_set_style_bg_color(grid_btns[idx][btn_idx], lv_color_hex(0x0F3460), 0);
    lv_obj_set_style_border_color(grid_btns[idx][btn_idx], lv_color_hex(0x00AAFF), 0);
    grid_selected_idx[idx] = btn_idx;

    int cat = shape_data[idx].current_cat;
    int global_idx;
    switch(cat) {
        case CAT_WF: global_idx = btn_idx; break;
        case CAT_FM: global_idx = 9 + btn_idx; break;
        case CAT_AM: global_idx = 17 + btn_idx; break;
        default: global_idx = btn_idx; break;
    }
    if (synth_id == SRC_A) g.wa = global_idx;
    else g.wb = global_idx;
    wave_shape(global_idx, synth_id);
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
            lv_obj_add_event_cb(btn, grid_btn_click, LV_EVENT_CLICKED, (void*)(uintptr_t)(count | (synth_id << 8)));
            count++;
        }
        if (*end == '\n') start = end + 1;
        else break;
    }
    grid_btn_count[idx] = count;
    grid_selected_idx[idx] = selected_idx;
    if (selected_idx < count) {
        lv_obj_set_style_bg_color(grid_btns[idx][selected_idx], lv_color_hex(0x0F3460), 0);
        lv_obj_set_style_border_color(grid_btns[idx][selected_idx], lv_color_hex(0x00AAFF), 0);
    }
}

void update_wave_plot(lv_obj_t *chart, lv_chart_series_t *serie, uint8_t shape_val, int wave_index) {
    if (wave_index == WF_SAW_INDEX || wave_index == WF_S8W_INDEX)
        lv_chart_set_series_color(chart, serie, lv_color_hex(0xFFFF00));
    else if (wave_index == WF_SQU_INDEX)
        lv_chart_set_series_color(chart, serie, lv_color_hex(0xFF0000));
    else if (wave_index == WF_SIN_INDEX)
        lv_chart_set_series_color(chart, serie, lv_color_hex(0x00AAFF));
    else
        lv_chart_set_series_color(chart, serie, lv_color_hex(0xFF8800));

    if (wave_index != WF_SAW_INDEX && wave_index != WF_S8W_INDEX && wave_index != WF_SQU_INDEX) {
        for (int i = 0; i < WAVE_POINTS; i++) lv_chart_set_next_value(chart, serie, 0);
        lv_chart_refresh(chart);
        return;
    }

    if (wave_index == WF_SQU_INDEX) {
        uint8_t duty = constrain(shape_val, 1, 99);
        float duty_float = duty / 100.0f;
        for (int i = 0; i < WAVE_POINTS; i++) {
            float frac = (float)i / WAVE_POINTS * 2.0f - floorf((float)i / WAVE_POINTS * 2.0f);
            int y = (frac < duty_float) ? 100 : 0;
            lv_chart_set_next_value(chart, serie, y);
        }
        lv_chart_refresh(chart);
        return;
    }

    if (wave_index == WF_SAW_INDEX || wave_index == WF_S8W_INDEX) {
        int cycles = (wave_index == WF_SAW_INDEX) ? 2 : 4;
        float pos = shape_val / 100.0f;
        float p = constrain(1.0f - pos, 0.001f, 0.999f);
        for (int i = 0; i < WAVE_POINTS; i++) {
            float phase = (float)i / WAVE_POINTS * cycles;
            float frac = phase - floorf(phase);
            float y = (frac <= p) ? 100.0f * frac / p : 100.0f * (1.0f - frac) / (1.0f - p);
            lv_chart_set_next_value(chart, serie, (int)y);
        }
        lv_chart_refresh(chart);
    }
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

// ========================== LED DI CATEGORIA ==========================
void update_leds(lv_obj_t **leds, int active_cat) {
    for (int i = 0; i < 3; i++) {
        lv_obj_set_style_bg_color(leds[i], (i == active_cat) ? lv_color_hex(0x00FF00) : lv_color_hex(0x333333), 0);
    }
}

// ========================== CAMBIO COLORE INDICATORI ==========================
void update_slider_color(int idx, bool active) {
    if (idx < 0 || idx > 5) return;
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
    lv_color_t target = active ? lv_color_hex(COLOR_SHAPE_SLIDER_ACTIVE) : lv_color_hex(COLOR_SHAPE_SLIDER_PASSED);
    lv_obj_set_style_bg_img_recolor(slider, target, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor_opa(slider, (sliderColorDepth * 255) / 100, LV_PART_INDICATOR);
}

// ========================== ARC ==========================
void earc_changed(lv_event_t *e) {
    lv_obj_t *arc = lv_event_get_target(e);
    int cur = lv_arc_get_value(arc);
    int prev = g.arc_last;
    int target = g.arc_target;
    g.arc_value = cur;

    if (g.arc_label_value && lv_obj_is_valid(g.arc_label_value))
        lv_label_set_text_fmt(g.arc_label_value, "%d", cur);

    if (g.arc_arrow && g.arc_obj && lv_obj_is_valid(g.arc_arrow) && lv_obj_is_valid(g.arc_obj)) {
        if (!g.arc_crossed) {
            bool crossed = (prev < target && cur >= target) ||
                           (prev > target && cur <= target) ||
                           (prev == target && cur != target);
            if (crossed) {
                g.arc_crossed = true;
                lv_obj_add_flag(g.arc_arrow, LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_style_arc_color(g.arc_obj, lv_color_hex(COLOR_ARC_PASSED), LV_PART_INDICATOR);
            } else {
                lv_obj_clear_flag(g.arc_arrow, LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_style_arc_color(g.arc_obj, lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
            }
        }
    }
    g.arc_last = cur;
}

void arc_with_image(lv_obj_t *parent, int x, int y, int w, int h) {
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, w, h);
    lv_obj_set_pos(arc, x, y);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, g.arc_value);

    lv_obj_set_style_arc_img_src(arc, &img_arc_bg, LV_PART_MAIN);
    lv_obj_set_style_arc_img_src(arc, &img_arc_indic, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
    g.arc_obj = arc;

    lv_obj_t *val_label = lv_label_create(parent);
    lv_obj_set_style_text_color(val_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(val_label, &lv_font_montserrat_24, 0);
    lv_label_set_text_fmt(val_label, "%d", g.arc_value);
    lv_obj_align_to(val_label, arc, LV_ALIGN_CENTER, 0, 0);
    g.arc_label_value = val_label;

    lv_obj_t *p1_label = lv_label_create(parent);
    lv_label_set_text(p1_label, "P1");
    lv_obj_set_style_text_color(p1_label, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(p1_label, &lv_font_montserrat_18, 0);
    lv_obj_align_to(p1_label, arc, LV_ALIGN_OUT_TOP_MID, 0, -5);
    g.arc_label_p1 = p1_label;

    int angle = (g.arc_target * 360 / 100) - 90;
    if (angle < 0) angle += 360;
    int radius = (w / 2) - 2 + 10;
    int center_x = x + w / 2;
    int center_y = y + h / 2;
    float rad = angle * PI / 180.0;
    int dot_x = center_x + radius * cosf(rad) - 4;
    int dot_y = center_y + radius * sinf(rad) - 4;

    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_pos(dot, dot_x, dot_y);
    g.arc_arrow = dot;

    if (g.arc_value > g.arc_target) {
        g.arc_crossed = true;
        lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_arc_color(arc, lv_color_hex(COLOR_ARC_PASSED), LV_PART_INDICATOR);
    } else {
        g.arc_crossed = false;
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_arc_color(arc, lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
    }

    lv_obj_add_event_cb(arc, earc_changed, LV_EVENT_VALUE_CHANGED, NULL);
}

// ========================== SUBMENU ==========================
void submenu(lv_obj_t *p) {
    lv_obj_add_flag(p, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_clip_corner(p, 0, 0);
    const char *n[] = {"DCO","VCF","MOD","VCA"};
    lv_color_t cols[] = {lv_color_hex(0x00FF00), lv_color_hex(0xCC3300), lv_color_hex(0x0099FF), lv_color_hex(0xFFCC33)};
    for (int i=0; i<4; i++) btn(p, n[i], 30+i*155, 360, 120, 90, cols[i], 10+i);
    home_btn(p, -1);
}

// ========================== METER ==========================
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
     float ph=0; ph += 0.1f; if (ph > 6.28f) ph -= 6.28f;
    float vL = constrain(0.5f + 0.5f*sinf(ph) + 0.05f*((float)random(0,100)/100.0f - 0.5f), 0, 1);
    float vR = constrain(0.5f + 0.5f*cosf(ph*0.7f) + 0.05f*((float)random(0,100)/100.0f - 0.5f), 0, 1);
    float vC = constrain((vL+vR)*0.5f * (1.0f - 0.3f*(vL+vR)*0.5f), 0, 1);
    float dk=0.95f;
    pkL = max(vL, pkL*dk); pkR = max(vR, pkR*dk); pkC = max(vC, pkC*dk);
    if (mL) lv_slider_set_value(mL, (int)(pkL*100), LV_ANIM_OFF);
    if (mR) lv_slider_set_value(mR, (int)(pkR*100), LV_ANIM_OFF);
    if (mC) lv_slider_set_value(mC, (int)(pkC*100), LV_ANIM_OFF);
}

void stop_meters() {
    if (mt) { lv_timer_del(mt); mt=0; }
    mL=mR=mC=0; pkL=pkR=pkC=0;
}

// ========================== LOG E TOAST ==========================
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

// ========================== EEPROM ==========================
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

// ========================== RESET FRECCINE ==========================
void reset_arrows() {
    for (int i=0; i<8; i++) {
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
    g.arc_crossed = false;
    if (g.arc_obj && lv_obj_is_valid(g.arc_obj)) {
        if (g.arc_arrow && lv_obj_is_valid(g.arc_arrow)) {
            lv_obj_clear_flag(g.arc_arrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_arc_color(g.arc_obj, lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
        } else g.arc_arrow = 0;
    } else { g.arc_obj = 0; g.arc_arrow = 0; }
    log_add("Freccine ripristinate", lv_color_hex(0x66AAFF));
    toast_show("Freccine ripristinate!", lv_color_hex(0x66AAFF), TOAST_DUR);
}

void wave_shape(int id, int src) {
    const char *sn = (src == SRC_A) ? "Synth A" : "Synth B";
    char b[60]; snprintf(b, 60, "[%s] Selezionato: %s", sn, WAVE_DEFS[id].name);
    log_add(b, lv_color_hex(0x66AAFF));
}

// ========================== HOME ==========================
void create_home() {
    pendingPresetA = pendingPresetB = -1;
    if (blink_timer) { lv_timer_del(blink_timer); blink_timer = NULL; blink_state = false; }
    close_rename_window();
    if (g.play) { stop_meters(); g.play = 0; }
    if (g.page) { lv_obj_del(g.page); g.page = 0; }
    g.synth = 0; g.err = g.log_v = g.toast_v = 0;
    g.log_c = g.log_f = g.toast = 0;
    for (int i=0; i<6; i++) { arr[i]=0; slider_objs[i]=0; crs[i]=0; last[i]=0; }
    g.shape_arrow_A = g.shape_arrow_B = 0;
    g.shape_slider_A = g.shape_slider_B = 0;
    g.arc_obj = g.arc_arrow = g.arc_label_value = g.arc_label_p1 = 0;
    preset_dropdown_A = preset_dropdown_B = NULL;
    preset_label_A = preset_label_B = NULL;

    lv_obj_t *m = lv_obj_create(lv_scr_act());
    lv_obj_set_size(m, lv_disp_get_hor_res(0), lv_disp_get_ver_res(0));
    lv_obj_set_style_radius(m, 0, 0);
    lv_obj_set_style_bg_color(m, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(m, 0, 0);
    lv_obj_clear_flag(m, LV_OBJ_FLAG_SCROLLABLE);
    g.page = m;

    lv_obj_t *t1 = lv_label_create(m);
    lv_label_set_text(t1, "LUD-WS");
    lv_obj_set_style_text_font(t1, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(t1, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(t1, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_t *t2 = lv_label_create(m);
    lv_label_set_text(t2, last_version);
    lv_obj_set_style_text_font(t2, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(t2, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align_to(t2, t1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, -5);

    lv_obj_t *sc = lv_obj_create(m);
    lv_obj_set_size(sc, 180, 50);
    lv_obj_align(sc, LV_ALIGN_TOP_RIGHT, -20, 20);
    lv_obj_set_style_bg_color(sc, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(sc, 0, 0);
    lv_obj_clear_flag(sc, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *sl = lv_slider_create(sc);
    lv_obj_set_size(sl, 120, 10);
    lv_obj_align(sl, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_slider_set_range(sl, 0, 100);
    lv_slider_set_value(sl, g.bright, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sl, lv_color_hex(0x999999), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sl, lv_color_hex(0x666666), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sl, lv_color_hex(0x333333), LV_PART_KNOB);
    lv_obj_set_style_radius(sl, 5, LV_PART_MAIN | LV_PART_INDICATOR);
    lv_obj_add_event_cb(sl, ebright, LV_EVENT_VALUE_CHANGED, 0);

    g.log_f = lv_obj_create(m);
    lv_obj_set_size(g.log_f, 700, 240);
    lv_obj_set_pos(g.log_f, 50, 97);
    lv_obj_set_style_bg_color(g.log_f, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_border_width(g.log_f, 2, 0);
    lv_obj_set_style_border_color(g.log_f, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(g.log_f, 4, 0);
    lv_obj_clear_flag(g.log_f, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(g.log_f, LV_OBJ_FLAG_HIDDEN);
    g.log_c = lv_obj_create(g.log_f);
    lv_obj_set_size(g.log_c, 680, 220);
    lv_obj_set_pos(g.log_c, 10, 10);
    lv_obj_set_style_bg_color(g.log_c, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_border_width(g.log_c, 0, 0);
    lv_obj_set_style_pad_all(g.log_c, 5, 0);
    lv_obj_set_style_pad_row(g.log_c, 3, 0);
    lv_obj_set_flex_flow(g.log_c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g.log_c, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(g.log_c, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(g.log_c, LV_OBJ_FLAG_SCROLLABLE);

    const char *n[] = {"PLAY","SYNTH A","SYNTH B","DRUM","SET UP"};
    lv_color_t cols[] = {lv_color_hex(0x00FF00), lv_color_hex(0xCC3300), lv_color_hex(0x0099FF), lv_color_hex(0xFFCC33), lv_color_hex(0x999999)};
    for (int i=0; i<5; i++) btn(m, n[i], 30+i*155, 360, 120, 90, cols[i], i);
}

// ========================== PAGINE SECONDARIE ==========================
void create_page(const char *title) {
    pendingPresetA = pendingPresetB = -1;
    if (blink_timer) { lv_timer_del(blink_timer); blink_timer = NULL; blink_state = false; }
    close_rename_window();
    if (g.play && strcmp(title, "PLAY") != 0) { stop_meters(); g.play = 0; }
    if (g.page) { lv_obj_del(g.page); g.page = 0; }
    g.log_c = g.log_f = g.toast = 0; g.toast_v = 0;
    for (int i=0; i<6; i++) { arr[i]=0; slider_objs[i]=0; crs[i]=0; last[i]=0; }
    g.shape_arrow_A = g.shape_arrow_B = 0;
    g.shape_slider_A = g.shape_slider_B = 0;
    g.arc_obj = g.arc_arrow = g.arc_label_value = g.arc_label_p1 = 0;
    preset_dropdown_A = preset_dropdown_B = NULL;
    preset_label_A = preset_label_B = NULL;

    lv_obj_t *m = lv_obj_create(lv_scr_act());
    lv_obj_set_size(m, lv_disp_get_hor_res(0), lv_disp_get_ver_res(0));
    lv_obj_set_style_radius(m, 0, 0);
    lv_obj_set_style_bg_color(m, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(m, 0, 0);
    lv_obj_clear_flag(m, LV_OBJ_FLAG_SCROLLABLE);
    g.page = m;

    lv_obj_t *t = NULL;
    if (strncmp(title, "DCO", 3) != 0) {
        t = lv_label_create(m);
        lv_label_set_text(t, title);
        lv_obj_set_style_text_font(t, &lv_font_montserrat_32, 0);
        lv_obj_set_style_text_color(t, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(t, LV_ALIGN_TOP_LEFT, 10, 5);
    }

    if (strcmp(title, "PLAY") == 0) {
        g.play = 1; int w=22, h=150;
        mL = meter(m, 685, 10, w, h, "L", 0);
        mR = meter(m, 709, 10, w, h, "R", 0);
        mC = meter(m, 750, 10, w, h, "C", 1);
        if (!mt) mt = lv_timer_create([](lv_timer_t*){ update_meters(); }, 33, 0);
        home_btn(m, -1);
    }
    else if (strcmp(title, "SYNTH A") == 0 || strcmp(title, "SYNTH B") == 0) {
        g.synth = title;
        int id = (strcmp(title, "SYNTH A") == 0) ? 0 : 1;
        const char *lbl = (id == 0) ? "Preset A" : "Preset B";
        int *ptr = (id == 0) ? &presetNumA : &presetNumB;
        char (*names)[32] = (id == 0) ? (char (*)[32])presetNamesA : (char (*)[32])presetNamesB;
        create_preset_selector(m, 450, 2, lbl, ptr, names);
 update_preset_dropdown_options();
        lv_obj_t *info = lv_label_create(m);
        char buf[64];
        if (id == 0) {
            snprintf(buf, sizeof(buf), "P%d %s", presetNumA, presetNamesA[presetNumA]);
            preset_label_A = info;
        } else {
            snprintf(buf, sizeof(buf), "P%d %s", presetNumB, presetNamesB[presetNumB]);
            preset_label_B = info;
        }
        lv_label_set_text(info, buf);
        lv_obj_set_style_text_color(info, lv_color_hex(0x00FF00), 0);
        lv_obj_set_style_text_font(info, &lv_font_montserrat_20, 0);
        if (t) lv_obj_align_to(info, t, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
        else lv_obj_align(info, LV_ALIGN_TOP_LEFT, 200, 10);

        lv_obj_t *rn = lv_btn_create(m);
        lv_obj_set_size(rn, 100, 40);
        lv_obj_set_pos(rn, 450, 110);
        lv_obj_set_style_bg_color(rn, lv_color_hex(0x000000), 0);
        lv_obj_t *rn_lbl = lv_label_create(rn);
        lv_label_set_text(rn_lbl, "Rinomina");
        lv_obj_set_style_text_color(rn_lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(rn_lbl);
        lv_obj_add_event_cb(rn, rename_btn_click, LV_EVENT_CLICKED, NULL);
        submenu(m);
        update_all_targets();
    }
    else if (strncmp(title, "DCO", 3) == 0) {
        int id = (g.synth && strcmp(g.synth, "SYNTH A") == 0) ? SRC_A : SRC_B;
        h_slider(m, id);
        home_btn(m, -2);
    }
    else if (strncmp(title, "MOD", 3) == 0) {
        arc_with_image(m, 300, 150, 108, 108);
        home_btn(m, -2);
    }
    else if (strcmp(title, "SET UP") == 0) {
        lv_obj_t *btn = lv_btn_create(m);
        lv_obj_set_size(btn, 160, 70);
        lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xCC3333), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x992222), LV_STATE_PRESSED);
        lv_obj_set_style_radius(btn, 10, 0);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(0xFF8888), 0);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, "init SD");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(btn, init_sd_btn_click, LV_EVENT_CLICKED, NULL);
        home_btn(m, -1);
    }
    else if (strncmp(title, "VCF", 3) == 0 || strncmp(title, "VCA", 3) == 0) {
        bool isA = (g.synth && strcmp(g.synth, "SYNTH A") == 0);
        lv_color_t active_color = isA ? lv_color_hex(COLOR_SLIDER_SYNTH_A_ACTIVE) : lv_color_hex(COLOR_SLIDER_SYNTH_B_ACTIVE);
        lv_color_t passed_color = isA ? lv_color_hex(COLOR_SLIDER_SYNTH_A_PASSED) : lv_color_hex(COLOR_SLIDER_SYNTH_B_PASSED);
        int base = (strncmp(title, "VCF", 3) == 0) ? 0 : 4;
        const char *labels[] = {"A", "D", "S", "R"};
        for (int i=0; i<4; i++) slider(m, 20 + i*80, 96, labels[i], base + i, active_color, passed_color);
        home_btn(m, -2);
    }
    else {
        lv_obj_t *lb = lv_label_create(m);
        lv_label_set_text_fmt(lb, "Contenuto di %s", title);
        lv_obj_set_style_text_font(lb, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(lb, lv_color_hex(0xAAAAAA), 0);
        lv_obj_align(lb, LV_ALIGN_CENTER, 0, -40);
        home_btn(m, -1);
    }
}

// ========================== GESTORI EVENTI ==========================
 void eb(lv_event_t *e) {
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    if (g.play && (id == -1 || (id >= 0 && id < 5))) { stop_meters(); g.play = 0; }
    if (id == -2) {
        if (g.synth) { log_add("← Torno al synth", lv_color_hex(0xFFFFFF)); lvgl_port_lock(-1); create_page(g.synth); lvgl_port_unlock(); }
        else { log_add("← HOME", lv_color_hex(0xFFFFFF)); lvgl_port_lock(-1); create_home(); lvgl_port_unlock(); }
        return;
    }
    if (id == -1) { log_add("← HOME", lv_color_hex(0xFFFFFF)); lvgl_port_lock(-1); create_home(); lvgl_port_unlock(); return; }
    if (id == 5) { save_bright(); return; }
    if (id >= 10 && id <= 13) {
        const char *sub[] = {"DCO","VCF","MOD","VCA"};
        char title[20];
        if (g.synth) {
            const char *sfx = (strcmp(g.synth, "SYNTH A") == 0) ? "A" : "B";
            snprintf(title, 20, "%s %s", sub[id-10], sfx);
        } else snprintf(title, 20, "%s", sub[id-10]);
        log_add(title, lv_color_hex(0xFFFFFF));
        lvgl_port_lock(-1); create_page(title); lvgl_port_unlock();
        return;
    }
    if (id >= 0 && id < 5) {
        const char *pages[] = {"PLAY","SYNTH A","SYNTH B","DRUM","SET UP"};
        char b[20]; snprintf(b, 20, "▶ Apro: %s", pages[id]);
        log_add(b, lv_color_hex(0xFFFFFF));
        lvgl_port_lock(-1); create_page(pages[id]); lvgl_port_unlock();
    }
}

 void ebright(lv_event_t *e) { set_bright(lv_slider_get_value(lv_event_get_target(e))); }

 void eslider(lv_event_t *e) {
    SliderData *d = (SliderData*)lv_event_get_user_data(e);
    int cur = lv_slider_get_value(lv_event_get_target(e));
    int prev = last[d->idx];
    int target = g.pre[d->idx];
    lv_obj_t *a = arr[d->idx];
    if (a && lv_obj_is_valid(a)) {
        if (!crs[d->idx]) {
            bool crossed = (prev < target && cur > target) ||
                           (prev > target && cur < target) ||
                           (prev == target && cur != target);
            if (crossed) {
                crs[d->idx] = 1;
                lv_obj_add_flag(a, LV_OBJ_FLAG_HIDDEN);
                update_slider_parameter(d->idx, cur);
                if (d->label && lv_obj_is_valid(d->label))
                    lv_obj_set_style_text_color(d->label, lv_color_hex(0xFFFFFF), 0);
                if (slider_objs[d->idx] && lv_obj_is_valid(slider_objs[d->idx]))
                    update_slider_color(d->idx, false);
            } else {
                lv_obj_clear_flag(a, LV_OBJ_FLAG_HIDDEN);
                if (d->label && lv_obj_is_valid(d->label))
                    lv_obj_set_style_text_color(d->label, lv_color_hex(0xFFA500), 0);
                if (slider_objs[d->idx] && lv_obj_is_valid(slider_objs[d->idx]))
                    update_slider_color(d->idx, true);
            }
        }
    } else arr[d->idx] = 0;
    last[d->idx] = cur;
}

 void ecat_btn(lv_event_t *e) {
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    int synth_id = id / 10;
    int cat = id % 10;
    int idx = (synth_id == SRC_A) ? 0 : 1;
    ShapeData *d = &shape_data[idx];
    if (!d->list) return;

    d->current_cat = cat;
    const char *items;
    switch(cat) {
        case CAT_WF: items = WF_ITEMS; break;
        case CAT_FM: items = FM_ITEMS; break;
        case CAT_AM: items = AM_ITEMS; break;
        default: items = WF_ITEMS; break;
    }
    d->list_items = items;
    populate_grid(d->list, items, 0, synth_id);
    update_leds(d->leds, cat);

    if (synth_id == SRC_A) g.wa = (cat == CAT_WF) ? 0 : (cat == CAT_FM ? 9 : 17);
    else g.wb = (cat == CAT_WF) ? 0 : (cat == CAT_FM ? 9 : 17);
    update_plotter_by_wave(synth_id);

    const char *cat_names[] = {"WF", "FM", "AM"};
    char logbuf[32];
    snprintf(logbuf, sizeof(logbuf), "Cat: %s (Synth %c)", cat_names[cat], (synth_id == SRC_A) ? 'A' : 'B');
    log_add(logbuf, lv_color_hex(0x66AAFF));
}

 void ehslider(lv_event_t *e) {
    ShapeData *d = (ShapeData*)lv_event_get_user_data(e);
    lv_obj_t *slider = lv_event_get_target(e);
    int cur = lv_slider_get_value(slider);

    if (d->id == SRC_A) g.shape_a = cur;
    else g.shape_b = cur;

    int target = (d->id == SRC_A) ? g.pre_shape_A : g.pre_shape_B;
    bool *crs_flag = (d->id == SRC_A) ? &g.shape_crs_A : &g.shape_crs_B;
    int *last_val = (d->id == SRC_A) ? &g.shape_last_A : &g.shape_last_B;
    lv_obj_t *arrow = (d->id == SRC_A) ? g.shape_arrow_A : g.shape_arrow_B;

    if (arrow && lv_obj_is_valid(arrow)) {
        if (!(*crs_flag)) {
            bool crossed = (*last_val < target && cur > target) ||
                           (*last_val > target && cur < target) ||
                           (*last_val == target && cur != target);
            if (crossed) {
                *crs_flag = true;
                lv_obj_add_flag(arrow, LV_OBJ_FLAG_HIDDEN);
                if (d->id == SRC_A && g.shape_slider_A && lv_obj_is_valid(g.shape_slider_A))
                    update_shape_slider_color(SRC_A, false);
                else if (d->id == SRC_B && g.shape_slider_B && lv_obj_is_valid(g.shape_slider_B))
                    update_shape_slider_color(SRC_B, false);
            } else {
                lv_obj_clear_flag(arrow, LV_OBJ_FLAG_HIDDEN);
                if (d->id == SRC_A && g.shape_slider_A && lv_obj_is_valid(g.shape_slider_A))
                    update_shape_slider_color(SRC_A, true);
                else if (d->id == SRC_B && g.shape_slider_B && lv_obj_is_valid(g.shape_slider_B))
                    update_shape_slider_color(SRC_B, true);
            }
        }
    } else {
        if (d->id == SRC_A) g.shape_arrow_A = 0;
        else g.shape_arrow_B = 0;
    }
    *last_val = cur;

    update_plotter_by_wave(d->id);
    char logbuf[32];
    snprintf(logbuf, sizeof(logbuf), "SHAPE %c: %d%%", (d->id == SRC_A) ? 'A' : 'B', cur);
    log_add(logbuf, lv_color_hex(0x66AAFF));
}

 void elist(lv_event_t *e) {
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *list = lv_event_get_target(e);
    int sel = lv_btnmatrix_get_selected_btn(list);
    if (sel < 0) return;
    int idx = (id == SRC_A) ? 0 : 1;
    ShapeData *d = &shape_data[idx];
    int cat = d->current_cat;
    int global_idx;
    switch(cat) {
        case CAT_WF: global_idx = sel; break;
        case CAT_FM: global_idx = 9 + sel; break;
        case CAT_AM: global_idx = 17 + sel; break;
        default: global_idx = sel; break;
    }
    if (id == SRC_A) g.wa = global_idx;
    else g.wb = global_idx;

    const char *text = lv_btnmatrix_get_btn_text(list, sel);
    if (text) {
        lv_obj_t *parent = lv_obj_get_parent(list);
        lv_obj_t *label = lv_obj_get_child(parent, 0);
        if (label) {
            char buf[32];
            snprintf(buf, sizeof(buf), "Wave Shape: %s", text);
            lv_label_set_text(label, buf);
        }
    }
    wave_shape(global_idx, id);
    update_plotter_by_wave(id);
}

// ========================== NUOVE FUNZIONI PER PRESET E TARGET ==========================

void initPresetNamesA() {
    for (int i = 0; i < MAX_preset; i++)
        snprintf(presetNamesA[i], MAX_timbrA, "Preset %d", i + 1);
}

void initPresetNamesB() {
    for (int i = 0; i < MAX_preset; i++)
        snprintf(presetNamesB[i], MAX_timbrB, "Preset %d", i + 1);
}

void update_preset_labels() {
    char buf[64];
    if (preset_label_A && lv_obj_is_valid(preset_label_A)) {
        snprintf(buf, sizeof(buf), "P%d %s", presetNumA, presetNamesA[presetNumA]);
        lv_label_set_text(preset_label_A, buf);
    }
    if (preset_label_B && lv_obj_is_valid(preset_label_B)) {
        snprintf(buf, sizeof(buf), "P%d %s", presetNumB, presetNamesB[presetNumB]);
        lv_label_set_text(preset_label_B, buf);
    }
}

void update_preset_dropdown(lv_obj_t *dd, int *numPtr) {
    if (!dd || !lv_obj_is_valid(dd) || !numPtr) return;
    lv_dropdown_set_selected(dd, *numPtr);
    update_preset_labels();
}

void update_preset_dropdown_options() {
    char optsA[1024] = {0};   // buffer più grande
    char optsB[1024] = {0};
    size_t lenA = 0, lenB = 0;
    
    for (int i = 0; i < MAX_preset; i++) {
        // Aggiungi nome A
        if (lenA > 0) {
            optsA[lenA++] = '\n';
        }
        // Assicurati che la stringa sia terminata da null prima di concatenare
        // Usa strcat in modo sicuro
        if (lenA < sizeof(optsA) - 1) {
            strcat(optsA, presetNamesA[i]);
            lenA = strlen(optsA);
        }
        // Fai lo stesso per B
        if (lenB > 0) {
            optsB[lenB++] = '\n';
        }
        if (lenB < sizeof(optsB) - 1) {
            strcat(optsB, presetNamesB[i]);
            lenB = strlen(optsB);
        }
    }
    
    // Imposta le opzioni se i dropdown esistono
    if (preset_dropdown_A && lv_obj_is_valid(preset_dropdown_A)) {
        lv_dropdown_set_options(preset_dropdown_A, optsA);
        lv_dropdown_set_selected(preset_dropdown_A, (pendingPresetA >= 0) ? pendingPresetA : presetNumA);
    }
    if (preset_dropdown_B && lv_obj_is_valid(preset_dropdown_B)) {
        lv_dropdown_set_options(preset_dropdown_B, optsB);
        lv_dropdown_set_selected(preset_dropdown_B, (pendingPresetB >= 0) ? pendingPresetB : presetNumB);
    }
    update_preset_labels();
}

void apply_pending_preset(int synth_id) {
    if (synth_id == 0 && pendingPresetA >= 0) {
        presetNumA = pendingPresetA;
        pendingPresetA = -1;
        if (preset_dropdown_A && lv_obj_is_valid(preset_dropdown_A))
            lv_obj_set_style_text_color(preset_dropdown_A, lv_color_hex(0xFFFFFF), 0);
        update_preset_labels();
        update_preset_dropdown_options();
        log_add("Preset A applicato", lv_color_hex(0x00FF00));
        update_all_targets();
    } else if (synth_id == 1 && pendingPresetB >= 0) {
        presetNumB = pendingPresetB;
        pendingPresetB = -1;
        if (preset_dropdown_B && lv_obj_is_valid(preset_dropdown_B))
            lv_obj_set_style_text_color(preset_dropdown_B, lv_color_hex(0xFFFFFF), 0);
        update_preset_labels();
        update_preset_dropdown_options();
        log_add("Preset B applicato", lv_color_hex(0x00FF00));
        update_all_targets();
    }
    if (pendingPresetA < 0 && pendingPresetB < 0 && blink_timer) {
        lv_timer_del(blink_timer);
        blink_timer = NULL;
        blink_state = false;
    }
}

 void blink_timer_cb(lv_timer_t *timer) {
    blink_state = !blink_state;
    lv_color_t c = blink_state ? lv_color_hex(0xFFFF00) : lv_color_hex(0xFFFFFF);
    if (pendingPresetA >= 0 && preset_dropdown_A && lv_obj_is_valid(preset_dropdown_A))
        lv_obj_set_style_text_color(preset_dropdown_A, c, 0);
    if (pendingPresetB >= 0 && preset_dropdown_B && lv_obj_is_valid(preset_dropdown_B))
        lv_obj_set_style_text_color(preset_dropdown_B, c, 0);
    if (pendingPresetA < 0 && pendingPresetB < 0 && blink_timer) {
        lv_timer_del(blink_timer);
        blink_timer = NULL;
        blink_state = false;
    }
}

 void preset_dropdown_event(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    if (!dd || !lv_obj_is_valid(dd)) return;
    int sel = lv_dropdown_get_selected(dd);
    if (sel < 0 || sel >= MAX_preset) return;
    if (dd == preset_dropdown_A) {
        pendingPresetA = sel;
        lv_obj_set_style_text_color(dd, lv_color_hex(0xFFFF00), 0);
        if (!blink_timer) blink_timer = lv_timer_create(blink_timer_cb, 500, NULL);
        log_add("Preset A in attesa", lv_color_hex(0xFFAA00));
    } else if (dd == preset_dropdown_B) {
        pendingPresetB = sel;
        lv_obj_set_style_text_color(dd, lv_color_hex(0xFFFF00), 0);
        if (!blink_timer) blink_timer = lv_timer_create(blink_timer_cb, 500, NULL);
        log_add("Preset B in attesa", lv_color_hex(0xFFAA00));
    }
}

 void preset_btn_plus(lv_event_t *e) {
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    if (id == 0) {
        int val = (pendingPresetA >= 0) ? pendingPresetA : presetNumA;
        pendingPresetA = (++val >= MAX_preset) ? 0 : val;
        if (preset_dropdown_A && lv_obj_is_valid(preset_dropdown_A)) {
            lv_dropdown_set_selected(preset_dropdown_A, pendingPresetA);
            lv_obj_set_style_text_color(preset_dropdown_A, lv_color_hex(0xFFFF00), 0);
            lv_dropdown_open(preset_dropdown_A);
        }
        if (!blink_timer) blink_timer = lv_timer_create(blink_timer_cb, 500, NULL);
    } else {
        int val = (pendingPresetB >= 0) ? pendingPresetB : presetNumB;
        pendingPresetB = (++val >= MAX_preset) ? 0 : val;
        if (preset_dropdown_B && lv_obj_is_valid(preset_dropdown_B)) {
            lv_dropdown_set_selected(preset_dropdown_B, pendingPresetB);
            lv_obj_set_style_text_color(preset_dropdown_B, lv_color_hex(0xFFFF00), 0);
            lv_dropdown_open(preset_dropdown_B);
        }
        if (!blink_timer) blink_timer = lv_timer_create(blink_timer_cb, 500, NULL);
    }
    update_preset_labels();
    char buf[32]; snprintf(buf, sizeof(buf), "Preset +: A=%d B=%d", presetNumA, presetNumB);
    log_add(buf, lv_color_hex(0x66AAFF));
}

 void preset_btn_minus(lv_event_t *e) {
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    if (id == 0) {
        int val = (pendingPresetA >= 0) ? pendingPresetA : presetNumA;
        pendingPresetA = (--val < 0) ? MAX_preset - 1 : val;
        if (preset_dropdown_A && lv_obj_is_valid(preset_dropdown_A)) {
            lv_dropdown_set_selected(preset_dropdown_A, pendingPresetA);
            lv_obj_set_style_text_color(preset_dropdown_A, lv_color_hex(0xFFFF00), 0);
            lv_dropdown_open(preset_dropdown_A);
        }
        if (!blink_timer) blink_timer = lv_timer_create(blink_timer_cb, 500, NULL);
    } else {
        int val = (pendingPresetB >= 0) ? pendingPresetB : presetNumB;
        pendingPresetB = (--val < 0) ? MAX_preset - 1 : val;
        if (preset_dropdown_B && lv_obj_is_valid(preset_dropdown_B)) {
            lv_dropdown_set_selected(preset_dropdown_B, pendingPresetB);
            lv_obj_set_style_text_color(preset_dropdown_B, lv_color_hex(0xFFFF00), 0);
            lv_dropdown_open(preset_dropdown_B);
        }
        if (!blink_timer) blink_timer = lv_timer_create(blink_timer_cb, 500, NULL);
    }
    update_preset_labels();
    char buf[32]; snprintf(buf, sizeof(buf), "Preset -: A=%d B=%d", presetNumA, presetNumB);
    log_add(buf, lv_color_hex(0x66AAFF));
}

 void preset_btn_sel(lv_event_t *e) {
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    apply_pending_preset(id);
}

 static void preset_btn_save(lv_event_t *e) {
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    bool ok = false;
    if (id == 0) {
        ok = saveCurrentPresetA();    // salva i valori
        if (ok) ok = saveNamesToSD(0); // salva i nomi
    } else {
        ok = saveCurrentPresetB();
        if (ok) ok = saveNamesToSD(1);
    }
    if (ok) {
        log_add(id == 0 ? "Preset A salvato" : "Preset B salvato", lv_color_hex(0x00FF00));
        toast_show(id == 0 ? "Preset A salvato!" : "Preset B salvato!", lv_color_hex(0x00FF00), TOAST_DUR);
    } else {
        log_add("Errore salvataggio", lv_color_hex(0xFF0000));
        toast_show("Errore salvataggio!", lv_color_hex(0xFF0000), TOAST_DUR);
    }
}

lv_obj_t* create_preset_selector(lv_obj_t *parent, int x, int y, const char *label_text, int *numPtr, char (*names)[32]) {
    int id = (label_text[7] == 'A') ? 0 : 1;
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 380, 320);
    lv_obj_set_pos(cont, x - 30, y);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, label_text);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(label, 0, 0);

    lv_obj_t *dd = lv_dropdown_create(cont);
    lv_obj_set_size(dd, 220, 50);
    lv_obj_set_pos(dd, 0, 25);
    lv_obj_set_style_bg_color(dd, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(dd, 2, 0);
    lv_obj_set_style_border_color(dd, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_color(dd, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(dd, &lv_font_montserrat_16, 0);
    lv_obj_set_style_pad_left(dd, 10, 0);
    lv_obj_set_style_pad_right(dd, 10, 0);

    char opts[512] = "";
    for (int i = 0; i < MAX_preset; i++) {
        strcat(opts, names[i]);
        if (i < MAX_preset - 1) strcat(opts, "\n");
    }
    lv_dropdown_set_options(dd, opts);
    lv_dropdown_set_selected(dd, *numPtr);

    lv_obj_t *list = lv_dropdown_get_list(dd);
    lv_obj_set_height(list, preset_visible_count * 30);
    lv_obj_set_style_bg_color(list, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_text_color(list, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(list, &lv_font_montserrat_16, 0);
    lv_obj_set_style_pad_all(list, 5, 0);
    lv_obj_add_event_cb(dd, preset_dropdown_event, LV_EVENT_VALUE_CHANGED, NULL);

    int y_plus = 25;
    int y_step = 60 + 6;
    const char *btn_labels[] = {"+", "-", "Sel", "Save"};
    lv_color_t btn_colors[] = {lv_color_hex(0x1A6B4A), lv_color_hex(0x6B1A1A), lv_color_hex(0x0055AA), lv_color_hex(0x885500)};
    lv_color_t btn_borders[] = {lv_color_hex(0x00FF88), lv_color_hex(0xFF4444), lv_color_hex(0x88CCFF), lv_color_hex(0xFFAA44)};
    lv_event_cb_t callbacks[] = {preset_btn_plus, preset_btn_minus, preset_btn_sel, preset_btn_save};

    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_btn_create(cont);
        lv_obj_set_size(btn, 90, 60);
        lv_obj_set_pos(btn, 235, y_plus + i * y_step);
        lv_obj_set_style_bg_color(btn, btn_colors[i], 0);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_set_style_border_color(btn, btn_borders[i], 0);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btn_labels[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl, (i == 3) ? &lv_font_montserrat_18 : &lv_font_montserrat_32, 0);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(btn, callbacks[i], LV_EVENT_CLICKED, (void*)(uintptr_t)id);
    }

    if (id == 0) preset_dropdown_A = dd;
    else preset_dropdown_B = dd;
    return dd;
}

// ========================== RINOMINA ==========================
void close_rename_window() {
    if (rename_win) { lv_obj_del(rename_win); rename_win = NULL; rename_ta = NULL; }
}

 void rename_confirm(lv_event_t *e) {
    const char *name = lv_textarea_get_text(rename_ta);
    if (strlen(name) > 0) {
        if (g.synth && strcmp(g.synth, "SYNTH A") == 0) {
            strncpy(presetNamesA[rename_preset_idx], name, MAX_timbrA - 1);
            presetNamesA[rename_preset_idx][MAX_timbrA - 1] = '\0';
			 saveNamesToSD(0);
        } else if (g.synth && strcmp(g.synth, "SYNTH B") == 0) {
            strncpy(presetNamesB[rename_preset_idx], name, MAX_timbrB - 1);
            presetNamesB[rename_preset_idx][MAX_timbrB - 1] = '\0';
			 saveNamesToSD(1);
        }
        update_preset_dropdown_options();
        log_add("Preset rinominato", lv_color_hex(0x66AAFF));
    }
    close_rename_window();
}

 void rename_cancel(lv_event_t *e) { close_rename_window(); }

void open_rename_window(int idx) {
    if (rename_win) return;
    rename_preset_idx = idx;
    rename_win = lv_win_create(lv_scr_act(), 0);
    lv_obj_set_size(rename_win, 580, 360);
    lv_obj_set_pos(rename_win, 120, 15);
    lv_obj_set_style_bg_color(rename_win, lv_color_hex(0x111111), 0);
    lv_obj_set_style_border_color(rename_win, lv_color_hex(0x666666), 0);
    lv_obj_set_style_border_width(rename_win, 2, 0);
    lv_obj_set_style_radius(rename_win, 8, 0);

    lv_obj_t *client = lv_win_get_content(rename_win);
    lv_obj_set_style_pad_all(client, 10, 0);
    lv_obj_set_style_bg_color(client, lv_color_hex(0x000000), 0);
    lv_obj_set_flex_flow(client, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(client, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(client, 10, 0);

    rename_ta = lv_textarea_create(client);
    lv_obj_set_size(rename_ta, 460, 45);
    lv_obj_set_style_bg_color(rename_ta, lv_color_hex(0x222222), 0);
    lv_obj_set_style_text_color(rename_ta, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(rename_ta, 1, 0);
    lv_obj_set_style_border_color(rename_ta, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(rename_ta, &lv_font_montserrat_18, 0);

    if (g.synth && strcmp(g.synth, "SYNTH A") == 0) {
        lv_textarea_set_text(rename_ta, presetNamesA[idx]);
        lv_textarea_set_max_length(rename_ta, MAX_timbrA - 1);
    } else {
        lv_textarea_set_text(rename_ta, presetNamesB[idx]);
        lv_textarea_set_max_length(rename_ta, MAX_timbrB - 1);
    }

    lv_obj_t *kb = lv_keyboard_create(client);
    lv_obj_set_size(kb, 550, 220);
    lv_obj_set_style_text_font(kb, &lv_font_montserrat_18, 0);
    lv_keyboard_set_textarea(kb, rename_ta);
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
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(btn_cont, 20, 0);

    lv_obj_t *ok_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(ok_btn, 120, 40);
    lv_obj_set_style_bg_color(ok_btn, lv_color_hex(0x1A6B4A), 0);
    lv_obj_t *ok_lbl = lv_label_create(ok_btn);
    lv_label_set_text(ok_lbl, "OK");
    lv_obj_set_style_text_color(ok_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(ok_lbl);
    lv_obj_add_event_cb(ok_btn, rename_confirm, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cancel_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(cancel_btn, 120, 40);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0x6B1A1A), 0);
    lv_obj_t *cancel_lbl = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_lbl, "Annulla");
    lv_obj_set_style_text_color(cancel_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(cancel_lbl);
    lv_obj_add_event_cb(cancel_btn, rename_cancel, LV_EVENT_CLICKED, NULL);
}

 void rename_btn_click(lv_event_t *e) {
    if (g.synth && strcmp(g.synth, "SYNTH A") == 0) open_rename_window(presetNumA);
    else if (g.synth && strcmp(g.synth, "SYNTH B") == 0) open_rename_window(presetNumB);
}

// ========================== TARGET SLIDER ==========================
void update_slider_parameter(int idx, int value) {
    bool isA = (g.synth && strcmp(g.synth, "SYNTH A") == 0);
    if (isA) {
        if (idx < 4) {
            switch (idx) {
                case 0: timbrA[presetNumA][vir_ATTACK_A] = value; break;
                case 1: timbrA[presetNumA][vir_DECAY_A] = value; break;
                case 2: timbrA[presetNumA][vir_SUSTAIN_A] = value; break;
                case 3: timbrA[presetNumA][vir_RELEASE_A] = value; break;
            }
        } else {
            switch (idx) {
                case 4: timbrA[presetNumA][ana_ATTACK_A] = value; break;
                case 5: timbrA[presetNumA][ana_DECAY_A] = value; break;
                case 6: timbrA[presetNumA][ana_SUSTAIN_A] = value; break;
                case 7: timbrA[presetNumA][ana_RELEASE_A] = value; break;
            }
        }
    } else {
        if (idx < 4) {
            switch (idx) {
                case 0: timbrB[presetNumB][vir_BTTACK_B] = value; break;
                case 1: timbrB[presetNumB][vir_DECAY_B] = value; break;
                case 2: timbrB[presetNumB][vir_SUSTAIN_B] = value; break;
                case 3: timbrB[presetNumB][vir_RELEASE_B] = value; break;
            }
        } else {
            switch (idx) {
                case 4: timbrB[presetNumB][ana_BTTACK_B] = value; break;
                case 5: timbrB[presetNumB][ana_DECAY_B] = value; break;
                case 6: timbrB[presetNumB][ana_SUSTAIN_B] = value; break;
                case 7: timbrB[presetNumB][ana_RELEASE_B] = value; break;
            }
        }
    }
}

void update_slider_target(int idx) {
    if (idx < 0 || idx > 7) return;
    int val = 0;
    bool isA = (g.synth && strcmp(g.synth, "SYNTH A") == 0);
    if (isA) {
        if (idx < 4) {
            switch (idx) {
                case 0: val = timbrA[presetNumA][vir_ATTACK_A]; break;
                case 1: val = timbrA[presetNumA][vir_DECAY_A]; break;
                case 2: val = timbrA[presetNumA][vir_SUSTAIN_A]; break;
                case 3: val = timbrA[presetNumA][vir_RELEASE_A]; break;
            }
        } else {
            switch (idx) {
                case 4: val = timbrA[presetNumA][ana_ATTACK_A]; break;
                case 5: val = timbrA[presetNumA][ana_DECAY_A]; break;
                case 6: val = timbrA[presetNumA][ana_SUSTAIN_A]; break;
                case 7: val = timbrA[presetNumA][ana_RELEASE_A]; break;
            }
        }
    } else {
        if (idx < 4) {
            switch (idx) {
                case 0: val = timbrB[presetNumB][vir_BTTACK_B]; break;
                case 1: val = timbrB[presetNumB][vir_DECAY_B]; break;
                case 2: val = timbrB[presetNumB][vir_SUSTAIN_B]; break;
                case 3: val = timbrB[presetNumB][vir_RELEASE_B]; break;
            }
        } else {
            switch (idx) {
                case 4: val = timbrB[presetNumB][ana_BTTACK_B]; break;
                case 5: val = timbrB[presetNumB][ana_DECAY_B]; break;
                case 6: val = timbrB[presetNumB][ana_SUSTAIN_B]; break;
                case 7: val = timbrB[presetNumB][ana_RELEASE_B]; break;
            }
        }
    }
    g.pre[idx] = constrain(val, 0, 255);
    if (arr[idx]) {
        int x = slider_base_x[idx];
        int y = slider_base_y[idx];
        int yf = map(g.pre[idx], 0, 255, (y+242)-13, y+30);
        lv_obj_set_pos(arr[idx], x-18, yf);
        crs[idx] = false;
        lv_obj_clear_flag(arr[idx], LV_OBJ_FLAG_HIDDEN);
        if (slider_objs[idx] && lv_obj_is_valid(slider_objs[idx])) update_slider_color(idx, true);
        if (sd[idx].label && lv_obj_is_valid(sd[idx].label))
            lv_obj_set_style_text_color(sd[idx].label, lv_color_hex(0xFFA500), 0);
        last[idx] = lv_slider_get_value(slider_objs[idx]);
    }
}

void update_all_targets() {
    for (int i = 0; i < 8; i++) update_slider_target(i);
}

// ========================== CONFERMA INIT SD ==========================
 void confirm_yes_click(lv_event_t *e) {
    if (confirm_win) { lv_obj_del(confirm_win); confirm_win = NULL; }
    lv_timer_t *timer = lv_timer_create([](lv_timer_t *t) {
        init_sd();
        log_add("SD inizializzata", lv_color_hex(0x00FF00));
        toast_show("SD inizializzata!", lv_color_hex(0x00FF00), TOAST_DUR);
        lv_timer_del(t);
    }, 100, NULL);
}

 void confirm_no_click(lv_event_t *e) {
    if (confirm_win) { lv_obj_del(confirm_win); confirm_win = NULL; }
    log_add("Inizializzazione SD annullata", lv_color_hex(0xFFAA00));
}

 void init_sd_btn_click(lv_event_t *e) {
    if (confirm_win) return;
    confirm_win = lv_obj_create(lv_scr_act());
    lv_obj_set_size(confirm_win, 300, 160);
    lv_obj_center(confirm_win);
    lv_obj_set_style_bg_color(confirm_win, lv_color_hex(0x222222), 0);
    lv_obj_set_style_border_width(confirm_win, 2, 0);
    lv_obj_set_style_border_color(confirm_win, lv_color_hex(0x666666), 0);
    lv_obj_set_style_radius(confirm_win, 8, 0);

    lv_obj_t *lbl = lv_label_create(confirm_win);
    lv_label_set_text(lbl, "ELIMINO I PRESET?");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *cont = lv_obj_create(confirm_win);
    lv_obj_set_size(cont, 260, 50);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(cont, 20, 0);

    lv_obj_t *yes = lv_btn_create(cont);
    lv_obj_set_size(yes, 100, 40);
    lv_obj_set_style_bg_color(yes, lv_color_hex(0x1A6B4A), 0);
    lv_obj_t *l_yes = lv_label_create(yes);
    lv_label_set_text(l_yes, "Sì");
    lv_obj_set_style_text_color(l_yes, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(l_yes);
    lv_obj_add_event_cb(yes, confirm_yes_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t *no = lv_btn_create(cont);
    lv_obj_set_size(no, 100, 40);
    lv_obj_set_style_bg_color(no, lv_color_hex(0x6B1A1A), 0);
    lv_obj_t *l_no = lv_label_create(no);
    lv_label_set_text(l_no, "No");
    lv_obj_set_style_text_color(l_no, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(l_no);
    lv_obj_add_event_cb(no, confirm_no_click, LV_EVENT_CLICKED, NULL);
}
// ========================== DEFINIZIONI DELLE FUNZIONI DI PRESET_SD ==========================
// (spostate da preset_sd.h per evitare duplicazioni)

void initPresetValues() {
    for (int p = 0; p < MAX_PRESET; p++) {
        for (int i = 0; i < MAX_timbrA; i++) {
            timbrA[p][i] = (i == 0 || i == 1 || i == 19) ? 0 : random(0, 256);
        }
        for (int i = 0; i < MAX_timbrB; i++) {
            timbrB[p][i] = (i == 0 || i == 1 || i == 22) ? 0 : random(0, 256);
        }
    }
}

bool savePresetToSD(int synth, int num) {
    if (num < 0 || num >= MAX_PRESET) return false;
    String path = getPresetPath(synth, num);
    ensureDirectory((synth == 0) ? PRESET_PATH_SYNTH_A : PRESET_PATH_SYNTH_B);
    File f = SD.open(path, FILE_WRITE);
    if (!f) return false;
    String line;
    if (synth == 0) {
        for (int i = 0; i < MAX_timbrA; i++) {
            line += String(timbrA[num][i]);
            if (i < MAX_timbrA - 1) line += ",";
        }
    } else {
        for (int i = 0; i < MAX_timbrB; i++) {
            line += String(timbrB[num][i]);
            if (i < MAX_timbrB - 1) line += ",";
        }
    }
    line += "\n";
    bool ok = f.print(line) == line.length();
    f.close();
    return ok;
}

bool loadPresetFromSD(int synth, int num) {
    if (num < 0 || num >= MAX_PRESET) return false;
    String path = getPresetPath(synth, num);
    if (!SD.exists(path)) return false;
    File f = SD.open(path, FILE_READ);
    if (!f) return false;
    String line = f.readStringUntil('\n');
    f.close();
    line.trim();
    if (line.length() == 0) return false;
    int count = 0, start = 0, end;
    while ((end = line.indexOf(',', start)) != -1 && count < 30) {
        int val = line.substring(start, end).toInt();
        if (synth == 0 && count < MAX_timbrA) timbrA[num][count] = val;
        else if (count < MAX_timbrB) timbrB[num][count] = val;
        start = end + 1;
        count++;
    }
    if (start < line.length()) {
        int val = line.substring(start).toInt();
        if (synth == 0 && count < MAX_timbrA) timbrA[num][count] = val;
        else if (count < MAX_timbrB) timbrB[num][count] = val;
    }
    return true;
}

bool saveNamesToSD(int synth) {
    String path = (synth == 0) ? String(PRESET_PATH_SYNTH_A) + "nomi.csv" : String(PRESET_PATH_SYNTH_B) + "nomi.csv";
    File f = SD.open(path, FILE_WRITE);
    if (!f) return false;
    if (synth == 0) {
        for (int i = 0; i < MAX_PRESET; i++) f.println(presetNamesA[i]);
    } else {
        for (int i = 0; i < MAX_PRESET; i++) f.println(presetNamesB[i]);
    }
    f.close();
    return true;
}

bool loadNamesFromSD(int synth) {
    String path = (synth == 0) ? String(PRESET_PATH_SYNTH_A) + "nomi.csv" : String(PRESET_PATH_SYNTH_B) + "nomi.csv";
    if (!SD.exists(path)) return false;
    File f = SD.open(path, FILE_READ);
    if (!f) return false;
    if (synth == 0) {
        for (int i = 0; i < MAX_PRESET && f.available(); i++) {
            String name = f.readStringUntil('\n');
            name.trim();
			name.replace("\r", "");
            if (name.length() > 0) {
                strncpy(presetNamesA[i], name.c_str(), MAX_timbrA - 1);
                presetNamesA[i][MAX_timbrA - 1] = '\0';
            }
        }
    } else {
        for (int i = 0; i < MAX_PRESET && f.available(); i++) {
            String name = f.readStringUntil('\n');
            name.trim();
			name.replace("\r", "");
            if (name.length() > 0) {
                strncpy(presetNamesB[i], name.c_str(), MAX_timbrB - 1);
                presetNamesB[i][MAX_timbrB - 1] = '\0';
            }
        }
    }
    f.close();
    return true;
}

bool saveAllToSD() {
    bool ok = true;
    for (int i = 0; i < MAX_PRESET; i++) {
        if (!savePresetToSD(0, i)) ok = false;
        if (!savePresetToSD(1, i)) ok = false;
    }
    if (!saveNamesToSD(0)) ok = false;
    if (!saveNamesToSD(1)) ok = false;
    return ok;
}

bool loadAllFromSD() {
    bool ok = true;
    for (int i = 0; i < MAX_PRESET; i++) {
        if (!loadPresetFromSD(0, i)) ok = false;
        if (!loadPresetFromSD(1, i)) ok = false;
    }
    if (!loadNamesFromSD(0)) ok = false;
    if (!loadNamesFromSD(1)) ok = false;
    return ok;
}

bool saveCurrentPresetA() { return savePresetToSD(0, presetNumA); }
bool saveCurrentPresetB() { return savePresetToSD(1, presetNumB); }
bool loadCurrentPresetA() { return loadPresetFromSD(0, presetNumA); }
bool loadCurrentPresetB() { return loadPresetFromSD(1, presetNumB); }

void init_sd() {
    const char* folders[] = {"/preset", PRESET_PATH_SYNTH_A, PRESET_PATH_SYNTH_B,
                             "/preset/Chorus", "/preset/dlyA", "/preset/dlyB",
                             "/preset/FM", "/preset/RevFV1"};
    for (int i = 0; i < 8; i++) ensureDirectory(folders[i]);
    initPresetValues();
    initPresetNamesA();
    initPresetNamesB();
    saveAllToSD();
    log_add("SD inizializzata con preset di default", lv_color_hex(0x00FF00));
}