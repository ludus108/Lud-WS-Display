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
uint8_t   kit_cur_kit = 0;   // 0..15

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
// Rinomina KIT
// ============================================================
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

// Callback applicata dalla modal globale in Core quando l'utente conferma.
// user_data = indice kit (0..15) come intptr_t.
static void kit_rename_apply(const char *new_name, void *user_data) {
    if (!new_name || strlen(new_name) == 0) return;
    int idx = (int)(intptr_t)user_data;
    if (idx < 0 || idx >= 16) return;

    strncpy(kit_names[idx], new_name, KIT_NAME_LEN - 1);
    kit_names[idx][KIT_NAME_LEN - 1] = '\0';
    kit_refresh_dd_options();
}

// Wrapper: la vecchia finestra locale è stata sostituita dalla modal
// globale in Core. Mantengo la firma per non toccare lvglGrafPages.cpp.
void kit_close_rename_window() {
    close_rename_modal();
}

static void kit_rename_btn_cb(lv_event_t *e) {
    (void)e;
    if (rename_modal_is_open()) return;
    open_rename_modal("Rinomina Kit",
                      kit_names[kit_cur_kit],
                      KIT_NAME_LEN - 1,
                      0xFF8800,
                      kit_rename_apply,
                      (void*)(intptr_t)kit_cur_kit);
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
    if (k > 15) k = 15;

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
    if (sel < 0 || sel > 15) return;
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
// Salva
// ============================================================
static void kit_save_btn_cb(lv_event_t *e) {
    (void)e;
    // TODO: invio bulk kitArr + kit_names via LWS
    Serial.printf("[KIT] send to Teensy (stub) kit=%u\n",
                  (unsigned)kit_cur_kit);
    log_add("KIT salvato", lv_color_hex(0x00FF00));
    toast_show("KIT salvato!", lv_color_hex(0x00FF00), TOAST_DUR);
}

// ============================================================
// Pagina KIT
// ============================================================
void kit_page_create(lv_obj_t *parent) {
    // ---- Home bottom-left ----
    mkbtn(parent, 5, 415, 55, 45,
          0x9B59B6, LV_SYMBOL_LEFT,
          &lv_font_montserrat_24,
          eb, -3, 6, 2);

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
    kit_preset_dd = styled_dropdown(parent, 350, 5, 180, 45,
                                    0xFF8800,
                                    &lv_font_montserrat_16,
                                    kit_opts);
    lv_dropdown_set_selected(kit_preset_dd, kit_cur_kit);
    lv_obj_add_event_cb(kit_preset_dd, kit_preset_dd_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);

    // ---- Dropdown REV 1..16 (top-right) ----
    const char *rev_opts =
        "REV 1\nREV 2\nREV 3\nREV 4\nREV 5\nREV 6\nREV 7\nREV 8\n"
        "REV 9\nREV 10\nREV 11\nREV 12\nREV 13\nREV 14\nREV 15\nREV 16";
    kit_rev_dd = styled_dropdown(parent, 540, 5, 120, 45,
                                 0xAA0000,
                                 &lv_font_montserrat_16,
                                 rev_opts);
    lv_obj_add_event_cb(kit_rev_dd, kit_rev_dd_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);

    // ---- Bottone RINOMINA (top-right) ----
    mkbtn(parent, 670, 5, 120, 45,
          0xFF8800, "RINOMINA",
          &lv_font_montserrat_14,
          kit_rename_btn_cb, 0, 6, 2);

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
        kit_voice_dd[i] = styled_dropdown(parent,
                                          cx + 10, cy + 30,
                                          CELL_W - 20, 45,
                                          0x666666,
                                          &lv_font_montserrat_16,
                                          opts);
        lv_obj_add_event_cb(kit_voice_dd[i], kit_voice_dd_cb,
                            LV_EVENT_VALUE_CHANGED, NULL);
    }

    // ---- Applica la selezione del kit corrente ----
    kit_apply_current();

    // ---- Bottone Salva (bottom-right, stile SONG/SEQ) ----
    mkbtn(parent, 680, 415, 110, 45,
          0xFF4444, "Salva",
          &lv_font_montserrat_16,
          kit_save_btn_cb, 0, 6, 2);
}