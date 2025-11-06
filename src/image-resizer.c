#include <gdk-pixbuf/gdk-pixbuf.h>

int main(int argc, char **argv) {
    GdkPixbuf *pixbuf, *scaled_pixbuf;
    gchar * src_path = NULL;
    gchar * out_path = NULL;
    int out_width = 0;
    int out_height = 0;
    GError *error = NULL;

    if(argc != 5){
        g_printerr("Incorrect number of argument provided. (e.g. ./image-resizer <source_file> <output_file>)\n");
        return 1;
    }

    src_path = argv[1];
    out_path = argv[2];
    out_width = atoi(argv[3]);
    out_height = atoi(argv[4]);

    g_print("Source file : %s\n",src_path);
    g_print("Output file : %s\n", out_path);
    g_print("Output size : %dx%d\n", out_width,out_height);

    if(out_width <= 0 || out_height <= 0){
        g_printerr("Invalid side provided : %dx%d", out_width, out_height);
        return 1;
    }

    pixbuf = gdk_pixbuf_new_from_file(src_path, NULL);
    if (pixbuf == NULL) {
        g_printerr("Failed to load image: %s\n", src_path);
        return 1;
    }

    scaled_pixbuf = gdk_pixbuf_scale_simple(pixbuf, out_width, out_height, GDK_INTERP_BILINEAR);
    
    if (!gdk_pixbuf_save(scaled_pixbuf, out_path, "png", &error, NULL)) {
        g_printerr("Error saving pixbuf: %s\n", error->message);
        g_error_free(error);
    }

    g_object_unref(pixbuf);
    g_object_unref(scaled_pixbuf); 

    return 0;
}