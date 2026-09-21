// ============================================================
// lvglGrafFunc.cpp - UI principale: widget, plotter, timeline,
//                    pot container, pagine, eventi
// ============================================================
#include "globals.h"
#include "lvglGraf.h"
#include "preset_sd.h"
#include <Arduino.h>
#include <math.h>
#include "lvgl_v8_port.h"   // per lvgl_port_lock/unlock

// ========================== COSTANTI LOCALI ==========================
#define TL_ACTIVE_OPA  70
#define TL_IDLE_OPA      0
#define ENV_POINTS     60

// ========================== STATICHE LOCALI ==========================
static void disc_btn_click(lv_event_t *e) {
    (void)e;
    discover_request_restart();
}

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
    int cat_btn_w = 80, cat_btn_h = 90, cat_gap = 15, cat_start_x = 10, cat_y = 94;
    for (int i = 0; i < 3; i++) {
        lv_obj_t *led = lv_obj_create(p);
        lv_obj_set_size(led, 12, 12);
        lv_obj_set_style_radius(led, 6, 0);
        lv_obj_set_style_bg_color(led, lv_color_hex(0x333333), 0);
        lv_obj_set_style_border_width(led, 0, 0);
        int led_x = cat_start_x + i * (cat_btn_w + cat_gap) + cat_btn_w/2 - 6;
        lv_obj_set_pos(led, led_x, 80);
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

    int plotter_x = 400, plotter_y = 110;
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
 //   wave_shape(global_idx, synth_id);
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
    lv_color_t target = active ? lv_color_hex(COLOR_SHAPE_SLIDER_ACTIVE) : lv_color_hex(COLOR_SHAPE_SLIDER_PASSED);
    lv_obj_set_style_bg_img_recolor(slider, target, LV_PART_INDICATOR);
    lv_obj_set_style_bg_img_recolor_opa(slider, (sliderColorDepth * 255) / 100, LV_PART_INDICATOR);
}

// ========================== ARC ==========================
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
                lv_obj_set_style_arc_color(g.arc_obj[idx], lv_color_hex(COLOR_ARC_PASSED), LV_PART_INDICATOR);
            } else {
                lv_obj_clear_flag(g.arc_arrow[idx], LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_style_arc_color(g.arc_obj[idx], lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
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

// ========================== POT CONTAINER ==========================
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

// ========================== TIMELINE ==========================
static void format_time(uint32_t ms, char *buf, size_t buflen) {
    uint32_t total_s = ms / 1000;
    uint32_t m = total_s / 60;
    uint32_t s = total_s % 60;
    if (m > 99) m = 99;
    snprintf(buf, buflen, "%02u:%02u", (unsigned)m, (unsigned)s);
}

void tl_set_buttons(int active) {
    if (timeline_btn_init && lv_obj_is_valid(timeline_btn_init))
        lv_obj_set_style_img_recolor_opa(timeline_btn_init,
            (active == 0) ? TL_ACTIVE_OPA : TL_IDLE_OPA, 0);
    if (timeline_btn_stop && lv_obj_is_valid(timeline_btn_stop))
        lv_obj_set_style_img_recolor_opa(timeline_btn_stop,
            (active == 1) ? TL_ACTIVE_OPA : TL_IDLE_OPA, 0);
    if (timeline_btn_play && lv_obj_is_valid(timeline_btn_play))
        lv_obj_set_style_img_recolor_opa(timeline_btn_play,
            (active == 2) ? TL_ACTIVE_OPA : TL_IDLE_OPA, 0);
}

static void tl_init_cb(lv_event_t *e) {
    (void)e;
    if (timeline_playing) return;
    timeline_demo_ms = 0;
    update_timeline(0, 60000);
    tl_set_buttons(0);
}

static void tl_stop_cb(lv_event_t *e) {
    (void)e;
    timeline_playing = false;
    tl_set_buttons(1);
}

static void tl_play_cb(lv_event_t *e) {
    (void)e;
    timeline_playing = true;
    tl_set_buttons(2);
}

lv_obj_t* create_timeline_controls(lv_obj_t *parent, int x, int y) {
    const int btn_w = 100;
    const int btn_h = 80;
    const int gap   = 5;

    timeline_btn_init = lv_img_create(parent);
    lv_img_set_src(timeline_btn_init, &img_bott_init);
    lv_obj_set_pos(timeline_btn_init, x, y);
    lv_obj_set_size(timeline_btn_init, btn_w, btn_h);
    lv_obj_add_flag(timeline_btn_init, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_img_recolor(timeline_btn_init, lv_color_hex(0x00AAFF), 0);
    lv_obj_set_style_img_recolor_opa(timeline_btn_init, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(timeline_btn_init, tl_init_cb, LV_EVENT_CLICKED, NULL);

    timeline_btn_stop = lv_img_create(parent);
    lv_img_set_src(timeline_btn_stop, &img_bott_stop);
    lv_obj_set_pos(timeline_btn_stop, x + btn_w + gap, y);
    lv_obj_set_size(timeline_btn_stop, btn_w, btn_h);
    lv_obj_add_flag(timeline_btn_stop, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_img_recolor(timeline_btn_stop, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_style_img_recolor_opa(timeline_btn_stop, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(timeline_btn_stop, tl_stop_cb, LV_EVENT_CLICKED, NULL);

    timeline_btn_play = lv_img_create(parent);
    lv_img_set_src(timeline_btn_play, &img_bott_play);
    lv_obj_set_pos(timeline_btn_play, x + 2*(btn_w + gap), y);
    lv_obj_set_size(timeline_btn_play, btn_w, btn_h);
    lv_obj_add_flag(timeline_btn_play, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_img_recolor(timeline_btn_play, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_img_recolor_opa(timeline_btn_play, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(timeline_btn_play, tl_play_cb, LV_EVENT_CLICKED, NULL);

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
                lv_obj_set_style_arc_color(g.arc_obj[i], lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
            } else {
                g.arc_arrow[i] = NULL;
            }
        } else {
            g.arc_obj[i]   = NULL;
            g.arc_arrow[i] = NULL;
        }
    }

    log_add("Freccine ripristinate", lv_color_hex(0x66AAFF));
    toast_show("Freccine ripristinate!", lv_color_hex(0x66AAFF), TOAST_DUR);
}

/* void wave_shape(int id, int src) {
    const char *sn = (src == SRC_A) ? "Synth A" : "Synth B";
    char b[60]; snprintf(b, 60, "[%s] Selezionato: %s", sn, WAVE_DEFS[id].name);
    log_add(b, lv_color_hex(0x66AAFF));
} */

// ========================== LOG WIDGET ==========================
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
// ========================== PRESET LABEL (riutilizzabile) ==========================
// Crea la label con il nome del preset attivo accanto al titolo della pagina.
// - title_lbl : puntatore al titolo esistente (es. "SYNTH A", "VCF A").
//               Se NULL, la label va a coordinate fisse.
// - isA       : true per Synth A, false per Synth B.
// Assegna anche preset_label_A / preset_label_B per update_preset_labels().
static lv_obj_t* create_preset_label(lv_obj_t *parent, lv_obj_t *title_lbl, bool isA) {
    lv_obj_t *info = lv_label_create(parent);
    char buf[64];
    if (isA) snprintf(buf, sizeof(buf), "Pn%d %s", presetNumA+1, presetNamesA[presetNumA]);
    else     snprintf(buf, sizeof(buf), "Pn%d %s", presetNumB+1, presetNamesB[presetNumB]);
    lv_label_set_text(info, buf);
    lv_obj_set_style_text_color(info, lv_color_hex(0xFFFFFF), 0);   // BIANCO
    lv_obj_set_style_text_font(info, &lv_font_montserrat_24, 0);    // font grande
    if (title_lbl) lv_obj_align_to(info, title_lbl, LV_ALIGN_OUT_RIGHT_MID, 30, 0);
    else           lv_obj_align(info, LV_ALIGN_TOP_LEFT, 250, 8);
    if (isA) preset_label_A = info;
    else     preset_label_B = info;
    return info;
}
// ========================== HOME ==========================
void create_home() {
    pendingPresetA = pendingPresetB = -1;
    if (blink_timer) { lv_timer_del(blink_timer); blink_timer = NULL; blink_state = false; }
    close_rename_window();
    if (g.play) { stop_meters(); g.play = 0; }
    if (tdt) { lv_timer_del(tdt); tdt = nullptr; timeline_demo_ms = 0; }
    if (g.page) { lv_obj_del(g.page); g.page = 0; }
    g.synth = 0; g.err = g.log_v = g.toast_v = 0;
    g.log_c = g.log_f = g.toast = 0;
    for (int i=0; i<MAX_SLIDERS; i++) { arr[i]=0; slider_objs[i]=0; crs[i]=0; last[i]=0; }
    g.shape_arrow_A = g.shape_arrow_B = 0;
    g.shape_slider_A = g.shape_slider_B = 0;
    for (int i = 0; i < MAX_ARCS; i++){ g.arc_obj[i] = g.arc_arrow[i] = g.arc_label_value[i] = g.arc_label_p[i] = NULL;}
    pot_container = NULL;
    preset_dropdown_A = preset_dropdown_B = NULL;
    preset_label_A = preset_label_B = NULL;
    timeline_obj = timeline_bar_bg = timeline_bar_fill = NULL;
    timeline_cursor = timeline_label_cur = timeline_label_tot = NULL;
    timeline_btn_init = timeline_btn_stop = timeline_btn_play = nullptr;
    timeline_playing  = false;
    timeline_demo_ms  = 0;
    env_chart_A = env_chart_B = nullptr;
    env_serie_A = env_serie_B = nullptr;
    env_serie_tgt_A = env_serie_tgt_B = nullptr;
	keyboard_obj = nullptr;
for (int i = 0; i < KB_WHITE_KEYS; i++) kb_white[i] = nullptr;
for (int i = 0; i < KB_BLACK_KEYS; i++) kb_black[i] = nullptr;
drum_pattern_label = nullptr;

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

    create_log_widget(m, 50, 97, 700, 240);

    const char *n[] = {"PLAY","SYNTH A","SYNTH B","DRUM","SET UP"};
    lv_color_t cols[] = {lv_color_hex(0x00FF00), lv_color_hex(0xCC3300), lv_color_hex(0x0099FF), lv_color_hex(0xFFCC33), lv_color_hex(0x999999)};
    for (int i=0; i<5; i++) btn(m, n[i], 30+i*155, 360, 120, 90, cols[i], i);
}

// ========================== ENVELOPE PLOTTER ==========================
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
static void midi_split_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    midi_split = lv_dropdown_get_selected(dd);
    update_keyboard_colors();    // <-- ridisegna la tastiera
   /*  char buf[48];
    snprintf(buf, sizeof(buf), "SPLIT = %d", midi_split);
    log_add(buf, lv_color_hex(0x66AAFF)); */
}
// ========================== MIDI CONFIG EVENTS ==========================
static void midi_ch_a_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    midi_channel_A = lv_dropdown_get_selected(dd) + 1;
    char buf[40];
    snprintf(buf, sizeof(buf), "SynthA MIDI CH = %d", midi_channel_A);
    log_add(buf, lv_color_hex(0x66AAFF));
}

static void midi_ch_b_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    midi_channel_B = lv_dropdown_get_selected(dd) + 1;
    char buf[40];
    snprintf(buf, sizeof(buf), "SynthB MIDI CH = %d", midi_channel_B);
    log_add(buf, lv_color_hex(0x66AAFF));
}
static void midi_ch_d_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    midi_channel_D = lv_dropdown_get_selected(dd) + 1;
    char buf[40];
    snprintf(buf, sizeof(buf), "DRUM MIDI CH = %d", midi_channel_D);
    log_add(buf, lv_color_hex(0x66AAFF));
}
// ========================== KEYBOARD WIDGET ==========================
// Tastiera 32 tasti (F3..C6). Layout pianistico:
//   19 tasti bianchi, 13 tasti neri sovrapposti.
// Colori:
//   indice <= midi_split  ->  giallo leggero
//   indice >  midi_split  ->  azzurro leggero
lv_obj_t* create_keyboard(lv_obj_t *parent, int x, int y, int w, int h) {
    // Note index (0..31):
    //   0=F3, 1=F#3, 2=G3, ... 31=C6
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

    // --- Tasti bianchi ---
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

    // --- Tasti neri sovrapposti ---
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
    // Tasti bianchi: giallo tenue / azzurro tenue
    lv_color_t w_yellow = lv_color_hex(0xFFFFAA);
    lv_color_t w_blue   = lv_color_hex(0xAADDFF);
    // Tasti neri: varianti scure per mantenere leggibilità
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
// ========================== PAGINE SECONDARIE ==========================
void create_page(const char *title) {
    pendingPresetA = pendingPresetB = -1;
    if (blink_timer) { lv_timer_del(blink_timer); blink_timer = NULL; blink_state = false; }
    close_rename_window();
    if (g.play && strcmp(title, "PLAY") != 0) { stop_meters(); g.play = 0; }
    if (g.page) { lv_obj_del(g.page); g.page = 0; }
    g.log_c = g.log_f = g.toast = 0; g.toast_v = 0;
    for (int i=0; i<MAX_SLIDERS; i++) { arr[i]=0; slider_objs[i]=0; crs[i]=0; last[i]=0; }
    g.shape_arrow_A = g.shape_arrow_B = 0;
    g.shape_slider_A = g.shape_slider_B = 0;
    for (int i = 0; i < MAX_ARCS; i++){ g.arc_obj[i] = g.arc_arrow[i] = g.arc_label_value[i] = g.arc_label_p[i] = NULL;}
    pot_container = NULL;
    preset_dropdown_A = preset_dropdown_B = NULL;
    preset_label_A = preset_label_B = NULL;
    timeline_obj = timeline_bar_bg = timeline_bar_fill = NULL;
    timeline_cursor = timeline_label_cur = timeline_label_tot = NULL;
    timeline_btn_init = timeline_btn_stop = timeline_btn_play = nullptr;
    timeline_playing  = false;
    timeline_demo_ms  = 0;
    env_chart_A = env_chart_B = nullptr;
    env_serie_A = env_serie_B = nullptr;
    env_serie_tgt_A = env_serie_tgt_B = nullptr;
	drum_pattern_label = nullptr;
	for (int i = 0; i < KB_WHITE_KEYS; i++) kb_white[i] = nullptr;
for (int i = 0; i < KB_BLACK_KEYS; i++) kb_black[i] = nullptr;
	
	
    if (tdt) { lv_timer_del(tdt); tdt = nullptr; }
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
        g.play = 1;
        int w = 22, h = 150;

        mL = meter(m, 685, 10, w, h, "L", 0);
        mR = meter(m, 709, 10, w, h, "R", 0);
        mC = meter(m, 750, 10, w, h, "C", 1);
        if (!mt) mt = lv_timer_create([](lv_timer_t*){ update_meters(); }, 33, 0);

        create_timeline(m, 50, 220, 700, 40);
        create_timeline_controls(m, 50, 265);

        timeline_playing = false;
        timeline_demo_ms = 0;
        update_timeline(0, 60000);
        tl_set_buttons(1);

        if (!tdt) {
            tdt = lv_timer_create([](lv_timer_t*){
                if (timeline_playing) {
                    timeline_demo_ms += 100;
                    if (timeline_demo_ms > 60000) timeline_demo_ms = 0;
                    update_timeline(timeline_demo_ms, 60000);
                }
            }, 100, 0);
        }

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
		
		        create_preset_label(m, t, (id == 0));

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
    bool isA = (g.synth && strcmp(g.synth, "SYNTH A") == 0);
    int id   = isA ? SRC_A : SRC_B;
    h_slider(m, id);
    create_preset_label(m, NULL, isA);   // t è NULL in DCO, posizione fissa (250, 8)
    home_btn(m, -2);
}
 else if (strncmp(title, "MOD", 3) == 0) {
    bool isA = (g.synth && strcmp(g.synth, "SYNTH A") == 0);
    Serial.printf("[MOD] title='%s' synth=%s isA=%d\n",
                  title, g.synth ? g.synth : "(null)", isA);
    if (isA) {
        create_pot_container(m, 100, 100);
        Serial.printf("[MOD] pot_container=%p\n", (void*)pot_container);
		   create_preset_label(m, NULL, isA);   // t è NULL in DCO, posizione fissa (250, 8)
    }
    home_btn(m, -2);
}
    else if (strcmp(title, "SET UP") == 0) {
        lv_obj_t *btn1 = lv_btn_create(m);
        lv_obj_set_size(btn1, 160, 70);
        lv_obj_set_pos(btn1, 50, 80);
        lv_obj_set_style_bg_color(btn1, lv_color_hex(0xCC3333), 0);
        lv_obj_set_style_bg_color(btn1, lv_color_hex(0x992222), LV_STATE_PRESSED);
        lv_obj_set_style_radius(btn1, 10, 0);
        lv_obj_set_style_border_width(btn1, 2, 0);
        lv_obj_set_style_border_color(btn1, lv_color_hex(0xFF8888), 0);
        lv_obj_t *lbl1 = lv_label_create(btn1);
        lv_label_set_text(lbl1, "init SD");
        lv_obj_set_style_text_color(lbl1, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl1, &lv_font_montserrat_20, 0);
        lv_obj_center(lbl1);
        lv_obj_add_event_cb(btn1, init_sd_btn_click, LV_EVENT_CLICKED, NULL);

        lv_obj_t *btn_disc = lv_btn_create(m);
        lv_obj_set_size(btn_disc, 220, 70);
        lv_obj_set_pos(btn_disc, 240, 80);
        lv_obj_set_style_bg_color(btn_disc, lv_color_hex(0x1A4B6B), 0);
        lv_obj_set_style_bg_color(btn_disc, lv_color_hex(0x123348), LV_STATE_PRESSED);
        lv_obj_set_style_radius(btn_disc, 10, 0);
        lv_obj_set_style_border_width(btn_disc, 2, 0);
        lv_obj_set_style_border_color(btn_disc, lv_color_hex(0x44AAFF), 0);
        lv_obj_t *lbl_disc = lv_label_create(btn_disc);
        lv_label_set_text(lbl_disc, "Riavvia Discovery");
        lv_obj_set_style_text_color(lbl_disc, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl_disc, &lv_font_montserrat_20, 0);
        lv_obj_center(lbl_disc);
        lv_obj_add_event_cb(btn_disc, disc_btn_click, LV_EVENT_CLICKED, NULL);

        // --- Pulsante "Salva SetUp" ---
        lv_obj_t *btn_save = lv_btn_create(m);
        lv_obj_set_size(btn_save, 200, 70);
        lv_obj_set_pos(btn_save, 480, 80);
        lv_obj_set_style_bg_color(btn_save, lv_color_hex(0x1A6B4A), 0);
        lv_obj_set_style_bg_color(btn_save, lv_color_hex(0x0F4A2E), LV_STATE_PRESSED);
        lv_obj_set_style_radius(btn_save, 10, 0);
        lv_obj_set_style_border_width(btn_save, 2, 0);
        lv_obj_set_style_border_color(btn_save, lv_color_hex(0x00FF88), 0);
        lv_obj_t *lbl_save = lv_label_create(btn_save);
        lv_label_set_text(lbl_save, "Salva SetUp");
        lv_obj_set_style_text_color(lbl_save, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl_save, &lv_font_montserrat_20, 0);
        lv_obj_center(lbl_save);
        lv_obj_add_event_cb(btn_save, eb, LV_EVENT_CLICKED, (void*)(uintptr_t)21);

        // --- Pulsante "MIDI" (stessa posizione dei bottoni submenu synth) ---
        lv_obj_t *btn_midi = lv_btn_create(m);
        lv_obj_set_size(btn_midi, 120, 90);
        lv_obj_set_pos(btn_midi, 30, 360);
        lv_obj_set_style_bg_color(btn_midi, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_bg_color(btn_midi, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
        lv_obj_set_style_radius(btn_midi, 8, 0);
        lv_obj_set_style_border_width(btn_midi, 3, 0);
        lv_obj_set_style_border_color(btn_midi, lv_color_hex(0x44FF88), 0);
        lv_obj_t *lbl_midi = lv_label_create(btn_midi);
        lv_label_set_text(lbl_midi, "MIDI");
        lv_obj_set_style_text_color(lbl_midi, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl_midi, &lv_font_montserrat_18, 0);
        lv_obj_center(lbl_midi);
        lv_obj_add_event_cb(btn_midi, eb, LV_EVENT_CLICKED, (void*)(uintptr_t)20);

        home_btn(m, -1);
    }
	
	  else if (strcmp(title, "MIDI") == 0) {
    // ============ RIGA SynthA: [SynthA] [CH] [dd] [SPLIT] [dd] ============
    lv_obj_t *lbl_a = lv_label_create(m);
    lv_label_set_text(lbl_a, "SynthA");
    lv_obj_set_style_text_color(lbl_a, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_a, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(lbl_a, 40, 105);
        // Tastiera 32 tasti: x allineato al dropdown SPLIT, y tra top e dropdown
        create_keyboard(m, 380, 5, 360, 85);
    // CH (colonna 1)
    lv_obj_t *lbl_ch_a = lv_label_create(m);
    lv_label_set_text(lbl_ch_a, "CH");
    lv_obj_set_style_text_color(lbl_ch_a, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(lbl_ch_a, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(lbl_ch_a, 160, 112);

    lv_obj_t *dd_a = lv_dropdown_create(m);
    lv_obj_set_size(dd_a, 80, 45);
    lv_obj_set_pos(dd_a, 200, 100);
    lv_obj_set_style_bg_color(dd_a, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(dd_a, 2, 0);
    lv_obj_set_style_border_color(dd_a, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_color(dd_a, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(dd_a, &lv_font_montserrat_18, 0);
    lv_obj_set_style_pad_left(dd_a, 8, 0);
    lv_dropdown_set_options(dd_a, "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16");
    lv_dropdown_set_selected(dd_a, (midi_channel_A >= 1 && midi_channel_A <= 16) ? midi_channel_A - 1 : 0);
    lv_obj_add_event_cb(dd_a, midi_ch_a_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // SPLIT (colonna 2)
    lv_obj_t *lbl_sp_a = lv_label_create(m);
    lv_label_set_text(lbl_sp_a, "SPLIT");
    lv_obj_set_style_text_color(lbl_sp_a, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(lbl_sp_a, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(lbl_sp_a, 310, 112);

    lv_obj_t *dd_sp_a = lv_dropdown_create(m);
    lv_obj_set_size(dd_sp_a, 100, 45);
    lv_obj_set_pos(dd_sp_a, 380, 100);
    lv_obj_set_style_bg_color(dd_sp_a, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(dd_sp_a, 2, 0);
    lv_obj_set_style_border_color(dd_sp_a, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_color(dd_sp_a, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(dd_sp_a, &lv_font_montserrat_18, 0);
    lv_obj_set_style_pad_left(dd_sp_a, 8, 0);
    lv_dropdown_set_options(dd_sp_a,
        "F3\nF#3\nG3\nG#3\nA3\nA#3\nB3\n"
        "C4\nC#4\nD4\nD#4\nE4\nF4\nF#4\nG4\nG#4\nA4\nA#4\nB4\n"
        "C5\nC#5\nD5\nD#5\nE5\nF5\nF#5\nG5\nG#5\nA5\nA#5\nB5\nC6");
    lv_dropdown_set_selected(dd_sp_a, (midi_split <= 31) ? midi_split : 0);
    lv_obj_add_event_cb(dd_sp_a, midi_split_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // ============ RIGA SynthB ============
    lv_obj_t *lbl_b = lv_label_create(m);
    lv_label_set_text(lbl_b, "SynthB");
    lv_obj_set_style_text_color(lbl_b, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_b, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(lbl_b, 40, 205);

    lv_obj_t *lbl_ch_b = lv_label_create(m);
    lv_label_set_text(lbl_ch_b, "CH");
    lv_obj_set_style_text_color(lbl_ch_b, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(lbl_ch_b, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(lbl_ch_b, 160, 212);

    lv_obj_t *dd_b = lv_dropdown_create(m);
    lv_obj_set_size(dd_b, 80, 45);
    lv_obj_set_pos(dd_b, 200, 200);
    lv_obj_set_style_bg_color(dd_b, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(dd_b, 2, 0);
    lv_obj_set_style_border_color(dd_b, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_color(dd_b, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(dd_b, &lv_font_montserrat_18, 0);
    lv_obj_set_style_pad_left(dd_b, 8, 0);
    lv_dropdown_set_options(dd_b, "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16");
    lv_dropdown_set_selected(dd_b, (midi_channel_B >= 1 && midi_channel_B <= 16) ? midi_channel_B - 1 : 1);
    lv_obj_add_event_cb(dd_b, midi_ch_b_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // ============ RIGA DRUM ============
    lv_obj_t *lbl_d = lv_label_create(m);
    lv_label_set_text(lbl_d, "DRUM");
    lv_obj_set_style_text_color(lbl_d, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_d, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(lbl_d, 40, 305);

    lv_obj_t *lbl_ch_d = lv_label_create(m);
    lv_label_set_text(lbl_ch_d, "CH");
    lv_obj_set_style_text_color(lbl_ch_d, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(lbl_ch_d, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(lbl_ch_d, 160, 312);

    lv_obj_t *dd_d = lv_dropdown_create(m);
    lv_obj_set_size(dd_d, 80, 45);
    lv_obj_set_pos(dd_d, 200, 300);
    lv_obj_set_style_bg_color(dd_d, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(dd_d, 2, 0);
    lv_obj_set_style_border_color(dd_d, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_color(dd_d, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(dd_d, &lv_font_montserrat_18, 0);
    lv_obj_set_style_pad_left(dd_d, 8, 0);
    lv_dropdown_set_options(dd_d, "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16");
    lv_dropdown_set_selected(dd_d, (midi_channel_D >= 1 && midi_channel_D <= 16) ? midi_channel_D - 1 : 2);
    lv_obj_add_event_cb(dd_d, midi_ch_d_cb, LV_EVENT_VALUE_CHANGED, NULL);

    home_btn(m, -1);
}
      else if (strcmp(title, "DRUM") == 0) {
        // ---- Label pattern drum (aggiornata via LWS dal Teensy) ----
        drum_pattern_label = lv_label_create(m);
        lv_label_set_text(drum_pattern_label, "PTN --: --");
        lv_obj_set_style_text_color(drum_pattern_label, lv_color_hex(0x00FFFF), 0);
        lv_obj_set_style_text_font(drum_pattern_label, &lv_font_montserrat_32, 0);
        lv_obj_set_pos(drum_pattern_label, 50, 80);

        // (opzionale) etichetta statica sopra
        lv_obj_t *lbl_h = lv_label_create(m);
        lv_label_set_text(lbl_h, "Drum Pattern");
        lv_obj_set_style_text_color(lbl_h, lv_color_hex(0xAAAAAA), 0);
        lv_obj_set_style_text_font(lbl_h, &lv_font_montserrat_18, 0);
        lv_obj_set_pos(lbl_h, 50, 55);

        home_btn(m, -1);
    }
    else if (strncmp(title, "VCF", 3) == 0 || strncmp(title, "VCA", 3) == 0) {
    bool isA = (g.synth && strcmp(g.synth, "SYNTH A") == 0);
    lv_color_t active_color = isA ? lv_color_hex(COLOR_SLIDER_SYNTH_A_ACTIVE) : lv_color_hex(COLOR_SLIDER_SYNTH_B_ACTIVE);
    lv_color_t passed_color = isA ? lv_color_hex(COLOR_SLIDER_SYNTH_A_PASSED) : lv_color_hex(COLOR_SLIDER_SYNTH_B_PASSED);
    int base = (strncmp(title, "VCF", 3) == 0) ? 0 : 4;
    const char *labels[] = {"A", "D", "S", "R"};
    for (int i=0; i<4; i++) slider(m, 20 + i*80, 96, labels[i], base + i, active_color, passed_color);

    if (base == 4) {
        create_env_plot(m, 620, 116, 150, 100, isA);
        update_env_plot(isA);
    }

    create_preset_label(m, t, isA);   // <-- NUOVO
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

    create_log_widget(m, 50, 200, 600, 220);
}

// ========================== GESTORI EVENTI ==========================
void eb(lv_event_t *e) {
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    if (g.play && (id == -1 || (id >= 0 && id < 5))) { stop_meters(); g.play = 0; }
    if (id == -2) {
        if (g.synth) { log_add("<- Torno al synth", lv_color_hex(0xFFFFFF)); lvgl_port_lock(-1); create_page(g.synth); lvgl_port_unlock(); }
        else { log_add("<- HOME", lv_color_hex(0xFFFFFF)); lvgl_port_lock(-1); create_home(); lvgl_port_unlock(); }
        return;
    }
    if (id == -1) { log_add("<- HOME", lv_color_hex(0xFFFFFF)); lvgl_port_lock(-1); create_home(); lvgl_port_unlock(); return; }
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
	    if (id == 21) {
        save_all_settings();
        return;
    }
	    if (id == 20) {
        log_add("Apro: MIDI", lv_color_hex(0xFFFFFF));
        lvgl_port_lock(-1);
        create_page("MIDI");
        lvgl_port_unlock();
        return;
    }
    if (id >= 0 && id < 5) {
        const char *pages[] = {"PLAY","SYNTH A","SYNTH B","DRUM","SET UP"};
        char b[20]; snprintf(b, 20, "> Apro: %s", pages[id]);
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

    if (d->idx >= 4 && d->idx <= 7) {
        bool isA = (g.synth && strcmp(g.synth, "SYNTH A") == 0);
        update_slider_parameter(d->idx, cur);
        update_env_plot(isA);
    }
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

    /* const char *cat_names[] = {"WF", "FM", "AM"};
    char logbuf[32];
    snprintf(logbuf, sizeof(logbuf), "Cat: %s (Synth %c)", cat_names[cat], (synth_id == SRC_A) ? 'A' : 'B');
    log_add(logbuf, lv_color_hex(0x66AAFF)); */
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
  //  wave_shape(global_idx, id);
    update_plotter_by_wave(id);
}