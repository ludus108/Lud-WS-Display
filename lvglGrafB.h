#pragma once
#ifndef LVGL_GRAF_B_H
#define LVGL_GRAF_B_H

#include "lvglGrafA.h"   // fornisce le funzioni base e le variabili globali

// ========================== VARIABILI PRESET (esterne) ==========================
extern char presetNames[MAX_preset][32];
extern lv_obj_t *preset_dropdown_A;
extern lv_obj_t *preset_dropdown_B;
extern int preset_visible_count;
extern lv_obj_t *preset_label_A;
extern lv_obj_t *preset_label_B;
extern int pendingPresetA;
extern int pendingPresetB;
extern bool blink_state;
extern lv_timer_t *blink_timer;
extern lv_obj_t *rename_win;
extern lv_obj_t *rename_ta;
extern int rename_preset_idx;

// ========================== PROTOTIPI FUNZIONI PRESET ==========================
void initPresetNames();
void update_preset_labels();
void update_preset_dropdown(lv_obj_t *dd, int *numPtr);
void apply_pending_preset(int synth_id);
void blink_timer_cb(lv_timer_t *timer);
void preset_dropdown_event(lv_event_t *e);
void preset_btn_sel(lv_event_t *e);
void preset_btn_plus(lv_event_t *e);
void preset_btn_minus(lv_event_t *e);
void update_preset_dropdown_options();
lv_obj_t* create_preset_selector(lv_obj_t *parent, int x, int y, const char *label_text, int *numPtr);
void close_rename_window();
void rename_confirm(lv_event_t *e);
void rename_cancel(lv_event_t *e);
void open_rename_window(int preset_idx);
void rename_btn_click(lv_event_t *e);

// ========================== DEFINIZIONI FUNZIONI PRESET ==========================
char presetNames[MAX_preset][32];  // definizione (se non già definita altrove)

// Inizializza i nomi dei preset
void initPresetNames() {
    for (int i = 0; i < MAX_preset; i++) {
        snprintf(presetNames[i], sizeof(presetNames[i]), "Preset %d", i + 1);
    }
}

// Aggiorna le label di stato (Pn)
void update_preset_labels() {
    char buf[64];
    if (preset_label_A && lv_obj_is_valid(preset_label_A)) {
        snprintf(buf, sizeof(buf), "P%d %s", presetNumA, presetNames[presetNumA]);
        lv_label_set_text(preset_label_A, buf);
    } else {
        preset_label_A = NULL;
    }
    if (preset_label_B && lv_obj_is_valid(preset_label_B)) {
        snprintf(buf, sizeof(buf), "P%d %s", presetNumB, presetNames[presetNumB]);
        lv_label_set_text(preset_label_B, buf);
    } else {
        preset_label_B = NULL;
    }
}

// Aggiorna un dropdown (seleziona il preset corrente)
void update_preset_dropdown(lv_obj_t *dd, int *numPtr) {
    if (!dd || !lv_obj_is_valid(dd) || !numPtr) return;
    lv_dropdown_set_selected(dd, *numPtr);
    update_preset_labels();
}

// Applica un pending preset
void apply_pending_preset(int synth_id) {
    if (synth_id == 0 && pendingPresetA >= 0) {
        presetNumA = pendingPresetA;
        pendingPresetA = -1;
        if (preset_dropdown_A && lv_obj_is_valid(preset_dropdown_A)) {
            lv_obj_set_style_text_color(preset_dropdown_A, lv_color_hex(0xFFFFFF), 0);
        }
        update_preset_labels();
        update_preset_dropdown_options();
        log_add("Preset A applicato", lv_color_hex(0x00FF00));
    } else if (synth_id == 1 && pendingPresetB >= 0) {
        presetNumB = pendingPresetB;
        pendingPresetB = -1;
        if (preset_dropdown_B && lv_obj_is_valid(preset_dropdown_B)) {
            lv_obj_set_style_text_color(preset_dropdown_B, lv_color_hex(0xFFFFFF), 0);
        }
        update_preset_labels();
        update_preset_dropdown_options();
        log_add("Preset B applicato", lv_color_hex(0x00FF00));
    }
    if (pendingPresetA < 0 && pendingPresetB < 0 && blink_timer) {
        lv_timer_del(blink_timer);
        blink_timer = NULL;
        blink_state = false;
    }
}

// Timer per il lampeggio
void blink_timer_cb(lv_timer_t *timer) {
    blink_state = !blink_state;
    if (pendingPresetA >= 0 && preset_dropdown_A && lv_obj_is_valid(preset_dropdown_A)) {
        lv_color_t color = blink_state ? lv_color_hex(0xFFFF00) : lv_color_hex(0xFFFFFF);
        lv_obj_set_style_text_color(preset_dropdown_A, color, 0);
    }
    if (pendingPresetB >= 0 && preset_dropdown_B && lv_obj_is_valid(preset_dropdown_B)) {
        lv_color_t color = blink_state ? lv_color_hex(0xFFFF00) : lv_color_hex(0xFFFFFF);
        lv_obj_set_style_text_color(preset_dropdown_B, color, 0);
    }
    if (pendingPresetA < 0 && pendingPresetB < 0 && blink_timer) {
        lv_timer_del(blink_timer);
        blink_timer = NULL;
        blink_state = false;
    }
}

// Evento selezione dal dropdown (mette in pending)
void preset_dropdown_event(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    if (!dd || !lv_obj_is_valid(dd)) return;
    int selected = lv_dropdown_get_selected(dd);
    if (selected < 0 || selected >= MAX_preset) return;

    if (dd == preset_dropdown_A && lv_obj_is_valid(preset_dropdown_A)) {
        pendingPresetA = selected;
        lv_obj_set_style_text_color(dd, lv_color_hex(0xFFFF00), 0);
        if (!blink_timer) {
            blink_timer = lv_timer_create(blink_timer_cb, 500, NULL);
        }
        log_add("Preset A in attesa", lv_color_hex(0xFFAA00));
    } else if (dd == preset_dropdown_B && lv_obj_is_valid(preset_dropdown_B)) {
        pendingPresetB = selected;
        lv_obj_set_style_text_color(dd, lv_color_hex(0xFFFF00), 0);
        if (!blink_timer) {
            blink_timer = lv_timer_create(blink_timer_cb, 500, NULL);
        }
        log_add("Preset B in attesa", lv_color_hex(0xFFAA00));
    }
}

// Pulsante "Sel": applica il pending
void preset_btn_sel(lv_event_t *e) {
    int synth_id = (int)(uintptr_t)lv_event_get_user_data(e);
    apply_pending_preset(synth_id);
}

// Pulsante "+": incrementa il pending o il valore corrente
void preset_btn_plus(lv_event_t *e) {
    int synth_id = (int)(uintptr_t)lv_event_get_user_data(e);
    if (synth_id == 0) {
        int newVal = (pendingPresetA >= 0) ? pendingPresetA : presetNumA;
        newVal++;
        if (newVal >= MAX_preset) newVal = 0;
        pendingPresetA = newVal;
        if (preset_dropdown_A && lv_obj_is_valid(preset_dropdown_A)) {
            lv_dropdown_set_selected(preset_dropdown_A, pendingPresetA);
            lv_obj_set_style_text_color(preset_dropdown_A, lv_color_hex(0xFFFF00), 0);
            lv_dropdown_open(preset_dropdown_A);
        }
        if (!blink_timer) {
            blink_timer = lv_timer_create(blink_timer_cb, 500, NULL);
        }
    } else {
        int newVal = (pendingPresetB >= 0) ? pendingPresetB : presetNumB;
        newVal++;
        if (newVal >= MAX_preset) newVal = 0;
        pendingPresetB = newVal;
        if (preset_dropdown_B && lv_obj_is_valid(preset_dropdown_B)) {
            lv_dropdown_set_selected(preset_dropdown_B, pendingPresetB);
            lv_obj_set_style_text_color(preset_dropdown_B, lv_color_hex(0xFFFF00), 0);
            lv_dropdown_open(preset_dropdown_B);
        }
        if (!blink_timer) {
            blink_timer = lv_timer_create(blink_timer_cb, 500, NULL);
        }
    }
    update_preset_labels();
    char buf[32];
    snprintf(buf, sizeof(buf), "Preset +: A=%d B=%d", presetNumA, presetNumB);
    log_add(buf, lv_color_hex(0x66AAFF));
}

// Pulsante "-": decrementa il pending o il valore corrente
void preset_btn_minus(lv_event_t *e) {
    int synth_id = (int)(uintptr_t)lv_event_get_user_data(e);
    if (synth_id == 0) {
        int newVal = (pendingPresetA >= 0) ? pendingPresetA : presetNumA;
        newVal--;
        if (newVal < 0) newVal = MAX_preset - 1;
        pendingPresetA = newVal;
        if (preset_dropdown_A && lv_obj_is_valid(preset_dropdown_A)) {
            lv_dropdown_set_selected(preset_dropdown_A, pendingPresetA);
            lv_obj_set_style_text_color(preset_dropdown_A, lv_color_hex(0xFFFF00), 0);
            lv_dropdown_open(preset_dropdown_A);
        }
        if (!blink_timer) {
            blink_timer = lv_timer_create(blink_timer_cb, 500, NULL);
        }
    } else {
        int newVal = (pendingPresetB >= 0) ? pendingPresetB : presetNumB;
        newVal--;
        if (newVal < 0) newVal = MAX_preset - 1;
        pendingPresetB = newVal;
        if (preset_dropdown_B && lv_obj_is_valid(preset_dropdown_B)) {
            lv_dropdown_set_selected(preset_dropdown_B, pendingPresetB);
            lv_obj_set_style_text_color(preset_dropdown_B, lv_color_hex(0xFFFF00), 0);
            lv_dropdown_open(preset_dropdown_B);
        }
        if (!blink_timer) {
            blink_timer = lv_timer_create(blink_timer_cb, 500, NULL);
        }
    }
    update_preset_labels();
    char buf[32];
    snprintf(buf, sizeof(buf), "Preset -: A=%d B=%d", presetNumA, presetNumB);
    log_add(buf, lv_color_hex(0x66AAFF));
}

// Aggiorna le opzioni di entrambi i dropdown (dopo rename o applicazione)
void update_preset_dropdown_options() {
    char options[512] = "";
    for (int i = 0; i < MAX_preset; i++) {
        strcat(options, presetNames[i]);
        if (i < MAX_preset - 1) strcat(options, "\n");
    }
    if (preset_dropdown_A && lv_obj_is_valid(preset_dropdown_A)) {
        lv_dropdown_set_options(preset_dropdown_A, options);
        lv_dropdown_set_selected(preset_dropdown_A, presetNumA);
        if (pendingPresetA >= 0) {
            lv_dropdown_set_selected(preset_dropdown_A, pendingPresetA);
        }
    } else {
        preset_dropdown_A = NULL;
    }
    if (preset_dropdown_B && lv_obj_is_valid(preset_dropdown_B)) {
        lv_dropdown_set_options(preset_dropdown_B, options);
        lv_dropdown_set_selected(preset_dropdown_B, presetNumB);
        if (pendingPresetB >= 0) {
            lv_dropdown_set_selected(preset_dropdown_B, pendingPresetB);
        }
    } else {
        preset_dropdown_B = NULL;
    }
    update_preset_labels();
}

// Crea il selettore di preset (dropdown + pulsanti + Sel)
lv_obj_t* create_preset_selector(lv_obj_t *parent, int x, int y, const char *label_text, int *numPtr) {
    lv_obj_t *container = lv_obj_create(parent);
    lv_obj_set_size(container, 340, 270);
    lv_obj_set_pos(container, x - 20, y);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(container);
    lv_label_set_text(label, label_text);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(label, 0, 0);

    lv_obj_t *dd = lv_dropdown_create(container);
    lv_obj_set_size(dd, 220, 50);
    lv_obj_set_pos(dd, 0, 25);
    lv_obj_set_style_bg_color(dd, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(dd, 2, 0);
    lv_obj_set_style_border_color(dd, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_color(dd, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(dd, &lv_font_montserrat_16, 0);
    lv_obj_set_style_pad_left(dd, 10, 0);
    lv_obj_set_style_pad_right(dd, 10, 0);

    char options[512] = "";
    for (int i = 0; i < MAX_preset; i++) {
        strcat(options, presetNames[i]);
        if (i < MAX_preset - 1) strcat(options, "\n");
    }
    lv_dropdown_set_options(dd, options);
    lv_dropdown_set_selected(dd, *numPtr);

    lv_obj_t *list = lv_dropdown_get_list(dd);
    lv_obj_set_height(list, preset_visible_count * 30);
    lv_obj_set_style_bg_color(list, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_text_color(list, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(list, &lv_font_montserrat_16, 0);
    lv_obj_set_style_pad_all(list, 5, 0);

    lv_obj_add_event_cb(dd, preset_dropdown_event, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *btn_plus = lv_btn_create(container);
    lv_obj_set_size(btn_plus, 70, 70);
    lv_obj_set_pos(btn_plus, 235, 25);
    lv_obj_set_style_bg_color(btn_plus, lv_color_hex(0x1A6B4A), 0);
    lv_obj_set_style_radius(btn_plus, 8, 0);
    lv_obj_set_style_border_width(btn_plus, 2, 0);
    lv_obj_set_style_border_color(btn_plus, lv_color_hex(0x00FF88), 0);
    lv_obj_t *lbl_plus = lv_label_create(btn_plus);
    lv_label_set_text(lbl_plus, "+");
    lv_obj_set_style_text_color(lbl_plus, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_plus, &lv_font_montserrat_32, 0);
    lv_obj_center(lbl_plus);
    lv_obj_add_event_cb(btn_plus, preset_btn_plus, LV_EVENT_CLICKED, (void*)(uintptr_t)(label_text[7] == 'A' ? 0 : 1));

    lv_obj_t *btn_minus = lv_btn_create(container);
    lv_obj_set_size(btn_minus, 70, 70);
    lv_obj_set_pos(btn_minus, 235, 105);
    lv_obj_set_style_bg_color(btn_minus, lv_color_hex(0x6B1A1A), 0);
    lv_obj_set_style_radius(btn_minus, 8, 0);
    lv_obj_set_style_border_width(btn_minus, 2, 0);
    lv_obj_set_style_border_color(btn_minus, lv_color_hex(0xFF4444), 0);
    lv_obj_t *lbl_minus = lv_label_create(btn_minus);
    lv_label_set_text(lbl_minus, "-");
    lv_obj_set_style_text_color(lbl_minus, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_minus, &lv_font_montserrat_32, 0);
    lv_obj_center(lbl_minus);
    lv_obj_add_event_cb(btn_minus, preset_btn_minus, LV_EVENT_CLICKED, (void*)(uintptr_t)(label_text[7] == 'A' ? 0 : 1));

    // Pulsante Sel
    lv_obj_t *btn_sel = lv_btn_create(container);
    lv_obj_set_size(btn_sel, 100, 40);
    lv_obj_set_pos(btn_sel, 120, 185);
    lv_obj_set_style_bg_color(btn_sel, lv_color_hex(0x0055AA), 0);
    lv_obj_set_style_radius(btn_sel, 8, 0);
    lv_obj_set_style_border_width(btn_sel, 2, 0);
    lv_obj_set_style_border_color(btn_sel, lv_color_hex(0x88CCFF), 0);
    lv_obj_t *lbl_sel = lv_label_create(btn_sel);
    lv_label_set_text(lbl_sel, "Sel");
    lv_obj_set_style_text_color(lbl_sel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_sel, &lv_font_montserrat_20, 0);
    lv_obj_center(lbl_sel);
    lv_obj_add_event_cb(btn_sel, preset_btn_sel, LV_EVENT_CLICKED, (void*)(uintptr_t)(label_text[7] == 'A' ? 0 : 1));

    if (label_text[7] == 'A') {
        preset_dropdown_A = dd;
    } else {
        preset_dropdown_B = dd;
    }

    return dd;
}

// ---- RINOMINA PRESET ----
void close_rename_window() {
    if (rename_win) {
        lv_obj_del(rename_win);
        rename_win = NULL;
        rename_ta = NULL;
    }
}

void rename_confirm(lv_event_t *e) {
    const char *new_name = lv_textarea_get_text(rename_ta);
    if (strlen(new_name) > 0) {
        strncpy(presetNames[rename_preset_idx], new_name, 31);
        presetNames[rename_preset_idx][31] = '\0';
        update_preset_dropdown_options();
        log_add("Preset rinominato", lv_color_hex(0x66AAFF));
    }
    close_rename_window();
}

void rename_cancel(lv_event_t *e) {
    close_rename_window();
}

void open_rename_window(int preset_idx) {
    if (rename_win) return;
    rename_preset_idx = preset_idx;

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
    lv_textarea_set_text(rename_ta, presetNames[preset_idx]);
    lv_textarea_set_max_length(rename_ta, 31);

    lv_obj_t *kb = lv_keyboard_create(client);
    lv_obj_set_size(kb, 550, 220);
    lv_obj_set_style_text_font(kb, &lv_font_montserrat_18, 0);
    lv_keyboard_set_textarea(kb, rename_ta);

    lv_obj_set_style_bg_color(kb, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_color(kb, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_color(kb, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_color(kb, lv_color_hex(0x444444), LV_STATE_PRESSED);
    lv_obj_set_style_text_color(kb, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    lv_obj_t *btn_container = lv_obj_create(client);
    lv_obj_set_size(btn_container, 460, 50);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_container, 0, 0);
    lv_obj_clear_flag(btn_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(btn_container, 20, 0);

    lv_obj_t *btn_ok = lv_btn_create(btn_container);
    lv_obj_set_size(btn_ok, 120, 40);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0x1A6B4A), 0);
    lv_obj_t *lbl_ok = lv_label_create(btn_ok);
    lv_label_set_text(lbl_ok, "OK");
    lv_obj_set_style_text_color(lbl_ok, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl_ok);
    lv_obj_add_event_cb(btn_ok, rename_confirm, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_cancel = lv_btn_create(btn_container);
    lv_obj_set_size(btn_cancel, 120, 40);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x6B1A1A), 0);
    lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(lbl_cancel, "Annulla");
    lv_obj_set_style_text_color(lbl_cancel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl_cancel);
    lv_obj_add_event_cb(btn_cancel, rename_cancel, LV_EVENT_CLICKED, NULL);
}

void rename_btn_click(lv_event_t *e) {
    if (g.synth && strcmp(g.synth, "SYNTH A") == 0) {
        open_rename_window(presetNumA);
    } else if (g.synth && strcmp(g.synth, "SYNTH B") == 0) {
        open_rename_window(presetNumB);
    }
}

#endif