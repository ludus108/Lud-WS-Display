// ============================================================
// lvglGrafFmEdit.cpp — FM Edit (DCO A / DCO B)
// ============================================================
#include "globals.h"
#include "lvglGraf.h"
#include "lvglGraf_internal.h"
#include "lvgl_v8_port.h"
#include "src/preset/preset_ui.h"
#include <math.h>

// ============================================================
// Stato
// ============================================================
static bool      fm_is_A       = true;
static uint8_t   fm_cur_preset = 0;   // 0..7

static lv_obj_t  *fm_dd        = nullptr;
static lv_obj_t  *fm_arc[6] = {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};
static lv_obj_t  *fm_val[6] = {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};
static lv_obj_t  *fm_chart     = nullptr;
static lv_chart_series_t *fm_serie = nullptr;

// Ordine slider: Sin1, Sin2, Sin3, Div1, Div2, Div3
static const char *fm_names[6] = {"Sin1","Sin2","Sin3","Div1","Div2","Div3"};

// ============================================================
// Plotter — replica di plot_apply_fm con l'fmSel corrente
// ============================================================
void fmEdit_update_plot() {
    if (!fm_chart || !lv_obj_is_valid(fm_chart)) return;
    if (!fm_serie) return;

    const float PIx2 = 2.0f * M_PI;
    float mm_f = 60.0f;   // indice fisso per visualizzazione (dà buona visibilità)

    float r0 = (float)fmSetSin[fm_cur_preset][0] /
               (float)fmSetDiv[fm_cur_preset][0];
    float r1 = (float)fmSetSin[fm_cur_preset][1] /
               (float)fmSetDiv[fm_cur_preset][1];
    float r2 = (float)fmSetSin[fm_cur_preset][2] /
               (float)fmSetDiv[fm_cur_preset][2];

    // Topologia 3-op FM (case 0)
    for (int i = 0; i < 256; i++) {
        float v = sinf(PIx2*i/256
            + mm_f/128 * sinf(PIx2*r0*i/256
            + mm_f/128 * sinf(PIx2*r1*i/256
            + mm_f/128 * sinf(PIx2*r2*i/256)))) * 511.0f;
        // Normalizza -511..511 → 0..100
        int y = (int)((v + 511.0f) * 100.0f / 1022.0f);
        if (y < 0)   y = 0;
        if (y > 100) y = 100;
        lv_chart_set_next_value(fm_chart, fm_serie, y);
    }
    lv_chart_refresh(fm_chart);
}

// ============================================================
// Aggiorna label valori slider
// ============================================================

static void fm_update_slider_labels() {
    for (int i = 0; i < 6; i++) {
        if (!fm_val[i] || !lv_obj_is_valid(fm_val[i])) continue;
        int v = fm_arc[i] && lv_obj_is_valid(fm_arc[i])
                ? lv_arc_get_value(fm_arc[i]) : 0;
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", v);
        lv_label_set_text(fm_val[i], buf);
    }
}
// ============================================================
// Applica preset corrente → carica valori negli slider + plot
// ============================================================

static void fm_apply_preset() {
    for (int i = 0; i < 3; i++) {
        int sinv = fmSetSin[fm_cur_preset][i];
        int divv = fmSetDiv[fm_cur_preset][i];
        if (divv < 1) divv = 1;

        if (fm_arc[i] && lv_obj_is_valid(fm_arc[i]))
            lv_arc_set_value(fm_arc[i], sinv);
        if (fm_arc[3+i] && lv_obj_is_valid(fm_arc[3+i]))
            lv_arc_set_value(fm_arc[3+i], divv);
    }
    fm_update_slider_labels();
    fmEdit_update_plot();
}

// ============================================================
// Callback slider
// ============================================================
struct FmSliderCtx { int idx; };  // idx: 0..2 = Sin, 3..5 = Div
static FmSliderCtx fm_ctx[6];

static void fm_slider_cb(lv_event_t *e) {
    lv_obj_t *s = lv_event_get_target(e);
    FmSliderCtx *ctx = (FmSliderCtx*)lv_event_get_user_data(e);
    if (!ctx) return;
    int i = ctx->idx;
    if (i < 0 || i > 5) return;

       int v = lv_arc_get_value(s);

    // Salva in fmSetSin/fmSetDiv
    if (i < 3) {
        fmSetSin[fm_cur_preset][i] = (uint8_t)v;
    } else {
        int divv = (v < 1) ? 1 : v;   // Div minimo 1
        fmSetDiv[fm_cur_preset][i - 3] = (uint8_t)divv;
    }

    // Aggiorna label valore
    if (fm_val[i] && lv_obj_is_valid(fm_val[i])) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", v);
        lv_label_set_text(fm_val[i], buf);
    }

    // Ridisegna plot
    fmEdit_update_plot();
}

// ============================================================
// Dropdown FM preset
// ============================================================
static void fm_dd_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    int sel = lv_dropdown_get_selected(dd);
    if (sel < 0 || sel > 7) return;
    fm_cur_preset = (uint8_t)sel;
    fm_apply_preset();
}

// ============================================================
// Salva
// ============================================================
static void fm_save_cb(lv_event_t *e) {
    (void)e;
    // TODO: SD + invio LWS
    log_add("FM salvato", lv_color_hex(0x00FF00));
    toast_show("FM salvato!", lv_color_hex(0x00FF00), TOAST_DUR);
}

// ============================================================
// Creazione slider senza freccina (con nome sopra, valore sotto)
// ============================================================
static void fm_arc_create(lv_obj_t *parent, int idx,
                           int x, int y, int w, int h,
                           const char *name, uint32_t col) {
    // Valore corrente
    int v = (idx < 3) ? fmSetSin[fm_cur_preset][idx]
                      : fmSetDiv[fm_cur_preset][idx - 3];
    if (idx >= 3 && v < 1) v = 1;

    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, w, h);
    lv_obj_set_pos(arc, x, y);
    lv_arc_set_range(arc, 0, 20);
    lv_arc_set_value(arc, v);

    lv_obj_set_style_arc_img_src(arc, &img_arc_bg,    LV_PART_MAIN);
    lv_obj_set_style_arc_img_src(arc, &img_arc_indic, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color  (arc, lv_color_hex(col), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa     (arc, LV_OPA_TRANSP,  LV_PART_KNOB);
    fm_arc[idx] = arc;

    // Nome (centro)
    lv_obj_t *plabel = lv_label_create(parent);
    lv_label_set_text(plabel, name);
    lv_obj_set_style_text_color(plabel, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_style_text_font (plabel, &lv_font_montserrat_16, 0);
    lv_obj_align_to(plabel, arc, LV_ALIGN_CENTER, 0, 0);

    // Valore (sotto)
    lv_obj_t *val_lbl = lv_label_create(parent);
    lv_obj_set_style_text_color(val_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font (val_lbl, &lv_font_montserrat_18, 0);
    lv_label_set_text_fmt(val_lbl, "%d", v);
    lv_obj_align_to(val_lbl, arc, LV_ALIGN_OUT_BOTTOM_MID, 0, -8);
    fm_val[idx] = val_lbl;

    fm_ctx[idx].idx = idx;
    lv_obj_add_event_cb(arc, fm_slider_cb, LV_EVENT_VALUE_CHANGED, &fm_ctx[idx]);
}
// ============================================================
// Reset puntatori
// ============================================================
void fmEdit_reset_pointers() {
    fm_dd = nullptr;
    for (int i = 0; i < 6; i++) {
        fm_arc[i] = nullptr;
        fm_val[i] = nullptr;
    }
    fm_chart = nullptr;
    fm_serie = nullptr;
}

// ============================================================
// Pagina FM Edit
// ============================================================
void fmEdit_page_create(lv_obj_t *parent, bool isA) {
    fm_is_A = isA;

    // Reset locale
    fm_dd = nullptr;
    
    fm_chart = nullptr; fm_serie = nullptr;

    // ---- Dropdown "FM 1..8" a sinistra ----
    fm_dd = styled_dropdown(parent, 10, 5, 120, 45,
                             0xFF8800,
                             &lv_font_montserrat_16,
                             "FM 1\nFM 2\nFM 3\nFM 4\n"
                             "FM 5\nFM 6\nFM 7\nFM 8");
    lv_dropdown_set_selected(fm_dd, fm_cur_preset);
    lv_obj_add_event_cb(fm_dd, fm_dd_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // ---- Salva completamente a destra ----
    mkbtn(parent, 680, 5, 110, 45,
          0xFF4444, "Salva",
          &lv_font_montserrat_16,
          fm_save_cb, 0, 6, 2);

    // ---- Plotter grande ----
    const int CH_X = 10, CH_Y = 60;
    const int CH_W = 780, CH_H = 285;

    fm_chart = lv_chart_create(parent);
    lv_obj_set_size(fm_chart, CH_W, CH_H);
    lv_obj_set_pos(fm_chart, CH_X, CH_Y);
    lv_chart_set_type(fm_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(fm_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_point_count(fm_chart, 256);
    lv_obj_set_style_bg_color(fm_chart, lv_color_hex(0x0A0A14), 0);
    lv_obj_set_style_border_width(fm_chart, 1, 0);
    lv_obj_set_style_border_color(fm_chart, lv_color_hex(0x666666), 0);
    lv_obj_set_style_radius(fm_chart, 4, 0);
    lv_chart_set_div_line_count(fm_chart, 0, 0);
    lv_obj_clear_flag(fm_chart, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(fm_chart, LV_OBJ_FLAG_CLICKABLE);

    fm_serie = lv_chart_add_series(fm_chart,
                                    lv_color_hex(0xFF8800),
                                    LV_CHART_AXIS_PRIMARY_Y);


    // ---- 6 arc/pot in basso (a partire da x=110) ----
    //   Area utile: 110..780 = 670px
    //   6 arc da 90 + 5 gap da 26 = 6*90 + 5*26 = 670
    const int ARC_W = 90;
    const int ARC_H = 90;
    const int DX    = 116;      // 90 + 26
    const int X0    = 110;
    const int ARC_Y = 370;

    // Colori: Sin → azzurro; Div → verde
    const uint32_t col_sin = 0x00AAFF;
    const uint32_t col_div = 0x00DD00;

    for (int i = 0; i < 6; i++) {
        int x = X0 + i * DX;
        uint32_t col = (i < 3) ? col_sin : col_div;
        fm_arc_create(parent, i, x, ARC_Y, ARC_W, ARC_H,
                      fm_names[i], col);
    }

    // ---- Home (fondo a sinistra) ----
    mkbtn(parent, 10, 375, 90, 90,
          0x9B59B6, LV_SYMBOL_LEFT,
          &lv_font_montserrat_32,
          eb, -4, 8, 3);

    // Applica preset corrente
    fm_apply_preset();
	
}