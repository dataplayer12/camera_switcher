#include <gst/gst.h>
#include <gtk/gtk.h>
#include <gst/video/videooverlay.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

static GstElement *pipeline;
static GtkWidget *drawing_area;
static GMainLoop *main_loop;

// you need to configure this for your own setup
static gchar *streams[] = {
    "rtsp://user1:password1@192.168.1.x:554/Streaming/channels/1/",
    "rtsp://user2:password2@192.168.1.y:554/Streaming/channels/1/",
    "rtsp://user3:password3@192.168.1.z:554/Streaming/channels/1/",
    "rtsp://user4:password4@192.168.1.w:554/Streaming/channels/1/"
};
// End of config

static guint current_stream_index = 0;

static void initialize_pipeline();
static void switch_stream();
static gboolean switch_stream_gtk_thread(gpointer data);

static void on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    g_idle_add(switch_stream_gtk_thread, NULL);
}

static void realize_cb(GtkWidget *widget, gpointer data) {
    GdkWindow *window = gtk_widget_get_window(widget);
#ifdef GDK_WINDOWING_X11
    GstVideoOverlay *overlay = GST_VIDEO_OVERLAY(gst_bin_get_by_name(GST_BIN(pipeline), "sink"));
    gst_video_overlay_set_window_handle(overlay, GDK_WINDOW_XID(window));
#endif
}

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *) data;

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;

            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);

            g_printerr("Error: %s\n", error->message);
            g_error_free(error);

            g_main_loop_quit(loop);
            break;
        }
        default:
            break;
    }

    return TRUE;
}

static void clean_up() {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(pipeline));
        pipeline = NULL;
    }
}

static void initialize_pipeline() {
    clean_up();
    gchar *pipeline_desc = g_strdup_printf("rtspsrc name=source protocols=GST_RTSP_LOWER_TRANS_TCP location=%s ! rtph265depay ! h265parse ! omxh265dec ! videoconvert ! xvimagesink name=sink", streams[current_stream_index]);
    pipeline = gst_parse_launch(pipeline_desc, NULL);
    g_free(pipeline_desc);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, bus_call, main_loop);
    gst_object_unref(bus);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

static void switch_stream() {
    current_stream_index = (current_stream_index + 1) % G_N_ELEMENTS(streams);
    printf("Switched to stream %d\n", (int)current_stream_index);
    initialize_pipeline();
}

static gboolean switch_stream_gtk_thread(gpointer data) {
    switch_stream();
    return G_SOURCE_REMOVE;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    gst_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    drawing_area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), drawing_area);
    g_signal_connect(window, "delete-event", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(drawing_area, "realize", G_CALLBACK(realize_cb), NULL);
    g_signal_connect(window, "button-press-event", G_CALLBACK(on_button_press), NULL);

    main_loop = g_main_loop_new(NULL, FALSE);
    initialize_pipeline();

    gtk_widget_show_all(window);
    gtk_widget_realize(drawing_area);

    g_main_loop_run(main_loop);

    clean_up();
    g_main_loop_unref(main_loop);

    return 0;
}

