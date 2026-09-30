// ============================================================
// lvglGrafPages.cpp — Pagine + eventi generali
// ============================================================
#include "globals.h"
#include "lvglGraf.h"
#include "lvglGraf_internal.h"
#include "lvgl_v8_port.h"
#include "src/preset/preset_ui.h"

// ============================================================
// LIFECYCLE: reset + prologo comune
// ============================================================
void reset_ui_pointers() {
    DBG("reset_ui_pointers\n");

    for (int i = 0; i < MAX_SLIDERS; i++) {
        arr[i]=0; slider_objs[i]=0; crs[i]=0; last[i]=0;
    }
    g.shape_arrow_A  = g.shape_arrow_B  = 0;
    g.shape_slider_A = g.shape_slider_B = 0;
	
	       for (int i = 0; i < 2; i++) {
        shape_data[i].cat_btn     = nullptr;
        shape_data[i].cat_btn_lbl = nullptr;
        shape_data[i].edit_btn    = nullptr;   // NUOVO
    }
	
    for (int i = 0; i < MAX_ARCS; i++) {
        g.arc_obj[i] = g.arc_arrow[i] = g.arc_label_value[i] = g.arc_label_p[i] = NULL;
    }
    pot_container = NULL;

    preset_dropdown_A = preset_dropdown_B = NULL;
    preset_label_A    = preset_label_B    = NULL;

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
    for (int r = 0; r < DRUM_ROWS; r++)
        for (int c = 0; c < DRUM_COLS; c++) {
            drum_cell[r][c] = nullptr;
            drum_dot [r][c] = nullptr;
        }
    drum_cursor       = nullptr;
    drum_play_btn     = nullptr;
    drum_play_lbl     = nullptr;
    drum_playing      = false;
    drum_step_counter = 0;

       sB_vcf_btn = sB_vcf_btn_lbl = nullptr;
    sB_sub_btn = sB_sub_lbl = nullptr;
    sB_filter_type_btn = sB_filter_type_lbl = nullptr;
    for (int i = 0; i < 3; i++) {
        sB_vcf_arc[i]       = nullptr;
        sB_vcf_arc_dot[i]   = nullptr;
        sB_vcf_arc_lbl[i]   = nullptr;
        sB_vcf_val_lbl[i]   = nullptr;
        sB_vcf_top_lbl[i]   = nullptr;
        for (int t = 0; t < VCF_TICK_COUNT; t++)
            sB_vcf_tick[i][t] = nullptr;
        for (int t = 0; t < VCF_SCALE_MAX; t++)
            sB_vcf_scale_lbl[i][t] = nullptr;
    }
    sB_wov_env_att_dd = sB_wov_env_sus_dd = sB_wov_env_rel_dd = nullptr;
    sB_wov_env_att_lbl = sB_wov_env_sus_lbl = sB_wov_env_rel_lbl = nullptr;

    pot_size_FV1 = pot_LF_FV1 = pot_HF_FV1 = nullptr;
    fx_rev_btn = fx_rev_lbl = fx_preset_dd = fx_save_btn = nullptr;
	    sB_vcf_env_chart = nullptr;
    sB_vcf_env_serie = nullptr;
	     for (int i = 0; i < 3; i++) {
        sB_vcfb_arc[i]     = nullptr;
        sB_vcfb_lbl[i]     = nullptr;
        sB_vcfb_val_lbl[i] = nullptr;
        sB_vcfb_dot[i]     = nullptr;
        sB_vcfb_target[i]  = 50;
        sB_vcfb_last[i]    = 0;
        sB_vcfb_crossed[i] = false;
        for (int t = 0; t < VCF_TICK_COUNT; t++)  sB_vcfb_tick[i][t] = nullptr;
    }
	    for (int i = 0; i < 4; i++) {
        drum_sec_btn[i] = nullptr;
        drum_sec_lbl[i] = nullptr;
    }
      for (int r = 0; r < DRUM_ROWS; r++) drum_row_meter[r] = nullptr;
    drum_ptn_dd = nullptr;
	    drum_fl_btn = nullptr;
    drum_fl_lbl = nullptr;
       // SONG
    song_song_dd  = nullptr;
    song_fln_dd   = nullptr;
    song_page_lbl = nullptr;
    for (int i = 0; i < 4; i++) {
        song_sec_btn[i] = nullptr;
        song_sec_lbl[i] = nullptr;
    }
       for (int r = 0; r < SONG_ROWS; r++)
        for (int c = 0; c < SONG_COLS; c++) {
            song_slot[r][c]             = nullptr;
            song_slot_lbl[r][c]         = nullptr;
            song_slot_kit_lbl[r][c]     = nullptr;
            song_slot_num_lbl[r][c]     = nullptr;
        }
    song_sel_slot = -1;
	    song_kit_dd = nullptr;
		    rev_mode_btn = nullptr;
    rev_mode_lbl = nullptr;
    // rev_mode NON si resetta (persiste)
	    rev_preset_dd = nullptr;
    // rev_preset NON si resetta (persiste)
	    kit_preset_dd = nullptr;
    kit_rev_dd = nullptr;
    for (int i = 0; i < 9; i++) kit_voice_dd[i] = nullptr;
    // kit_cur_kit NON si resetta (persiste tra pagine)
}

void page_begin(const char *title) {
    DBG("page_begin: %s\n", title ? title : "HOME");

    pendingPresetA = pendingPresetB = -1;

    if (blink_timer) {
        lv_timer_del(blink_timer);
        blink_timer = NULL;
        blink_state = false;
    }
    close_rename_window();
	kit_close_rename_window();

    bool isHome = (title == nullptr);
    bool isPlay = (title && strcmp(title, "PLAY") == 0);
    if (g.play && (isHome || !isPlay)) {
        stop_meters();
        g.play = 0;
    }
    if (tdt) { lv_timer_del(tdt); tdt = nullptr; }

    reset_ui_pointers();
}

// ============================================================
// PRESET LABEL (helper locale alla pagina)
// ============================================================
static lv_obj_t* create_preset_label(lv_obj_t *parent, lv_obj_t *title_lbl, bool isA) {
    lv_obj_t *info = lv_label_create(parent);
    char buf[64];
    if (isA) snprintf(buf, sizeof(buf), "Pn%d %s", presetNumA+1, nome_presetA[presetNumA]);
    else     snprintf(buf, sizeof(buf), "Pn%d %s", presetNumB+1, nome_presetB[presetNumB]);
    lv_label_set_text(info, buf);
    lv_obj_set_style_text_color(info, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(info, &lv_font_montserrat_24, 0);
    if (title_lbl) lv_obj_align_to(info, title_lbl, LV_ALIGN_OUT_RIGHT_MID, 30, 0);
    else           lv_obj_align(info, LV_ALIGN_TOP_LEFT, 250, 8);
    if (isA) preset_label_A = info;
    else     preset_label_B = info;
    return info;
}

// ============================================================
// HOME
// ============================================================
void create_home() {
    page_begin(nullptr);

    if (g.page) { lv_obj_del(g.page); g.page = 0; }
    g.synth = 0;
    g.err = g.log_v = g.toast_v = 0;
    g.log_c = g.log_f = g.toast = 0;

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

    const char *n[] = {"PLAY","SYNTH A","SYNTH B","DRUM","FX","SET UP"};
    lv_color_t cols[] = {
        lv_color_hex(0x00FF00),   // PLAY
        lv_color_hex(0xCC3300),   // SYNTH A
        lv_color_hex(0x0099FF),   // SYNTH B
        lv_color_hex(0xFFCC33),   // DRUM
        lv_color_hex(0xBB88FF),   // FX
        lv_color_hex(0x999999)    // SET UP
    };
    for (int i = 0; i < 6; i++) btn(m, n[i], 30 + i * 127, 360, 105, 90, cols[i], i);
}

// ============================================================
// MIDI CALLBACKS
// ============================================================
static void midi_split_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    midi_split = lv_dropdown_get_selected(dd);
    update_keyboard_colors();
}

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

static void disc_btn_click(lv_event_t *e) {
    (void)e;
    discover_request_restart();
}
		  
// ============================================================
// CREATE PAGE
// ============================================================
void create_page(const char *title) {
    page_begin(title);

    if (g.page) { lv_obj_del(g.page); g.page = 0; }
    g.log_c = g.log_f = g.toast = 0;
    g.toast_v = 0;

    lv_obj_t *m = lv_obj_create(lv_scr_act());
    lv_obj_set_size(m, lv_disp_get_hor_res(0), lv_disp_get_ver_res(0));
	
			  
    lv_obj_set_style_radius(m, 0, 0);
    lv_obj_set_style_bg_color(m, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(m, 0, 0);
    lv_obj_clear_flag(m, LV_OBJ_FLAG_SCROLLABLE);
    g.page = m;

        lv_obj_t *t = NULL;
    if (strncmp(title, "DCO", 3) != 0
        && strcmp(title, "SEQ")  != 0
        && strcmp(title, "SONG") != 0) {
        t = lv_label_create(m);
        lv_label_set_text(t, title);
        lv_obj_set_style_text_font(t, &lv_font_montserrat_32, 0);
        lv_obj_set_style_text_color(t, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(t, LV_ALIGN_TOP_LEFT, 10, 5);
    }

    // ---------------- PLAY ----------------
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
    // ---------------- SYNTH A / B ----------------
    else if (strcmp(title, "SYNTH A") == 0 || strcmp(title, "SYNTH B") == 0) {
        g.synth = title;
        int id = (strcmp(title, "SYNTH A") == 0) ? 0 : 1;
        const char *lbl = (id == 0) ? "Preset A" : "Preset B";
        int *ptr = (id == 0) ? &presetNumA : &presetNumB;
        char (*names)[PRESET_NAME_LEN] = (id == 0) ? nome_presetA : nome_presetB;
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
    // ---------------- DCO A / B ----------------
    else if (strncmp(title, "DCO", 3) == 0) {
        bool isA = (g.synth && strcmp(g.synth, "SYNTH A") == 0);
        int id   = isA ? SRC_A : SRC_B;
        h_slider(m, id);
        create_preset_label(m, NULL, isA);
        home_btn(m, -2);
    }
    // ---------------- MOD A / B ----------------
    else if (strncmp(title, "MOD", 3) == 0) {
        bool isA = (g.synth && strcmp(g.synth, "SYNTH A") == 0);
        if (isA) {
            create_pot_container(m, 100, 100);
            create_preset_label(m, NULL, isA);
        }
        home_btn(m, -2);
    }
    // ---------------- SET UP ----------------
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
    // ---------------- MIDI ----------------
    else if (strcmp(title, "MIDI") == 0) {
        lv_obj_t *lbl_a = lv_label_create(m);
        lv_label_set_text(lbl_a, "SynthA");
        lv_obj_set_style_text_color(lbl_a, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl_a, &lv_font_montserrat_24, 0);
        lv_obj_set_pos(lbl_a, 40, 105);

        create_keyboard(m, 380, 5, 360, 85);

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
    }        // ---- PTN (centrato) ----
     
    // ---------------- DRUM ----------------
    else if (strcmp(title, "DRUM") == 0) {
		  drum_pattern_label = lv_label_create(m);
        lv_label_set_text(drum_pattern_label, "PTN --");
        lv_obj_set_style_text_color(drum_pattern_label, lv_color_hex(0x00FFFF), 0);
        lv_obj_set_style_text_font(drum_pattern_label, &lv_font_montserrat_32, 0);
        if (t) lv_obj_align_to(drum_pattern_label, t, LV_ALIGN_OUT_RIGHT_MID, 30, 0);
        else   lv_obj_set_pos(drum_pattern_label, 130, 5);       
		
		   lv_obj_t *seq_btn = lv_btn_create(m);
        lv_obj_set_size(seq_btn, 90, 90);
        lv_obj_set_pos(seq_btn, 300, 360);
        lv_obj_set_style_bg_color(seq_btn, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_bg_color(seq_btn, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
        lv_obj_set_style_radius(seq_btn, 8, 0);
        lv_obj_set_style_border_width(seq_btn, 3, 0);
        lv_obj_set_style_border_color(seq_btn, lv_color_hex(0x00AAFF), 0);
        lv_obj_t *seq_lbl = lv_label_create(seq_btn);
        lv_label_set_text(seq_lbl, "PTN");
        lv_obj_set_style_text_color(seq_lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(seq_lbl, &lv_font_montserrat_18, 0);
        lv_obj_center(seq_lbl);
        lv_obj_add_event_cb(seq_btn, eb, LV_EVENT_CLICKED, (void*)(uintptr_t)30);

        // ---- SONG (destra di PTN) ----
        lv_obj_t *song_btn = lv_btn_create(m);
        lv_obj_set_size(song_btn, 90, 90);
        lv_obj_set_pos(song_btn, 410, 360);
        lv_obj_set_style_bg_color(song_btn, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_bg_color(song_btn, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
        lv_obj_set_style_radius(song_btn, 8, 0);
        lv_obj_set_style_border_width(song_btn, 3, 0);
        lv_obj_set_style_border_color(song_btn, lv_color_hex(0x00DD00), 0);
        lv_obj_t *song_lbl = lv_label_create(song_btn);
        lv_label_set_text(song_lbl, "SONG");
        lv_obj_set_style_text_color(song_lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(song_lbl, &lv_font_montserrat_16, 0);
        lv_obj_center(song_lbl);
        lv_obj_add_event_cb(song_btn, eb, LV_EVENT_CLICKED, (void*)(uintptr_t)32);

        // ---- KIT (arancione) ----
        lv_obj_t *kit_btn = lv_btn_create(m);
        lv_obj_set_size(kit_btn, 90, 90);
        lv_obj_set_pos(kit_btn, 530, 360);
        lv_obj_set_style_bg_color(kit_btn, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_bg_color(kit_btn, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
        lv_obj_set_style_radius(kit_btn, 8, 0);
        lv_obj_set_style_border_width(kit_btn, 3, 0);
        lv_obj_set_style_border_color(kit_btn, lv_color_hex(0xFF8800), 0);
        lv_obj_t *kit_lbl = lv_label_create(kit_btn);
        lv_label_set_text(kit_lbl, "KIT");
        lv_obj_set_style_text_color(kit_lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(kit_lbl, &lv_font_montserrat_18, 0);
        lv_obj_center(kit_lbl);
        lv_obj_add_event_cb(kit_btn, eb, LV_EVENT_CLICKED, (void*)(uintptr_t)33);

        // ---- REV (rosso scuro) ----
        lv_obj_t *rev_btn = lv_btn_create(m);
        lv_obj_set_size(rev_btn, 90, 90);
        lv_obj_set_pos(rev_btn, 640, 360);
        lv_obj_set_style_bg_color(rev_btn, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_bg_color(rev_btn, lv_color_hex(0x0F3460), LV_STATE_PRESSED);
        lv_obj_set_style_radius(rev_btn, 8, 0);
        lv_obj_set_style_border_width(rev_btn, 3, 0);
        lv_obj_set_style_border_color(rev_btn, lv_color_hex(0xAA0000), 0);
        lv_obj_t *rev_lbl = lv_label_create(rev_btn);
        lv_label_set_text(rev_lbl, "REV");
        lv_obj_set_style_text_color(rev_lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(rev_lbl, &lv_font_montserrat_18, 0);
        lv_obj_center(rev_lbl);
        lv_obj_add_event_cb(rev_btn, eb, LV_EVENT_CLICKED, (void*)(uintptr_t)34);

        home_btn(m, -1);
		
      
    }
    // ---------------- SEQ ----------------
    else if (strcmp(title, "SEQ") == 0) {
        if (t) lv_obj_set_pos(t, 10, 3);
        drum_seq_page_create(m, t);   // tutta la pagina SEQ in Modules
    }
	    // ---------------- SONG ----------------
    else if (strcmp(title, "SONG") == 0) {
        song_page_create(m);
    }
	      // ---------------- KIT ----------------
    else if (strcmp(title, "KIT") == 0) {
        kit_page_create(m);
    }
    // ---------------- REV ----------------
    else if (strcmp(title, "REV") == 0) {
        rev_page_create(m);
    }
    // ---------------- VCF / VCA ----------------
    else if (strncmp(title, "VCF", 3) == 0 || strncmp(title, "VCA", 3) == 0) {
        bool isA = (g.synth && strcmp(g.synth, "SYNTH A") == 0);
        lv_color_t active_color = isA ? lv_color_hex(COLOR_SLIDER_SYNTH_A_ACTIVE)
                                      : lv_color_hex(COLOR_SLIDER_SYNTH_B_ACTIVE);
        lv_color_t passed_color = isA ? lv_color_hex(COLOR_SLIDER_SYNTH_A_PASSED)
                                      : lv_color_hex(COLOR_SLIDER_SYNTH_B_PASSED);
        int base = (strncmp(title, "VCF", 3) == 0) ? 0 : 4;

         // ---------- Cornice grigia con label "ENV vir" ----------
        lv_obj_t *frame = lv_obj_create(m);
        lv_obj_set_size(frame, 280, 290);      // compattato (era 300x300)
        lv_obj_set_pos(frame, 470, 160);       // bottom = 450 (bottom di home)
        lv_obj_set_style_bg_opa(frame, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(frame, 1, 0);
        lv_obj_set_style_border_color(frame, lv_color_hex(0x888888), 0);
        lv_obj_set_style_radius(frame, 6, 0);
        lv_obj_set_style_pad_all(frame, 0, 0);
        lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(frame, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        lv_obj_set_style_clip_corner(frame, 0, 0);

        lv_obj_t *frame_lbl = lv_label_create(frame);
        lv_label_set_text(frame_lbl, "ENV vir");
        lv_obj_set_style_text_color(frame_lbl, lv_color_hex(0xCCCCCC), 0);
        lv_obj_set_style_text_font(frame_lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_bg_color(frame_lbl, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(frame_lbl, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_hor(frame_lbl, 8, 0);
        lv_obj_align(frame_lbl, LV_ALIGN_TOP_MID, 0, -20);   // sopra il bordo

              // ---------- 4 slider dentro la cornice ----------
        // 4 × 48 + 3 gap × 20 = 252 → start = (280 - 252) / 2 = 14
        const char *labels[] = {"A", "D", "S", "R"};
        lv_color_t vcfb_slider_color = lv_color_hex(0x0022CC);   // blu VCF-B
		
        for (int i = 0; i < 4; i++) {
            int sx = 14 + i * 68;
            if (!isA && base == 0) {
                // VCF B: senza freccina, indicator blu
                slider_plain(frame, sx, 26, labels[i],
                             base + i, vcfb_slider_color, vcfb_slider_color);
            } else {
                // VCF A, VCA A/B: invariato
                slider(frame, sx, 26, labels[i],
                       base + i, active_color, passed_color);
            }
        }

        // VCF B: modulo dedicato
        if (!isA && base == 0) {
            vcf_page_synthb_create(m);
        }

        create_preset_label(m, t, isA);
        home_btn(m, -2);
    }
    // ---------------- FX ----------------
    else if (strcmp(title, "FX") == 0) {
        fx_page_create(m);
        home_btn(m, -1);
    }
	    // ---------------- FM EDIT ----------------
    else if (strcmp(title, "FM Edit") == 0) {
        lv_obj_t *lb = lv_label_create(m);
        lv_label_set_text(lb, "FM Edit (work in progress)");
        lv_obj_set_style_text_font(lb, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(lb, lv_color_hex(0xAAAAAA), 0);
        lv_obj_align(lb, LV_ALIGN_CENTER, 0, -40);
        home_btn(m, -4);
    }
    // ---------------- DLY ----------------
    else if (strncmp(title, "DLY", 3) == 0) {
        lv_obj_t *lb = lv_label_create(m);
        lv_label_set_text(lb, "Delay (work in progress)");
        lv_obj_set_style_text_font(lb, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(lb, lv_color_hex(0xAAAAAA), 0);
        lv_obj_align(lb, LV_ALIGN_CENTER, 0, -40);
        home_btn(m, -2);
    }
    // ---------------- fallback ----------------
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

// ============================================================
// EVENT HANDLER GENERALI
// ============================================================
void eb(lv_event_t *e) {
    int id = (int)(uintptr_t)lv_event_get_user_data(e);
    if (g.play && (id == -1 || (id >= 0 && id < 6))) { stop_meters(); g.play = 0; }

    if (id == -2) {
        if (g.synth) {
            log_add("<- Torno al synth", lv_color_hex(0xFFFFFF));
            lvgl_port_lock(-1); create_page(g.synth); lvgl_port_unlock();
        } else {
            log_add("<- HOME", lv_color_hex(0xFFFFFF));
            lvgl_port_lock(-1); create_home(); lvgl_port_unlock();
        }
        return;
    }
    if (id == -1) {
        log_add("<- HOME", lv_color_hex(0xFFFFFF));
        lvgl_port_lock(-1); create_home(); lvgl_port_unlock();
        return;
    }
    if (id >= 10 && id <= 14) {
        const char *sub[] = {"DCO","VCF","MOD","VCA","DLY"};
        char title[20];
        if (g.synth) {
            const char *sfx = (strcmp(g.synth, "SYNTH A") == 0) ? "A" : "B";
            snprintf(title, 20, "%s %s", sub[id-10], sfx);
        } else snprintf(title, 20, "%s", sub[id-10]);
        log_add(title, lv_color_hex(0xFFFFFF));
        lvgl_port_lock(-1); create_page(title); lvgl_port_unlock();
        return;
    }
    if (id == 21) { save_all_settings(); return; }
    if (id == 20) {
        log_add("Apro: MIDI", lv_color_hex(0xFFFFFF));
        lvgl_port_lock(-1); create_page("MIDI"); lvgl_port_unlock();
        return;
    }
    if (id == 30) {
        log_add("Apro: SEQ", lv_color_hex(0xFFFFFF));
        lvgl_port_lock(-1); create_page("SEQ"); lvgl_port_unlock();
        return;
    }
	    if (id == 32) {
        log_add("Apro: SONG", lv_color_hex(0xFFFFFF));
        lvgl_port_lock(-1); create_page("SONG"); lvgl_port_unlock();
        return;
    }
	    if (id == 33) {
        log_add("Apro: KIT", lv_color_hex(0xFFFFFF));
        lvgl_port_lock(-1); create_page("KIT"); lvgl_port_unlock();
        return;
    }
    if (id == 34) {
        log_add("Apro: REV", lv_color_hex(0xFFFFFF));
        lvgl_port_lock(-1); create_page("REV"); lvgl_port_unlock();
        return;
    }
    if (id == 31) {
        send_param_update(ID_TEENSY, 'Y', 0);
        log_add("Pattern salvato -> Teensy", lv_color_hex(0x00FF00));
        toast_show("Pattern salvato!", lv_color_hex(0x00FF00), TOAST_DUR);
        return;
    }
    if (id == -3) {
        log_add("<- DRUM", lv_color_hex(0xFFFFFF));
        lvgl_port_lock(-1); create_page("DRUM"); lvgl_port_unlock();
        return;
    }
	    // FM Edit → torna a DCO A/B
    if (id == -4) {
        if (g.synth) {
            const char *sfx = (strcmp(g.synth, "SYNTH A") == 0) ? "A" : "B";
            char title[16];
            snprintf(title, sizeof(title), "DCO %s", sfx);
            log_add("<- DCO", lv_color_hex(0xFFFFFF));
            lvgl_port_lock(-1); create_page(title); lvgl_port_unlock();
        } else {
            lvgl_port_lock(-1); create_home(); lvgl_port_unlock();
        }
        return;
    }
    if (id >= 0 && id < 6) {
        const char *pages[] = {"PLAY","SYNTH A","SYNTH B","DRUM","FX","SET UP"};
        char b[20]; snprintf(b, 20, "> Apro: %s", pages[id]);
        log_add(b, lv_color_hex(0xFFFFFF));
        lvgl_port_lock(-1); create_page(pages[id]); lvgl_port_unlock();
    }
}

void ebright(lv_event_t *e) { set_bright(lv_slider_get_value(lv_event_get_target(e))); }


void eslider(lv_event_t *e) {
    SliderData *d = (SliderData*)lv_event_get_user_data(e);
    int cur  = lv_slider_get_value(lv_event_get_target(e));
    int prev = last[d->idx];
    int target = g.pre[d->idx];
    lv_obj_t *a = arr[d->idx];
    bool has_arrow = (a && lv_obj_is_valid(a));

    if (has_arrow) {
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
    } else {
        // Nessuna freccina → il valore viene inviato sempre
        arr[d->idx] = 0;
        update_slider_parameter(d->idx, cur);
    }
    last[d->idx] = cur;

    // VCA slider: aggiorna plotter envelope VCA
    if (d->idx >= 4 && d->idx <= 7) {
        bool isA = (g.synth && strcmp(g.synth, "SYNTH A") == 0);
        update_env_plot(isA);
    }

    // VCF slider (0..3): aggiorna il plotter ADSR di VCF-B (no-op se non presente)
    if (d->idx >= 0 && d->idx <= 3) {
        sB_vcf_env_plot_update();
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
    switch (cat) {
        case CAT_WF: items = WF_ITEMS; break;
        case CAT_FM: items = FM_ITEMS; break;
        case CAT_AM: items = AM_ITEMS; break;
        default:     items = WF_ITEMS; break;
    }
    d->list_items = items;
    populate_grid(d->list, items, 0, synth_id);
    update_leds(d->leds, cat);

    if (synth_id == SRC_A) {
        g.wa = (cat == CAT_WF) ? 0 : (cat == CAT_FM ? 9 : 17);
    } else {
        g.wb = 0;
        uiSetParamB_U8('m', (uint8_t)cat);
        uint8_t firstWave;
        if (cat == CAT_WF) firstWave = ui2fw_wave[0];
        else               firstWave = 0;
        uiSetParamB_U8('w', firstWave);
    }
    update_plotter_by_wave(synth_id);
}

// ============================================================
// Bottone unico WF/FM/AM ciclico (DCO A/B)
// Cicla: WF → FM → AM → WF, aggiorna testo, bordo e contenuto griglia.
// ============================================================

void ecat_cycle(lv_event_t *e) {
    int synth_id = (int)(uintptr_t)lv_event_get_user_data(e);
    int idx = (synth_id == SRC_A) ? 0 : 1;
    ShapeData *d = &shape_data[idx];
    if (!d->list) return;

    int cat = (d->current_cat + 1) % 3;
    d->current_cat = cat;

    const char *items;
    switch (cat) {
        case CAT_WF: items = WF_ITEMS; break;
        case CAT_FM: items = FM_ITEMS; break;
        case CAT_AM: items = AM_ITEMS; break;
        default:     items = WF_ITEMS; break;
    }
    d->list_items = items;
    populate_grid(d->list, items, 0, synth_id);

    uint32_t color; const char *name;
    switch (cat) {
        case CAT_FM: color = 0xFF8800; name = "FM"; break;
        case CAT_AM: color = 0xAA44FF; name = "AM"; break;
        default:     color = 0x00AAFF; name = "WF"; break;
    }
    if (d->cat_btn && lv_obj_is_valid(d->cat_btn))
        lv_obj_set_style_border_color(d->cat_btn, lv_color_hex(color), 0);
    if (d->cat_btn_lbl && lv_obj_is_valid(d->cat_btn_lbl))
        lv_label_set_text(d->cat_btn_lbl, name);

    // Mostra Edit solo in FM
    if (d->edit_btn && lv_obj_is_valid(d->edit_btn)) {
        if (cat == CAT_FM) lv_obj_clear_flag(d->edit_btn, LV_OBJ_FLAG_HIDDEN);
        else               lv_obj_add_flag  (d->edit_btn, LV_OBJ_FLAG_HIDDEN);
    }

    if (synth_id == SRC_A) {
        g.wa = (cat == CAT_WF) ? 0 : (cat == CAT_FM ? 9 : 17);
    } else {
        g.wb = 0;
        uiSetParamB_U8('m', (uint8_t)cat);
        uint8_t firstWave = (cat == CAT_WF) ? ui2fw_wave[0] : 0;
        uiSetParamB_U8('w', firstWave);
    }
    update_plotter_by_wave(synth_id);
}

// ============================================================
// Apri sotto-pagina "FM Edit"
// ============================================================
void fm_edit_btn_cb(lv_event_t *e) {
    (void)e;
    log_add("Apro: FM Edit", lv_color_hex(0xFFFFFF));
    lvgl_port_lock(-1);
    create_page("FM Edit");
    lvgl_port_unlock();
}

void ehslider(lv_event_t *e) {
    ShapeData *d = (ShapeData*)lv_event_get_user_data(e);
    lv_obj_t *slider = lv_event_get_target(e);
    int cur = lv_slider_get_value(slider);

    if (d->id == SRC_A) {
        g.shape_a = cur;
    } else {
        g.shape_b = cur;
        int32_t modInB = map(cur, 0, 100, 0, 1023);
        uiSetParamB_I32('i', modInB);
    }

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
        else                g.shape_arrow_B = 0;
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
    update_plotter_by_wave(id);
}