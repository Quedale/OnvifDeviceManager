
#include <gst/gst.h>
#include <stdio.h>
#include "src_retriever.h"
#include "../alsa/alsa_utils.h"

GST_DEBUG_CATEGORY_STATIC (ext_srcret_debug);
#define GST_CAT_DEFAULT (ext_srcret_debug)

int  
test_audiosrc(char * srcname, char * device){

    GST_DEBUG("Test Audio Src [%s] device [%s]\n",srcname,device);
    //Create temporary pipeline to use AutoDetect element
    GstElement * pipeline = gst_pipeline_new ("audiotest-pipeline");
    GstElement * src = gst_element_factory_make (srcname, "src");
    GstElement * volume = gst_element_factory_make ("volume", "vol");
    GstElement * sink = gst_element_factory_make ("fakesink", "detectaudiosink");
    g_object_set (G_OBJECT (volume), "mute", TRUE, NULL); 

    if(!strcmp(srcname,"alsasrc")){
        g_object_set (G_OBJECT (src), "device", device, NULL); 
    }

    //Validate elements
    if (!pipeline || \
        !src || \
        !volume || \
        !sink) {
        GST_ERROR ("One of the elements wasn't created... Exiting\n");
        return FALSE;
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
        GST_WARNING ("Linking part (A)-2 Fail...");
        return FALSE;
    }


    //Play pipeline to construct it.
    GstStateChangeReturn ret;
    ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        GST_ERROR ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (pipeline);
        return FALSE;
    }

    //Stop pipeline
    ret = gst_element_set_state (pipeline, GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        GST_ERROR ("Unable to set the pipeline to the ready state.\n");
        gst_object_unref (pipeline);
        return FALSE;
    }

    //Clean up
    ret = gst_element_set_state (pipeline, GST_STATE_NULL);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        GST_ERROR ("Unable to set the pipeline to the null state.\n");
        gst_object_unref (pipeline);
        return FALSE;
    }
    gst_object_unref (pipeline);
    
    return TRUE;
}

void
retrieve_audiosrc(char * element, char * device){
    GST_DEBUG_CATEGORY_INIT (ext_srcret_debug, "ext-srcret", 0, "Extension to support Alsa capabilities");

    int ret;
    ret = test_audiosrc("pulsesrc", NULL);
    if(ret){
        strcpy(element,"pulsesrc");
        device[0] = '\0';
        return;
    }

    AlsaDevices* dev_list = get_alsa_device_list(SND_PCM_STREAM_CAPTURE);
    int i;
    for(i=0;i<dev_list->count;i++){
        AlsaDevice * dev = dev_list->devices[i];

        char hw_dev[7];  
        hw_dev[0] = 'h';
        hw_dev[1] = 'w';
        hw_dev[2] = ':';
        hw_dev[3] = dev->card_index + '0';
        hw_dev[4] = ',';
        hw_dev[5] = dev->dev_index + '0';
        hw_dev[6] = '\0';

        ret = test_audiosrc("alsasrc",hw_dev);
        if(ret){
            strcpy(element,"alsasrc");
            strcpy(device,hw_dev);
            AlsaDevices__destroy(dev_list);
            return;
        }
    }

    AlsaDevices__destroy(dev_list);
    element[0] = '\0';
    device[0] = '\0';
}