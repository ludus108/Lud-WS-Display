// ============================================================
// lvglGrafVcfB.cpp — SynthB VCF page
// ============================================================
#include "globals.h"
#include "lvglGraf.h"
#include "lvglGraf_internal.h"
#include "lvgl_v8_port.h"
#include "src/preset/preset_ui.h"
#include <math.h>

// ============================================================
// SYNTHB VCF PAGE — stato globale
// ============================================================
lv_obj_t *sB_vcf_btn         = nullptr;
lv_obj_t *sB_vcf_btn_lbl     = nullptr;
uint8_t   sB_vcf_mode        = 0;

lv_obj_t *sB_sub_btn         = nullptr;
lv_obj_t *sB_sub_lbl         = nullptr;
uint8_t   sB_filter_submode  = 0;
uint8_t   sB_wovel_submode   = 0;

lv_obj_t *sB_filter_type_btn = nullptr;
lv_obj_t *sB_filter_type_lbl = nullptr;
uint8_t   sB_filter_type     = 0;

lv_obj_t *sB_vcf_arc[3]      = {nullptr,nullptr,nullptr};
lv_obj_t *sB_vcf_arc_dot[3]  = {nullptr,nullptr,nullptr};
lv_obj_t *sB_vcf_arc_lbl[3]  = {nullptr,nullptr,nullptr};
lv_obj_t *sB_vcf_val_lbl[3]  = {nullptr,nullptr,nullptr};
lv_obj_t *sB_vcf_top_lbl[3]  = {nullptr,nullptr,nullptr};
lv_obj_t *sB_vcf_tick[3][VCF_TICK_COUNT]       = {};
lv_obj_t *sB_vcf_scale_lbl[3][VCF_SCALE_MAX]   = {};
lv_obj_t *sB_vcfb_arc[3]     = {nullptr,nullptr,nullptr};
lv_obj_t *sB_vcfb_lbl[3]     = {nullptr,nullptr,nullptr};
lv_obj_t *sB_vcfb_val_lbl[3] = {nullptr,nullptr,nullptr};
lv_obj_t *sB_vcfb_dot[3]     = {nullptr,nullptr,nullptr};
lv_obj_t *sB_vcfb_tick[3][VCF_TICK_COUNT] = {};
uint8_t   sB_vcfb_target[3]  = {50, 50, 50};
int       sB_vcfb_last[3]    = {0, 0, 0};
bool      sB_vcfb_crossed[3] = {false, false, false};

uint8_t   sB_vcf_cut[3]      = {180, 180, 180};
uint8_t   sB_vcf_target[3]   = {50, 50, 50};
int       sB_vcf_last[3]     = {50, 50, 50};
bool      sB_vcf_crossed[3]  = {false, false, false};

lv_obj_t *sB_wov_env_att_dd  = nullptr;
lv_obj_t *sB_wov_env_sus_dd  = nullptr;
lv_obj_t *sB_wov_env_rel_dd  = nullptr;
lv_obj_t *sB_wov_env_att_lbl = nullptr;
lv_obj_t *sB_wov_env_sus_lbl = nullptr;
lv_obj_t *sB_wov_env_rel_lbl = nullptr;
lv_obj_t          *sB_vcf_env_chart = nullptr;
lv_chart_series_t *sB_vcf_env_serie = nullptr;

#define VCF_MODE_COLOR_FILTER   0x0A5A2A
#define VCF_MODE_COLOR_WOVEL    0xFF8800
#define FILTER_TYPE_LP_COLOR    0x0088FF
#define FILTER_TYPE_BP_COLOR    0xFF2222

#define SUB_FILTER_COLOR        0x44DD44
#define SUB_WOVEL_COLOR         0xFFDD33
#define SUB_BG_DARK             0x1A1A2E
#define SUB_BG_DARK_PRESSED     0x0F3460

#define VCF_DISABLED_LBL_COLOR  0x555555

static const char *SUB_LABELS_FILTER[3] = {"UNI","SLV","FRE"};
static const char *SUB_LABELS_WOVEL[3]  = {"POT","ENV","RND"};

// ============================================================
// ArcSet — contesto unificato per un blocco di 3 arc
//   (sostituisce le coppie sB_vcf_* / sB_vcfb_* duplicate)
// ============================================================
struct ArcSet {
    lv_obj_t **arc;         // [3]
    lv_obj_t **dot;         // [3]
    lv_obj_t **arc_lbl;     // [3]
    lv_obj_t **val_lbl;     // [3]
    lv_obj_t **top_lbl;     // [3] oppure nullptr
    lv_obj_t **tick;        // [3 * VCF_TICK_COUNT] flattened
    lv_obj_t **scale_lbl;   // [3 * VCF_SCALE_MAX] flattened oppure nullptr
    uint8_t   *target;      // [3]
    int       *last;        // [3]
    bool      *crossed;     // [3]
    uint8_t   *cut;         // [3] oppure nullptr
    char       lws_key_base;   // 0 = nessun invio LWS; '1'..'9' = key base
    bool       map_to_255;     // true = mappa cur 0..100 → 0..255 (e invia cut)
    bool       has_scale;      // true = disegna 6 scale label (solo top)
    bool       has_top;        // true = disegna top label
};

static ArcSet s_set_top = {
    sB_vcf_arc, sB_vcf_arc_dot, sB_vcf_arc_lbl, sB_vcf_val_lbl, sB_vcf_top_lbl,
    &sB_vcf_tick[0][0],
    &sB_vcf_scale_lbl[0][0],
    sB_vcf_target, sB_vcf_last, sB_vcf_crossed, sB_vcf_cut,
    '1', true, true, true
};

static ArcSet s_set_bot = {
    sB_vcfb_arc, sB_vcfb_dot, sB_vcfb_lbl, sB_vcfb_val_lbl, nullptr,
    &sB_vcfb_tick[0][0],
    nullptr,
    sB_vcfb_target, sB_vcfb_last, sB_vcfb_crossed, nullptr,
    0, false, false, false
};

// ============================================================
// Prototipi interni (unificati)
// ============================================================
static void vcf_arc_create(lv_obj_t *parent, ArcSet *set, int idx,
                            int x, int y, int w, int h, const char *pname);
static void vcf_arc_reset_default(ArcSet *set, int idx);
static void vcf_arc_disable(ArcSet *set, int idx);
static void vcf_arc_hide(ArcSet *set, int idx);
static void vcf_arc_set_top_label(ArcSet *set, int idx, const char *txt);

static void sB_vcf_apply_state();
static void sB_vcf_show_scale_letter(int i, int t, const char *txt);
static void sB_vcf_show_env_dropdowns(bool show);
static void sB_vcf_env_plot_create(lv_obj_t *parent);

// ============================================================
// Sub button + mode button
// ============================================================
static void sB_update_sub_buttons() {
    if (!sB_sub_btn || !lv_obj_is_valid(sB_sub_btn)) return;
    if (!sB_sub_lbl || !lv_obj_is_valid(sB_sub_lbl)) return;

    bool isFilter = (sB_vcf_mode == 0);
    uint8_t idx = isFilter ? sB_filter_submode : sB_wovel_submode;
    if (idx > 2) idx = 2;

    const char * const *labels = isFilter ? SUB_LABELS_FILTER : SUB_LABELS_WOVEL;
    uint32_t color = isFilter ? SUB_FILTER_COLOR : SUB_WOVEL_COLOR;

    lv_label_set_text(sB_sub_lbl, labels[idx]);
    lv_obj_set_style_text_color(sB_sub_lbl, lv_color_hex(color), 0);
    lv_obj_set_style_border_color(sB_sub_btn, lv_color_hex(color), 0);
}

static void sB_sub_btn_cb(lv_event_t *e) {
    (void)e;
    if (sB_vcf_mode == 0) {
        sB_filter_submode = (uint8_t)((sB_filter_submode + 1) % 3);
        uiSetParamB_U8('C', sB_filter_submode);
    } else {
        sB_wovel_submode = (uint8_t)((sB_wovel_submode + 1) % 3);
        uiSetParamB_U8('K', sB_wovel_submode);
    }
    sB_update_sub_buttons();
    sB_vcf_apply_state();
}

static void vcf_mode_btn_cb(lv_event_t *e) {
    (void)e;
    sB_vcf_mode = (sB_vcf_mode == 0) ? 1 : 0;
    sB_filter_type = (sB_vcf_mode == 1) ? 1 : 0;

    if (sB_filter_type_btn && lv_obj_is_valid(sB_filter_type_btn)) {
        lv_obj_set_style_border_color(sB_filter_type_btn,
            lv_color_hex(sB_filter_type == 0 ? FILTER_TYPE_LP_COLOR
                                             : FILTER_TYPE_BP_COLOR), 0);
    }
    if (sB_filter_type_lbl && lv_obj_is_valid(sB_filter_type_lbl)) {
        lv_label_set_text(sB_filter_type_lbl, sB_filter_type == 0 ? "LP" : "BP");
    }
    uiSetParamB_U8('e', sB_filter_type);

    if (sB_vcf_btn && lv_obj_is_valid(sB_vcf_btn)) {
        lv_obj_set_style_border_color(sB_vcf_btn,
            lv_color_hex(sB_vcf_mode == 0 ? VCF_MODE_COLOR_FILTER
                                          : VCF_MODE_COLOR_WOVEL), 0);
    }
    if (sB_vcf_btn_lbl && lv_obj_is_valid(sB_vcf_btn_lbl)) {
        lv_label_set_text(sB_vcf_btn_lbl, sB_vcf_mode == 0 ? "FLT" : "WOV");
    }
    sB_update_sub_buttons();
    sB_vcf_apply_state();

    uiSetParamB_U8('B', sB_vcf_mode);
    if (sB_vcf_mode == 0) uiSetParamB_U8('C', sB_filter_submode);
    else                  uiSetParamB_U8('K', sB_wovel_submode);
}

static void sB_filter_type_cb(lv_event_t *e) {
    (void)e;
    sB_filter_type = (sB_filter_type == 0) ? 1 : 0;

    if (sB_filter_type_btn && lv_obj_is_valid(sB_filter_type_btn)) {
        lv_obj_set_style_border_color(sB_filter_type_btn,
            lv_color_hex(sB_filter_type == 0 ? FILTER_TYPE_LP_COLOR
                                             : FILTER_TYPE_BP_COLOR), 0);
    }
    if (sB_filter_type_lbl && lv_obj_is_valid(sB_filter_type_lbl)) {
        lv_label_set_text(sB_filter_type_lbl, sB_filter_type == 0 ? "LP" : "BP");
    }
    uiSetParamB_U8('e', sB_filter_type);
}

static void wov_env_dd_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    int sel = lv_dropdown_get_selected(dd);
    uint8_t val = (uint8_t)map(sel, 0, 4, 0, 255);

    char key = 0;
    if      (dd == sB_wov_env_att_dd) key = 'V';
    else if (dd == sB_wov_env_sus_dd) key = 'Z';
    else if (dd == sB_wov_env_rel_dd) key = 'L';
    if (key) uiSetParamB_U8(key, val);
}

// ============================================================
// Callback arc unificata
// ============================================================
struct ArcCbData {
    ArcSet *set;
    int     idx;
};

static ArcCbData s_cb_top[3];
static ArcCbData s_cb_bot[3];

static void vcf_arc_cb(lv_event_t *e) {
    lv_obj_t *arc = lv_event_get_target(e);
    ArcCbData *cb = (ArcCbData *)lv_event_get_user_data(e);
    if (!cb || !cb->set) return;
    ArcSet *set = cb->set;
    int idx = cb->idx;
    if (idx < 0 || idx > 2) return;

    int cur    = lv_arc_get_value(arc);
    int prev   = set->last[idx];
    int target = set->target[idx];

    if (set->val_lbl[idx] && lv_obj_is_valid(set->val_lbl[idx])) {
        lv_label_set_text_fmt(set->val_lbl[idx], "%d", cur);
        if (set->arc[idx] && lv_obj_is_valid(set->arc[idx])) {
            lv_obj_update_layout(set->val_lbl[idx]);
            lv_obj_align_to(set->val_lbl[idx], set->arc[idx],
                            LV_ALIGN_OUT_BOTTOM_MID, 0, -8);
        }
    }

    if (!set->crossed[idx] &&
        set->dot[idx] && lv_obj_is_valid(set->dot[idx])) {

        bool crossed = (prev < target && cur >= target) ||
                       (prev > target && cur <= target) ||
                       (prev == target && cur != target);

        if (crossed) {
            set->crossed[idx] = true;
            lv_obj_add_flag(set->dot[idx], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_arc_color(arc,
                lv_color_hex(COLOR_ARC_PASSED), LV_PART_INDICATOR);
        } else {
            lv_obj_clear_flag(set->dot[idx], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_arc_color(arc,
                lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
        }
    }
    set->last[idx] = cur;

    if (set->map_to_255 && set->cut != nullptr) {
        set->cut[idx] = (uint8_t)map(cur, 0, 100, 0, 255);
    }
    if (set->lws_key_base != 0) {
        char key = set->lws_key_base + (char)idx;
        uint8_t value = (set->cut != nullptr) ? set->cut[idx] : (uint8_t)cur;
        uiSetParamB_U8(key, value);
    }
}

// ============================================================
// Creazione arc unificata
// ============================================================
static void vcf_arc_create(lv_obj_t *parent, ArcSet *set, int idx,
                            int x, int y, int w, int h,
                            const char *pname) {
    if (idx < 0 || idx > 2) return;

    int initVal = 0;
    if (set->cut != nullptr) {
        initVal = map(set->cut[idx], 0, 255, 0, 100);
    }
    int cx = x + w / 2;
    int cy = y + h / 2;

    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, w, h);
    lv_obj_set_pos(arc, x, y);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, initVal);

    lv_obj_set_style_arc_img_src(arc, &img_arc_bg,    LV_PART_MAIN);
    lv_obj_set_style_arc_img_src(arc, &img_arc_indic, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color  (arc, lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa     (arc, LV_OPA_TRANSP,  LV_PART_KNOB);
    set->arc[idx] = arc;

    lv_obj_t *plabel = lv_label_create(parent);
    lv_label_set_text(plabel, pname);
    lv_obj_set_style_text_color(plabel, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_style_text_font (plabel, &lv_font_montserrat_20, 0);
    lv_obj_align_to(plabel, arc, LV_ALIGN_CENTER, 0, 0);
    set->arc_lbl[idx] = plabel;

    lv_obj_t *val_lbl = lv_label_create(parent);
    lv_obj_set_style_text_color(val_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font (val_lbl, &lv_font_montserrat_20, 0);
    lv_label_set_text_fmt(val_lbl, "%d", initVal);
    lv_obj_align_to(val_lbl, arc, LV_ALIGN_OUT_BOTTOM_MID, 0, -8);
    set->val_lbl[idx] = val_lbl;

    // red dot target
    float angle = 135.0f + (set->target[idx] * 2.7f);
    if (angle >= 360.0f) angle -= 360.0f;
    if (angle <  0.0f)   angle += 360.0f;
    int radius = (w / 2) - 2 + 10;
    float rad  = angle * PI / 180.0f;
    int dx = cx + (int)(radius * cosf(rad)) - 4;
    int dy = cy + (int)(radius * sinf(rad)) - 4;

    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_pos(dot, dx, dy);
    set->dot[idx] = dot;

    // top label (solo se has_top)
    if (set->has_top && set->top_lbl != nullptr) {
        lv_obj_t *top = lv_label_create(parent);
        lv_label_set_text(top, "");
        lv_obj_set_style_text_color(top, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(top, &lv_font_montserrat_16, 0);
        lv_obj_align_to(top, arc, LV_ALIGN_OUT_TOP_MID, 0, -17);
        lv_obj_add_flag(top, LV_OBJ_FLAG_HIDDEN);
        set->top_lbl[idx] = top;
    }

    // tick radiali
    if (set->tick != nullptr) {
        const int r_out = w / 2;
        for (int t = 0; t < VCF_TICK_COUNT; t++) {
            float deg = 135.0f + t * 67.5f;
            if (deg >= 360.0f) deg -= 360.0f;
            float trad = deg * PI / 180.0f;
            float cosv = cosf(trad);
            float sinv = sinf(trad);
            int px = cx + (int)(r_out * cosv);
            int py = cy + (int)(r_out * sinv);

            lv_obj_t *tick = lv_obj_create(parent);
            lv_obj_set_size(tick, 1, 15);
            lv_obj_set_style_bg_color(tick, lv_color_hex(0x888888), 0);
            lv_obj_set_style_bg_opa(tick, LV_OPA_COVER, 0);
            lv_obj_set_style_border_width(tick, 0, 0);
            lv_obj_set_style_radius(tick, 0, 0);
            lv_obj_set_style_pad_all(tick, 0, 0);
            lv_obj_clear_flag(tick, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(tick, LV_OBJ_FLAG_CLICKABLE);

            lv_obj_set_style_transform_pivot_x(tick, 0, 0);
            lv_obj_set_style_transform_pivot_y(tick, 15, 0);
            float alpha = 90.0f + deg;
            while (alpha >= 360.0f) alpha -= 360.0f;
            while (alpha <    0.0f) alpha += 360.0f;
            lv_obj_set_style_transform_angle(tick, (int16_t)(alpha * 10.0f), 0);
            lv_obj_set_pos(tick, px, py - 15);
            set->tick[idx * VCF_TICK_COUNT + t] = tick;
        }
    }

    // scale label (solo se has_scale)
    if (set->has_scale && set->scale_lbl != nullptr) {
        const int r_scale = w / 2 + 14;
        for (int t = 0; t < VCF_SCALE_MAX; t++) {
            float deg = 135.0f + t * 54.0f;
            if (deg >= 360.0f) deg -= 360.0f;
            float trad = deg * PI / 180.0f;
            int sx = cx + (int)(r_scale * cosf(trad));
            int sy = cy + (int)(r_scale * sinf(trad));

            lv_obj_t *lbl = lv_label_create(parent);
            lv_label_set_text(lbl, "");
            lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
            lv_obj_set_pos(lbl, sx - 5, sy - 9);
            lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
            set->scale_lbl[idx * VCF_SCALE_MAX + t] = lbl;
        }
    }

    // stato iniziale
    set->crossed[idx] = false;
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(COLOR_ARC_ACTIVE), LV_PART_INDICATOR);
    set->last[idx] = initVal;

    // callback con contesto
    ArcCbData *cbd = (set == &s_set_top) ? &s_cb_top[idx] : &s_cb_bot[idx];
    cbd->set = set;
    cbd->idx = idx;
    lv_obj_add_event_cb(arc, vcf_arc_cb, LV_EVENT_VALUE_CHANGED, cbd);
}

// ============================================================
// Reset / Disable / Hide unificati
// ============================================================
static void vcf_arc_reset_default(ArcSet *set, int idx) {
    if (!set || idx < 0 || idx > 2) return;
    lv_obj_t *a = set->arc[idx];
    if (!a || !lv_obj_is_valid(a)) return;

    lv_obj_clear_flag(a, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(a, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_opa(a, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(a,
        lv_color_hex(set->crossed[idx] ? COLOR_ARC_PASSED : COLOR_ARC_ACTIVE),
        LV_PART_INDICATOR);

    if (set->arc_lbl[idx] && lv_obj_is_valid(set->arc_lbl[idx])) {
        lv_obj_clear_flag(set->arc_lbl[idx], LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(set->arc_lbl[idx], lv_color_hex(0xFFAA00), 0);
    }
    if (set->val_lbl[idx] && lv_obj_is_valid(set->val_lbl[idx])) {
        lv_obj_clear_flag(set->val_lbl[idx], LV_OBJ_FLAG_HIDDEN);
    }
    if (set->dot[idx] && lv_obj_is_valid(set->dot[idx])) {
        if (!set->crossed[idx])
            lv_obj_clear_flag(set->dot[idx], LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(set->dot[idx], LV_OBJ_FLAG_HIDDEN);
    }
    if (set->tick != nullptr) {
        for (int t = 0; t < VCF_TICK_COUNT; t++) {
            lv_obj_t *tk = set->tick[idx * VCF_TICK_COUNT + t];
            if (tk && lv_obj_is_valid(tk))
                lv_obj_clear_flag(tk, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (set->has_top && set->top_lbl != nullptr) {
        if (set->top_lbl[idx] && lv_obj_is_valid(set->top_lbl[idx]))
            lv_obj_add_flag(set->top_lbl[idx], LV_OBJ_FLAG_HIDDEN);
    }
    if (set->has_scale && set->scale_lbl != nullptr) {
        for (int t = 0; t < VCF_SCALE_MAX; t++) {
            lv_obj_t *sl = set->scale_lbl[idx * VCF_SCALE_MAX + t];
            if (sl && lv_obj_is_valid(sl))
                lv_obj_add_flag(sl, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void vcf_arc_disable(ArcSet *set, int idx) {
    if (!set || idx < 0 || idx > 2) return;
    lv_obj_t *a = set->arc[idx];
    if (!a || !lv_obj_is_valid(a)) return;

    lv_obj_set_style_arc_opa(a, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_clear_flag(a, LV_OBJ_FLAG_CLICKABLE);

    if (set->arc_lbl[idx] && lv_obj_is_valid(set->arc_lbl[idx]))
        lv_obj_set_style_text_color(set->arc_lbl[idx],
                                    lv_color_hex(VCF_DISABLED_LBL_COLOR), 0);

    if (set->dot[idx] && lv_obj_is_valid(set->dot[idx]))
        lv_obj_add_flag(set->dot[idx], LV_OBJ_FLAG_HIDDEN);

    if (set->val_lbl[idx] && lv_obj_is_valid(set->val_lbl[idx]))
        lv_obj_add_flag(set->val_lbl[idx], LV_OBJ_FLAG_HIDDEN);
}

static void vcf_arc_hide(ArcSet *set, int idx) {
    if (!set || idx < 0 || idx > 2) return;

    lv_obj_t *objs[] = {
        set->arc[idx], set->arc_lbl[idx], set->val_lbl[idx], set->dot[idx]
    };
    for (size_t k = 0; k < sizeof(objs)/sizeof(objs[0]); k++) {
        if (objs[k] && lv_obj_is_valid(objs[k]))
            lv_obj_add_flag(objs[k], LV_OBJ_FLAG_HIDDEN);
    }

    if (set->has_top && set->top_lbl != nullptr) {
        if (set->top_lbl[idx] && lv_obj_is_valid(set->top_lbl[idx]))
            lv_obj_add_flag(set->top_lbl[idx], LV_OBJ_FLAG_HIDDEN);
    }
    if (set->tick != nullptr) {
        for (int t = 0; t < VCF_TICK_COUNT; t++) {
            lv_obj_t *tk = set->tick[idx * VCF_TICK_COUNT + t];
            if (tk && lv_obj_is_valid(tk))
                lv_obj_add_flag(tk, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (set->has_scale && set->scale_lbl != nullptr) {
        for (int t = 0; t < VCF_SCALE_MAX; t++) {
            lv_obj_t *sl = set->scale_lbl[idx * VCF_SCALE_MAX + t];
            if (sl && lv_obj_is_valid(sl))
                lv_obj_add_flag(sl, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

// ============================================================
// Top label unificato
// ============================================================
static void vcf_arc_set_top_label(ArcSet *set, int idx, const char *txt) {
    if (!set || idx < 0 || idx > 2) return;
    if (!set->has_top || set->top_lbl == nullptr) return;
    if (!set->top_lbl[idx] || !lv_obj_is_valid(set->top_lbl[idx])) return;

    if (txt == nullptr) {
        lv_obj_add_flag(set->top_lbl[idx], LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_label_set_text(set->top_lbl[idx], txt);
    lv_obj_set_style_text_color(set->top_lbl[idx], lv_color_hex(0xFFFFFF), 0);
    lv_obj_clear_flag(set->top_lbl[idx], LV_OBJ_FLAG_HIDDEN);

    if (set->arc[idx] && lv_obj_is_valid(set->arc[idx])) {
        lv_obj_update_layout(set->top_lbl[idx]);
        lv_obj_align_to(set->top_lbl[idx], set->arc[idx],
                        LV_ALIGN_OUT_TOP_MID, 0, -17);
    }
}

// ============================================================
// Scale letter (solo top set, WOV POT)
// ============================================================
static void sB_vcf_show_scale_letter(int i, int t, const char *txt) {
    if (i < 0 || i > 2 || t < 0 || t >= VCF_SCALE_MAX) return;
    lv_obj_t *lbl = sB_vcf_scale_lbl[i][t];
    if (!lbl || !lv_obj_is_valid(lbl)) return;
    lv_label_set_text(lbl, txt);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_clear_flag(lbl, LV_OBJ_FLAG_HIDDEN);
}

// ============================================================
// Env dropdowns WOV
// ============================================================
static void sB_vcf_show_env_dropdowns(bool show) {
    lv_obj_t *dds[3]  = {sB_wov_env_att_dd,  sB_wov_env_sus_dd,  sB_wov_env_rel_dd};
    lv_obj_t *lbls[3] = {sB_wov_env_att_lbl, sB_wov_env_sus_lbl, sB_wov_env_rel_lbl};
    for (int i = 0; i < 3; i++) {
        if (dds[i] && lv_obj_is_valid(dds[i])) {
            if (show) lv_obj_clear_flag(dds[i], LV_OBJ_FLAG_HIDDEN);
            else      lv_obj_add_flag  (dds[i], LV_OBJ_FLAG_HIDDEN);
        }
        if (lbls[i] && lv_obj_is_valid(lbls[i])) {
            if (show) lv_obj_clear_flag(lbls[i], LV_OBJ_FLAG_HIDDEN);
            else      lv_obj_add_flag  (lbls[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

// ============================================================
// Apply state (riscritto con le funzioni unificate)
// ============================================================
static void sB_vcf_apply_state() {
    bool isFilter = (sB_vcf_mode == 0);
    uint8_t sub = isFilter ? sB_filter_submode : sB_wovel_submode;

    for (int i = 0; i < 3; i++) vcf_arc_reset_default(&s_set_top, i);
    for (int i = 0; i < 3; i++) vcf_arc_reset_default(&s_set_bot, i);

    sB_vcf_show_env_dropdowns(false);

    if (isFilter) {
        const char *top[3] = {nullptr, nullptr, nullptr};
        if (sub == 0) {
            top[0] = "CUT"; top[1] = "DET";
            vcf_arc_disable(&s_set_top, 2);
        } else if (sub == 1) {
            top[0] = "CUT"; top[1] = "INT"; top[2] = "INT";
        } else {
            top[0] = "CUT"; top[1] = "CUT"; top[2] = "CUT";
        }
        for (int i = 0; i < 3; i++)
            vcf_arc_set_top_label(&s_set_top, i, top[i]);
    } else {
        if (sub == 0) {
            const char *letters[6] = {"A","E","I","O","U","A"};
            for (int t = 0; t < 6; t++)
                sB_vcf_show_scale_letter(0, t, letters[t]);
            vcf_arc_set_top_label(&s_set_top, 1, "FORM");
            vcf_arc_disable(&s_set_top, 2);
        } else if (sub == 1) {
            vcf_arc_hide(&s_set_top, 0);
            sB_vcf_show_env_dropdowns(true);
            vcf_arc_set_top_label(&s_set_top, 1, "FORM");
            vcf_arc_set_top_label(&s_set_top, 2, "TIME");
        } else {
            vcf_arc_disable(&s_set_top, 0);
            vcf_arc_set_top_label(&s_set_top, 1, "FORM");
            vcf_arc_set_top_label(&s_set_top, 2, "TIME");
        }
    }

    if (!isFilter) {
        vcf_arc_disable(&s_set_bot, 1);
    }
}

// ============================================================
// Env dropdowns WOV — creazione
// ============================================================
static void sB_vcf_env_dropdowns_create(lv_obj_t *parent, const int arc_x[3], int arc_y) {
    (void)arc_x;
    const int dd_w    = 50;
    const int dd_h    = 45;
    const int dd_gap  = 5;
    const int dd_y    = arc_y + 20;
    const int start_x = 5;

    const char *lbl_txt[3] = {"ATT", "SUS", "REL"};
    lv_obj_t **dd_ptrs[3]  = {&sB_wov_env_att_dd,  &sB_wov_env_sus_dd,  &sB_wov_env_rel_dd};
    lv_obj_t **lbl_ptrs[3] = {&sB_wov_env_att_lbl, &sB_wov_env_sus_lbl, &sB_wov_env_rel_lbl};

    for (int i = 0; i < 3; i++) {
        int x = start_x + i * (dd_w + dd_gap);

        lv_obj_t *lbl = lv_label_create(parent);
        lv_label_set_text(lbl, lbl_txt[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_set_pos(lbl, x + dd_w/2 - 15, dd_y - 22);
        lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
        *lbl_ptrs[i] = lbl;

        lv_obj_t *dd = lv_dropdown_create(parent);
        lv_obj_set_size(dd, dd_w, dd_h);
        lv_obj_set_pos(dd, x, dd_y);
        lv_obj_set_style_bg_color(dd, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_border_width(dd, 2, 0);
        lv_obj_set_style_border_color(dd, lv_color_hex(0x666666), 0);
        lv_obj_set_style_text_color(dd, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(dd, &lv_font_montserrat_16, 0);
        lv_obj_set_style_pad_left(dd, 4, 0);
        lv_obj_set_style_pad_right(dd, 2, 0);
        lv_dropdown_set_options(dd, "A\nE\nI\nO\nU");
        lv_dropdown_set_selected(dd, 0);
        lv_obj_add_flag(dd, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_event_cb(dd, wov_env_dd_cb, LV_EVENT_VALUE_CHANGED, NULL);
        *dd_ptrs[i] = dd;
    }
}

// ============================================================
// Env plotter
// ============================================================
static void sB_vcf_env_plot_create(lv_obj_t *parent) {
    const int PX = 470, PY = 58, PW = 280, PH = 100;

    sB_vcf_env_chart = lv_chart_create(parent);
    lv_obj_set_size(sB_vcf_env_chart, PW, PH);
    lv_obj_set_pos(sB_vcf_env_chart, PX, PY);
    lv_chart_set_type(sB_vcf_env_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(sB_vcf_env_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_point_count(sB_vcf_env_chart, 60);
    lv_obj_set_style_bg_color(sB_vcf_env_chart, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(sB_vcf_env_chart, 1, 0);
    lv_obj_set_style_border_color(sB_vcf_env_chart, lv_color_hex(0x666666), 0);
    lv_obj_set_style_radius(sB_vcf_env_chart, 4, 0);
    lv_chart_set_div_line_count(sB_vcf_env_chart, 0, 0);
    lv_obj_clear_flag(sB_vcf_env_chart, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(sB_vcf_env_chart, LV_OBJ_FLAG_CLICKABLE);

    sB_vcf_env_serie = lv_chart_add_series(sB_vcf_env_chart,
                                           lv_color_hex(0x0088FF),
                                           LV_CHART_AXIS_PRIMARY_Y);

    sB_vcf_env_plot_update();
}

void sB_vcf_env_plot_update() {
    if (!sB_vcf_env_chart || !lv_obj_is_valid(sB_vcf_env_chart)) return;
    if (!sB_vcf_env_serie) return;

    int cA = (slider_objs[0] && lv_obj_is_valid(slider_objs[0]))
             ? lv_slider_get_value(slider_objs[0]) : 0;
    int cD = (slider_objs[1] && lv_obj_is_valid(slider_objs[1]))
             ? lv_slider_get_value(slider_objs[1]) : 0;
    int cS = (slider_objs[2] && lv_obj_is_valid(slider_objs[2]))
             ? lv_slider_get_value(slider_objs[2]) : 0;
    int cR = (slider_objs[3] && lv_obj_is_valid(slider_objs[3]))
             ? lv_slider_get_value(slider_objs[3]) : 0;

    env_plot_draw(sB_vcf_env_chart, sB_vcf_env_serie, cA, cD, cS, cR);
}

// ============================================================
// Pagina VCF-B
// ============================================================
void vcf_page_synthb_create(lv_obj_t *parent) {
    const int ARC_Y  = 80;
    const int ARC_Y2 = 216;
    const int BTN_W  = 90;
    const int BTN_H  = 90;
    const int BTN_Y  = 360;
    const int BTN1_X = 110;
    const int BTN2_X = 205;
    const int BTN3_X = 300;

    // ---- Bottone LP/BP ----
    sB_filter_type_btn = mkbtn(parent, BTN1_X, BTN_Y, BTN_W, BTN_H,
        (sB_filter_type == 0 ? FILTER_TYPE_LP_COLOR : FILTER_TYPE_BP_COLOR),
        "", &lv_font_montserrat_24,
        sB_filter_type_cb, 0, 8, 3);
    sB_filter_type_lbl = lv_obj_get_child(sB_filter_type_btn, 0);
    lv_label_set_text(sB_filter_type_lbl, sB_filter_type == 0 ? "LP" : "BP");

    // ---- Bottone FLT/WOV ----
    sB_vcf_btn = mkbtn(parent, BTN2_X, BTN_Y, BTN_W, BTN_H,
        (sB_vcf_mode == 0 ? VCF_MODE_COLOR_FILTER : VCF_MODE_COLOR_WOVEL),
        "", &lv_font_montserrat_24,
        vcf_mode_btn_cb, 0, 8, 3);
    sB_vcf_btn_lbl = lv_obj_get_child(sB_vcf_btn, 0);
    lv_label_set_text(sB_vcf_btn_lbl, sB_vcf_mode == 0 ? "FLT" : "WOV");

    // ---- Bottone UNI/SLV/FRE o POT/ENV/RND ----
    sB_sub_btn = lv_btn_create(parent);
    lv_obj_set_size(sB_sub_btn, BTN_W, BTN_H);
    lv_obj_set_pos(sB_sub_btn, BTN3_X, BTN_Y);
    lv_obj_set_style_bg_color(sB_sub_btn, lv_color_hex(SUB_BG_DARK), 0);
    lv_obj_set_style_bg_color(sB_sub_btn, lv_color_hex(SUB_BG_DARK_PRESSED), LV_STATE_PRESSED);
    lv_obj_set_style_radius(sB_sub_btn, 8, 0);
    lv_obj_set_style_border_width(sB_sub_btn, 3, 0);
    sB_sub_lbl = lv_label_create(sB_sub_btn);
    lv_label_set_text(sB_sub_lbl, "---");
    lv_obj_set_style_text_color(sB_sub_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(sB_sub_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(sB_sub_lbl);
    lv_obj_add_event_cb(sB_sub_btn, sB_sub_btn_cb, LV_EVENT_CLICKED, NULL);
    sB_update_sub_buttons();

    // ---- Arc F1/F2/F3 (top set) ----
    static const char *vcf_names[3] = {"F1", "F2", "F3"};
    const int arc_x[3] = { 35, 170, 305 };
    for (int i = 0; i < 3; i++) {
        vcf_arc_create(parent, &s_set_top, i,
                       arc_x[i], ARC_Y, 80, 80, vcf_names[i]);
    }

    // ---- Arc RES/EnvA/EnvV (bottom set) ----
    static const char *vcfb_names[3] = {"RES", "EnvA", "EnvV"};
    for (int i = 0; i < 3; i++) {
        vcf_arc_create(parent, &s_set_bot, i,
                       arc_x[i], ARC_Y2, 80, 80, vcfb_names[i]);
    }

    sB_vcf_env_dropdowns_create(parent, arc_x, ARC_Y);
    sB_vcf_apply_state();
    sB_vcf_env_plot_create(parent);
}