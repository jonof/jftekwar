#include "compat.h"

#include <gtk/gtk.h>

#include "baselayer.h"
#include "startwin.h"
#include "build.h"

#define TAB_CONFIG 0
#define TAB_MESSAGES 1

static struct soundQuality_t {
    int frequency;
    int samplesize;
    int channels;
} soundQualities[] = {
    { 44100, 16, 2 },
    { 22050, 16, 2 },
    { 11025, 16, 2 },
    { 0, 0, 0 },    // May be overwritten by custom sound settings.
    { 0, 0, 0 },
};

static GtkWindow *startwin;
static struct {
    GtkWidget *startbutton;
    GtkWidget *cancelbutton;

    GtkWidget *tabs;
    GtkWidget *configbox;
    GtkWidget *alwaysshowcheck;

    GtkWidget *messagestext;

    GtkWidget *vmode3dcombo;
    GtkListStore *vmode3dlist;
    GtkWidget *fullscreencheck;

    GtkWidget *usemousecheck;
    GtkWidget *usejoystickcheck;
    GtkWidget *soundqualitycombo;
    GtkListStore *soundqualitylist;

    GtkWidget *chooseimportbutton;
    GtkWidget *importinfobutton;

    GtkWindow *importstatuswindow;
    GtkWidget *importstatustext;
    GtkWidget *importstatuscancelbutton;
} controls;

static gboolean startwinloop = FALSE;
static struct startwin_settings *settings;
static gboolean quiteventonclose = FALSE;
static int retval = -1;


extern int gtkenabled;

// -- SUPPORT FUNCTIONS -------------------------------------------------------

static GObject * get_and_connect_signal(GtkBuilder *builder, const char *name, const char *signal_name, GCallback handler)
{
    GObject *object;

    object = gtk_builder_get_object(builder, name);
    if (!object) {
        buildprintf("gtk_builder_get_object: %s not found\n", name);
        return 0;
    }
    g_signal_connect(object, signal_name, handler, NULL);
    return object;
}

static void foreach_gtk_widget_set_sensitive(GtkWidget *widget, gpointer data)
{
    gtk_widget_set_sensitive(widget, (gboolean)(intptr_t)data);
}

static void populate_video_modes(gboolean firsttime)
{
    int i, mode3d = -1;
    int xdim = 0, ydim = 0, bpp = 0, fullscreen = 0;
    char modestr[64];
    int cd[] = { 32, 24, 16, 15, 8, 0 };
    GtkTreeIter iter;

    if (firsttime) {
        getvalidmodes();
        xdim = settings->xdim3d;
        ydim = settings->ydim3d;
        bpp  = settings->bpp3d;
        fullscreen = settings->fullscreen;
    } else {
        // Read back the current resolution information selected in the combobox.
        fullscreen = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controls.fullscreencheck));
        if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(controls.vmode3dcombo), &iter)) {
            gtk_tree_model_get(GTK_TREE_MODEL(controls.vmode3dlist), &iter, 1 /*index*/, &mode3d, -1);
        }
        if (mode3d >= 0) {
            xdim = validmode[mode3d].xdim;
            ydim = validmode[mode3d].ydim;
            bpp = validmode[mode3d].bpp;
        }
    }

    // Find an ideal match.
    mode3d = checkvideomode(&xdim, &ydim, bpp, fullscreen, 1);
    if (mode3d < 0) {
        for (i=0; cd[i]; ) { if (cd[i] >= bpp) i++; else break; }
        for ( ; cd[i]; i++) {
            mode3d = checkvideomode(&xdim, &ydim, cd[i], fullscreen, 1);
            if (mode3d < 0) continue;
            break;
        }
    }

    // Repopulate the list.
    gtk_list_store_clear(controls.vmode3dlist);
    for (i = 0; i < validmodecnt; i++) {
        if (validmode[i].fs != fullscreen) continue;

        sprintf(modestr, "%d \xc3\x97 %d %d-bpp",
            validmode[i].xdim, validmode[i].ydim, validmode[i].bpp);
        gtk_list_store_insert_with_values(controls.vmode3dlist,
            &iter, -1,
            0, modestr, 1, i, -1);
        if (i == mode3d) {
            gtk_combo_box_set_active_iter(GTK_COMBO_BOX(controls.vmode3dcombo), &iter);
        }
    }
}

static void populate_sound_quality(gboolean firsttime)
{
    int i, curidx = -1;
    char modestr[64];
    GtkTreeIter iter;

    if (firsttime) {
        for (i = 0; soundQualities[i].frequency > 0; i++) {
            if (soundQualities[i].frequency == settings->samplerate &&
                soundQualities[i].samplesize == settings->bitspersample &&
                soundQualities[i].channels == settings->channels) {
                curidx = i;
                break;
            }
        }
        if (curidx < 0) {
            soundQualities[i].frequency = settings->samplerate;
            soundQualities[i].samplesize = settings->bitspersample;
            soundQualities[i].channels = settings->channels;
        }
    }

    gtk_list_store_clear(controls.soundqualitylist);
    for (i = 0; soundQualities[i].frequency > 0; i++) {
        sprintf(modestr, "%d kHz, %d-bit, %s",
            soundQualities[i].frequency / 1000,
            soundQualities[i].samplesize,
            soundQualities[i].channels == 1 ? "Mono" : "Stereo");
        gtk_list_store_insert_with_values(controls.soundqualitylist,
            &iter, -1,
            0, modestr, 1, i, -1);
        if (i == curidx) {
            gtk_combo_box_set_active_iter(GTK_COMBO_BOX(controls.soundqualitycombo), &iter);
        }
    }
}

static void set_settings(struct startwin_settings *thesettings)
{
    settings = thesettings;
}

static void setup_config_mode(void)
{
    gtk_notebook_set_current_page(GTK_NOTEBOOK(controls.tabs), TAB_CONFIG);

    // Enable all the controls on the Configuration page.
    gtk_container_foreach(GTK_CONTAINER(controls.configbox),
            foreach_gtk_widget_set_sensitive, (gpointer)TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.alwaysshowcheck), settings->forcesetup);
    gtk_widget_set_sensitive(controls.alwaysshowcheck, TRUE);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.fullscreencheck), settings->fullscreen);
    gtk_widget_set_sensitive(controls.fullscreencheck, TRUE);

    populate_video_modes(TRUE);
    gtk_widget_set_sensitive(controls.vmode3dcombo, TRUE);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.usemousecheck), settings->usemouse);
    gtk_widget_set_sensitive(controls.usemousecheck, TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.usejoystickcheck), settings->usejoy);
    gtk_widget_set_sensitive(controls.usejoystickcheck, TRUE);

    populate_sound_quality(TRUE);
    gtk_widget_set_sensitive(controls.soundqualitycombo, TRUE);

    gtk_widget_set_sensitive(controls.chooseimportbutton, TRUE);
    gtk_widget_set_sensitive(controls.importinfobutton, TRUE);

    gtk_widget_set_sensitive(controls.cancelbutton, TRUE);
    gtk_widget_set_sensitive(controls.startbutton, TRUE);
}

static void setup_messages_mode(gboolean allowcancel)
{
    gtk_notebook_set_current_page(GTK_NOTEBOOK(controls.tabs), TAB_MESSAGES);

    // Disable all the controls on the Configuration page.
    gtk_container_foreach(GTK_CONTAINER(controls.configbox),
            foreach_gtk_widget_set_sensitive, (gpointer)FALSE);
    gtk_widget_set_sensitive(controls.alwaysshowcheck, FALSE);

    gtk_widget_set_sensitive(controls.chooseimportbutton, FALSE);
    gtk_widget_set_sensitive(controls.importinfobutton, FALSE);

    gtk_widget_set_sensitive(controls.cancelbutton, allowcancel);
    gtk_widget_set_sensitive(controls.startbutton, FALSE);
}

// -- EVENT CALLBACKS AND CREATION STUFF --------------------------------------

static void on_fullscreencheck_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    (void)togglebutton; (void)user_data;
    populate_video_modes(FALSE);
}

static void on_cancelbutton_clicked(GtkButton *button, gpointer user_data)
{
    (void)button; (void)user_data;
    startwinloop = FALSE;   // Break the loop.
    retval = STARTWIN_CANCEL;
    quitevent = quitevent || quiteventonclose;
}

static void on_startbutton_clicked(GtkButton *button, gpointer user_data)
{
    int mode = -1;
    GtkTreeIter iter;

    (void)button; (void)user_data;

    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(controls.vmode3dcombo), &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(controls.vmode3dlist), &iter, 1 /*index*/, &mode, -1);
    }
    if (mode >= 0) {
        settings->xdim3d = validmode[mode].xdim;
        settings->ydim3d = validmode[mode].ydim;
        settings->bpp3d = validmode[mode].bpp;
        settings->fullscreen = validmode[mode].fs;
    }

    settings->usemouse = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controls.usemousecheck));
    settings->usejoy = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controls.usejoystickcheck));

    mode = -1;
    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(controls.soundqualitycombo), &iter)) {
        gtk_tree_model_get(GTK_TREE_MODEL(controls.soundqualitylist), &iter, 1 /*index*/, &mode, -1);
    }
    if (mode >= 0) {
        settings->samplerate = soundQualities[mode].frequency;
        settings->bitspersample = soundQualities[mode].samplesize;
        settings->channels = soundQualities[mode].channels;
    }

    settings->forcesetup = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controls.alwaysshowcheck));

    startwinloop = FALSE;   // Break the loop.
    retval = STARTWIN_RUN;
}

static gboolean on_startgtk_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    (void)widget; (void)event; (void)user_data;
    startwinloop = FALSE;   // Break the loop.
    retval = STARTWIN_CANCEL;
    quitevent = quitevent || quiteventonclose;
    return TRUE;    // FALSE would let the event go through. We want the game to decide when to close.
}

static void on_importstatus_cancelbutton_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    g_cancellable_cancel((GCancellable *)user_data);
}

static int set_importstatus_text(void *text)
{
    // Called in the main thread via g_main_context_invoke in the import thread.
    gtk_label_set_text(GTK_LABEL(controls.importstatustext), text);
    free(text);
    return 0;
}

static void importmeta_progress(void *data, const char *path)
{
    // Called in the import thread.
    (void)data;
    g_main_context_invoke(NULL, set_importstatus_text, (gpointer)strdup(path));
}

static int importmeta_cancelled(void *data)
{
    // Called in the import thread.
    return g_cancellable_is_cancelled((GCancellable *)data);
}

static void import_thread_func(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
    char *filename = (char *)task_data;
    struct importdatameta meta = {
        (void *)cancellable,
        importmeta_progress,
        importmeta_cancelled
    };
    (void)source_object;
    g_task_return_int(task, ImportDataFromPath(filename, &meta));
}

static void on_chooseimportbutton_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *dialog;
    char *filename = NULL;

    (void)button; (void)user_data;

    dialog = gtk_file_chooser_dialog_new("Import game data", startwin,
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Import", GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), TRUE);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *label = gtk_label_new("Select a folder to search.");
    gtk_widget_show(label);
    gtk_widget_set_margin_top(label, 7);
    gtk_widget_set_margin_bottom(label, 7);
    gtk_container_add(GTK_CONTAINER(content), label);
    gtk_box_reorder_child(GTK_BOX(content), label, 0);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);
    }
    gtk_widget_destroy(dialog);

    if (filename) {
        GTask *task = NULL;
        GError *err = NULL;
        GCancellable *cancellable = NULL;
        gulong clickhandlerid;

        cancellable = g_cancellable_new();
        task = g_task_new(NULL, cancellable, NULL, NULL);
        g_task_set_check_cancellable(task, FALSE);

        // Pass the filename as task data.
        g_task_set_task_data(task, (gpointer)filename, NULL);

        // Connect the import status cancel button passing the GCancellable* as user data.
        clickhandlerid = g_signal_connect(controls.importstatuscancelbutton, "clicked",
            G_CALLBACK(on_importstatus_cancelbutton_clicked), (gpointer)cancellable);

        // Show the status window, run the import thread, and while it's running, pump the Gtk queue.
        gtk_widget_show(GTK_WIDGET(controls.importstatuswindow));
        g_task_run_in_thread(task, import_thread_func);
        while (!g_task_get_completed(task)) gtk_main_iteration();

        // Get the return value from the import thread, then hide the status window.
        (void) g_task_propagate_int(task, &err);
        gtk_widget_hide(GTK_WIDGET(controls.importstatuswindow));

        // Disconnect the cancel button and clean up.
        g_signal_handler_disconnect(controls.importstatuscancelbutton, clickhandlerid);
        if (err) g_error_free(err);
        g_object_unref(cancellable);
        g_object_unref(task);

        g_free(filename);
    }
}

static void on_importinfobutton_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *dialog;

    (void)button; (void)user_data;

    dialog = gtk_message_dialog_new(startwin, GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
        "JFTekWar can scan locations of your choosing for TekWar game data");
    gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(dialog),
        "Click the 'Choose a location...' button, then locate a folder to scan.\n\n"
        "Common locations to check include:\n"
        " • CD/DVD drives\n"
        " • Unzipped data from copies of the full DOS game");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static GtkWindow *create_window(void)
{
    GtkBuilder *builder = NULL;
    GError *error = NULL;
    GtkWidget *window = NULL;
    GtkImage *appicon = NULL;

    builder = gtk_builder_new();
    if (!builder) {
        goto fail;
    }
    if (!gtk_builder_add_from_resource(builder, "/startgtk.glade", &error)) {
        buildprintf("gtk_builder_add_from_resource error: %s\n", error->message);
        goto fail;
    }

    // Get the window widget.
    window = GTK_WIDGET(get_and_connect_signal(builder, "startgtk",
        "delete-event", G_CALLBACK(on_startgtk_delete_event)));
    if (!window) {
        goto fail;
    }

    // Set the appicon image.
    appicon = GTK_IMAGE(gtk_builder_get_object(builder, "appicon"));
    if (appicon) {
        gtk_image_set_from_resource(appicon, "/appicon.png");
    }

    // Get the window widgets we need and wire them up as appropriate.
    controls.startbutton = GTK_WIDGET(get_and_connect_signal(builder, "startbutton",
        "clicked", G_CALLBACK(on_startbutton_clicked)));
    controls.cancelbutton = GTK_WIDGET(get_and_connect_signal(builder, "cancelbutton",
        "clicked", G_CALLBACK(on_cancelbutton_clicked)));

    controls.tabs = GTK_WIDGET(gtk_builder_get_object(builder, "tabs"));
    controls.configbox = GTK_WIDGET(gtk_builder_get_object(builder, "configbox"));
    controls.alwaysshowcheck = GTK_WIDGET(gtk_builder_get_object(builder, "alwaysshowcheck"));

    controls.messagestext = GTK_WIDGET(gtk_builder_get_object(builder, "messagestext"));

    controls.vmode3dcombo = GTK_WIDGET(gtk_builder_get_object(builder, "vmode3dcombo"));
    controls.vmode3dlist = GTK_LIST_STORE(gtk_builder_get_object(builder, "vmode3dlist"));
    controls.fullscreencheck = GTK_WIDGET(get_and_connect_signal(builder, "fullscreencheck",
        "toggled", G_CALLBACK(on_fullscreencheck_toggled)));

    controls.usemousecheck = GTK_WIDGET(gtk_builder_get_object(builder, "usemousecheck"));
    controls.usejoystickcheck = GTK_WIDGET(gtk_builder_get_object(builder, "usejoystickcheck"));
    controls.soundqualitycombo = GTK_WIDGET(gtk_builder_get_object(builder, "soundqualitycombo"));
    controls.soundqualitylist = GTK_LIST_STORE(gtk_builder_get_object(builder, "soundqualitylist"));

    controls.chooseimportbutton = GTK_WIDGET(get_and_connect_signal(builder, "chooseimportbutton",
        "clicked", G_CALLBACK(on_chooseimportbutton_clicked)));
    controls.importinfobutton = GTK_WIDGET(get_and_connect_signal(builder, "importinfobutton",
        "clicked", G_CALLBACK(on_importinfobutton_clicked)));

    controls.importstatuswindow = GTK_WINDOW(gtk_builder_get_object(builder, "importstatuswindow"));
    controls.importstatustext = GTK_WIDGET(gtk_builder_get_object(builder, "importstatustext"));
    controls.importstatuscancelbutton = GTK_WIDGET(gtk_builder_get_object(builder, "importstatuscancelbutton"));

    g_object_unref(G_OBJECT(builder));

    return GTK_WINDOW(window);

fail:
    if (window) {
        gtk_widget_destroy(window);
    }
    if (builder) {
        g_object_unref(G_OBJECT(builder));
    }
    return 0;
}




// -- BUILD ENTRY POINTS ------------------------------------------------------

int startwin_open(void)
{
    if (!gtkenabled) return 0;
    if (startwin) return 1;

    startwin = create_window();
    if (!startwin) {
        return -1;
    }

    quiteventonclose = TRUE;
    setup_messages_mode(TRUE);
    gtk_widget_show_all(GTK_WIDGET(startwin));
    return 0;
}

int startwin_close(void)
{
    if (!gtkenabled) return 0;
    if (!startwin) return 1;

    quiteventonclose = FALSE;
    gtk_widget_destroy(GTK_WIDGET(startwin));
    startwin = NULL;

    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    return 0;
}

static gboolean startwin_puts_inner(gpointer str)
{
    GtkTextBuffer *textbuffer;
    GtkTextIter enditer;
    GtkTextMark *mark;
    const char *aptr, *bptr;

    textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(controls.messagestext));

    gtk_text_buffer_get_end_iter(textbuffer, &enditer);
    for (aptr = bptr = (const char *)str; *aptr != 0; ) {
        switch (*bptr) {
            case '\b':
                if (bptr > aptr) {
                    // Insert any normal characters seen so far.
                    gtk_text_buffer_insert(textbuffer, &enditer, (const gchar *)aptr, (gint)(bptr-aptr)-1);
                }
                gtk_text_buffer_backspace(textbuffer, &enditer, FALSE, TRUE);
                aptr = ++bptr;
                break;
            case 0:
                if (bptr > aptr) {
                    gtk_text_buffer_insert(textbuffer, &enditer, (const gchar *)aptr, (gint)(bptr-aptr));
                }
                aptr = bptr;
                break;
            default:
                bptr++;
                break;
        }
    }

    mark = gtk_text_buffer_create_mark(textbuffer, NULL, &enditer, 1);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(controls.messagestext), mark, 0.0, FALSE, 0.0, 1.0);
    gtk_text_buffer_delete_mark(textbuffer, mark);

    free(str);
    return FALSE;
}

int startwin_puts(const char *str)
{
    // Called in either the main thread or the import thread via buildprintf.
    if (!gtkenabled || !str) return 0;
    if (!startwin) return 1;

    g_main_context_invoke(NULL, startwin_puts_inner, (gpointer)strdup(str));

    return 0;
}

int startwin_settitle(const char *title)
{
    if (!gtkenabled) return 0;

    if (startwin) {
        gtk_window_set_title(startwin, title);
    }

    return 0;
}

int startwin_idle(void *s)
{
    (void)s;
    return 0;
}

int startwin_run(struct startwin_settings *settings)
{
    if (!gtkenabled || !startwin) return STARTWIN_RUN;

    set_settings(settings);
    setup_config_mode();
    startwinloop = TRUE;
    while (startwinloop) {
        gtk_main_iteration_do(TRUE);
    }
    setup_messages_mode(FALSE);
    set_settings(NULL);

    return retval;
}

