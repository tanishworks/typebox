/*
 * typebox — spotlight-style floating input bar for screensharing
 *
 * Build:  gcc -o typebox typebox.c $(pkg-config --cflags --libs gtk4) -O2 -lm
 *
 * Keys:
 *   Type        — append text
 *   Backspace   — delete last char
 *   Enter       — copy to clipboard, flash confirmation, clear
 *   Escape      — quit
 *   Ctrl+A      — select all
 *   Ctrl+C      — copy selection (or all)
 *   Ctrl+V      — paste
 *   Ctrl+X      — cut
 *   Ctrl+Z      — undo
 */

#include <gtk/gtk.h>
#include <gdk/gdk.h>

/* ── State ───────────────────────────────────────────────────────────── */
static GtkWidget *window;
static GtkWidget *input;        /* GtkText — handles cursor/selection natively */
static GtkWidget *hint_label;
static guint      hint_timeout_id = 0;

/* ── Hint flash ──────────────────────────────────────────────────────── */
static gboolean
hide_hint(gpointer data)
{
    (void)data;
    gtk_widget_set_opacity(hint_label, 0.0);
    hint_timeout_id = 0;
    return G_SOURCE_REMOVE;
}

static void
show_hint(const char *msg)
{
    gtk_label_set_text(GTK_LABEL(hint_label), msg);
    gtk_widget_set_opacity(hint_label, 1.0);
    if (hint_timeout_id)
        g_source_remove(hint_timeout_id);
    hint_timeout_id = g_timeout_add(700, hide_hint, NULL);
}

/* ── Copy buffer text to clipboard ──────────────────────────────────── */
static void
copy_and_clear(void)
{
    GtkEntryBuffer *buf = gtk_text_get_buffer(GTK_TEXT(input));
    const char *txt = gtk_entry_buffer_get_text(buf);
    if (!txt || *txt == '\0') return;

    GdkClipboard *cb = gdk_display_get_clipboard(gdk_display_get_default());
    gdk_clipboard_set_text(cb, txt);
    show_hint("copied!");

    gtk_entry_buffer_delete_text(buf, 0, -1);
}

/* ── Key handler (capture phase — runs before GtkText sees the key) ── */
static gboolean
on_key_pressed(GtkEventControllerKey *ctrl,
               guint keyval, guint keycode,
               GdkModifierType state, gpointer user_data)
{
    (void)ctrl; (void)keycode; (void)user_data;

    gboolean ctrl_held = (state & GDK_CONTROL_MASK) != 0;

    /* Escape — quit */
    if (keyval == GDK_KEY_Escape) {
        gtk_window_destroy(GTK_WINDOW(window));
        return TRUE;
    }

    /* Enter — copy + clear (Ctrl+Enter also works) */
    if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
        copy_and_clear();
        return TRUE;
    }

    /* Ctrl+Enter alias already covered above */
    (void)ctrl_held;

    /* Everything else (Ctrl+A, Ctrl+C, Ctrl+V, Ctrl+X, Ctrl+Z,
     * cursor movement, etc.) falls through to GtkText natively */
    return FALSE;
}

/* ── Draw: rounded rect with alpha ──────────────────────────────────── */
static void
on_draw(GtkDrawingArea *area, cairo_t *cr,
        int w, int h, gpointer data)
{
    (void)area; (void)data;

    double r = 18.0;

    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_restore(cr);

    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

    cairo_new_path(cr);
    cairo_arc(cr, r,   r,   r, M_PI,     3*M_PI/2);
    cairo_arc(cr, w-r, r,   r, 3*M_PI/2, 2*M_PI);
    cairo_arc(cr, w-r, h-r, r, 0,         M_PI/2);
    cairo_arc(cr, r,   h-r, r, M_PI/2,    M_PI);
    cairo_close_path(cr);

    cairo_set_source_rgba(cr, 0.06, 0.06, 0.08, 0.3);
    cairo_fill_preserve(cr);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.08);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    cairo_restore(cr);
}

/* ── App activate ────────────────────────────────────────────────────── */
static void
activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;

    /* ── Window ──────────────────────────────────────────────────────── */
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "typebox");
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_default_size(GTK_WINDOW(window), 720, 76);

    /* ── CSS ─────────────────────────────────────────────────────────── */
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css,
        /* Transparent window */
        "window {"
        "  background: transparent;"
        "}"

        /* GtkText node — the actual input area */
        ".typebox-input {"
        "  background: transparent;"
        "  border: none;"
        "  outline: none;"
        "  box-shadow: none;"
        "  padding: 0;"
        "  font-family: 'JetBrains Mono', 'JetBrainsMono Nerd Font', monospace;"
        "  font-size: 25px;"
        "  font-weight: 500;"
        "  color: rgba(255,255,255,0.92);"
        "  caret-color: rgba(255,255,255,0.85);"
        "}"

        /* Selection highlight */
        ".typebox-input selection {"
        "  background-color: rgba(255,255,255,0.15);"
        "  color: rgba(255,255,255,0.95);"
        "}"

        /* "copied!" hint */
        ".hint-text {"
        "  font-family: 'JetBrains Mono', monospace;"
        "  font-size: 10px;"
        "  font-weight: 400;"
        "  color: rgba(255,255,255,0.35);"
        "  letter-spacing: 0.14em;"
        "}");

    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    /* ── Canvas (glass bg) ───────────────────────────────────────────── */
    GtkWidget *canvas = gtk_drawing_area_new();
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(canvas), on_draw, NULL, NULL);

    /* ── GtkText input ───────────────────────────────────────────────── */
    input = gtk_text_new();
    gtk_widget_add_css_class(input, "typebox-input");
    gtk_widget_set_hexpand(input, TRUE);
    gtk_widget_set_valign(input, GTK_ALIGN_CENTER);
    gtk_text_set_placeholder_text(GTK_TEXT(input), "say something…");

    /* ── "copied!" label ─────────────────────────────────────────────── */
    hint_label = gtk_label_new("");
    gtk_widget_add_css_class(hint_label, "hint-text");
    gtk_widget_set_opacity(hint_label, 0.0);
    gtk_widget_set_valign(hint_label, GTK_ALIGN_END);
    gtk_widget_set_halign(hint_label, GTK_ALIGN_END);
    gtk_widget_set_margin_bottom(hint_label, 8);
    gtk_widget_set_margin_end(hint_label, 20);

    /* ── Inner box (input centred vertically) ────────────────────────── */
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_start(hbox, 28);
    gtk_widget_set_margin_end(hbox, 28);
    gtk_widget_set_valign(hbox, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(hbox), input);

    /* ── Overlay: canvas behind, widgets on top ──────────────────────── */
    GtkWidget *overlay = gtk_overlay_new();
    gtk_overlay_set_child(GTK_OVERLAY(overlay), canvas);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), hbox);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), hint_label);

    gtk_window_set_child(GTK_WINDOW(window), overlay);

    /* ── Key controller (CAPTURE so we see Enter/Esc before GtkText) ── */
    GtkEventController *key_ctrl = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(key_ctrl, GTK_PHASE_CAPTURE);
    g_signal_connect(key_ctrl, "key-pressed",
                     G_CALLBACK(on_key_pressed), NULL);
    gtk_widget_add_controller(window, key_ctrl);

    gtk_window_present(GTK_WINDOW(window));
    gtk_widget_grab_focus(input);
}

/* ── Main ─────────────────────────────────────────────────────────────── */
int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new(
        "dev.typebox", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
