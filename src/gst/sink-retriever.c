#include <gst/gst.h>
#include <stdio.h>
#include "sink-retriever.h"

typedef struct {
    SupportedAudioSinkTypes type;
} AudioRetValue;

void child_added_callback (GstChildProxy * self,
                      GObject * object,
                      gchar * name,
                      gpointer user_data){
    AudioRetValue * ret_val = (AudioRetValue *) user_data;
    if(!strcmp(name,"detectaudiosink-actual-sink-pulse")){
        ret_val->type = ONVIF_PULSE;
        g_info("Detector found pulse sink\n");
    } else if(!strcmp(name,"detectaudiosink-actual-sink-alsa")){
        ret_val->type = ONVIF_ASLA;
    } else if(!strcmp(name,"detectaudiosink-actual-sink-omxhdmiaudio")){
        ret_val->type = ONVIF_OMX;
    } else if(!strcmp(name,"fake-audio-sink")){
        g_info("Detector ignore fake sink.");
    } else {
        g_warning("Unexpected audio sink. '%s'",name);
    }
}

SupportedAudioSinkTypes  
retrieve_audiosink(void){

    //Create temporary pipeline to use AutoDetect element
    GstElement * pipeline = gst_pipeline_new ("audiotest-pipeline");
    GstElement * src = gst_element_factory_make ("audiotestsrc", "src");
    GstElement * volume = gst_element_factory_make ("volume", "vol");
    GstElement * sink = gst_element_factory_make ("autoaudiosink", "detectaudiosink");
    g_object_set (G_OBJECT (volume), "mute", TRUE, NULL); 

    //Initializing return value
    AudioRetValue * ret_val = malloc(sizeof(AudioRetValue));
    ret_val->type = ONVIF_NA;

    //Validate elements
    if (!pipeline || \
        !src || \
        !volume || \
        !sink) {
        g_printerr ("One of the elements wasn't created... Exiting\n");
        return ret_val->type;
    }

    //Add elements to pipeline
    gst_bin_add_many (GST_BIN (pipeline), \
        src, \
        volume, \
        sink, NULL);

    //Link elements
    if (!gst_element_link_many (src, \
        volume, \
        sink, NULL)){
        g_warning ("Linking part (A)-2 Fail...");
        return ret_val->type;
    }

    //Add callback to find actual-sink name
    if(! g_signal_connect (sink, "child-added", G_CALLBACK (child_added_callback),ret_val))
    {
        g_warning ("Linking part (1) with part (A)-1 Fail...");
    }

    //Play pipeline to construct it.
    GstStateChangeReturn ret;
    ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (pipeline);
        return ret_val->type;
    }

    //Stop pipeline
    ret = gst_element_set_state (pipeline, GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the ready state.\n");
        gst_object_unref (pipeline);
        return ret_val->type;
    }

    //Clean up
    ret = gst_element_set_state (pipeline, GST_STATE_NULL);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the null state.\n");
        gst_object_unref (pipeline);
        return ret_val->type;
    }
    gst_object_unref (pipeline);

    return ret_val->type;
}