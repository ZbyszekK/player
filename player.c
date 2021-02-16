#include <gst/gst.h>

typedef struct bus_watch_context {
    GstElement* pipeline;
    GMainLoop* loop;
} bus_watch_context;

static gboolean callback(GstBus* bus, GstMessage* message, void* context)
{
    bus_watch_context* data = (bus_watch_context*) context;
    GstState old, new, pending;
    GError *err;
    gchar *info;

    switch (GST_MESSAGE_TYPE(message))
    {
        case GST_MESSAGE_STATE_CHANGED:
        gst_message_parse_state_changed(message, &old, &new, &pending);
        // only interested in pipeline state changes
        if (GST_MESSAGE_SRC(message) == GST_OBJECT(data->pipeline)) 
                g_print("State changed [%s] %s > %s\n", 
                                GST_OBJECT_NAME(message->src), 
                                gst_element_state_get_name(old),
                                gst_element_state_get_name(new));
        break;
    
        case GST_MESSAGE_ERROR:
        gst_message_parse_error(message, &err, &info);
        g_printerr("Error received [%s] (%s, %s)\n", GST_OBJECT_NAME(message->src), err->message, (info?info:""));
        g_clear_error(&err);
        g_free(info);
        g_main_loop_quit(data->loop);
        break;

        case GST_MESSAGE_EOS:
        g_print("EOS\n");
        g_main_loop_quit(data->loop);
        break;
        
        default:
        //g_printerr("Unknown message [%s] %s\n",GST_OBJECT_NAME(message->src), GST_MESSAGE_TYPE_NAME(message));
        break;
    }

    return TRUE;
}


int main(int argc, char *argv[])
{
    if (argc <2) {
        g_print("Usage: %s [STREAM_URL]\n", argv[0]);
        return -1;
    }

    // initialize gstreamer
    gst_init(&argc, &argv);

    // print gstreamer debug info  
    guint major, minor, micro, nano;
    gst_version(&major, &minor, &micro, &nano);
    g_print("Running on gstremer %d:%d:%d:%d\n", major, minor, micro, nano);

    // create pipeline
    GstElement* pipeline = gst_element_factory_make("playbin", "pipeline");
    if (!pipeline) {
        g_printerr("Can't create playback pipeline\n");
        return -1;
    }

    // set content uri
    g_object_set(pipeline, "uri", argv[1], NULL);

    // create main gstreamer loop
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);

    // register callback for events from pipeline
    GstBus* bus = gst_element_get_bus(pipeline);
    bus_watch_context context;
    context.pipeline = pipeline;
    context.loop = loop;
    gst_bus_add_watch(bus, callback, &context);

    // start playback
    gint result = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (result == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Can't start playback\n");
        g_main_loop_unref(loop);
        gst_object_unref(bus);
        gst_object_unref(pipeline);
        return -1;
    }

    g_print("Waiting on GMainLoop ... \n");
    g_main_loop_run(loop);

    g_print("End of playback\n");
    g_main_loop_unref(loop);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}

