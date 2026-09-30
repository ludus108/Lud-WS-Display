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
// SONG — mirror di songArr del Teensy
//   songArr[s][0][i] = tipo (0=A 1=B 2=C 3=D 4=fill)
//   songArr[s][1][i] = num  (per A/B/C/D: id song; per fill: 0..15)
//   songArr[s][2][i] = kit  (0=none, 1..16)
//   songLen[s]       = slot effettivi (1..256, mai 0)
// ============================================================
uint8_t  songArr[16][3][SONG_SLOTS] = {};
uint16_t songLen[16]                = {};
static bool    songArr_initialized = false;

uint8_t   song_cur_song = 0;
uint8_t   song_cur_page = 0;
int       song_sel_slot = -1;

lv_obj_t *song_song_dd    = nullptr;
lv_obj_t *song_fln_dd     = nullptr;
lv_obj_t *song_kit_dd     = nullptr;
lv_obj_t *song_sec_btn[4] = {nullptr,nullptr,nullptr,nullptr};
lv_obj_t *song_sec_lbl[4] = {nullptr,nullptr,nullptr,nullptr};
lv_obj_t *song_page_lbl   = nullptr;
lv_obj_t *song_slot            [SONG_ROWS][SONG_COLS] = {};
lv_obj_t *song_slot_lbl        [SONG_ROWS][SONG_COLS] = {};
lv_obj_t *song_slot_kit_lbl    [SONG_ROWS][SONG_COLS] = {};
lv_obj_t *song_slot_num_lbl    [SONG_ROWS][SONG_COLS] = {};

// Inizializza: ogni song ha almeno 1 slot = sezione A
static void songArr_init_if_needed() {
    if (songArr_initialized) return;
    for (int s = 0; s < 16; s++) {
        songLen[s]       = 1;
        songArr[s][0][0] = 0;   // tipo A
        songArr[s][1][0] = s;   // pattern = stesso id song
        songArr[s][2][0] = 1;   // kit 1 (permanente su slot 1)
        for (int i = 1; i < SONG_SLOTS; i++) {
            songArr[s][0][i] = 0;
            songArr[s][1][i] = 0;
            songArr[s][2][i] = 0;
        }
    }
    songArr_initialized = true;
    Serial.println("[DRUM] songArr inizializzato");
}

static uint8_t song_pages_count() {
    uint8_t pages = (uint8_t)((songLen[song_cur_song] + SONG_SLOTS_PER_PAGE - 1)
                    / SONG_SLOTS_PER_PAGE);
    if (pages == 0) pages = 1;
    if (pages > SONG_SLOTS / SONG_SLOTS_PER_PAGE)
        pages = SONG_SLOTS / SONG_SLOTS_PER_PAGE;
    return pages;
}

static void song_slot_update_visual(int r, int c) {
    int slot_idx = song_cur_page * SONG_SLOTS_PER_PAGE
                 + r * SONG_COLS + c;
    lv_obj_t *box  = song_slot[r][c];
    lv_obj_t *lbl  = song_slot_lbl[r][c];
    lv_obj_t *lblK = song_slot_kit_lbl[r][c];
    lv_obj_t *lblN = song_slot_num_lbl[r][c];
    if (!box || !lv_obj_is_valid(box)) return;

    bool in_song  = (slot_idx < (int)songLen[song_cur_song]);
    bool selected = (slot_idx == song_sel_slot) && in_song;

    if (!in_song) {
        if (lbl)  lv_label_set_text(lbl, "");
        if (lblK) lv_obj_add_flag(lblK, LV_OBJ_FLAG_HIDDEN);
        if (lblN) lv_label_set_text(lblN, "");
        lv_obj_set_style_bg_color(box, lv_color_hex(0x0A0A14), 0);
        lv_obj_set_style_border_color(box, lv_color_hex(0x333333), 0);
        lv_obj_set_style_border_width(box, 1, 0);
        return;
    }

    uint8_t tipo = songArr[song_cur_song][0][slot_idx];
    uint8_t num  = songArr[song_cur_song][1][slot_idx];
    uint8_t kit  = songArr[song_cur_song][2][slot_idx];

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

    if (lblK) {
        if (kit != 0) {
            char txt_kit[12];
            snprintf(txt_kit, sizeof(txt_kit), "KIT %u", (unsigned)kit);
            lv_label_set_text(lblK, txt_kit);
            lv_obj_clear_flag(lblK, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(lblK, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (lblN) {
        char txt_n[8];
        snprintf(txt_n, sizeof(txt_n), "%u", (unsigned)(slot_idx + 1));
        lv_label_set_text(lblN, txt_n);
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
    int tipo_sel = -1;
    if (song_sel_slot >= 0 && song_sel_slot < (int)songLen[song_cur_song]) {
        tipo_sel = songArr[song_cur_song][0][song_sel_slot];
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
}

static void song_set_sel_slot(uint8_t tipo, uint8_t num) {
    if (song_sel_slot < 0 || song_sel_slot >= (int)songLen[song_cur_song]) return;
    songArr[song_cur_song][0][song_sel_slot] = tipo;
    songArr[song_cur_song][1][song_sel_slot] = num;
    song_grid_update_all();
}

static void song_set_sel_kit(uint8_t kit) {
    if (song_sel_slot < 0 || song_sel_slot >= (int)songLen[song_cur_song]) return;
    if (song_sel_slot == 0) return;
    songArr[song_cur_song][2][song_sel_slot] = kit;
    song_grid_update_all();
}

static void song_song_dd_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    int sel = lv_dropdown_get_selected(dd);
    if (sel < 0) return;
    song_cur_song = (uint8_t)sel;
    song_cur_page = 0;
    song_sel_slot = -1;
    song_grid_update_all();
    song_sec_apply_visual();
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
    uint8_t kit = (sel == 0) ? 0 : (uint8_t)sel;
    song_set_sel_kit(kit);
}

static void song_add_slot_cb(lv_event_t *e) {
    (void)e;
    if (songLen[song_cur_song] >= SONG_SLOTS) return;
    uint16_t new_idx = songLen[song_cur_song];
    songLen[song_cur_song]++;
    songArr[song_cur_song][0][new_idx] = 0;
    songArr[song_cur_song][1][new_idx] = song_cur_song;
    songArr[song_cur_song][2][new_idx] = 0;

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
// Pagina SONG
// ============================================================
void song_page_create(lv_obj_t *parent) {
    songArr_init_if_needed();

    // ========================================================
    // TOP ROW: SNG | KIT | A B C D | FLN
    // ========================================================

    // ---- SNG ----
    song_song_dd = lv_dropdown_create(parent);
    lv_obj_set_size(song_song_dd, 95, 45);
    lv_obj_set_pos(song_song_dd, 5, 3);
    lv_obj_set_style_bg_color(song_song_dd, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(song_song_dd, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(song_song_dd, 2, 0);
    lv_obj_set_style_border_color(song_song_dd, lv_color_hex(0x00DD00), 0);
    lv_obj_set_style_radius(song_song_dd, 6, 0);
    lv_obj_set_style_text_color(song_song_dd, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(song_song_dd, &lv_font_montserrat_24, 0);
    lv_obj_set_style_pad_left(song_song_dd, 6, 0);
    lv_obj_set_style_pad_right(song_song_dd, 4, 0);
    lv_dropdown_set_options(song_song_dd,
        "SNG 1\nSNG 2\nSNG 3\nSNG 4\nSNG 5\nSNG 6\nSNG 7\nSNG 8\n"
        "SNG 9\nSNG 10\nSNG 11\nSNG 12\nSNG 13\nSNG 14\nSNG 15\nSNG 16");
    lv_dropdown_set_selected(song_song_dd, song_cur_song);
    lv_dropdown_set_symbol(song_song_dd, NULL);
    { lv_obj_t *l = lv_dropdown_get_list(song_song_dd);
      if (l) { lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
               lv_obj_set_style_bg_color(l, lv_color_hex(0x1A1A2E), 0);
               lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), 0);
               lv_obj_set_style_max_height(l, 380, 0); } }
    lv_obj_add_event_cb(song_song_dd, song_song_dd_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);

    // ---- KIT (16 item, "NO KIT" + "n Nome") ----
    song_kit_dd = lv_dropdown_create(parent);
    lv_obj_set_size(song_kit_dd, 180, 45);
    lv_obj_set_pos(song_kit_dd, 105, 3);
    lv_obj_set_style_bg_color(song_kit_dd, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_bg_color(song_kit_dd, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(song_kit_dd, 2, 0);
    lv_obj_set_style_border_color(song_kit_dd, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(song_kit_dd, 6, 0);
    lv_obj_set_style_text_color(song_kit_dd, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(song_kit_dd, &lv_font_montserrat_16, 0);
    lv_obj_set_style_pad_left(song_kit_dd, 6, 0);
    lv_obj_set_style_pad_right(song_kit_dd, 4, 0);
    {
        static char kit_opts[640];
        kit_opts[0] = '\0';
        strncat(kit_opts, "NO KIT",
                sizeof(kit_opts) - strlen(kit_opts) - 1);
        for (int i = 0; i < 16; i++) {
            strncat(kit_opts, "\n",
                    sizeof(kit_opts) - strlen(kit_opts) - 1);
            char line[40];
            snprintf(line, sizeof(line), "%d %s", i + 1, kit_names[i]);
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
    const int SB_W = 55, SB_H = 45, SB_GAP = 5;
    const int SB_Y = 3, SB_X0 = 300;
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
    lv_obj_set_pos(song_fln_dd, 545, 3);
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

            // Label "KIT n" (top), font 14
            lv_obj_t *lblK = lv_label_create(box);
            lv_label_set_text(lblK, "");
            lv_obj_set_style_text_color(lblK, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_font(lblK, &lv_font_montserrat_14, 0);
            lv_obj_align(lblK, LV_ALIGN_TOP_MID, 0, 4);
            lv_obj_add_flag(lblK, LV_OBJ_FLAG_HIDDEN);

            // Label tipo (center)
            lv_obj_t *lbl = lv_label_create(box);
            lv_label_set_text(lbl, "");
            lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
            lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);

            // Label numero slot (bottom), 1-based
            lv_obj_t *lblN = lv_label_create(box);
            lv_label_set_text(lblN, "");
            lv_obj_set_style_text_color(lblN, lv_color_hex(0xAAAAAA), 0);
            lv_obj_set_style_text_font(lblN, &lv_font_montserrat_16, 0);
            lv_obj_align(lblN, LV_ALIGN_BOTTOM_MID, 0, -2);

            song_slot[r][c]             = box;
            song_slot_lbl[r][c]         = lbl;
            song_slot_kit_lbl[r][c]     = lblK;
            song_slot_num_lbl[r][c]     = lblN;
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

    // ========================================================
    // BOTTOM ROW: Home | ◄ | 1/N | ► | + | -
    // ========================================================

    // ---- Home ----
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

    // ---- ◄ ----
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

    // ---- Label pagina N/M ----
    song_page_lbl = lv_label_create(parent);
    lv_label_set_text(song_page_lbl, "1/1");
    lv_obj_set_style_text_color(song_page_lbl, lv_color_hex(0x00DD00), 0);
    lv_obj_set_style_text_font(song_page_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(song_page_lbl, 122, 427);

    // ---- ► ----
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
    lv_obj_t *add_btn = lv_btn_create(parent);
    lv_obj_set_size(add_btn, 50, 45);
    lv_obj_set_pos(add_btn, 230, 415);
    lv_obj_set_style_bg_color(add_btn, lv_color_hex(0x1A6B4A), 0);
    lv_obj_set_style_bg_color(add_btn, lv_color_hex(0x0F4A2E), LV_STATE_PRESSED);
    lv_obj_set_style_radius(add_btn, 6, 0);
    lv_obj_set_style_border_width(add_btn, 2, 0);
    lv_obj_set_style_border_color(add_btn, lv_color_hex(0x00FF88), 0);
    lv_obj_t *al = lv_label_create(add_btn);
    lv_label_set_text(al, "+");
    lv_obj_set_style_text_color(al, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(al, &lv_font_montserrat_24, 0);
    lv_obj_center(al);
    lv_obj_add_event_cb(add_btn, song_add_slot_cb, LV_EVENT_CLICKED, NULL);

    // ---- − ----
    lv_obj_t *rm_btn = lv_btn_create(parent);
    lv_obj_set_size(rm_btn, 50, 45);
    lv_obj_set_pos(rm_btn, 290, 415);
    lv_obj_set_style_bg_color(rm_btn, lv_color_hex(0x6B1A1A), 0);
    lv_obj_set_style_bg_color(rm_btn, lv_color_hex(0x4A0F0F), LV_STATE_PRESSED);
    lv_obj_set_style_radius(rm_btn, 6, 0);
    lv_obj_set_style_border_width(rm_btn, 2, 0);
    lv_obj_set_style_border_color(rm_btn, lv_color_hex(0xFF4444), 0);
    lv_obj_t *rl = lv_label_create(rm_btn);
    lv_label_set_text(rl, "-");
    lv_obj_set_style_text_color(rl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(rl, &lv_font_montserrat_24, 0);
    lv_obj_center(rl);
    lv_obj_add_event_cb(rm_btn, song_rm_slot_cb, LV_EVENT_CLICKED, NULL);

    song_grid_update_all();
    song_sec_apply_visual();
}