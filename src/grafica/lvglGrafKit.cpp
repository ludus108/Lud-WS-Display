// ============================================================
// lvglGrafKit.cpp — KIT editor (visualizzazione set per voce)
// ============================================================
#include "globals.h"
#include "lvglGraf.h"
#include "lvglGraf_internal.h"
#include "lvgl_v8_port.h"
#include "src/preset/preset_ui.h"
#include <math.h>

// ============================================================
// Stato KIT
// ============================================================
lv_obj_t *kit_preset_dd = nullptr;
lv_obj_t *kit_rev_dd    = nullptr;
lv_obj_t *kit_voice_dd[9] = {nullptr,nullptr,nullptr,nullptr,nullptr,
                              nullptr,nullptr,nullptr,nullptr};
uint8_t   kit_cur_kit = 0;   // 0..10

#define KIT_VOICE_COUNT 9
// ---- Nomi dei 16 kit (editabili) ----
char kit_names[16][KIT_NAME_LEN] = {
    "Lud1",    // 1
    "Lud2",    // 2
    "Lud3",    // 3
    "808",     // 4
    "909",     // 5
    "Linn",    // 6
    "miniPop", // 7
    "TR76",    // 8
    "CR77",    // 9
    "CR78",    // 10
    "Hammond", // 11
    "MPC60",   // 12
    "DMX",     // 13
    "SP1200",  // 14
    "RX5",     // 15
    "User"     // 16
};
// ---- Nomi delle voci (label sopra i dropdown) ----
static const char *kit_voice_names[KIT_VOICE_COUNT] = {
    "BD", "SD", "HH", "OH", "H2", "CLAP", "PERC1", "PERC2", "PERC3"
};

// ---- Numero di set disponibili per voce ----
static const int kit_voice_set_count[KIT_VOICE_COUNT] = {
    12, 12, 14, 11, 14, 5, 12, 13, 13
};

// ---- Nomi dei set (estratti da lista.h, senza suffisso "arr") ----
static const char* kit_bd_names[12] = {
    "LUDBD","BDBAN","BDMOS1","BDMOS2","BDLINN","BD808",
    "BD808L","CR78BD","CR77BD","TR76BD","MPOPBD","HAMBD"
};
static const char* kit_sd_names[12] = {
    "LUDSD","SDKRI","SD808","SD808B","SDLINN","MPOPSD",
    "TR76SD","CR78SD","SIMMSD","HAMSDA","HAMSDB","SIMMRIM"
};
static const char* kit_hh_names[14] = {
    "LUDHH","HHLINN","LINNHHB","CH808","MPOPHH","HAMHH","TR76HH",
    "CR77HH","CR78HH","MAR808","CABLINN","CYM808A","CYM808B","RIDLINN"
};
static const char* kit_oh_names[11] = {
    "LUDOH","OHKRI","OH808","OH808B","OH909A","OHLINN",
    "OHBLINN","MPOPHO","HAMHOB","TR76HO","CR78HO"
};
static const char* kit_h2_names[14] = {
    "LUDHH","HHLINN","LINNHHB","CH808","MPOPHH","HAMHH","TR76HH",
    "CR77HH","CR78HH","MAR808","CABLINN","CYM808A","CYM808B","RIDLINN"
};
static const char* kit_clap_names[5] = {
    "KANO","KANOBR","KANOBL","CLAPLINN","CLAP808"
};
static const char* kit_perc1_names[12] = {
    "LC808A","LC808B","LC808C","LC808D","LT808A","LT808B",
    "TOMLINN","SIMMLT","CONGLLINN","MPOMXL","TR76PER1C","CR78RIM"
};
static const char* kit_perc2_names[13] = {
    "MC808A","MC808B","MC808C","MT808A","MT808B","MT808C","SIMMMT",
    "CONGMLINN","BNGLINN","MPOPCON","TR76PER2C","CR78BLO1","CR78GUI"
};
static const char* kit_perc3_names[13] = {
    "HCA808","HCB808","HCC808","HCD808","HCE808","HT808A","HT808B",
    "SIMMHT","CONGHLINN","MPOPCL","TR76PER3C","CR78BLO2","CR78COW"
};

static const char** kit_set_names[KIT_VOICE_COUNT] = {
    kit_bd_names, kit_sd_names, kit_hh_names, kit_oh_names, kit_h2_names,
    kit_clap_names, kit_perc1_names, kit_perc2_names, kit_perc3_names
};

// ---- Copia di kitArr[10][16] da lista.h ----
//   righe 0..8 = voci BD..PERC3, riga 9 = REV
//   colonne 0..10 = kit 1..11 (11 kit), colonne 11..15 = riservate
static const int kitArr[10][16] = {
    { 1, 1,  2, 5, 2, 4, 10, 9, 8, 7, 11, 0, 0, 0, 0, 0 }, // BD
    { 0, 1,  2, 5, 3, 4, 10, 9, 8, 7,  4, 0, 0, 0, 0, 0 }, // SD
    { 0, 1,  2, 5, 3, 4,  6, 0, 8, 7,  4, 0, 0, 0, 0, 0 }, // HH
    { 0, 1,  2, 5, 3, 6, 10, 9, 8, 7,  4, 0, 0, 0, 0, 0 }, // OH
    { 1, 2,  5, 3, 6, 2,  1, 8, 7, 4,  0, 0, 0, 0, 0, 0 }, // HH2
    { 1, 2,  0, 3, 4, 0,  1, 2, 3, 4,  0, 0, 0, 0, 0, 0 }, // CLAP
    { 1, 2,  0, 3, 4, 0,  1, 2, 3, 4,  0, 0, 0, 0, 0, 0 }, // PERC1
    { 2, 0,  3, 4, 0, 1,  2, 3, 4, 1,  0, 0, 0, 0, 0, 0 }, // PERC2
    { 0, 3,  4, 0, 1, 2,  3, 4, 1, 2,  0, 0, 0, 0, 0, 0 }, // PERC3
    { 0, 3,  4, 0, 1, 2,  3, 4, 1, 2,  0, 0, 0, 0, 0, 0 }  // Rev
};

// ---- Copia di maxArr[9] da lista.h (indici massimi 0-based per voce) ----
static const int maxArr[9] = { 11, 11, 13, 10, 13, 4, 11, 12, 12 };
// ============================================================
// Rinomina KIT (window modale)
// ============================================================
static lv_obj_t *kit_rename_win = NULL;
static lv_obj_t *kit_rename_ta  = NULL;
static int       kit_rename_idx = 0;

void kit_close_rename_window() {
    if (kit_rename_win) {
        lv_obj_del(kit_rename_win);
        kit_rename_win = NULL;
        kit_rename_ta  = NULL;
    }
}

static void kit_refresh_dd_options() {
    if (!kit_preset_dd || !lv_obj_is_valid(kit_preset_dd)) return;
    static char kit_opts[512];
    kit_opts[0] = '\0';
    for (int i = 0; i < 16; i++) {
        if (i > 0) strncat(kit_opts, "\n",
                          sizeof(kit_opts) - strlen(kit_opts) - 1);
        char line[40];
       snprintf(line, sizeof(line), "%d %.20s", i + 1, kit_names[i]);
        strncat(kit_opts, line,
                sizeof(kit_opts) - strlen(kit_opts) - 1);
    }
    int sel = lv_dropdown_get_selected(kit_preset_dd);
    lv_dropdown_set_options(kit_preset_dd, kit_opts);
    lv_dropdown_set_selected(kit_preset_dd, sel);
    lv_dropdown_set_symbol(kit_preset_dd, NULL);
}

static void kit_rename_confirm(lv_event_t *e) {
    (void)e;
    if (!kit_rename_ta) return;
    const char *name = lv_textarea_get_text(kit_rename_ta);
    if (strlen(name) > 0) {
        strncpy(kit_names[kit_rename_idx], name, KIT_NAME_LEN - 1);
        kit_names[kit_rename_idx][KIT_NAME_LEN - 1] = '\0';
        kit_refresh_dd_options();
    }
    kit_close_rename_window();
}

static void kit_rename_cancel(lv_event_t *e) {
    (void)e;
    kit_close_rename_window();
}

static void kit_rename_btn_cb(lv_event_t *e) {
    (void)e;
    if (kit_rename_win) return;
    kit_rename_idx = kit_cur_kit;

    kit_rename_win = lv_win_create(lv_scr_act(), 0);
    lv_obj_set_size(kit_rename_win, 580, 360);
    lv_obj_set_pos(kit_rename_win, 110, 15);
    lv_obj_set_style_bg_color(kit_rename_win, lv_color_hex(0x111111), 0);
    lv_obj_set_style_border_color(kit_rename_win,
                                  lv_color_hex(0xFF8800), 0);
    lv_obj_set_style_border_width(kit_rename_win, 2, 0);
    lv_obj_set_style_radius(kit_rename_win, 8, 0);

    lv_obj_t *client = lv_win_get_content(kit_rename_win);
    lv_obj_set_style_pad_all(client, 10, 0);
    lv_obj_set_style_bg_color(client, lv_color_hex(0x000000), 0);
    lv_obj_set_flex_flow(client, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(client, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(client, 10, 0);

    lv_obj_t *lbl = lv_label_create(client);
    lv_label_set_text(lbl, "Rinomina Kit");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);

    kit_rename_ta = lv_textarea_create(client);
    lv_obj_set_size(kit_rename_ta, 460, 45);
    lv_obj_set_style_bg_color(kit_rename_ta, lv_color_hex(0x222222), 0);
    lv_obj_set_style_text_color(kit_rename_ta, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(kit_rename_ta, 1, 0);
    lv_obj_set_style_border_color(kit_rename_ta, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(kit_rename_ta, &lv_font_montserrat_18, 0);
    lv_textarea_set_text(kit_rename_ta, kit_names[kit_rename_idx]);
    lv_textarea_set_max_length(kit_rename_ta, KIT_NAME_LEN - 1);
    lv_textarea_set_one_line(kit_rename_ta, true);

    lv_obj_t *kb = lv_keyboard_create(client);
    lv_obj_set_size(kb, 550, 200);
    lv_obj_set_style_text_font(kb, &lv_font_montserrat_18, 0);
    lv_keyboard_set_textarea(kb, kit_rename_ta);
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
    lv_obj_add_event_cb(ok, kit_rename_confirm, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cancel = lv_btn_create(btn_cont);
    lv_obj_set_size(cancel, 120, 40);
    lv_obj_set_style_bg_color(cancel, lv_color_hex(0x6B1A1A), 0);
    lv_obj_t *cn_l = lv_label_create(cancel);
    lv_label_set_text(cn_l, "Annulla");
    lv_obj_set_style_text_color(cn_l, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(cn_l);
    lv_obj_add_event_cb(cancel, kit_rename_cancel, LV_EVENT_CLICKED, NULL);
}
// ============================================================
// Helper
// ============================================================
static void kit_build_options(int voice, char *buf, size_t bufSize) {
    buf[0] = '\0';
    const char **names = kit_set_names[voice];
    int n = kit_voice_set_count[voice];
    for (int i = 0; i < n; i++) {
        if (i > 0) strncat(buf, "\n", bufSize - strlen(buf) - 1);
        strncat(buf, names[i], bufSize - strlen(buf) - 1);
    }
}

// Applica la selezione del kit corrente a tutti i dropdown
static void kit_apply_current() {
    int k = kit_cur_kit;
    if (k > 10) k = 10;

    for (int i = 0; i < KIT_VOICE_COUNT; i++) {
        if (!kit_voice_dd[i] || !lv_obj_is_valid(kit_voice_dd[i])) continue;
        int sel = kitArr[i][k];
        if (sel < 0 || sel > maxArr[i]) sel = 0;
        lv_dropdown_set_selected(kit_voice_dd[i], sel);
    }

    if (kit_rev_dd && lv_obj_is_valid(kit_rev_dd)) {
        int sel = kitArr[9][k];
        if (sel < 0 || sel > 15) sel = 0;
        lv_dropdown_set_selected(kit_rev_dd, sel);
    }
}

// ============================================================
// Callbacks
// ============================================================
static void kit_preset_dd_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    int sel = lv_dropdown_get_selected(dd);
    if (sel < 0 || sel > 10) return;
    kit_cur_kit = (uint8_t)sel;
    kit_apply_current();
}

static void kit_rev_dd_cb(lv_event_t *e) {
    (void)e;
    // Per ora sola visualizzazione: il preset REV e' determinato dal KIT.
}

static void kit_voice_dd_cb(lv_event_t *e) {
    (void)e;
    // Per ora sola visualizzazione.
}

// ============================================================
// Dropdown helper
// ============================================================
static lv_obj_t* kit_dropdown_create(lv_obj_t *parent,
                                      int x, int y, int w, int h,
                                      const char *options,
                                      lv_event_cb_t cb) {
    lv_obj_t *dd = lv_dropdown_create(parent);
    lv_obj_set_size(dd, w, h);
    lv_obj_set_pos(dd, x, y);
    lv_obj_set_style_bg_color(dd, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(dd, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(dd, 2, 0);
    lv_obj_set_style_border_color(dd, lv_color_hex(0x666666), 0);
    lv_obj_set_style_radius(dd, 6, 0);
    lv_obj_set_style_text_color(dd, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(dd, &lv_font_montserrat_16, 0);
    lv_obj_set_style_pad_left(dd, 4, 0);
    lv_obj_set_style_pad_right(dd, 4, 0);
    lv_dropdown_set_options(dd, options);
    lv_dropdown_set_symbol(dd, NULL);

    lv_obj_t *list = lv_dropdown_get_list(dd);
    if (list) {
        lv_obj_set_style_text_font(list, &lv_font_montserrat_14, 0);
        lv_obj_set_style_bg_color(list, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_text_color(list, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_max_height(list, 380, 0);
    }

    if (cb) lv_obj_add_event_cb(dd, cb, LV_EVENT_VALUE_CHANGED, NULL);
    return dd;
}

// ============================================================
// Pagina KIT
// ============================================================
void kit_page_create(lv_obj_t *parent) {
    // ---- Home bottom-left ----
    lv_obj_t *home = lv_btn_create(parent);
    lv_obj_set_size(home, 55, 45);
    lv_obj_set_pos(home, 5, 415);
    lv_obj_set_style_bg_color(home, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(home, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_radius(home, 6, 0);
    lv_obj_set_style_border_width(home, 2, 0);
    lv_obj_set_style_border_color(home, lv_color_hex(0x9B59B6), 0);
    lv_obj_t *home_lbl = lv_label_create(home);
    lv_label_set_text(home_lbl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(home_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(home_lbl, &lv_font_montserrat_24, 0);
    lv_obj_center(home_lbl);
    lv_obj_add_event_cb(home, eb, LV_EVENT_CLICKED, (void*)(uintptr_t)-3);

       // ---- Dropdown KIT 1..16 (top-right, mostra "n Nome") ----
    static char kit_opts[512];
    kit_opts[0] = '\0';
    for (int i = 0; i < 16; i++) {
        if (i > 0) strncat(kit_opts, "\n",
                          sizeof(kit_opts) - strlen(kit_opts) - 1);
        char line[40];
        snprintf(line, sizeof(line), "%d %.20s", i + 1, kit_names[i]);
        strncat(kit_opts, line,
                sizeof(kit_opts) - strlen(kit_opts) - 1);
    }
    kit_preset_dd = kit_dropdown_create(parent, 350, 5, 180, 45,
                                        kit_opts, kit_preset_dd_cb);
    lv_obj_set_style_border_color(kit_preset_dd,
                                  lv_color_hex(0xFF8800), 0);
    lv_dropdown_set_selected(kit_preset_dd, kit_cur_kit);

    // ---- Dropdown REV 1..16 (top-right) ----
    const char *rev_opts =
        "REV 1\nREV 2\nREV 3\nREV 4\nREV 5\nREV 6\nREV 7\nREV 8\n"
        "REV 9\nREV 10\nREV 11\nREV 12\nREV 13\nREV 14\nREV 15\nREV 16";
    kit_rev_dd = kit_dropdown_create(parent, 540, 5, 120, 45,
                                     rev_opts, kit_rev_dd_cb);
    lv_obj_set_style_border_color(kit_rev_dd,
                                  lv_color_hex(0xAA0000), 0);

    // ---- Bottone RINOMINA (top-right) ----
    lv_obj_t *rn_btn = lv_btn_create(parent);
    lv_obj_set_size(rn_btn, 120, 45);
    lv_obj_set_pos(rn_btn, 670, 5);
    lv_obj_set_style_bg_color(rn_btn, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(rn_btn, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_radius(rn_btn, 6, 0);
    lv_obj_set_style_border_width(rn_btn, 2, 0);
    lv_obj_set_style_border_color(rn_btn, lv_color_hex(0xFF8800), 0);
    lv_obj_t *rn_lbl = lv_label_create(rn_btn);
    lv_label_set_text(rn_lbl, "RINOMINA");
    lv_obj_set_style_text_color(rn_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(rn_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(rn_lbl);
    lv_obj_add_event_cb(rn_btn, kit_rename_btn_cb, LV_EVENT_CLICKED, NULL);

    // ---- Griglia 3x3 di dropdown voce ----
    //   Cella = 240 x 110, griglia totale = 720 x 330, centrata a (40, 70)
    const int CELL_W = 240;
    const int CELL_H = 110;
    const int GRID_X = (800 - 3 * CELL_W) / 2;   // = 40
    const int GRID_Y = 70;

    for (int i = 0; i < KIT_VOICE_COUNT; i++) {
        int row = i / 3;
        int col = i % 3;
        int cx = GRID_X + col * CELL_W;
        int cy = GRID_Y + row * CELL_H;

        // Label voce (sopra il dropdown)
        lv_obj_t *lbl = lv_label_create(parent);
        lv_label_set_text(lbl, kit_voice_names[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_set_pos(lbl, cx + 10, cy + 5);

        // Dropdown con i set della voce
        char opts[256];
        kit_build_options(i, opts, sizeof(opts));
        kit_voice_dd[i] = kit_dropdown_create(parent,
                                              cx + 10, cy + 30,
                                              CELL_W - 20, 45,
                                              opts, kit_voice_dd_cb);
    }

    // ---- Applica la selezione del kit corrente ----
    kit_apply_current();
}