#include "overlay.h"
#include <cairo.h>
#include <stdio.h>
#include "player.h"

void prepare_overlay (GstElement * overlay, GstCaps * caps, gint window_width,
    gint window_height, gpointer user_data)
{

  OverlayState *s = (OverlayState *) user_data;

  if (gst_video_info_from_caps (&s->info, caps))
    s->valid = TRUE;
  else
    s->valid = FALSE;

}

GstBuffer * create_bar_buffer(double width, double height){

  guint stride;
  cairo_format_t format;
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

GstVideoOverlayComposition *
draw_overlay (GstElement * overlay, GstSample * sample, gpointer user_data)
{

  OnvifPlayer *player = (OnvifPlayer *)user_data;
  GstVideoOverlayRectangle *rect;
  GstVideoOverlayComposition *comp;
  GstVideoMeta *vmeta;
  gint x, y;
  GstBuffer * buff;
  
  //Dont bother if the overlay is not prepared
  //Dont bother if the video is not showing
  //Dont bother if no sound is detected
  if(!player->overlay_state->valid ||
      player->overlay_state->info.height == 0 || 
      player->overlay_state->info.width == 0 || 
      player->level < 1){
    return NULL;
  }

  gdouble margin = 20; //Space betweem border and bar
  gdouble bwidth = 20; //Bar width
  gdouble pheight = player->overlay_state->info.height - margin*2; //Height available for drawing
  gdouble bheight = player->level * pheight / 100; //Actual bar height calculated from level

  buff = create_bar_buffer(bwidth,bheight); //Create GstBuffer

  vmeta = gst_buffer_get_video_meta (buff); //Extract video metadata

  //Determine x position considering margin (Aligned left)
  x= player->overlay_state->info.width-(margin+bwidth);
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