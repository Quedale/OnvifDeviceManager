
#include "../src/animations/gtk/gtk_dotted_slider_widget.h"
#include "clogger.h"
#include "portable_thread.h"


/* This function is called when the main window is closed */
static void delete_event_cb (GtkWidget *widget, GdkEvent *event, gpointer data) {
    gtk_main_quit();
}

static void faster_dot_cb (GtkButton *button, GtkDottedSlider * slider) {
    gdouble anim_time = gtk_dotted_slider_get_animation_time(slider);
    gtk_dotted_slider_set_animation_time(slider,anim_time*0.90);
}

static void slower_dot_cb (GtkButton *button, GtkDottedSlider * slider) {
    gdouble anim_time = gtk_dotted_slider_get_animation_time(slider);
    gtk_dotted_slider_set_animation_time(slider,anim_time*1.10);
}

static void add_dot_cb (GtkButton *button, GtkDottedSlider * slider) {
    gint count = gtk_dotted_slider_get_item_count(slider);
    gtk_dotted_slider_set_item_count(slider,count+1);
}

static void remove_cb (GtkButton *button, GtkDottedSlider * slider) {
    gint count = gtk_dotted_slider_get_item_count(slider);
    gtk_dotted_slider_set_item_count(slider,count-1);
}

GtkWidget * create_buttons(GtkDottedSlider * slider){
    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    GtkWidget * faster_btn = gtk_button_new_with_label("Faster");
    g_signal_connect (G_OBJECT(faster_btn), "clicked", G_CALLBACK (faster_dot_cb), slider);
    gtk_box_pack_start(GTK_BOX(hbox), faster_btn,     FALSE, FALSE, 0);

    GtkWidget * add_btn = gtk_button_new_with_label("Add Dot");
    g_signal_connect (G_OBJECT(add_btn), "clicked", G_CALLBACK (add_dot_cb), slider);
    gtk_box_pack_start(GTK_BOX(hbox), add_btn,     FALSE, FALSE, 0);

    GtkWidget * remove_btn = gtk_button_new_with_label("Remove Dot");
    g_signal_connect (G_OBJECT(remove_btn), "clicked", G_CALLBACK (remove_cb), slider);
    gtk_box_pack_start(GTK_BOX(hbox), remove_btn,     FALSE, FALSE, 0);

    GtkWidget * slower_btn = gtk_button_new_with_label("Slower");
    g_signal_connect (G_OBJECT(slower_btn), "clicked", G_CALLBACK (slower_dot_cb), slider);
    gtk_box_pack_start(GTK_BOX(hbox), slower_btn,     FALSE, FALSE, 0);

    gtk_widget_set_halign(hbox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(hbox, GTK_ALIGN_CENTER);

    return hbox;

}

int main(int argc, char *argv[])
{
    /* Initialize GTK */
    gtk_init (&argc, &argv);
    c_log_set_level(C_ALL_E);
    c_log_set_thread_color(ANSI_COLOR_DRK_GREEN, P_THREAD_ID);
    
    GtkWidget * window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (delete_event_cb), NULL);
    gtk_window_set_title (GTK_WINDOW (window), "GtkDottedSlider Widget demo");

    GtkWidget * slider = gtk_dotted_slider_new(GTK_ORIENTATION_HORIZONTAL, 5,10,1);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), slider,     TRUE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window),vbox);

    GtkWidget * btn_bar = create_buttons(GTK_DOTTED_SLIDER(slider));
    gtk_box_pack_start(GTK_BOX(vbox), btn_bar,     TRUE, FALSE, 0);

    gtk_window_set_default_size(GTK_WINDOW(window),100,100);
    gtk_widget_show_all (window);

    gtk_main ();

}