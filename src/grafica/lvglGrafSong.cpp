// ============================================================
// lvglGrafSong.cpp — SONG editor
// ============================================================
#include "globals.h"
#include "lvglGraf.h"
#include "lvglGraf_internal.h"
#include "lvgl_v8_port.h"
#include "src/preset/preset_ui.h"
#include <math.h>

// ============================================================
// SONG — mirror di songArr del Teensy (rev 13)
//   songArr[s][0][i] = tipo (0=A 1=B 2=C 3=D 4=fill)
//   songArr[s][1][i] = num  (pattern o fill: 0..15)
//   songArr[s][2][i] = kit  (0..15, indice diretto)
//   songArr[s][3][i] = rev  (0..15, indice diretto)
//   songLen[s]       = slot effettivi (1..256)
// ============================================================
lv_obj_t *drum_play_song_btn = nullptr;
lv_obj_t *drum_play_song_lbl = nullptr;

lv_obj_t *drum_song_dd    = nullptr;
lv_obj_t *drum_monitor_lbl = nullptr;

uint8_t  songArr[16][4][SONG_SLOTS] = {};
uint16_t songLen[16]                = {};
static bool    songArr_initialized = false;

char song_names[16][SONG_NAME_LEN] = {
    "Song 1",  "Song 2",  "Song 3",  "Song 4",
    "Song 5",  "Song 6",  "Song 7",  "Song 8",
    "Song 9",  "Song 10", "Song 11", "Song 12",
    "Song 13", "Song 14", "Song 15", "Song 16"
};

uint8_t   song_cur_song = 0;
uint8_t   song_cur_page = 0;
int       song_sel_slot = -1;

lv_obj_t *song_song_dd    = nullptr;
lv_obj_t *song_fln_dd     = nullptr;
lv_obj_t *song_kit_dd     = nullptr;
lv_obj_t *song_rev_dd     = nullptr;
lv_obj_t *song_sec_btn[4] = {nullptr,nullptr,nullptr,nullptr};
lv_obj_t *song_sec_lbl[4] = {nullptr,nullptr,nullptr,nullptr};
lv_obj_t *song_page_lbl   = nullptr;
lv_obj_t *song_slot            [SONG_ROWS][SONG_COLS] = {};
lv_obj_t *song_slot_lbl        [SONG_ROWS][SONG_COLS] = {};
lv_obj_t *song_slot_kit_lbl    [SONG_ROWS][SONG_COLS] = {};
lv_obj_t *song_slot_num_lbl    [SONG_ROWS][SONG_COLS] = {};
lv_obj_t *song_slot_rev_lbl    [SONG_ROWS][SONG_COLS] = {};
// ============================================================
// Play song
// ============================================================
lv_obj_t *song_play_btn = nullptr;
lv_obj_t *song_play_lbl = nullptr;
lv_obj_t *song_cursor   = nullptr;

static lv_timer_t *song_play_timer = nullptr;
int          song_play_slot = -1;    // indice globale 0..255
int          song_play_step = 0;     // 0..15 dentro lo slot
bool         song_playing   = false;
bool      song_loop     = true;   // default: Loop attivo
lv_obj_t *song_loop_btn = nullptr;
lv_obj_t *song_loop_lbl = nullptr;
// Geometria griglia (per posizionare il cursore)
static int song_grid_x  = 0;
static int song_grid_y  = 0;
static int song_slot_w  = 0;
static int song_slot_h  = 0;

// ============================================================
// Init
// ============================================================
void songArr_init_if_needed() {
    if (songArr_initialized) return;
    for (int s = 0; s < 16; s++) {
        songLen[s] = 1;
        songArr[s][SONG_COL_TIPO][0] = 0;   // tipo A
        songArr[s][SONG_COL_NUM][0]  = s;
        songArr[s][SONG_COL_KIT][0]  = 0;   // kit 0 (indice diretto)
        songArr[s][SONG_COL_REV][0]  = 0;   // REV 1
        for (int i = 1; i < SONG_SLOTS; i++) {
            songArr[s][SONG_COL_TIPO][i] = 0;
            songArr[s][SONG_COL_NUM][i]  = 0;
            songArr[s][SONG_COL_KIT][i]  = 0;
            songArr[s][SONG_COL_REV][i]  = 0;
        }
    }
    songArr_initialized = true;
    Serial.printf("[INIT] songLen[0]=%u\n", (unsigned)songLen[0]);
    Serial.println("[DRUM] songArr inizializzato (rev 17)");
}

int song_current_section() {
    if (!song_playing) return -1;
    if (song_play_slot < 0 || song_play_slot >= (int)songLen[song_cur_song]) return -1;
    uint8_t tipo = songArr[song_cur_song][SONG_COL_TIPO][song_play_slot];
    if (tipo <= 3) return tipo;
    return -1;   // fill
}

int song_current_pattern() {
    if (!song_playing) return -1;
    if (song_play_slot < 0 || song_play_slot >= (int)songLen[song_cur_song]) return -1;
    return songArr[song_cur_song][SONG_COL_NUM][song_play_slot];
}

bool song_current_is_fill() {
    if (!song_playing) return false;
    if (song_play_slot < 0 || song_play_slot >= (int)songLen[song_cur_song]) return false;
    return songArr[song_cur_song][SONG_COL_TIPO][song_play_slot] == 4;
}

static uint8_t song_pages_count() {
    uint8_t pages = (uint8_t)((songLen[song_cur_song] + SONG_SLOTS_PER_PAGE - 1)
                    / SONG_SLOTS_PER_PAGE);
    if (pages == 0) pages = 1;
    if (pages > SONG_SLOTS / SONG_SLOTS_PER_PAGE)
        pages = SONG_SLOTS / SONG_SLOTS_PER_PAGE;
    return pages;
}

// ============================================================
// Visual slot
// ============================================================
static void song_slot_update_visual(int r, int c) {
    int slot_idx = song_cur_page * SONG_SLOTS_PER_PAGE
                 + r * SONG_COLS + c;
    lv_obj_t *box  = song_slot[r][c];
    lv_obj_t *lbl  = song_slot_lbl[r][c];
    lv_obj_t *lblK = song_slot_kit_lbl[r][c];
    lv_obj_t *lblN = song_slot_num_lbl[r][c];
    lv_obj_t *lblR = song_slot_rev_lbl[r][c];
    if (!box || !lv_obj_is_valid(box)) return;

    bool in_song  = (slot_idx < (int)songLen[song_cur_song]);
    bool selected = (slot_idx == song_sel_slot) && in_song;

    if (!in_song) {
        if (lbl)  lv_label_set_text(lbl, "");
        if (lblK) lv_label_set_text(lblK, "");
        if (lblN) lv_label_set_text(lblN, "");
        if (lblR) lv_label_set_text(lblR, "");
        lv_obj_set_style_bg_color(box, lv_color_hex(0x0A0A14), 0);
        lv_obj_set_style_border_color(box, lv_color_hex(0x333333), 0);
        lv_obj_set_style_border_width(box, 1, 0);
        return;
    }

    uint8_t tipo = songArr[song_cur_song][SONG_COL_TIPO][slot_idx];
    uint8_t num  = songArr[song_cur_song][SONG_COL_NUM] [slot_idx];
    uint8_t kit  = songArr[song_cur_song][SONG_COL_KIT] [slot_idx];
    uint8_t rev  = songArr[song_cur_song][SONG_COL_REV] [slot_idx];

    char txt[12];
    uint32_t col = 0xAAAAAA;
    switch (tipo) {
        case 0: snprintf(txt, sizeof(txt), "A");    col = 0x66CCFF; break;
        case 1: snprintf(txt, sizeof(txt), "B");    col = 0x66CCFF; break;
        case 2: snprintf(txt, sizeof(txt), "C");    col = 0x66CCFF; break;
        case 3: snprintf(txt, sizeof(txt), "D");    col = 0x66CCFF; break;
        case 4: snprintf(txt, sizeof(txt), "FLN %u", (unsigned)(num + 1));
                                                     col = 0xFFCC33; break;
        default: snprintf(txt, sizeof(txt), "?"); break;
    }

    if (lbl) {
        lv_label_set_text(lbl, txt);
        lv_obj_set_style_text_color(lbl, lv_color_hex(col), 0);
    }

    // KIT (top-left): mostrato solo se DIVERSO dallo slot precedente.
    //   Slot 0: sempre mostrato (kit iniziale della song).
    if (lblK) {
        bool show_kit = true;
        if (slot_idx > 0) {
            uint8_t prev_kit =
                songArr[song_cur_song][SONG_COL_KIT][slot_idx - 1];
            if (prev_kit == kit) show_kit = false;
        }
        if (show_kit) {
            char txt_kit[12];
            snprintf(txt_kit, sizeof(txt_kit), "KIT %u", (unsigned)(kit + 1));
            lv_label_set_text(lblK, txt_kit);
        } else {
            lv_label_set_text(lblK, "");
        }
    }

    // Numero slot (top-right, 1-based)
    if (lblN) {
        char txt_n[8];
        snprintf(txt_n, sizeof(txt_n), "%u", (unsigned)(slot_idx + 1));
        lv_label_set_text(lblN, txt_n);
    }

    // REV (bottom, indice + 1) — mostrato solo se DIVERSO dal precedente
    if (lblR) {
        bool show_rev = true;
        if (slot_idx > 0) {
            uint8_t prev_rev =
                songArr[song_cur_song][SONG_COL_REV][slot_idx - 1];
            if (prev_rev == rev) show_rev = false;
        }
        if (show_rev) {
            char txt_rev[12];
            snprintf(txt_rev, sizeof(txt_rev), "REV %u", (unsigned)(rev + 1));
            lv_label_set_text(lblR, txt_rev);
        } else {
            lv_label_set_text(lblR, "");
        }
    }

    if (selected) {
        lv_obj_set_style_bg_color(box, lv_color_hex(0x2A2A4E), 0);
        lv_obj_set_style_border_color(box, lv_color_hex(0xFFAA00), 0);
        lv_obj_set_style_border_width(box, 3, 0);
    } else {
        lv_obj_set_style_bg_color(box, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_border_color(box, lv_color_hex(col), 0);
        lv_obj_set_style_border_width(box, 2, 0);
    }
}

static void song_grid_update_all() {
    uint8_t pages = song_pages_count();
    if (song_cur_page >= pages) song_cur_page = pages - 1;

    for (int r = 0; r < SONG_ROWS; r++)
        for (int c = 0; c < SONG_COLS; c++)
            song_slot_update_visual(r, c);

    char buf[16];
    snprintf(buf, sizeof(buf), "%u/%u",
             (unsigned)(song_cur_page + 1), (unsigned)pages);
    if (song_page_lbl && lv_obj_is_valid(song_page_lbl))
        lv_label_set_text(song_page_lbl, buf);
}

static void song_sec_apply_visual() {
    // Referente: slot in play (se attivo) oppure slot selezionato
    int ref = -1;
    if (song_playing && song_play_slot >= 0) ref = song_play_slot;
    else if (song_sel_slot >= 0) ref = song_sel_slot;

    int tipo_sel = -1;
    if (ref >= 0 && ref < (int)songLen[song_cur_song]) {
        tipo_sel = songArr[song_cur_song][SONG_COL_TIPO][ref];
    }
    for (int i = 0; i < 4; i++) {
        if (!song_sec_btn[i] || !lv_obj_is_valid(song_sec_btn[i])) continue;
        bool on = (tipo_sel == i);
        lv_obj_set_style_border_width(song_sec_btn[i], on ? 3 : 2, 0);
        lv_obj_set_style_border_color(song_sec_btn[i],
            lv_color_hex(on ? 0xFFAA00 : 0x666666), 0);
        if (song_sec_lbl[i] && lv_obj_is_valid(song_sec_lbl[i])) {
            lv_obj_set_style_text_color(song_sec_lbl[i],
                lv_color_hex(on ? 0xFFAA00 : 0xFFFFFF), 0);
        }
    }
    // FLN: evidenzia se tipo=4
    if (song_fln_dd && lv_obj_is_valid(song_fln_dd)) {
        lv_obj_set_style_border_color(song_fln_dd,
            lv_color_hex(tipo_sel == 4 ? 0xFFAA00 : 0xFFCC33), 0);
    }
}

// ============================================================
// Modifica slot
// ============================================================
static void song_set_sel_slot(uint8_t tipo, uint8_t num) {
    if (song_sel_slot < 0 || song_sel_slot >= (int)songLen[song_cur_song]) return;
    songArr[song_cur_song][SONG_COL_TIPO][song_sel_slot] = tipo;
    songArr[song_cur_song][SONG_COL_NUM] [song_sel_slot] = num;
    song_grid_update_all();
}

static void song_set_sel_kit(uint8_t kit) {
    if (song_sel_slot < 0 || song_sel_slot >= (int)songLen[song_cur_song]) return;
    songArr[song_cur_song][SONG_COL_KIT][song_sel_slot] = kit;
    song_grid_update_all();
}

static void song_set_sel_rev(uint8_t rev) {
    if (song_sel_slot < 0 || song_sel_slot >= (int)songLen[song_cur_song]) return;
    songArr[song_cur_song][SONG_COL_REV][song_sel_slot] = rev;
    song_grid_update_all();
}

// ============================================================
// Dropdown SNG: ricostruzione opzioni "n Nome"
// ============================================================
void song_build_dd_options(char *buf, size_t bufSize) {
    buf[0] = '\0';
    for (int i = 0; i < 16; i++) {
        if (i > 0) strncat(buf, "\n", bufSize - strlen(buf) - 1);
        char line[40];
        snprintf(line, sizeof(line), "%d %.20s", i + 1, song_names[i]);
        strncat(buf, line, bufSize - strlen(buf) - 1);
    }
}

static void song_refresh_dd_options() {
    if (!song_song_dd || !lv_obj_is_valid(song_song_dd)) return;
    static char opts[512];
    song_build_dd_options(opts, sizeof(opts));
    int sel = lv_dropdown_get_selected(song_song_dd);
    lv_dropdown_set_options(song_song_dd, opts);
    lv_dropdown_set_selected(song_song_dd, sel);
    lv_dropdown_set_symbol(song_song_dd, NULL);
}

void drum_monitor_update() {
    songArr_init_if_needed();
    if (!drum_monitor_lbl || !lv_obj_is_valid(drum_monitor_lbl)) return;

    uint8_t s = song_cur_song;
    if (songLen[s] == 0) {
        lv_label_set_text(drum_monitor_lbl, "--");
        return;
    }

    int slot = 0;
    if (song_playing && song_play_slot >= 0
        && song_play_slot < (int)songLen[s]) {
        slot = song_play_slot;
    }

    uint8_t tipo = songArr[s][SONG_COL_TIPO][slot];
    uint8_t num  = songArr[s][SONG_COL_NUM] [slot];
    uint8_t kit  = songArr[s][SONG_COL_KIT] [slot];
    uint8_t rev  = songArr[s][SONG_COL_REV] [slot];

    if (kit > 15) kit = 15;

    char buf[96];
    if (tipo == 4) {
        snprintf(buf, sizeof(buf),
                 "POS %u   KIT %s   FLN %u   REV %u",
                 (unsigned)(slot + 1),
                 kit_names[kit],
                 (unsigned)(num + 1),
                 (unsigned)(rev + 1));
    } else {
        char sec = (char)('A' + tipo);
        snprintf(buf, sizeof(buf),
                 "POS %u   KIT %s   PTN %u %c   REV %u",
                 (unsigned)(slot + 1),
                 kit_names[kit],
                 (unsigned)(num + 1),
                 sec,
                 (unsigned)(rev + 1));
    }
    lv_label_set_text(drum_monitor_lbl, buf);
}

// ============================================================
// Callbacks
// ============================================================
static void song_song_dd_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    int sel = lv_dropdown_get_selected(dd);
    if (sel < 0) return;
    song_cur_song = (uint8_t)sel;
    song_cur_page = 0;
    song_sel_slot = -1;
    song_grid_update_all();
    song_sec_apply_visual();
    drum_monitor_update();
}

static void song_page_prev_cb(lv_event_t *e) {
    (void)e;
    if (song_cur_page > 0) song_cur_page--;
    song_sel_slot = -1;
    song_grid_update_all();
    song_sec_apply_visual();
}

static void song_page_next_cb(lv_event_t *e) {
    (void)e;
    if (song_cur_page + 1 < song_pages_count()) song_cur_page++;
    song_sel_slot = -1;
    song_grid_update_all();
    song_sec_apply_visual();
}

static void song_slot_cb(lv_event_t *e) {
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    if (idx >= (int)songLen[song_cur_song]) return;
    if (song_sel_slot == idx) song_sel_slot = -1;
    else                       song_sel_slot = idx;
    song_grid_update_all();
    song_sec_apply_visual();
}

static void song_sec_btn_cb(lv_event_t *e) {
    int sec = (int)(uintptr_t)lv_event_get_user_data(e);
    if (sec < 0 || sec > 3) return;
    song_set_sel_slot((uint8_t)sec, song_cur_song);
    song_sec_apply_visual();
}

static void song_fln_dd_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    int sel = lv_dropdown_get_selected(dd);
    if (sel < 0) return;
    song_set_sel_slot(4, (uint8_t)sel);
}

static void song_kit_dd_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    int sel = lv_dropdown_get_selected(dd);
    if (sel < 0) return;
    song_set_sel_kit((uint8_t)sel);   // 0..15 indice diretto
}

static void song_rev_dd_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    int sel = lv_dropdown_get_selected(dd);
    if (sel < 0) return;
    song_set_sel_rev((uint8_t)sel);
}

static void song_add_slot_cb(lv_event_t *e) {
    (void)e;
    if (songLen[song_cur_song] >= SONG_SLOTS) return;
    uint16_t new_idx = songLen[song_cur_song];
    songLen[song_cur_song]++;

    // Eredita kit/rev dall'ultimo slot esistente
    uint8_t prev_kit = 0, prev_rev = 0;
    if (new_idx > 0) {
        prev_kit = songArr[song_cur_song][SONG_COL_KIT][new_idx - 1];
        prev_rev = songArr[song_cur_song][SONG_COL_REV][new_idx - 1];
    }
    songArr[song_cur_song][SONG_COL_TIPO][new_idx] = 0;
    songArr[song_cur_song][SONG_COL_NUM] [new_idx] = song_cur_song;
    songArr[song_cur_song][SONG_COL_KIT] [new_idx] = prev_kit;
    songArr[song_cur_song][SONG_COL_REV] [new_idx] = prev_rev;

    uint8_t pages = song_pages_count();
    song_cur_page = pages - 1;
    song_sel_slot = new_idx;
    song_grid_update_all();
    song_sec_apply_visual();
}

static void song_rm_slot_cb(lv_event_t *e) {
    (void)e;
    if (songLen[song_cur_song] <= 1) return;
    songLen[song_cur_song]--;
    if (song_sel_slot >= (int)songLen[song_cur_song]) song_sel_slot = -1;
    song_grid_update_all();
    song_sec_apply_visual();
}

// ============================================================
// Invio bulk song al Teensy: 'q' / 'x' / 'k'
// ============================================================
#define SONG_CHUNK_SLOTS   32
#define SONG_CHUNK_BYTES   (SONG_CHUNK_SLOTS * 4)

void song_send_to_teensy(uint8_t song) {
    if (song >= 16) return;

    // CRC sui 256 slot completi (1024 byte: tipo,numero,kit,rev)
    static uint8_t payload[SONG_SLOTS * 4];
    for (int i = 0; i < SONG_SLOTS; i++) {
        payload[i*4 + 0] = songArr[song][SONG_COL_TIPO][i];
        payload[i*4 + 1] = songArr[song][SONG_COL_NUM] [i];
        payload[i*4 + 2] = songArr[song][SONG_COL_KIT] [i];
        payload[i*4 + 3] = songArr[song][SONG_COL_REV] [i];
    }
    uint8_t crc = lws_crc8(payload, sizeof(payload));

    // BEGIN 'q' [song][len_lo][len_hi]
    uint16_t len = songLen[song];
    uint8_t pb[3] = { song, (uint8_t)(len & 0xFF), (uint8_t)(len >> 8) };
    lws_send_frame(Serial1, ID_DISPLAY, next_seq(), 'q', pb, 3);
    delay(2);

    // CHUNK 'x' × 8
    for (int chunk = 0; chunk < SONG_SLOTS / SONG_CHUNK_SLOTS; chunk++) {
        int off = chunk * SONG_CHUNK_SLOTS;
        uint8_t cb[2 + SONG_CHUNK_BYTES];
        cb[0] = (uint8_t)(off & 0xFF);
        cb[1] = (uint8_t)(off >> 8);
        memcpy(&cb[2], &payload[off * 4], SONG_CHUNK_BYTES);
        lws_send_frame(Serial1, ID_DISPLAY, next_seq(), 'x',
                       cb, 2 + SONG_CHUNK_BYTES);
        delay(2);
    }

    // END 'k' [crc8]
    uint8_t eb[1] = { crc };
    lws_send_frame(Serial1, ID_DISPLAY, next_seq(), 'k', eb, 1);

    char msg[32];
    snprintf(msg, sizeof(msg), "SNG %u inviata", (unsigned)(song + 1));
    log_add(msg, lv_color_hex(0x00FF00));
    toast_show(msg, lv_color_hex(0x00FF00), TOAST_DUR);
}

static void song_save_btn_cb(lv_event_t *e) {
    (void)e;
    song_send_to_teensy(song_cur_song);
}

// ============================================================
// Rinomina Song (modal globale in Core)
// ============================================================
// Wrapper: la finestra locale è stata sostituita dalla modal globale.
// Mantengo la firma per non toccare lvglGrafPages.cpp.
void song_close_rename_window() {
    close_rename_modal();
}

// Callback applicata dalla modal quando l'utente conferma.
// user_data = indice song (0..15) come intptr_t.
static void song_rename_apply(const char *new_name, void *user_data) {
    if (!new_name || strlen(new_name) == 0) return;
    int idx = (int)(intptr_t)user_data;
    if (idx < 0 || idx >= 16) return;

    strncpy(song_names[idx], new_name, SONG_NAME_LEN - 1);
    song_names[idx][SONG_NAME_LEN - 1] = '\0';

    // Aggiorna il dropdown nella pagina SONG
    song_refresh_dd_options();

    // Aggiorna anche il dropdown nella pagina DRUM, se esiste
    if (drum_song_dd && lv_obj_is_valid(drum_song_dd)) {
        static char opts[512];
        song_build_dd_options(opts, sizeof(opts));
        int sel = lv_dropdown_get_selected(drum_song_dd);
        lv_dropdown_set_options(drum_song_dd, opts);
        lv_dropdown_set_selected(drum_song_dd, sel);
        lv_dropdown_set_symbol(drum_song_dd, NULL);
    }
}

static void song_rename_btn_cb(lv_event_t *e) {
    (void)e;
    if (rename_modal_is_open()) return;
    open_rename_modal("Rinomina Song",
                      song_names[song_cur_song],
                      SONG_NAME_LEN - 1,
                      0x00DD00,
                      song_rename_apply,
                      (void*)(intptr_t)song_cur_song);
}

// ============================================================
// Play song
// ============================================================
static void song_update_cursor_position() {
    if (!song_cursor || !lv_obj_is_valid(song_cursor)) return;
    if (song_play_slot < 0) return;

    // Se lo slot è in un'altra pagina, cambia pagina
    uint8_t page = (uint8_t)(song_play_slot / SONG_SLOTS_PER_PAGE);
    if (page != song_cur_page) {
        song_cur_page = page;
        song_grid_update_all();
    }

    int slot_in_page = song_play_slot % SONG_SLOTS_PER_PAGE;
    int r = slot_in_page / SONG_COLS;
    int c = slot_in_page % SONG_COLS;

    int x = c * song_slot_w;
    int y = r * song_slot_h;
    lv_obj_set_pos(song_cursor, x, y);
    lv_obj_clear_flag(song_cursor, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(song_cursor);
}

static void song_update_ui_for_slot(int idx) {
    if (idx < 0 || idx >= (int)songLen[song_cur_song]) return;

    uint8_t kit = songArr[song_cur_song][SONG_COL_KIT][idx];
    uint8_t rev = songArr[song_cur_song][SONG_COL_REV][idx];

    if (song_kit_dd && lv_obj_is_valid(song_kit_dd))
        lv_dropdown_set_selected(song_kit_dd, kit);
    if (song_rev_dd && lv_obj_is_valid(song_rev_dd))
        lv_dropdown_set_selected(song_rev_dd, rev);

    song_sec_apply_visual();
    drum_monitor_update();
}

static void song_play_tick_cb(lv_timer_t *t) {
    (void)t;
    if (!song_playing) return;
    if (song_play_slot < 0) return;
    if (song_play_slot >= (int)songLen[song_cur_song]) {
        song_stop();
        return;
    }

    // Meter MIX (DRUM)
    drum_mix_meters_trigger_from_slot(song_play_slot, (uint8_t)song_play_step);

    // Se siamo in PTN, sincronizza la pagina
    if (g_current_page && strcmp(g_current_page, "SEQ") == 0) {
        drum_meters_update_from_slot(song_play_slot, (uint8_t)song_play_step);

        // Aggiorna cursore PTN allo step corrente
        drum_step_counter = (uint8_t)song_play_step;
        if (drum_cursor && lv_obj_is_valid(drum_cursor)) {
            lv_obj_set_x(drum_cursor,
                drum_grid_x + drum_cursor_off_x
                + song_play_step * drum_cell_w);
        }
    }

    // Avanza lo step
    song_play_step++;
    if (song_play_step >= 16) {
        song_play_step = 0;
        song_play_slot++;

        if (song_play_slot >= (int)songLen[song_cur_song]) {
            if (song_loop) {
                song_play_slot = 0;
                song_update_ui_for_slot(0);
            } else {
                song_stop();
                return;
            }
        } else {
            song_update_ui_for_slot(song_play_slot);
        }

        // Cambio slot: se PTN è aperta, ricarica la griglia
        if (g_current_page && strcmp(g_current_page, "SEQ") == 0) {
            extern void drum_load_song_slot_into_view();
            drum_load_song_slot_into_view();
        }
    }

    song_update_cursor_position();
}

void song_stop() {
    song_playing = false;
    if (song_play_timer) {
        lv_timer_del(song_play_timer);
        song_play_timer = nullptr;
    }
    if (song_cursor && lv_obj_is_valid(song_cursor))
        lv_obj_add_flag(song_cursor, LV_OBJ_FLAG_HIDDEN);

    song_play_slot = -1;
    song_play_step = 0;

    if (song_play_lbl && lv_obj_is_valid(song_play_lbl))
        lv_label_set_text(song_play_lbl, LV_SYMBOL_PLAY);
    if (song_play_btn && lv_obj_is_valid(song_play_btn))
        lv_obj_set_style_border_color(song_play_btn,
            lv_color_hex(0x00FF00), 0);

    if (drum_play_song_lbl && lv_obj_is_valid(drum_play_song_lbl))
        lv_label_set_text(drum_play_song_lbl, LV_SYMBOL_PLAY);
    if (drum_play_song_btn && lv_obj_is_valid(drum_play_song_btn))
        lv_obj_set_style_border_color(drum_play_song_btn,
            lv_color_hex(0x00FF00), 0);

    drum_mix_meters_reset();

    if (g_current_page && strcmp(g_current_page, "SEQ") == 0) {
        extern void drum_reload_manual_view();
        drum_reload_manual_view();
    }
    song_sec_apply_visual();
}

void song_play() {
    songArr_init_if_needed();

    song_playing   = true;
    song_play_slot = 0;
    song_play_step = 0;

    song_update_ui_for_slot(0);
    song_update_cursor_position();

    if (song_play_lbl && lv_obj_is_valid(song_play_lbl))
        lv_label_set_text(song_play_lbl, LV_SYMBOL_STOP);
    if (song_play_btn && lv_obj_is_valid(song_play_btn))
        lv_obj_set_style_border_color(song_play_btn,
            lv_color_hex(0x888888), 0);

    if (drum_play_song_lbl && lv_obj_is_valid(drum_play_song_lbl))
        lv_label_set_text(drum_play_song_lbl, LV_SYMBOL_STOP);
    if (drum_play_song_btn && lv_obj_is_valid(drum_play_song_btn))
        lv_obj_set_style_border_color(drum_play_song_btn,
            lv_color_hex(0x888888), 0);

    uint32_t period = (60000 / (uint32_t)drum_bpm) / 4;
    if (period < 5) period = 5;
    Serial.printf("[PLAY] creo timer period=%lu\n", (unsigned long)period);

    if (song_play_timer) {
        lv_timer_set_period(song_play_timer, period);
        lv_timer_reset(song_play_timer);
    } else {
        song_play_timer = lv_timer_create(song_play_tick_cb, period, NULL);
    }
    Serial.println("[PLAY] timer OK");
}

void song_update_bpm(uint16_t bpm) {
    if (!song_playing || !song_play_timer) return;
    uint32_t period = (60000 / (uint32_t)bpm) / 4;
    if (period < 5) period = 5;
    lv_timer_set_period(song_play_timer, period);
}

static void song_play_btn_cb(lv_event_t *e) {
    (void)e;
    if (song_playing) song_stop();
    else              song_play();
}

static void song_loop_btn_cb(lv_event_t *e) {
    (void)e;
    song_loop = !song_loop;
    if (song_loop_lbl && lv_obj_is_valid(song_loop_lbl)) {
        lv_label_set_text(song_loop_lbl, song_loop ? "Loop" : "1Shot");
    }
}

void drum_play_song_btn_cb(lv_event_t *e) {
    (void)e;
    if (song_playing) song_stop();
    else              song_play();
}

// ============================================================
// Pagina SONG
// ============================================================
void song_page_create(lv_obj_t *parent) {
    songArr_init_if_needed();

    // ========================================================
    // TOP ROW: SNG | KIT | A B C D | FLN | REV
    // ========================================================

    // ---- SNG ("n Nome") ----
    song_song_dd = lv_dropdown_create(parent);
    lv_obj_set_size(song_song_dd, 150, 45);
    lv_obj_set_pos(song_song_dd, 5, 3);
    lv_obj_set_style_bg_color(song_song_dd, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(song_song_dd, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(song_song_dd, 2, 0);
    lv_obj_set_style_border_color(song_song_dd, lv_color_hex(0x00DD00), 0);
    lv_obj_set_style_radius(song_song_dd, 6, 0);
    lv_obj_set_style_text_color(song_song_dd, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(song_song_dd, &lv_font_montserrat_14, 0);
    lv_obj_set_style_pad_left(song_song_dd, 6, 0);
    lv_obj_set_style_pad_right(song_song_dd, 4, 0);
    lv_dropdown_set_selected(song_song_dd, song_cur_song);
    song_refresh_dd_options();
    { lv_obj_t *l = lv_dropdown_get_list(song_song_dd);
      if (l) { lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
               lv_obj_set_style_bg_color(l, lv_color_hex(0x1A1A2E), 0);
               lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), 0);
               lv_obj_set_style_max_height(l, 380, 0); } }
    lv_obj_add_event_cb(song_song_dd, song_song_dd_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);

    // ---- KIT ("n Nome") ----
    song_kit_dd = lv_dropdown_create(parent);
    lv_obj_set_size(song_kit_dd, 150, 45);
    lv_obj_set_pos(song_kit_dd, 160, 3);
    lv_obj_set_style_bg_color(song_kit_dd, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(song_kit_dd, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(song_kit_dd, 2, 0);
    lv_obj_set_style_border_color(song_kit_dd, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(song_kit_dd, 6, 0);
    lv_obj_set_style_text_color(song_kit_dd, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(song_kit_dd, &lv_font_montserrat_14, 0);
    lv_obj_set_style_pad_left(song_kit_dd, 6, 0);
    lv_obj_set_style_pad_right(song_kit_dd, 4, 0);
    {
        static char kit_opts[640];
        kit_opts[0] = '\0';
        for (int i = 0; i < 16; i++) {
            if (i > 0) strncat(kit_opts, "\n",
                              sizeof(kit_opts) - strlen(kit_opts) - 1);
            char line[40];
            snprintf(line, sizeof(line), "%d %.20s", i + 1, kit_names[i]);
            strncat(kit_opts, line,
                    sizeof(kit_opts) - strlen(kit_opts) - 1);
        }
        lv_dropdown_set_options(song_kit_dd, kit_opts);
    }
    lv_dropdown_set_selected(song_kit_dd, 0);
    lv_dropdown_set_symbol(song_kit_dd, NULL);
    { lv_obj_t *l = lv_dropdown_get_list(song_kit_dd);
      if (l) { lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
               lv_obj_set_style_bg_color(l, lv_color_hex(0x1A1A2E), 0);
               lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), 0);
               lv_obj_set_style_max_height(l, 380, 0); } }
    lv_obj_add_event_cb(song_kit_dd, song_kit_dd_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);

    // ---- A / B / C / D ----
    static const char *sn[4] = {"A","B","C","D"};
    const int SB_W = 50, SB_H = 45, SB_GAP = 5;
    const int SB_Y = 3, SB_X0 = 320;
    for (int i = 0; i < 4; i++) {
        lv_obj_t *b = lv_btn_create(parent);
        lv_obj_set_size(b, SB_W, SB_H);
        lv_obj_set_pos(b, SB_X0 + i * (SB_W + SB_GAP), SB_Y);
        lv_obj_set_style_bg_color(b, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_bg_color(b, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
        lv_obj_set_style_radius(b, 6, 0);
        lv_obj_set_style_border_width(b, 2, 0);
        lv_obj_set_style_border_color(b, lv_color_hex(0x666666), 0);

        lv_obj_t *l = lv_label_create(b);
        lv_label_set_text(l, sn[i]);
        lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_24, 0);
        lv_obj_center(l);

        song_sec_btn[i] = b;
        song_sec_lbl[i] = l;
        lv_obj_add_event_cb(b, song_sec_btn_cb, LV_EVENT_CLICKED,
                            (void*)(uintptr_t)i);
    }

    // ---- FLN ----
    song_fln_dd = lv_dropdown_create(parent);
    lv_obj_set_size(song_fln_dd, 95, 45);
    lv_obj_set_pos(song_fln_dd, 550, 3);
    lv_obj_set_style_bg_color(song_fln_dd, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(song_fln_dd, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(song_fln_dd, 2, 0);
    lv_obj_set_style_border_color(song_fln_dd, lv_color_hex(0xFFCC33), 0);
    lv_obj_set_style_radius(song_fln_dd, 6, 0);
    lv_obj_set_style_text_color(song_fln_dd, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(song_fln_dd, &lv_font_montserrat_24, 0);
    lv_obj_set_style_pad_left(song_fln_dd, 6, 0);
    lv_obj_set_style_pad_right(song_fln_dd, 4, 0);
    lv_dropdown_set_options(song_fln_dd,
        "FLN 1\nFLN 2\nFLN 3\nFLN 4\nFLN 5\nFLN 6\nFLN 7\nFLN 8\n"
        "FLN 9\nFLN 10\nFLN 11\nFLN 12\nFLN 13\nFLN 14\nFLN 15\nFLN 16");
    lv_dropdown_set_selected(song_fln_dd, 0);
    lv_dropdown_set_symbol(song_fln_dd, NULL);
    { lv_obj_t *l = lv_dropdown_get_list(song_fln_dd);
      if (l) { lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
               lv_obj_set_style_bg_color(l, lv_color_hex(0x1A1A2E), 0);
               lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), 0);
               lv_obj_set_style_max_height(l, 380, 0); } }
    lv_obj_add_event_cb(song_fln_dd, song_fln_dd_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);

    // ---- REV ----
    song_rev_dd = lv_dropdown_create(parent);
    lv_obj_set_size(song_rev_dd, 95, 45);
    lv_obj_set_pos(song_rev_dd, 655, 3);
    lv_obj_set_style_bg_color(song_rev_dd, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(song_rev_dd, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(song_rev_dd, 2, 0);
    lv_obj_set_style_border_color(song_rev_dd, lv_color_hex(0xCC0066), 0);
    lv_obj_set_style_radius(song_rev_dd, 6, 0);
    lv_obj_set_style_text_color(song_rev_dd, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(song_rev_dd, &lv_font_montserrat_24, 0);
    lv_obj_set_style_pad_left(song_rev_dd, 6, 0);
    lv_obj_set_style_pad_right(song_rev_dd, 4, 0);
    lv_dropdown_set_options(song_rev_dd,
        "REV 1\nREV 2\nREV 3\nREV 4\nREV 5\nREV 6\nREV 7\nREV 8\n"
        "REV 9\nREV 10\nREV 11\nREV 12\nREV 13\nREV 14\nREV 15\nREV 16");
    lv_dropdown_set_selected(song_rev_dd, 0);
    lv_dropdown_set_symbol(song_rev_dd, NULL);
    { lv_obj_t *l = lv_dropdown_get_list(song_rev_dd);
      if (l) { lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
               lv_obj_set_style_bg_color(l, lv_color_hex(0x1A1A2E), 0);
               lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), 0);
               lv_obj_set_style_max_height(l, 380, 0); } }
    lv_obj_add_event_cb(song_rev_dd, song_rev_dd_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);

    // ========================================================
    // FRAME + griglia 4×8
    // ========================================================
    const int GX = 32, GY = 55, GW = 736, GH = 352;

    lv_obj_t *frame = lv_obj_create(parent);
    lv_obj_set_size(frame, GW, GH);
    lv_obj_set_pos(frame, GX, GY);
    lv_obj_set_style_bg_color(frame, lv_color_hex(0x0F0F1A), 0);
    lv_obj_set_style_border_width(frame, 2, 0);
    lv_obj_set_style_border_color(frame, lv_color_hex(0x886600), 0);
    lv_obj_set_style_radius(frame, 4, 0);
    lv_obj_set_style_pad_all(frame, 0, 0);
    lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);

    int slot_w = GW / SONG_COLS;
    int slot_h = GH / SONG_ROWS;
    int gap    = 4;
    int cw     = slot_w - gap;
    int ch     = slot_h - gap;

    song_grid_x = GX;
    song_grid_y = GY;
    song_slot_w = slot_w;
    song_slot_h = slot_h;

    for (int r = 0; r < SONG_ROWS; r++) {
        for (int c = 0; c < SONG_COLS; c++) {
            int x = c * slot_w + gap / 2;
            int y = r * slot_h + gap / 2;

            lv_obj_t *box = lv_obj_create(frame);
            lv_obj_set_size(box, cw, ch);
            lv_obj_set_pos(box, x, y);
            lv_obj_set_style_bg_color(box, lv_color_hex(0x0A0A14), 0);
            lv_obj_set_style_border_width(box, 1, 0);
            lv_obj_set_style_border_color(box, lv_color_hex(0x333333), 0);
            lv_obj_set_style_radius(box, 4, 0);
            lv_obj_set_style_pad_all(box, 0, 0);
            lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(box, LV_OBJ_FLAG_CLICKABLE);

            int slot_idx = song_cur_page * SONG_SLOTS_PER_PAGE
                         + r * SONG_COLS + c;
            lv_obj_add_event_cb(box, song_slot_cb, LV_EVENT_CLICKED,
                                (void*)(uintptr_t)slot_idx);

            // KIT (top-left, 14pt)
            lv_obj_t *lblK = lv_label_create(box);
            lv_label_set_text(lblK, "");
            lv_obj_set_style_text_color(lblK, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_font(lblK, &lv_font_montserrat_14, 0);
            lv_obj_align(lblK, LV_ALIGN_TOP_LEFT, 4, 4);

            // Numero slot (top-right, grigio 14pt)
            lv_obj_t *lblN = lv_label_create(box);
            lv_label_set_text(lblN, "");
            lv_obj_set_style_text_color(lblN, lv_color_hex(0xAAAAAA), 0);
            lv_obj_set_style_text_font(lblN, &lv_font_montserrat_14, 0);
            lv_obj_align(lblN, LV_ALIGN_TOP_RIGHT, -4, 4);

            // Tipo (center, 24pt)
            lv_obj_t *lbl = lv_label_create(box);
            lv_label_set_text(lbl, "");
            lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
            lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);

            // REV (bottom, 14pt rosso porpora)
            lv_obj_t *lblR = lv_label_create(box);
            lv_label_set_text(lblR, "");
            lv_obj_set_style_text_color(lblR, lv_color_hex(0xCC0066), 0);
            lv_obj_set_style_text_font(lblR, &lv_font_montserrat_14, 0);
            lv_obj_align(lblR, LV_ALIGN_BOTTOM_MID, 0, -2);

            song_slot[r][c]             = box;
            song_slot_lbl[r][c]         = lbl;
            song_slot_kit_lbl[r][c]     = lblK;
            song_slot_num_lbl[r][c]     = lblN;
            song_slot_rev_lbl[r][c]     = lblR;
        }
    }

    // ---- Linee orizzontali tra le righe ----
    for (int r = 1; r < SONG_ROWS; r++) {
        int ly = r * slot_h - gap;
        lv_obj_t *hline = lv_obj_create(frame);
        lv_obj_set_size(hline, GW, gap);
        lv_obj_set_pos(hline, 0, ly);
        lv_obj_set_style_bg_color(hline, lv_color_hex(0x666666), 0);
        lv_obj_set_style_bg_opa(hline, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(hline, 0, 0);
        lv_obj_set_style_radius(hline, 0, 0);
        lv_obj_set_style_pad_all(hline, 0, 0);
        lv_obj_clear_flag(hline, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(hline, LV_OBJ_FLAG_CLICKABLE);
    }

    // ---- Linea di separazione tra colonna 4 e 5 ----
    {
        const uint32_t SONG_SEP_COLOR = 0x886600;
        int sep_x = 4 * slot_w - gap / 2;
        int sep_w = gap;
        lv_obj_t *sep = lv_obj_create(frame);
        lv_obj_set_size(sep, sep_w, GH);
        lv_obj_set_pos(sep, sep_x, 0);
        lv_obj_set_style_bg_color(sep, lv_color_hex(SONG_SEP_COLOR), 0);
        lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(sep, 0, 0);
        lv_obj_set_style_radius(sep, 0, 0);
        lv_obj_set_style_pad_all(sep, 0, 0);
        lv_obj_clear_flag(sep, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(sep, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_move_foreground(sep);
    }
    // ---- Cursore song (quadrato rosso, contorno) ----
    song_cursor = lv_obj_create(frame);
    lv_obj_set_size(song_cursor, cw + 4, ch + 4);
    lv_obj_set_pos(song_cursor, 0, 0);
    lv_obj_set_style_bg_opa(song_cursor, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(song_cursor, 3, 0);
    lv_obj_set_style_border_color(song_cursor, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_radius(song_cursor, 4, 0);
    lv_obj_set_style_pad_all(song_cursor, 0, 0);
    lv_obj_clear_flag(song_cursor, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(song_cursor, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(song_cursor, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(song_cursor);

    // ========================================================
    // BOTTOM ROW: Home | ◄ | 1/N | ► | + | - | Play | Loop | Rinomina | Salva
    // ========================================================

    // ---- Home ----
    mkbtn(parent, 5, 415, 55, 45,
          0x9B59B6, LV_SYMBOL_LEFT,
          &lv_font_montserrat_24,
          eb, -3, 6, 2);

    // ---- Prev ----
    lv_obj_t *prev = lv_btn_create(parent);
    lv_obj_set_size(prev, 45, 45);
    lv_obj_set_pos(prev, 70, 415);
    lv_obj_set_style_bg_color(prev, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(prev, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_radius(prev, 6, 0);
    lv_obj_set_style_border_width(prev, 2, 0);
    lv_obj_set_style_border_color(prev, lv_color_hex(0x00DD00), 0);
    lv_obj_t *pl = lv_label_create(prev);
    lv_label_set_text(pl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(pl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(pl, &lv_font_montserrat_20, 0);
    lv_obj_center(pl);
    lv_obj_add_event_cb(prev, song_page_prev_cb, LV_EVENT_CLICKED, NULL);

    song_page_lbl = lv_label_create(parent);
    lv_label_set_text(song_page_lbl, "1/1");
    lv_obj_set_style_text_color(song_page_lbl, lv_color_hex(0x00DD00), 0);
    lv_obj_set_style_text_font(song_page_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(song_page_lbl, 122, 427);

    // ---- Next ----
    lv_obj_t *next = lv_btn_create(parent);
    lv_obj_set_size(next, 45, 45);
    lv_obj_set_pos(next, 170, 415);
    lv_obj_set_style_bg_color(next, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(next, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_radius(next, 6, 0);
    lv_obj_set_style_border_width(next, 2, 0);
    lv_obj_set_style_border_color(next, lv_color_hex(0x00DD00), 0);
    lv_obj_t *nl = lv_label_create(next);
    lv_label_set_text(nl, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(nl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(nl, &lv_font_montserrat_20, 0);
    lv_obj_center(nl);
    lv_obj_add_event_cb(next, song_page_next_cb, LV_EVENT_CLICKED, NULL);

    // ---- + ----
    mkbtn(parent, 230, 415, 50, 45,
          0x00FF88, "+",
          &lv_font_montserrat_24,
          song_add_slot_cb, 0, 6, 2);

    // ---- − ----
    mkbtn(parent, 290, 415, 50, 45,
          0xFF4444, "-",
          &lv_font_montserrat_24,
          song_rm_slot_cb, 0, 6, 2);

    // ---- Play/Stop ----
    song_play_btn = lv_btn_create(parent);
    lv_obj_set_size(song_play_btn, 70, 45);
    lv_obj_set_pos(song_play_btn, 360, 415);
    lv_obj_set_style_bg_color(song_play_btn, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(song_play_btn, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_radius(song_play_btn, 6, 0);
    lv_obj_set_style_border_width(song_play_btn, 2, 0);
    lv_obj_set_style_border_color(song_play_btn, lv_color_hex(0x00FF00), 0);
    song_play_lbl = lv_label_create(song_play_btn);
    lv_label_set_text(song_play_lbl, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_color(song_play_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(song_play_lbl, &lv_font_montserrat_24, 0);
    lv_obj_center(song_play_lbl);
    lv_obj_add_event_cb(song_play_btn, song_play_btn_cb, LV_EVENT_CLICKED, NULL);

    // ---- Loop / 1Shot ----
    song_loop_btn = lv_btn_create(parent);
    lv_obj_set_size(song_loop_btn, 80, 45);
    lv_obj_set_pos(song_loop_btn, 435, 415);
    lv_obj_set_style_bg_color(song_loop_btn, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(song_loop_btn, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_radius(song_loop_btn, 6, 0);
    lv_obj_set_style_border_width(song_loop_btn, 2, 0);
    lv_obj_set_style_border_color(song_loop_btn, lv_color_hex(0x0099FF), 0);
    song_loop_lbl = lv_label_create(song_loop_btn);
    lv_label_set_text(song_loop_lbl, song_loop ? "Loop" : "1Shot");
    lv_obj_set_style_text_color(song_loop_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(song_loop_lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(song_loop_lbl);
    lv_obj_add_event_cb(song_loop_btn, song_loop_btn_cb, LV_EVENT_CLICKED, NULL);

    // ---- Rinomina ----
    mkbtn(parent, 520, 415, 120, 45,
          0x00DD00, "RINOMINA",
          &lv_font_montserrat_14,
          song_rename_btn_cb, 0, 6, 2);

    // ---- Salva ----
    mkbtn(parent, 655, 415, 110, 45,
          0xFF4444, "Salva",
          &lv_font_montserrat_16,
          song_save_btn_cb, 0, 6, 2);

    song_grid_update_all();
    song_sec_apply_visual();
}