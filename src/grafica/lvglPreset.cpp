// ============================================================
// lvglPreset.cpp - Preset: dropdown, +/-, Sel, Save, rename
// ============================================================
// La logica SD e trasferimento vive in src/preset/.
// Questo file gestisce SOLO l'interazione utente.
// ============================================================

#include "globals.h"
#include "lvglGraf.h"
#include <Arduino.h>
#include <SD.h>


// ========================== STATICHE LOCALI ==========================
static lv_obj_t *rename_win        = NULL;
static lv_obj_t *rename_ta         = NULL;
static int       rename_preset_idx = 0;
static lv_obj_t *confirm_win       = NULL;

// ========================== DROPDOWN E LABEL ==========================
void update_preset_labels() {
    char buf[64];
    if (preset_label_A && lv_obj_is_valid(preset_label_A)) {
        snprintf(buf, sizeof(buf), "Pn%d %s",
                 presetNumA + 1, nome_presetA[presetNumA]);
        lv_label_set_text(preset_label_A, buf);
    }
    if (preset_label_B && lv_obj_is_valid(preset_label_B)) {
        snprintf(buf, sizeof(buf), "Pn%d %s",
                 presetNumB + 1, nome_presetB[presetNumB]);
        lv_label_set_text(preset_label_B, buf);
    }
}

void update_preset_dropdown(lv_obj_t *dd, int *numPtr) {
    if (!dd || !lv_obj_is_valid(dd) || !numPtr) return;
    lv_dropdown_set_selected(dd, *numPtr);
    update_preset_labels();
}

void update_preset_dropdown_options() {
    // Costruisci le stringhe di opzioni (una riga per preset)
    static char optsA[1024];
    static char optsB[1024];
    optsA[0] = '\0';
    optsB[0] = '\0';

    for (int i = 0; i < MAX_PRESET; i++) {
        if (i > 0) {
            strncat(optsA, "\n", sizeof(optsA) - strlen(optsA) - 1);
            strncat(optsB, "\n", sizeof(optsB) - strlen(optsB) - 1);
        }
        strncat(optsA, nome_presetA[i], sizeof(optsA) - strlen(optsA) - 1);
        strncat(optsB, nome_presetB[i], sizeof(optsB) - strlen(optsB) - 1);
    }

    if (preset_dropdown_A && lv_obj_is_valid(preset_dropdown_A)) {
        lv_dropdown_set_options(preset_dropdown_A, optsA);
        lv_dropdown_set_selected(preset_dropdown_A,
            (pendingPresetA >= 0) ? pendingPresetA : presetNumA);
    }
    if (preset_dropdown_B && lv_obj_is_valid(preset_dropdown_B)) {
        lv_dropdown_set_options(preset_dropdown_B, optsB);
        lv_dropdown_set_selected(preset_dropdown_B,
            (pendingPresetB >= 0) ? pendingPresetB : presetNumB);
    }
    update_preset_labels();
}

// ========================== APPLICA PRESET ==========================
void apply_pending_preset(int synth_id) {
    if (synth_id == 0 && pendingPresetA >= 0) {
        presetNumA = pendingPresetA;
        pendingPresetA = -1;
        if (preset_dropdown_A && lv_obj_is_valid(preset_dropdown_A))
            lv_obj_set_style_text_color(preset_dropdown_A,
                                        lv_color_hex(0xFFFFFF), 0);

        // Chiama il wrapper (definito nel .ino)
        requestPresetLoad(0, presetNumA);

        update_preset_labels();
        update_preset_dropdown_options();
        update_all_targets();
    } else if (synth_id == 1 && pendingPresetB >= 0) {
        presetNumB = pendingPresetB;
        pendingPresetB = -1;
        if (preset_dropdown_B && lv_obj_is_valid(preset_dropdown_B))
            lv_obj_set_style_text_color(preset_dropdown_B,
                                        lv_color_hex(0xFFFFFF), 0);

        requestPresetLoad(1, presetNumB);

        update_preset_labels();
        update_preset_dropdown_options();
        update_all_targets();
    }
    if (pendingPresetA < 0 && pendingPresetB < 0 && blink_timer) {
        lv_timer_del(blink_timer);
        blink_timer = NULL;
        blink_state = false;
    }
}

// ========================== BLINK ==========================
void blink_timer_cb(lv_timer_t *timer) {
    blink_state = !blink_state;
    lv_color_t c = blink_state ? lv_color_hex(0xFFFF00) : lv_color_hex(0xFFFFFF);
    if (pendingPresetA >= 0 && preset_dropdown_A &&
        lv_obj_is_valid(preset_dropdown_A))
        lv_obj_set_style_text_color(preset_dropdown_A, c, 0);
    if (pendingPresetB >= 0 && preset_dropdown_B &&
        lv_obj_is_valid(preset_dropdown_B))
        lv_obj_set_style_text_color(preset_dropdown_B, c, 0);
    if (pendingPresetA < 0 && pendingPresetB < 0 && blink_timer) {
        lv_timer_del(blink_timer);
        blink_timer = NULL;
        blink_state = false;
    }
}

// ========================== EVENTI DROPDOWN / BOTTONI ==========================
void preset_dropdown_event(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    if (!dd || !lv_obj_is_valid(dd)) return;
    int sel = lv_dropdown_get_selected(dd);
    if (sel < 0 || sel >= MAX_PRESET) return;

    if (dd == preset_dropdown_A) {
        pendingPresetA = sel;
        lv_obj_set_style_text_color(dd, lv_color_hex(0xFFFF00), 0);
        if (!blink_timer) blink_timer = lv_timer_create(blink_timer_cb, 500, NULL);
    } else if (dd == preset_dropdown_B) {
        pendingPresetB = sel;
        lv_obj_set_style_text_color(dd, lv_color_hex(0xFFFF00), 0);
        if (!blink_timer) blink_timer = lv_timer_create(blink_timer_cb, 500, NULL);
    }
}

void preset_btn_plus(lv_event_t *e) {
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    if (id == 0) {
        int val = (pendingPresetA >= 0) ? pendingPresetA : presetNumA;
        pendingPresetA = (++val >= MAX_PRESET) ? 0 : val;
        if (preset_dropdown_A && lv_obj_is_valid(preset_dropdown_A)) {
            lv_dropdown_set_selected(preset_dropdown_A, pendingPresetA);
            lv_obj_set_style_text_color(preset_dropdown_A, lv_color_hex(0xFFFF00), 0);
        }
    } else {
        int val = (pendingPresetB >= 0) ? pendingPresetB : presetNumB;
        pendingPresetB = (++val >= MAX_PRESET) ? 0 : val;
        if (preset_dropdown_B && lv_obj_is_valid(preset_dropdown_B)) {
            lv_dropdown_set_selected(preset_dropdown_B, pendingPresetB);
            lv_obj_set_style_text_color(preset_dropdown_B, lv_color_hex(0xFFFF00), 0);
        }
    }
    if (!blink_timer) blink_timer = lv_timer_create(blink_timer_cb, 500, NULL);
    update_preset_labels();
}

void preset_btn_minus(lv_event_t *e) {
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    if (id == 0) {
        int val = (pendingPresetA >= 0) ? pendingPresetA : presetNumA;
        pendingPresetA = (--val < 0) ? MAX_PRESET - 1 : val;
        if (preset_dropdown_A && lv_obj_is_valid(preset_dropdown_A)) {
            lv_dropdown_set_selected(preset_dropdown_A, pendingPresetA);
            lv_obj_set_style_text_color(preset_dropdown_A, lv_color_hex(0xFFFF00), 0);
        }
    } else {
        int val = (pendingPresetB >= 0) ? pendingPresetB : presetNumB;
        pendingPresetB = (--val < 0) ? MAX_PRESET - 1 : val;
        if (preset_dropdown_B && lv_obj_is_valid(preset_dropdown_B)) {
            lv_dropdown_set_selected(preset_dropdown_B, pendingPresetB);
            lv_obj_set_style_text_color(preset_dropdown_B, lv_color_hex(0xFFFF00), 0);
        }
    }
    if (!blink_timer) blink_timer = lv_timer_create(blink_timer_cb, 500, NULL);
    update_preset_labels();
}

void preset_btn_sel(lv_event_t *e) {
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    apply_pending_preset(id);
}

static void preset_btn_save(lv_event_t *e) {
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    int pid = (id == 0) ? presetNumA : presetNumB;
    bool ok = requestPresetSave(id, pid);
    if (ok) {
        log_add(id == 0 ? "Preset A salvato" : "Preset B salvato",
                lv_color_hex(0x00FF00));
        toast_show(id == 0 ? "Preset A salvato!" : "Preset B salvato!",
                   lv_color_hex(0x00FF00), TOAST_DUR);
    } else {
        log_add("Errore salvataggio", lv_color_hex(0xFF0000));
        toast_show("Errore salvataggio!", lv_color_hex(0xFF0000), TOAST_DUR);
    }
}

// ========================== SELECTOR ==========================
lv_obj_t* create_preset_selector(lv_obj_t *parent, int x, int y,
                                 const char *label_text, int *numPtr,
                                 char (*names)[PRESET_NAME_LEN]) {
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

    static char opts[1024];
    opts[0] = '\0';
    for (int i = 0; i < MAX_PRESET; i++) {
        if (i > 0) strncat(opts, "\n", sizeof(opts) - strlen(opts) - 1);
        strncat(opts, names[i], sizeof(opts) - strlen(opts) - 1);
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

    const char *btn_labels[]  = {"+", "-", "Sel", "Save"};
    lv_color_t btn_colors[]   = {lv_color_hex(0x1A6B4A), lv_color_hex(0x6B1A1A),
                                 lv_color_hex(0x0055AA), lv_color_hex(0x885500)};
    lv_color_t btn_borders[]  = {lv_color_hex(0x00FF88), lv_color_hex(0xFF4444),
                                 lv_color_hex(0x88CCFF), lv_color_hex(0xFFAA44)};
    lv_event_cb_t callbacks[] = {preset_btn_plus, preset_btn_minus,
                                 preset_btn_sel,  preset_btn_save};

    int y_plus = 25;
    int y_step = 60 + 6;

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
        lv_obj_set_style_text_font(lbl,
            (i == 3) ? &lv_font_montserrat_18 : &lv_font_montserrat_32, 0);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(b, callbacks[i], LV_EVENT_CLICKED,
                            (void*)(uintptr_t)id);
    }

    if (id == 0) preset_dropdown_A = dd;
    else         preset_dropdown_B = dd;
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
        int synth = 0;
        if (g.synth && strcmp(g.synth, "SYNTH A") == 0)      synth = 0;
        else if (g.synth && strcmp(g.synth, "SYNTH B") == 0) synth = 1;

        // Wrapper nel .ino
        requestRenameApply(synth, rename_preset_idx, name);

        update_preset_dropdown_options();
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
    lv_obj_set_flex_align(client, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(client, 10, 0);

    rename_ta = lv_textarea_create(client);
    lv_obj_set_size(rename_ta, 460, 45);
    lv_obj_set_style_bg_color(rename_ta, lv_color_hex(0x222222), 0);
    lv_obj_set_style_text_color(rename_ta, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(rename_ta, 1, 0);
    lv_obj_set_style_border_color(rename_ta, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(rename_ta, &lv_font_montserrat_18, 0);

    if (g.synth && strcmp(g.synth, "SYNTH A") == 0) {
        lv_textarea_set_text(rename_ta, nome_presetA[idx]);
        lv_textarea_set_max_length(rename_ta, PRESET_NAME_LEN - 1);
    } else {
        lv_textarea_set_text(rename_ta, nome_presetB[idx]);
        lv_textarea_set_max_length(rename_ta, PRESET_NAME_LEN - 1);
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
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
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
    if (g.synth && strcmp(g.synth, "SYNTH A") == 0)      open_rename_window(presetNumA);
    else if (g.synth && strcmp(g.synth, "SYNTH B") == 0) open_rename_window(presetNumB);
}

// ========================== SLIDER TARGET ==========================
// Nel nuovo schema, lo slider della UI scrive direttamente nella cache
// tramite uiSetParam* (definiti in src/preset/preset_ui.h).
// Questa funzione mantiene solo il comportamento grafico locale.
// La chiamata effettiva verso il synth avviene in eslider() (lvglGrafFunc.cpp).
void update_slider_parameter(int idx, int value) {
    // Aggiorna solo la "posizione target" della UI.
    // L'invio al synth è responsabilità del chiamante (eslider()).
    g.pre[idx] = constrain(value, 0, 255);

    // Nessuna scrittura in array di preset: la cache dei valori
    // sta in cacheA[voice] / cacheB e viene aggiornata da uiSetParam*.
}

void update_slider_target(int idx) {
    if (idx < 0 || idx > 7) return;
    // Nel nuovo schema non c'è lettura da array timbrA/B.
    // La posizione target è già in g.pre[idx].
    int val = g.pre[idx];
    if (arr[idx]) {
        int x = slider_base_x[idx];
        int y = slider_base_y[idx];
        int yf = map(val, 0, 255, (y + 242) - 13, y + 30);
        lv_obj_set_pos(arr[idx], x - 18, yf);
        crs[idx] = false;
        lv_obj_clear_flag(arr[idx], LV_OBJ_FLAG_HIDDEN);
        if (slider_objs[idx] && lv_obj_is_valid(slider_objs[idx]))
            update_slider_color(idx, true);
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
        init_sd();       // da src/preset/preset_sd.cpp
        presetCacheClearAll();   // da src/preset/preset_cache.cpp
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
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
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
// ========================== EEPROM SETTINGS ==========================
// Layout EEPROM (5 byte):
//   [0] bright          0..100
//   [1] midi_channel_A  1..16
//   [2] midi_channel_B  1..16
//   [3] midi_channel_D  1..16
//   [4] midi_split      0..31
// ==========================

void load_all_settings() {
    // Luminosità
    uint8_t v = EEPROM.read(0);
    g.bright = (v <= 100) ? v : 50;      // default 50 se EEPROM vergine
    set_bright(g.bright);

    // Canali MIDI (0xFF se EEPROM vergine -> fuori range -> default)
    uint8_t a = EEPROM.read(1);
    uint8_t b = EEPROM.read(2);
    uint8_t d = EEPROM.read(3);
    uint8_t s = EEPROM.read(4);

    midi_channel_A = (a >= 1 && a <= 16) ? a : 1;
    midi_channel_B = (b >= 1 && b <= 16) ? b : 2;
    midi_channel_D = (d >= 1 && d <= 16) ? d : 3;
    midi_split     = (s <= 31)         ? s : 14;
}

void save_all_settings() {
    EEPROM.write(0, g.bright);
    EEPROM.write(1, (uint8_t)midi_channel_A);
    EEPROM.write(2, (uint8_t)midi_channel_B);
    EEPROM.write(3, (uint8_t)midi_channel_D);
    EEPROM.write(4, (uint8_t)midi_split);
    EEPROM.commit();

    log_add("SetUp salvato in EEPROM", lv_color_hex(0x00FF00));
}