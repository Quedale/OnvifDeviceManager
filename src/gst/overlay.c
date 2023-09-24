#include "overlay.h"
#include <math.h>
#include <gtk/gtk.h>

typedef struct _OverlayState {
  gboolean valid;
  GstVideoInfo info;
  //Used to calculate level decay
  gdouble level;
} OverlayState;

OverlayState * OverlayState__create(){
  OverlayState * self = malloc(sizeof(OverlayState));
  OverlayState__init(self);
  return self;
}

void OverlayState__init(OverlayState * self){
  self->level = 0;
  self->valid = 0;
}

void OverlayState__destroy(OverlayState * self){
  if(self){
    free(self);
  }
}

void OverlayState__prepare_overlay (GstElement * overlay, GstCaps * caps, gint window_width, gint window_height, gpointer user_data){

  OverlayState *s = (OverlayState *) user_data;

  if (gst_video_info_from_caps (&s->info, caps))
    s->valid = TRUE;
  else
    s->valid = FALSE;

}

GstBuffer * create_bar_buffer(double width, double height){

  guint stride;
  cairo_surface_t *surface;
  cairo_t *cr;

  stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,width);
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,width,height);
  cr = cairo_create (surface);
  
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_set_source_rgba (cr, 0.9, 0.0, 0.1, 0.7);
  cairo_fill(cr);

  guchar *pixdata = cairo_image_surface_get_data(surface);

  GstBuffer * buff = gst_buffer_new_and_alloc (height * stride);
  gst_buffer_fill (buff, 0, pixdata + 24, height * stride);
  gst_buffer_add_video_meta (buff, GST_VIDEO_FRAME_FLAG_NONE,
      GST_VIDEO_OVERLAY_COMPOSITION_FORMAT_RGB, width, height);
  
  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  return buff;
}

GstVideoOverlayComposition * OverlayState__draw_overlay (GstElement * overlay, GstSample * sample, gpointer user_data){

  OverlayState *self = (OverlayState *)user_data;
  GstVideoOverlayRectangle *rect;
  GstVideoOverlayComposition *comp;
  GstVideoMeta *vmeta;
  gint x, y;
  GstBuffer * buff;
  
  //Dont bother if the overlay is not prepared
  //Dont bother if the video is not showing
  //Dont bother if no sound is detected
  if(!self->valid ||
      self->info.height == 0 || 
      self->info.width == 0 || 
      self->level < 1){
    return NULL;
  }

  gdouble margin = 20; //Space betweem border and bar
  gdouble bwidth = 20; //Bar width
  gdouble pheight = self->info.height - margin*2; //Height available for drawing
  gdouble bheight = self->level * pheight / 100; //Actual bar height calculated from level

  buff = create_bar_buffer(bwidth,bheight); //Create GstBuffer

  vmeta = gst_buffer_get_video_meta (buff); //Extract video metadata

  //Determine x position considering margin (Aligned left)
  x= self->info.width-(margin+bwidth);
  //Determine y position based on bar height (Aligned bottom)
  y= margin + pheight - bheight;

  //Create Gstreamer video overlay rectangle
  rect = gst_video_overlay_rectangle_new_raw (buff, x, y,
      vmeta->width, vmeta->height, GST_VIDEO_OVERLAY_FORMAT_FLAG_NONE);
  comp = gst_video_overlay_composition_new (rect);
  gst_video_overlay_rectangle_unref (rect);
  
  gst_buffer_unref(buff);
  return comp;
}

void OverlayState__level_handler(GstBus * bus, GstMessage * message, OverlayState *self, const GstStructure *s){

  gint channels;
  GstClockTime endtime;
  gdouble peak_dB;
  gdouble peak;
  const gdouble max_variance = 15;
  const GValue *list;
  const GValue *value;
  GValueArray *peak_arr;
  gdouble output = 0;
  gint i;

  if (!gst_structure_get_clock_time (s, "endtime", &endtime))
    g_warning ("Could not parse endtime");

  list = gst_structure_get_value (s, "peak");
  peak_arr = (GValueArray *) g_value_get_boxed (list);
  channels = peak_arr->n_values;

  for (i = 0; i < channels; ++i) {
    // FIXME 'g_value_array_get_nth' is deprecated: Use 'GArray' instead
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    value = g_value_array_get_nth (peak_arr, i);
    peak_dB = g_value_get_double (value);
    #pragma GCC diagnostic pop

    /* converting from dB to normal gives us a value between 0.0 and 1.0 */
    peak = pow (10, peak_dB / 20);

    //Add up all channels peaks
    output = output + peak * 100; 
  }

  //Optionally only get the highest peak instead of calculating average.
  //Average output of all channels
  output = output / channels;

  //Set Output value on player
  if(output == self->level){
    //Ignore
  } else if( output > self->level){
    //Set new peak
    self->level = output;
  } else {
    //Lower to a maximum drop of 15 for a graphical decay
    gdouble variance = self->level-output;
    if(variance > max_variance){
      self->level=self->level-max_variance; //The decay value has to match the interval set. We are dealing with 2x faster interval therefor 15/2=7.5
    } else {
      self->level = output;
    }
  }

}
