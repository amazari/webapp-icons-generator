#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <gdk/gdk.h>

// build with gcc `pkg-config --cflags --libs webkitgtk-3.0` main.c

// the idea here is to let the user toy with a widget (here a webview)
// then reparent it into an offscreen window, and finally get a pixbuf out of it
// issue : 
// (a.out:16311): Gdk-CRITICAL **: gdk_offscreen_window_get_surface: assertion `GDK_IS_WINDOW (window)' failed

static cairo_rectangle_t icons[5] = {
  {-20, -32, 256, 256},
  {-296, -52, 48, 48},
  {-303, -128, 32, 32},
  {-303, -179, 22, 22},
  {-303, -221, 16, 16},
};
static const int icons_count = 5;

void generate_icon (cairo_surface_t *full_surface, cairo_rectangle_t clip, char *template_filename)
{
	cairo_surface_t *clipped_surface;
	cairo_t *cr;

  clipped_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        clip.width, clip.height);
  cr = cairo_create (clipped_surface);
  cairo_set_source_surface (cr, full_surface, clip.x, clip.y);
  cairo_paint(cr);

  char* png_name = g_strdup_printf (template_filename,
				    clip.width, clip.height);
  cairo_surface_write_to_png(clipped_surface, png_name);

  g_free (png_name);
  cairo_destroy (cr);
  cairo_surface_destroy (clipped_surface);
}

void
ephy_web_view_create_snapshots (GtkWidget *view,
				const cairo_rectangle_t rects[], 
				char *template_filename)
{
  cairo_surface_t *surface, *surface2;
  cairo_t *cr, *cr2;
  GdkPixbuf *snapshot;
  GtkAllocation allocation;

  gtk_widget_get_allocation (GTK_WIDGET (view), &allocation);
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        allocation.width,
                                        allocation.height);
  cr = cairo_create (surface);
  gtk_widget_draw (GTK_WIDGET (view), cr);

  for (int i = 0; i < icons_count; i++)
    generate_icon (surface, rects[i], template_filename);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);

}


void on_button_clicked (GtkButton *button,
			GtkWidget* view)
{
  ephy_web_view_create_snapshots(view, icons, "app-icon-%.0fx%.0f.png");

}


typedef struct {
  char* name;
  char* value;
} attribute_t;

void 
set_attributes (WebKitDOMNode* node, attribute_t* attributes,
		int attributes_count)
{
  GError* error = NULL;

  WebKitDOMNamedNodeMap* dom_attributes = 
    webkit_dom_node_get_attributes (node);
  g_assert (attributes != NULL);

  for (int i = 0; i < attributes_count; i++) {
    g_print("%d\n", i);
    g_print("attr %s: %s",attributes[i].name, attributes[i].value);
    WebKitDOMNode* dom_attribute = 
      webkit_dom_named_node_map_get_named_item (dom_attributes,
						attributes[i].name);
    g_assert (dom_attribute != NULL);

    webkit_dom_attr_set_value (WEBKIT_DOM_ATTR (dom_attribute), attributes[i].value, &error);
  
    g_object_unref (dom_attribute);

    if (error != NULL) {
      g_warning (error->message);
      g_error_free (error);
    }
  }
  
  g_object_unref (dom_attributes);

}

void set_attributes_by_class (WebKitDOMDocument* document,
			      char* class, attribute_t* attributes, int attributes_count)
{
  WebKitDOMNodeList* nodes = 
    webkit_dom_document_get_elements_by_class_name(document, class);

  gulong number_of_nodes = webkit_dom_node_list_get_length (nodes);
  for (int i = 0; i < number_of_nodes; i++) {
    WebKitDOMNode* node = webkit_dom_node_list_item(nodes, i);
    set_attributes (node, attributes, attributes_count);
    g_object_unref (node);
  }
  g_object_unref (nodes);
}

void on_webview_loaded (WebKitWebView *view,
			gpointer data)
{

  WebKitDOMDocument* document = webkit_web_view_get_dom_document 
    (WEBKIT_WEB_VIEW(view));
  g_assert(document != NULL);

  GFile *screenshot = g_file_new_for_commandline_arg ("screenshot.png");

  GFileInfo *infos = g_file_query_info (screenshot,
					"*", G_FILE_QUERY_INFO_NONE,
					NULL, NULL);
  attribute_t screenshot_attributes[3] = {
    {"xlink:href", g_file_get_uri (screenshot)},
    {"width", "2732"},
    {"height", "1536"},
  };
  set_attributes_by_class (document, "screenshot",screenshot_attributes, 3);

  
  GFile *favicon = g_file_new_for_commandline_arg ("favicon.ico");
  attribute_t favicon_attributes[1] = {
    {"xlink:href", g_file_get_uri (favicon)},
  };
  //set_attributes_by_class (document, "favicon", favicon_attributes, 1);
  
  g_object_unref (document);
}


void quit()
{
  gtk_main_quit();
}

int main(int ac, char **av)
{
  GError *error = NULL;

  gtk_init(&ac, &av);


  GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "delete_event", (GCallback)quit, NULL);
  GtkWidget* box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);

  GtkWidget* view = webkit_web_view_new();
  webkit_web_view_set_transparent(WEBKIT_WEB_VIEW(view), TRUE);
  g_signal_connect (view, "onload-event", (GCallback)on_webview_loaded, NULL);

  GtkWidget* button = gtk_button_new_with_label("Generate icons");
  g_signal_connect (button, "clicked", (GCallback)on_button_clicked, view);

  gtk_container_add (GTK_CONTAINER(box), view);
  gtk_container_add (GTK_CONTAINER(box), button);
  gtk_container_add (GTK_CONTAINER(window), box);


  GFile* template = g_file_new_for_commandline_arg("template.svg");
  webkit_web_view_load_uri (WEBKIT_WEB_VIEW (view),
			    g_file_get_uri (template));    


  gtk_widget_show_all (window);

  gtk_main();
}
