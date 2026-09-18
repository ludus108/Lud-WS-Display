// ============================================================
// lvglPreset.cpp - Preset: gestione, dropdown, rename, SD
// ============================================================
#include "globals.h"
#include "lvglGraf.h"
#include "preset_sd.h"
#include <Arduino.h>
#include <SD.h>

// ========================== STATICHE LOCALI ==========================
static lv_obj_t *rename_win        = NULL;
static lv_obj_t *rename_ta         = NULL;
static int       rename_preset_idx = 0;
static lv_obj_t *confirm_win       = NULL;

// ========================== NOMI PRESET ==========================
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
    char optsA[1024] = {0};
    char optsB[1024] = {0};
    size_t lenA = 0, lenB = 0;

    for (int i = 0; i < MAX_preset; i++) {
        if (lenA > 0) optsA[lenA++] = '\n';
        if (lenA < sizeof(optsA) - 1) {
            strcat(optsA, presetNamesA[i]);
            lenA = strlen(optsA);
        }
        if (lenB > 0) optsB[lenB++] = '\n';
        if (lenB < sizeof(optsB) - 1) {
            strcat(optsB, presetNamesB[i]);
            lenB = strlen(optsB);
        }
    }

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
      //  log_add("Preset A applicato", lv_color_hex(0x00FF00));
        update_all_targets();
    } else if (synth_id == 1 && pendingPresetB >= 0) {
        presetNumB = pendingPresetB;
        pendingPresetB = -1;
        if (preset_dropdown_B && lv_obj_is_valid(preset_dropdown_B))
            lv_obj_set_style_text_color(preset_dropdown_B, lv_color_hex(0xFFFFFF), 0);
        update_preset_labels();
        update_preset_dropdown_options();
   //     log_add("Preset B applicato", lv_color_hex(0x00FF00));
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
       // log_add("Preset A in attesa", lv_color_hex(0xFFAA00));
    } else if (dd == preset_dropdown_B) {
        pendingPresetB = sel;
        lv_obj_set_style_text_color(dd, lv_color_hex(0xFFFF00), 0);
        if (!blink_timer) blink_timer = lv_timer_create(blink_timer_cb, 500, NULL);
   //     log_add("Preset B in attesa", lv_color_hex(0xFFAA00));
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
   /*  char buf[32]; snprintf(buf, sizeof(buf), "Preset +: A=%d B=%d", presetNumA, presetNumB);
    log_add(buf, lv_color_hex(0x66AAFF)); */
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
 /*    char buf[32]; snprintf(buf, sizeof(buf), "Preset -: A=%d B=%d", presetNumA, presetNumB);
    log_add(buf, lv_color_hex(0x66AAFF)); */
}

void preset_btn_sel(lv_event_t *e) {
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    apply_pending_preset(id);
}

static void preset_btn_save(lv_event_t *e) {
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    bool ok = false;
    if (id == 0) {
        ok = saveCurrentPresetA();
        if (ok) ok = saveNamesToSD(0);
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
        lv_obj_t *b = lv_btn_create(cont);
        lv_obj_set_size(b, 90, 60);
        lv_obj_set_pos(b, 235, y_plus + i * y_step);
        lv_obj_set_style_bg_color(b, btn_colors[i], 0);
        lv_obj_set_style_radius(b, 6, 0);
        lv_obj_set_style_border_width(b, 2, 0);
        lv_obj_set_style_border_color(b, btn_borders[i], 0);
        lv_obj_t *lbl = lv_label_create(b);
        lv_label_set_text(lbl, btn_labels[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl, (i == 3) ? &lv_font_montserrat_18 : &lv_font_montserrat_32, 0);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(b, callbacks[i], LV_EVENT_CLICKED, (void*)(uintptr_t)id);
    }

    if (id == 0) preset_dropdown_A = dd;
    else preset_dropdown_B = dd;
    return dd;
}

// ========================== RINOMINA ==========================
void close_rename_window() {
    if (rename_win) { lv_obj_del(rename_win); rename_win = NULL; rename_ta = NULL; }
}

static void rename_confirm(lv_event_t *e) {
    (void)e;
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
       // log_add("Preset rinominato", lv_color_hex(0x66AAFF));
    }
    close_rename_window();
}

static void rename_cancel(lv_event_t *e) {
    (void)e;
    close_rename_window();
}

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
    (void)e;
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
    for (int i = 0; i < MAX_SLIDERS; i++) update_slider_target(i);
}

// ========================== CONFERMA INIT SD ==========================
static void confirm_yes_click(lv_event_t *e) {
    (void)e;
    if (confirm_win) { lv_obj_del(confirm_win); confirm_win = NULL; }
    lv_timer_t *timer = lv_timer_create([](lv_timer_t *t) {
        init_sd();
        log_add("SD inizializzata", lv_color_hex(0x00FF00));
        toast_show("SD inizializzata!", lv_color_hex(0x00FF00), TOAST_DUR);
        lv_timer_del(t);
    }, 100, NULL);
}

static void confirm_no_click(lv_event_t *e) {
    (void)e;
    if (confirm_win) { lv_obj_del(confirm_win); confirm_win = NULL; }
    log_add("Inizializzazione SD annullata", lv_color_hex(0xFFAA00));
}

void init_sd_btn_click(lv_event_t *e) {
    (void)e;
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
    lv_label_set_text(l_yes, "Si");
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

// ========================== SD: STORAGE PRESET ==========================
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