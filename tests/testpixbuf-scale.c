#include "config.h"
#include <ctk/ctk.h>

#include <stdio.h>
#include <stdlib.h>

GdkInterpType interp_type = GDK_INTERP_BILINEAR;
int overall_alpha = 255;
GdkPixbuf *pixbuf;
CtkWidget *darea;
  
void
set_interp_type (CtkWidget *widget, gpointer data)
{
  guint types[] = { GDK_INTERP_NEAREST,
                    GDK_INTERP_BILINEAR,
                    GDK_INTERP_TILES,
                    GDK_INTERP_HYPER };

  interp_type = types[ctk_combo_box_get_active (CTK_COMBO_BOX (widget))];
  ctk_widget_queue_draw (darea);
}

void
overall_changed_cb (CtkAdjustment *adjustment, gpointer data)
{
  if (ctk_adjustment_get_value (adjustment) != overall_alpha)
    {
      overall_alpha = ctk_adjustment_get_value (adjustment);
      ctk_widget_queue_draw (darea);
    }
}

gboolean
draw_cb (CtkWidget *widget, cairo_t *cr, gpointer data)
{
  GdkPixbuf *dest;
  int width, height;

  width = ctk_widget_get_allocated_width (widget);
  height = ctk_widget_get_allocated_height (widget);

  dest = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, width, height);

  gdk_pixbuf_composite_color (pixbuf, dest,
			      0, 0, width, height,
			      0, 0,
                              (double) width / gdk_pixbuf_get_width (pixbuf),
                              (double) height / gdk_pixbuf_get_height (pixbuf),
			      interp_type, overall_alpha,
			      0, 0, 16, 0xaaaaaa, 0x555555);

  gdk_cairo_set_source_pixbuf (cr, dest, 0, 0);
  cairo_paint (cr);

  g_object_unref (dest);
  
  return TRUE;
}

int
main(int argc, char **argv)
{
	CtkWidget *window, *vbox;
        CtkWidget *combo_box;
	CtkWidget *hbox, *label, *hscale;
	CtkAdjustment *adjustment;
	CtkRequisition scratch_requisition;
        const gchar *creator;
        GError *error;

	ctk_init (&argc, &argv);

	if (argc != 2) {
		fprintf (stderr, "Usage: testpixbuf-scale FILE\n");
		exit (1);
	}

        error = NULL;
	pixbuf = gdk_pixbuf_new_from_file (argv[1], &error);
	if (!pixbuf) {
		fprintf (stderr, "Cannot load image: %s\n",
                         error->message);
                g_error_free (error);
		exit(1);
	}

        creator = gdk_pixbuf_get_option (pixbuf, "tEXt::Software");
        if (creator)
                g_print ("%s was created by '%s'\n", argv[1], creator);

	window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
	g_signal_connect (window, "destroy",
			  G_CALLBACK (ctk_main_quit), NULL);
	
	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
	ctk_container_add (CTK_CONTAINER (window), vbox);

        combo_box = ctk_combo_box_text_new ();

        ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_box), "NEAREST");
        ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_box), "BILINEAR");
        ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_box), "TILES");
        ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (combo_box), "HYPER");

        ctk_combo_box_set_active (CTK_COMBO_BOX (combo_box), 1);

        g_signal_connect (combo_box, "changed",
                          G_CALLBACK (set_interp_type),
                          NULL);
	
        ctk_widget_set_halign (combo_box, CTK_ALIGN_START);
	ctk_box_pack_start (CTK_BOX (vbox), combo_box, FALSE, FALSE, 0);

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 4);
	ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	label = ctk_label_new ("Overall Alpha:");
	ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, FALSE, 0);

	adjustment = ctk_adjustment_new (overall_alpha, 0, 255, 1, 10, 0);
	g_signal_connect (adjustment, "value_changed",
			  G_CALLBACK (overall_changed_cb), NULL);
	
	hscale = ctk_scale_new (CTK_ORIENTATION_HORIZONTAL, adjustment);
	ctk_scale_set_digits (CTK_SCALE (hscale), 0);
	ctk_box_pack_start (CTK_BOX (hbox), hscale, TRUE, TRUE, 0);

	ctk_widget_show_all (vbox);

	/* Compute the size without the drawing area, so we know how big to make the default size */
        ctk_widget_get_preferred_size ( (vbox),
                                   &scratch_requisition, NULL);

	darea = ctk_drawing_area_new ();
	ctk_box_pack_start (CTK_BOX (vbox), darea, TRUE, TRUE, 0);

	g_signal_connect (darea, "draw",
			  G_CALLBACK (draw_cb), NULL);

	ctk_window_set_default_size (CTK_WINDOW (window),
				     gdk_pixbuf_get_width (pixbuf),
				     scratch_requisition.height + gdk_pixbuf_get_height (pixbuf));
	
	ctk_widget_show_all (window);

	ctk_main ();

	return 0;
}
