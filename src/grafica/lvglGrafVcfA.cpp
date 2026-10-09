// ============================================================
// lvglGrafVcfA.cpp — SynthA VCF page
// ============================================================
#include "globals.h"
#include "lvglGraf.h"
#include "lvglGraf_internal.h"
#include "lvgl_v8_port.h"
#include "src/preset/preset_ui.h"
#include <math.h>

// ============================================================
// Stato locale
// ============================================================
static lv_obj_t          *vcfA_env_chart = nullptr;
static lv_chart_series_t *vcfA_env_serie = nullptr;

// ============================================================
// Plotter ADSR
// ============================================================
void vcfA_env_plot_update() {
    if (!vcfA_env_chart || !lv_obj_is_valid(vcfA_env_chart)) return;
    if (!vcfA_env_serie) return;

    int cA = (slider_objs[0] && lv_obj_is_valid(slider_objs[0]))
             ? lv_slider_get_value(slider_objs[0]) : 0;
    int cD = (slider_objs[1] && lv_obj_is_valid(slider_objs[1]))
             ? lv_slider_get_value(slider_objs[1]) : 0;
    int cS = (slider_objs[2] && lv_obj_is_valid(slider_objs[2]))
             ? lv_slider_get_value(slider_objs[2]) : 0;
    int cR = (slider_objs[3] && lv_obj_is_valid(slider_objs[3]))
             ? lv_slider_get_value(slider_objs[3]) : 0;

    env_plot_draw(vcfA_env_chart, vcfA_env_serie, cA, cD, cS, cR);
}

static void vcfA_env_plot_create(lv_obj_t *parent) {
    const int PX = 470, PY = 58, PW = 280, PH = 100;

    vcfA_env_chart = lv_chart_create(parent);
    lv_obj_set_size(vcfA_env_chart, PW, PH);
    lv_obj_set_pos(vcfA_env_chart, PX, PY);
    lv_chart_set_type(vcfA_env_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(vcfA_env_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_point_count(vcfA_env_chart, 60);
    lv_obj_set_style_bg_color(vcfA_env_chart, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(vcfA_env_chart, 1, 0);
    lv_obj_set_style_border_color(vcfA_env_chart, lv_color_hex(0x666666), 0);
    lv_obj_set_style_radius(vcfA_env_chart, 4, 0);
    lv_chart_set_div_line_count(vcfA_env_chart, 0, 0);
    lv_obj_clear_flag(vcfA_env_chart, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(vcfA_env_chart, LV_OBJ_FLAG_CLICKABLE);

    vcfA_env_serie = lv_chart_add_series(vcfA_env_chart,
                                          lv_color_hex(0x0088FF),
                                          LV_CHART_AXIS_PRIMARY_Y);

    vcfA_env_plot_update();
}

// ============================================================
// Reset puntatori
// ============================================================
void vcfA_reset_pointers() {
    vcfA_env_chart = nullptr;
    vcfA_env_serie = nullptr;
}

// ============================================================
// Pagina VCF A
// ============================================================
void vcfA_page_create(lv_obj_t *parent) {
    // Reset locale
    vcfA_env_chart = nullptr;
    vcfA_env_serie = nullptr;

    // ========================================================
    // Frame "ENV vir" con 4 slider ADSR (blu, senza freccina)
    // ========================================================
    const int FR_X = 470, FR_Y = 160;
    const int FR_W = 280, FR_H = 290;

    lv_obj_t *frame = lv_obj_create(parent);
    lv_obj_set_size(frame, FR_W, FR_H);
    lv_obj_set_pos(frame, FR_X, FR_Y);
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
    lv_obj_align(frame_lbl, LV_ALIGN_TOP_MID, 0, -20);

    const char *adsr_labels[] = {"A", "D", "S", "R"};
    lv_color_t vcf_slider_color = lv_color_hex(0x0022CC);
    for (int i = 0; i < 4; i++) {
        int sx = 14 + i * 68;
        slider_plain(frame, sx, 26, adsr_labels[i], i,
                     vcf_slider_color, vcf_slider_color);
    }

    // ========================================================
    // Plotter ADSR (sopra la cornice)
    // ========================================================
    vcfA_env_plot_create(parent);

    // ========================================================
    // 5 arc/pot con pallino — parte sinistra, 2 righe
    //   Riga superiore: CUT, RES
    //   Riga inferiore: EnvA, EnvV, LFO
    //   Slot globali 0..4 (condivisi con MOD A, riusati qui)
    // ========================================================
    for (int i = 0; i < 5; i++) {
        g.arc_value[i]   = 50;
        g.arc_target[i]  = 50;
        g.arc_crossed[i] = false;
        g.arc_last[i]    = 50;
    }

    // Riga superiore: 2 arc centrati
    //   Area sinistra utile: x 10..460 (centro 235)
    //   2 × 90 + 40 gap = 220 → inizio x = 125
    arc_with_image(parent, 0, 125, 90, 90, 90, "CUT");
    arc_with_image(parent, 1, 255, 90, 90, 90, "RES");

    // Riga inferiore: 3 arc
    //   3 × 90 + 2 × 20 gap = 310 → inizio x = 70
    arc_with_image(parent, 2,  70, 230, 90, 90, "EnvA");
    arc_with_image(parent, 3, 180, 230, 90, 90, "EnvV");
    arc_with_image(parent, 4, 290, 230, 90, 90, "LFO");
}