#include "screens.h"

#include <stdio.h>
#include <lvgl.h>

#include "theme.h"

namespace screens {

// Top row first: the channels that kill a 7.3 silently.
static const Channel CELL_ORDER[CH_COUNT] = {
    CH_ICP, CH_EGT, CH_FUEL, CH_EOT,
    CH_BOOST, CH_TRANS, CH_IPR, CH_MAP,
};

struct Cell {
    lv_obj_t* box;
    lv_obj_t* name;
    lv_obj_t* value;
    lv_obj_t* unit;
    lv_obj_t* bar;
    AlarmLevel shown_level;   // last styled level — restyle only on change
    bool shown_stale;
};

static Cell s_cells[CH_COUNT];          // indexed by Channel, not cell order
static lv_obj_t* s_crit_overlay = nullptr;
static lv_obj_t* s_crit_label = nullptr;
static bool s_overlay_shown = false;

static void style_cell_level(Cell& c, AlarmLevel level, bool is_stale) {
    if (level == c.shown_level && is_stale == c.shown_stale) return;
    c.shown_level = level;
    c.shown_stale = is_stale;

    lv_color_t accent = theme::ok();
    if (level == AlarmLevel::WARN) accent = theme::warn();
    if (level == AlarmLevel::CRIT) accent = theme::crit();
    if (is_stale) accent = theme::stale();

    lv_obj_set_style_text_color(c.value, is_stale ? theme::stale() : theme::text(), 0);
    lv_obj_set_style_bg_color(c.bar, accent, LV_PART_INDICATOR);
    lv_obj_set_style_border_color(c.box,
        (level >= AlarmLevel::WARN && !is_stale) ? accent : theme::cell_bg(), 0);
}

static void create_cell(lv_obj_t* parent, Channel ch, int col, int row) {
    const ChannelMeta& meta = CHANNEL_META[ch];
    Cell& c = s_cells[ch];

    c.box = lv_obj_create(parent);
    lv_obj_set_grid_cell(c.box, LV_GRID_ALIGN_STRETCH, col, 1,
                                LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_set_style_bg_color(c.box, theme::cell_bg(), 0);
    lv_obj_set_style_border_width(c.box, 2, 0);
    lv_obj_set_style_border_color(c.box, theme::cell_bg(), 0);
    lv_obj_set_style_radius(c.box, 4, 0);
    lv_obj_set_style_pad_all(c.box, 4, 0);
    lv_obj_clear_flag(c.box, LV_OBJ_FLAG_SCROLLABLE);

    c.name = lv_label_create(c.box);
    lv_label_set_text(c.name, meta.name);
    lv_obj_set_style_text_font(c.name, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(c.name, theme::text_dim(), 0);
    lv_obj_align(c.name, LV_ALIGN_TOP_LEFT, 0, 0);

    c.unit = lv_label_create(c.box);
    lv_label_set_text(c.unit, meta.unit);
    lv_obj_set_style_text_font(c.unit, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(c.unit, theme::text_dim(), 0);
    lv_obj_align(c.unit, LV_ALIGN_TOP_RIGHT, 0, 0);

    c.value = lv_label_create(c.box);
    lv_label_set_text(c.value, "---");
    lv_obj_set_style_text_font(c.value, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(c.value, theme::text(), 0);
    lv_obj_align(c.value, LV_ALIGN_CENTER, 0, 2);

    c.bar = lv_bar_create(c.box);
    lv_obj_set_size(c.bar, lv_pct(100), 6);
    lv_obj_align(c.bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    // bar works in integer permille of the display range
    lv_bar_set_range(c.bar, 0, 1000);
    lv_bar_set_value(c.bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(c.bar, theme::bar_track(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(c.bar, theme::ok(), LV_PART_INDICATOR);

    c.shown_level = AlarmLevel::OK;
    c.shown_stale = false;
}

void create() {
    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, theme::bg(), 0);
    lv_obj_set_style_pad_all(scr, 2, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    static int32_t cols[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
                              LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static int32_t rows[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    lv_obj_set_grid_dsc_array(scr, cols, rows);
    lv_obj_set_style_pad_column(scr, 2, 0);
    lv_obj_set_style_pad_row(scr, 2, 0);

    for (int i = 0; i < CH_COUNT; i++) {
        create_cell(scr, CELL_ORDER[i], i % 4, i / 4);
    }

    // Critical takeover overlay — hidden until a CRIT fires.
    s_crit_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_crit_overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(s_crit_overlay, theme::crit(), 0);
    lv_obj_set_style_bg_opa(s_crit_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_crit_overlay, 0, 0);
    lv_obj_clear_flag(s_crit_overlay, LV_OBJ_FLAG_SCROLLABLE);

    s_crit_label = lv_label_create(s_crit_overlay);
    lv_obj_set_style_text_font(s_crit_label, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(s_crit_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(s_crit_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(s_crit_label);

    lv_obj_add_flag(s_crit_overlay, LV_OBJ_FLAG_HIDDEN);
    s_overlay_shown = false;
}

static void format_value(char* buf, size_t n, const ChannelMeta& meta,
                         const ChannelReading& r, bool stale) {
    if (stale) {
        snprintf(buf, n, "---");
    } else if (meta.decimals == 0) {
        snprintf(buf, n, "%d", (int)(r.value + (r.value >= 0 ? 0.5f : -0.5f)));
    } else {
        snprintf(buf, n, "%.*f", meta.decimals, r.value);
    }
}

void update(const SensorSnapshot& snap, const AlarmEngine& alarms, uint32_t now_ms) {
    char buf[16];

    for (int i = 0; i < CH_COUNT; i++) {
        Channel ch = (Channel)i;
        Cell& c = s_cells[ch];
        const ChannelMeta& meta = CHANNEL_META[ch];
        const ChannelReading& r = snap.ch[ch];
        bool stale = snap.is_stale(ch, now_ms);

        format_value(buf, sizeof(buf), meta, r, stale);
        lv_label_set_text(c.value, buf);

        float span = meta.display_max - meta.display_min;
        int32_t permille = (int32_t)((r.value - meta.display_min) * 1000.0f / span);
        if (permille < 0) permille = 0;
        if (permille > 1000) permille = 1000;
        lv_bar_set_value(c.bar, stale ? 0 : permille, LV_ANIM_OFF);

        style_cell_level(c, alarms.level(ch), stale);
    }

    // Critical takeover: banner lists every channel currently CRIT.
    if (alarms.worst() == AlarmLevel::CRIT) {
        char msg[128];
        size_t off = snprintf(msg, sizeof(msg), "CRITICAL\n");
        for (int i = 0; i < CH_COUNT; i++) {
            Channel ch = (Channel)i;
            if (alarms.level(ch) == AlarmLevel::CRIT && off < sizeof(msg) - 32) {
                format_value(buf, sizeof(buf), CHANNEL_META[ch], snap.ch[ch],
                             snap.is_stale(ch, now_ms));
                off += snprintf(msg + off, sizeof(msg) - off, "%s %s %s\n",
                                CHANNEL_META[ch].name, buf, CHANNEL_META[ch].unit);
            }
        }
        lv_label_set_text(s_crit_label, msg);
        if (!s_overlay_shown) {
            lv_obj_clear_flag(s_crit_overlay, LV_OBJ_FLAG_HIDDEN);
            s_overlay_shown = true;
        }
    } else if (s_overlay_shown) {
        lv_obj_add_flag(s_crit_overlay, LV_OBJ_FLAG_HIDDEN);
        s_overlay_shown = false;
    }
}

} // namespace screens
