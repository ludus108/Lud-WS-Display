#pragma once
#ifndef LVGL_GRAF_A_H
#define LVGL_GRAF_A_H

#include <lvgl.h>
#include "lvgl_v8_port.h"
#include <math.h>
#include <Arduino.h>

// ========================== VARIABILI ESTERNE (definite nel .ino) ==========================
extern struct {
    lv_obj_t *page, *log_f, *log_c, *toast;
    const char *synth;
    uint8_t bright, wa, wb, shape_a, shape_b, pre[8];
    uint32_t log_t, toast_t;
    bool log_v, toast_v, err, play;
    uint8_t pre_shape_A, pre_shape_B;
    bool shape_crs_A, shape_crs_B;
    int shape_last_A, shape_last_B;
    lv_obj_t *shape_arrow_A, *shape_arrow_B;
    lv_obj_t *shape_slider_A, *shape_slider_B;
    uint8_t arc_value, arc_target;
    bool arc_crossed;
    int arc_last;
    lv_obj_t *arc_obj, *arc_arrow, *arc_label_value, *arc_label_p1;
} g;

extern const char* last_version;
extern lv_obj_t *arr[8];
extern lv_obj_t *slider_objs[8];
extern bool crs[8];
extern int last[8];
extern struct SliderData { lv_obj_t *label; int idx; } sd[8];
extern struct ShapeData {
    lv_obj_t *label;
    int id;
    lv_obj_t *chart;
    lv_chart_series_t *serie;
    lv_obj_t *list;
    lv_obj_t *leds[3];
    int current_cat;
    const char *list_items;
} shape_data[2];
extern lv_obj_t *mL, *mR, *mC;
extern lv_timer_t *mt;
extern float pkL, pkR, pkC;
extern uint8_t sliderColorDepth;
extern int MAX_preset;

// ========================== DICHIARAZIONI IMMAGINI ==========================
extern const lv_img_dsc_t img_slider_track;
extern const lv_img_dsc_t img_slider_indicator;
extern const lv_img_dsc_t img_slider_knob;
extern const lv_img_dsc_t freccina;
extern const lv_img_dsc_t freccina_oriz;
extern const lv_img_dsc_t img_meter_audio_track;
extern const lv_img_dsc_t img_meter_audio_indicator;
extern const lv_img_dsc_t img_meter_comp_track;
extern const lv_img_dsc_t img_meter_comp_indicator;
extern const lv_img_dsc_t img_slider_oriz_track;
extern const lv_img_dsc_t img_slider_oriz_indicator;
extern const lv_img_dsc_t img_slider_oriz_knob;
extern const lv_img_dsc_t img_arc_bg;
extern const lv_img_dsc_t img_arc_indic;
extern const lv_img_dsc_t home;

// ========================== PROTOTIPI FUNZIONI CORE ==========================
void btn(lv_obj_t *p, const char *l, int x, int y, int w, int h, lv_color_t c, int id);
void home_btn(lv_obj_t *p, int id);
void slider(lv_obj_t *p, int x, int y, const char *l, int idx,
            lv_color_t active_color, lv_color_t passed_color);
void h_slider(lv_obj_t *p, int id);
void submenu(lv_obj_t *p);
lv_obj_t* meter(lv_obj_t *p, int x, int y, int w, int h, const char *label, bool inv);
void update_meters();
void stop_meters();
void log_show();
void log_hide();
void log_add(const char *msg, lv_color_t c);
void toast_show(const char *msg, lv_color_t c, uint32_t dur);
void set_bright(uint8_t v);
void load_bright();
void save_bright();
void reset_arrows();
void wave_shape(int id, int src);
void create_home();
void create_page(const char *title);
void update_plotter_by_wave(int synth_id);
void grid_btn_click(lv_event_t *e);
void populate_grid(lv_obj_t *container, const char *items, int selected_idx, int synth_id);
void update_wave_plot(lv_obj_t *chart, lv_chart_series_t *serie, uint8_t shape_val, int wave_index);
void update_leds(lv_obj_t **leds, int active_cat);
void update_slider_color(int idx, bool active);
void update_shape_slider_color(int synth_id, bool active);
void arc_with_image(lv_obj_t *parent, int x, int y, int w, int h);
void earc_changed(lv_event_t *e);

// ========================== DICHIARAZIONI EVENT HANDLER ==========================
static void eb(lv_event_t *e);
static void ebright(lv_event_t *e);
static void eslider(lv_event_t *e);
static void ehslider(lv_event_t *e);
static void ecat_btn(lv_event_t *e);
static void elist(lv_event_t *e);

// ========================== DEFINIZIONI FUNZIONI ==========================
// (Qui vanno tutte le implementazioni di btn, home_btn, slider, h_slider, submenu,
// meter, update_meters, stop_meters, log_show, log_hide, log_add, toast_show,
// set_bright, load_bright, save_bright, reset_arrows, wave_shape, create_home,
// create_page, grid_btn_click, populate_grid, update_wave_plot, update_leds,
// update_slider_color, update_shape_slider_color, arc_with_image, earc_changed,
// e i gestori eventi eb, ebright, eslider, ecat_btn, ehslider, elist.
// Per brevità, li ho omessi qui, ma vanno copiati dal file originale.

// ATTUALE: copia tutto il codice delle funzioni che non riguardano i preset (dropdown, rename, pending)
// dal tuo precedente lvglGraf.h, a partire da "void btn(...)" fino a "static void elist(...)",
// ma ESCLUDENDO le funzioni che gestiscono i preset (quelle che in lvglGrafB.h).

#endif