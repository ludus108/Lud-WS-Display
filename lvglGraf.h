#ifndef LVGL_GRAF_H
#define LVGL_GRAF_H

#include "globals.h"

// ========================== PROTOTIPI FUNZIONI ESPORTATE ==========================
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
void populate_grid(lv_obj_t *container, const char *items, int selected_idx, int synth_id);
void update_wave_plot(lv_obj_t *chart, lv_chart_series_t *serie, uint8_t shape_val, int wave_index);
void update_leds(lv_obj_t **leds, int active_cat);
void update_slider_color(int idx, bool active);
void update_shape_slider_color(int synth_id, bool active);
void arc_with_image(lv_obj_t *parent, int x, int y, int w, int h);
void grid_btn_click(lv_event_t *e);
void earc_changed(lv_event_t *e);
void initPresetNamesA();
void initPresetNamesB();
void update_preset_labels();
void update_preset_dropdown(lv_obj_t *dd, int *numPtr);
void apply_pending_preset(int synth_id);
void update_preset_dropdown_options();
lv_obj_t* create_preset_selector(lv_obj_t *parent, int x, int y, const char *label_text, int *numPtr, char (*names)[32]);
void close_rename_window();
void open_rename_window(int preset_idx);
void update_slider_parameter(int idx, int value);
void update_slider_target(int idx);
void update_all_targets();
void init_sd();

#endif