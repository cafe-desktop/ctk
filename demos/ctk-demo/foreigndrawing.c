/* Foreign drawing
 *
 * Many applications can't use CTK+ widgets, for a variety of reasons,
 * but still want their user interface to appear integrated with the
 * rest of the desktop, and follow CTK+ themes. This demo shows how to
 * use CtkStyleContext and the ctk_render_ APIs to achieve this.
 *
 * Note that this is a very simple, non-interactive example.
 */

#include <ctk/ctk.h>
#include <string.h>

static void
append_element (CtkWidgetPath *path,
                const char    *selector)
{
  static const struct {
    const char    *name;
    CtkStateFlags  state_flag;
  } pseudo_classes[] = {
    { "active",        CTK_STATE_FLAG_ACTIVE },
    { "hover",         CTK_STATE_FLAG_PRELIGHT },
    { "selected",      CTK_STATE_FLAG_SELECTED },
    { "disabled",      CTK_STATE_FLAG_INSENSITIVE },
    { "indeterminate", CTK_STATE_FLAG_INCONSISTENT },
    { "focus",         CTK_STATE_FLAG_FOCUSED },
    { "backdrop",      CTK_STATE_FLAG_BACKDROP },
    { "dir(ltr)",      CTK_STATE_FLAG_DIR_LTR },
    { "dir(rtl)",      CTK_STATE_FLAG_DIR_RTL },
    { "link",          CTK_STATE_FLAG_LINK },
    { "visited",       CTK_STATE_FLAG_VISITED },
    { "checked",       CTK_STATE_FLAG_CHECKED },
    { "drop(active)",  CTK_STATE_FLAG_DROP_ACTIVE }
  };
  const char *next;
  char *name;
  guint i;

  next = strpbrk (selector, "#.:");
  if (next == NULL)
    next = selector + strlen (selector);

  name = g_strndup (selector, next - selector);
  if (g_ascii_isupper (selector[0]))
    {
      GType gtype;
      gtype = g_type_from_name (name);
      if (gtype == G_TYPE_INVALID)
        {
          g_critical ("Unknown type name `%s'", name);
          g_free (name);
          return;
        }
      ctk_widget_path_append_type (path, gtype);
    }
  else
    {
      /* Omit type, we're using name */
      ctk_widget_path_append_type (path, G_TYPE_NONE);
      ctk_widget_path_iter_set_object_name (path, -1, name);
    }
  g_free (name);

  while (*next != '\0')
    {
      char type;

      type = *next;
      selector = next + 1;
      next = strpbrk (selector, "#.:");
      if (next == NULL)
        next = selector + strlen (selector);
      name = g_strndup (selector, next - selector);

      switch (type)
        {
        case '#':
          ctk_widget_path_iter_set_name (path, -1, name);
          break;

        case '.':
          ctk_widget_path_iter_add_class (path, -1, name);
          break;

        case ':':
          for (i = 0; i < G_N_ELEMENTS (pseudo_classes); i++)
            {
              if (g_str_equal (pseudo_classes[i].name, name))
                {
                  ctk_widget_path_iter_set_state (path,
                                                  -1,
                                                  ctk_widget_path_iter_get_state (path, -1)
                                                  | pseudo_classes[i].state_flag);
                  break;
                }
            }
          if (i == G_N_ELEMENTS (pseudo_classes))
            g_critical ("Unknown pseudo-class :%s", name);
          break;

        default:
          g_assert_not_reached ();
          break;
        }

      g_free (name);
    }
}

static CtkStyleContext *
create_context_for_path (CtkWidgetPath   *path,
                         CtkStyleContext *parent)
{
  CtkStyleContext *context;

  context = ctk_style_context_new ();
  ctk_style_context_set_path (context, path);
  ctk_style_context_set_parent (context, parent);
  /* Unfortunately, we have to explicitly set the state again here
   * for it to take effect
   */
  ctk_style_context_set_state (context, ctk_widget_path_iter_get_state (path, -1));
  ctk_widget_path_unref (path);

  return context;
}

static CtkStyleContext *
get_style (CtkStyleContext *parent,
           const char      *selector)
{
  CtkWidgetPath *path;

  if (parent)
    path = ctk_widget_path_copy (ctk_style_context_get_path (parent));
  else
    path = ctk_widget_path_new ();

  append_element (path, selector);

  return create_context_for_path (path, parent);
}

static CtkStyleContext *
get_style_with_siblings (CtkStyleContext *parent,
                         const char      *selector G_GNUC_UNUSED,
                         const char     **siblings,
                         gint             position)
{
  CtkWidgetPath *path, *siblings_path;
  guint i;

  if (parent)
    path = ctk_widget_path_copy (ctk_style_context_get_path (parent));
  else
    path = ctk_widget_path_new ();

  siblings_path = ctk_widget_path_new ();
  for (i = 0; siblings[i]; i++)
    append_element (siblings_path, siblings[i]);

  ctk_widget_path_append_with_siblings (path, siblings_path, position);
  ctk_widget_path_unref (siblings_path);

  return create_context_for_path (path, parent);
}

static void
draw_style_common (CtkStyleContext *context,
                   cairo_t         *cr,
                   gint             x,
                   gint             y,
                   gint             width,
                   gint             height,
                   gint            *contents_x,
                   gint            *contents_y,
                   gint            *contents_width,
                   gint            *contents_height)
{
  CtkBorder margin, border, padding;
  int min_width, min_height;

  ctk_style_context_get_margin (context, ctk_style_context_get_state (context), &margin);
  ctk_style_context_get_border (context, ctk_style_context_get_state (context), &border);
  ctk_style_context_get_padding (context, ctk_style_context_get_state (context), &padding);

  ctk_style_context_get (context, ctk_style_context_get_state (context),
                         "min-width", &min_width,
                         "min-height", &min_height,
                         NULL);
  x += margin.left;
  y += margin.top;
  width -= margin.left + margin.right;
  height -= margin.top + margin.bottom;

  width = MAX (width, min_width);
  height = MAX (height, min_height);

  ctk_render_background (context, cr, x, y, width, height);
  ctk_render_frame (context, cr, x, y, width, height);

  if (contents_x)
    *contents_x = x + border.left + padding.left;
  if (contents_y)
    *contents_y = y + border.top + padding.top;
  if (contents_width)
    *contents_width = width - border.left - border.right - padding.left - padding.right;
  if (contents_height)
    *contents_height = height - border.top - border.bottom - padding.top - padding.bottom;
}

static void
query_size (CtkStyleContext *context,
            gint            *width,
            gint            *height)
{
  CtkBorder margin, border, padding;
  int min_width, min_height;

  ctk_style_context_get_margin (context, ctk_style_context_get_state (context), &margin);
  ctk_style_context_get_border (context, ctk_style_context_get_state (context), &border);
  ctk_style_context_get_padding (context, ctk_style_context_get_state (context), &padding);

  ctk_style_context_get (context, ctk_style_context_get_state (context),
                         "min-width", &min_width,
                         "min-height", &min_height,
                         NULL);

  min_width += margin.left + margin.right + border.left + border.right + padding.left + padding.right;
  min_height += margin.top + margin.bottom + border.top + border.bottom + padding.top + padding.bottom;

  if (width)
    *width = MAX (*width, min_width);
  if (height)
    *height = MAX (*height, min_height);
}

static void
draw_menu (CtkWidget *widget,
           cairo_t   *cr,
           gint       x,
           gint       y,
           gint       width,
           gint      *height)
{
  CtkStyleContext *menu_context;
  CtkStyleContext *menuitem_context;
  CtkStyleContext *hovermenuitem_context;
  CtkStyleContext *hoveredarrowmenuitem_context;
  CtkStyleContext *arrowmenuitem_context;
  CtkStyleContext *checkmenuitem_context;
  CtkStyleContext *disabledarrowmenuitem_context;
  CtkStyleContext *disabledcheckmenuitem_context;
  CtkStyleContext *radiomenuitem_context;
  CtkStyleContext *disablemenuitem_context;
  CtkStyleContext *disabledradiomenuitem_context;
  CtkStyleContext *separatormenuitem_context;
  gint menuitem1_height, menuitem2_height, menuitem3_height, menuitem4_height, menuitem5_height;
  gint contents_x, contents_y, contents_width, contents_height;
  gint menu_x, menu_y, menu_width, menu_height;
  gint arrow_width, arrow_height, arrow_size;
  gint toggle_x, toggle_y, toggle_width, toggle_height;

  /* This information is taken from the CtkMenu docs, see "CSS nodes" */
  menu_context = get_style (ctk_widget_get_style_context(widget), "menu");
  hovermenuitem_context = get_style (menu_context, "menuitem:hover");
  hoveredarrowmenuitem_context = get_style (hovermenuitem_context, "arrow.right:dir(ltr)");
  menuitem_context = get_style (menu_context, "menuitem");
  arrowmenuitem_context = get_style (menuitem_context, "arrow:dir(rtl)");
  disablemenuitem_context = get_style (menu_context, "menuitem:disabled");
  disabledarrowmenuitem_context = get_style (disablemenuitem_context, "arrow:dir(rtl)");
  checkmenuitem_context = get_style (menuitem_context, "check:checked");
  disabledcheckmenuitem_context = get_style (disablemenuitem_context, "check");
  separatormenuitem_context = get_style (menu_context, "separator:disabled");
  radiomenuitem_context = get_style (menuitem_context, "radio:checked");
  disabledradiomenuitem_context = get_style (disablemenuitem_context, "radio");

  *height = 0;
  query_size (menu_context, NULL, height);
  menuitem1_height = 0;
  query_size (hovermenuitem_context, NULL, &menuitem1_height);
  query_size (hoveredarrowmenuitem_context, NULL, &menuitem1_height);
  *height += menuitem1_height;
  menuitem2_height = 0;
  query_size (menu_context, NULL, &menuitem5_height);
  query_size (menuitem_context, NULL, &menuitem2_height);
  query_size (arrowmenuitem_context, NULL, &menuitem2_height);
  query_size (disabledarrowmenuitem_context, NULL, &menuitem2_height);
  *height += menuitem2_height;
  menuitem3_height = 0;
  query_size (menu_context, NULL, &menuitem5_height);
  query_size (menuitem_context, NULL, &menuitem3_height);
  query_size (checkmenuitem_context, NULL, &menuitem3_height);
  query_size (disabledcheckmenuitem_context, NULL, &menuitem3_height);
  *height += menuitem3_height;
  menuitem4_height = 0;
  query_size (menu_context, NULL, &menuitem5_height);
  query_size (separatormenuitem_context, NULL, &menuitem4_height);
  *height += menuitem4_height;
  menuitem5_height = 0;
  query_size (menu_context, NULL, &menuitem5_height);
  query_size (menuitem_context, NULL, &menuitem5_height);
  query_size (radiomenuitem_context, NULL, &menuitem5_height);
  query_size (disabledradiomenuitem_context, NULL, &menuitem5_height);
  *height += menuitem5_height;

  draw_style_common (menu_context, cr, x, y, width, *height,
                     &menu_x, &menu_y, &menu_width, &menu_height);

  /* Hovered with right arrow */
  ctk_style_context_get (hoveredarrowmenuitem_context, ctk_style_context_get_state (hoveredarrowmenuitem_context),
                         "min-width", &arrow_width, "min-height", &arrow_height, NULL);
  arrow_size = MIN (arrow_width, arrow_height);
  draw_style_common (hovermenuitem_context, cr, menu_x, menu_y, menu_width, menuitem1_height,
                     &contents_x, &contents_y, &contents_width, &contents_height);
  ctk_render_arrow (hoveredarrowmenuitem_context, cr, G_PI / 2,
                    contents_x + contents_width - arrow_size,
                    contents_y + (contents_height - arrow_size) / 2, arrow_size);

  /* Left arrow sensitive, and right arrow insensitive */
  draw_style_common (menuitem_context, cr, menu_x, menu_y + menuitem1_height, menu_width, menuitem2_height,
                     &contents_x, &contents_y, &contents_width, &contents_height);
  ctk_style_context_get (arrowmenuitem_context, ctk_style_context_get_state (arrowmenuitem_context),
                         "min-width", &arrow_width, "min-height", &arrow_height, NULL);
  arrow_size = MIN (arrow_width, arrow_height);
  ctk_render_arrow (arrowmenuitem_context, cr, G_PI / 2,
                    contents_x,
                    contents_y + (contents_height - arrow_size) / 2, arrow_size);
  ctk_style_context_get (disabledarrowmenuitem_context, ctk_style_context_get_state (disabledarrowmenuitem_context),
                         "min-width", &arrow_width, "min-height", &arrow_height, NULL);
  arrow_size = MIN (arrow_width, arrow_height);
  ctk_render_arrow (disabledarrowmenuitem_context, cr, G_PI / 2,
                    contents_x + contents_width - arrow_size,
                    contents_y + (contents_height - arrow_size) / 2, arrow_size);


  /* Left check enabled, sensitive, and right check unchecked, insensitive */
  draw_style_common (menuitem_context, cr, menu_x, menu_y + menuitem1_height + menuitem2_height, menu_width, menuitem3_height,
                     &contents_x, &contents_y, &contents_width, &contents_height);
  ctk_style_context_get (checkmenuitem_context, ctk_style_context_get_state (checkmenuitem_context),
                         "min-width", &toggle_width, "min-height", &toggle_height, NULL);
  draw_style_common (checkmenuitem_context, cr,
                     contents_x,
                     contents_y,
                     toggle_width, toggle_height,
                     &toggle_x, &toggle_y, &toggle_width, &toggle_height);
  ctk_render_check (checkmenuitem_context, cr, toggle_x, toggle_y, toggle_width, toggle_height);
  ctk_style_context_get (disabledcheckmenuitem_context, ctk_style_context_get_state (disabledcheckmenuitem_context),
                         "min-width", &toggle_width, "min-height", &toggle_height, NULL);
  draw_style_common (disabledcheckmenuitem_context, cr,
                     contents_x + contents_width - toggle_width,
                     contents_y,
                     toggle_width, toggle_height,
                     &toggle_x, &toggle_y, &toggle_width, &toggle_height);
  ctk_render_check (disabledcheckmenuitem_context, cr, toggle_x, toggle_y, toggle_width, toggle_height);

  /* Separator */
  draw_style_common (separatormenuitem_context, cr, menu_x, menu_y + menuitem1_height + menuitem2_height + menuitem3_height,
                     menu_width, menuitem4_height,
                     NULL, NULL, NULL, NULL);

  /* Left check enabled, sensitive, and right check unchecked, insensitive */
  draw_style_common (menuitem_context, cr, menu_x, menu_y + menuitem1_height + menuitem2_height + menuitem3_height + menuitem4_height,
                     menu_width, menuitem5_height,
                     &contents_x, &contents_y, &contents_width, &contents_height);
  ctk_style_context_get (radiomenuitem_context, ctk_style_context_get_state (radiomenuitem_context),
                         "min-width", &toggle_width, "min-height", &toggle_height, NULL);
  draw_style_common (radiomenuitem_context, cr,
                     contents_x,
                     contents_y,
                     toggle_width, toggle_height,
                     &toggle_x, &toggle_y, &toggle_width, &toggle_height);
  ctk_render_check (radiomenuitem_context, cr, toggle_x, toggle_y, toggle_width, toggle_height);
  ctk_style_context_get (disabledradiomenuitem_context, ctk_style_context_get_state (disabledradiomenuitem_context),
                         "min-width", &toggle_width, "min-height", &toggle_height, NULL);
  draw_style_common (disabledradiomenuitem_context, cr,
                     contents_x + contents_width - toggle_width,
                     contents_y,
                     toggle_width, toggle_height,
                     &toggle_x, &toggle_y, &toggle_width, &toggle_height);
  ctk_render_check (disabledradiomenuitem_context, cr, toggle_x, toggle_y, toggle_width, toggle_height);

  g_object_unref (menu_context);
  g_object_unref (menuitem_context);
  g_object_unref (hovermenuitem_context);
  g_object_unref (hoveredarrowmenuitem_context);
  g_object_unref (arrowmenuitem_context);
  g_object_unref (checkmenuitem_context);
  g_object_unref (disabledarrowmenuitem_context);
  g_object_unref (disabledcheckmenuitem_context);
  g_object_unref (radiomenuitem_context);
  g_object_unref (disablemenuitem_context);
  g_object_unref (disabledradiomenuitem_context);
  g_object_unref (separatormenuitem_context);
}

static void
draw_menubar (CtkWidget *widget G_GNUC_UNUSED,
              cairo_t   *cr,
              gint       x,
              gint       y,
              gint       width,
              gint      *height)
{
  CtkStyleContext *frame_context;
  CtkStyleContext *border_context;
  CtkStyleContext *menubar_context;
  CtkStyleContext *hovered_menuitem_context;
  CtkStyleContext *menuitem_context;
  gint contents_x, contents_y, contents_width, contents_height;
  gint item_width;

  /* Menubar background is the same color as our base background, so use a frame */
  frame_context = get_style (NULL, "frame");
  border_context = get_style (frame_context, "border");

  /* This information is taken from the CtkMenuBar docs, see "CSS nodes" */
  menubar_context = get_style (NULL, "menubar");
  hovered_menuitem_context = get_style (menubar_context, "menuitem:hover");
  menuitem_context = get_style (menubar_context, "menuitem");

  *height = 0;
  query_size (frame_context, NULL, height);
  query_size (border_context, NULL, height);
  query_size (menubar_context, NULL, height);
  query_size (hovered_menuitem_context, NULL, height);
  query_size (menuitem_context, NULL, height);

  draw_style_common (frame_context, cr, x, y, width, *height,
                     NULL, NULL, NULL, NULL);
  draw_style_common (border_context, cr, x, y, width, *height,
                     &contents_x, &contents_y, &contents_width, &contents_height);
  draw_style_common (menubar_context, cr, contents_x, contents_y, contents_width, contents_height,
                     NULL, NULL, NULL, NULL);
  item_width = contents_width / 3;
  draw_style_common (hovered_menuitem_context, cr, contents_x, contents_y, item_width, contents_height,
                     NULL, NULL, NULL, NULL);
  draw_style_common (menuitem_context, cr, contents_x + item_width * 2, contents_y, item_width, contents_height,
                     NULL, NULL, NULL, NULL);

  g_object_unref (menuitem_context);
  g_object_unref (hovered_menuitem_context);
  g_object_unref (menubar_context);
  g_object_unref (border_context);
  g_object_unref (frame_context);
}

static void
draw_notebook (CtkWidget     *widget G_GNUC_UNUSED,
               cairo_t       *cr,
               gint           x,
               gint           y,
               gint           width,
               gint           height)
{
  CtkStyleContext *notebook_context;
  CtkStyleContext *header_context;
  CtkStyleContext *tabs_context;
  CtkStyleContext *tab1_context, *tab2_context;
  CtkStyleContext *stack_context;
  gint header_height;
  gint contents_x, contents_y, contents_width, contents_height;

  /* This information is taken from the CtkNotebook docs, see "CSS nodes" */
  notebook_context = get_style (NULL, "notebook.frame");
  header_context = get_style (notebook_context, "header.top");
  tabs_context = get_style (header_context, "tabs");
  tab1_context = get_style (tabs_context, "tab:checked");
  tab2_context = get_style (tabs_context, "tab:hover");
  stack_context = get_style (notebook_context, "stack");

  header_height = 0;
  query_size (notebook_context, NULL, &header_height);
  query_size (header_context, NULL, &header_height);
  query_size (tabs_context, NULL, &header_height);
  query_size (tab1_context, NULL, &header_height);
  query_size (tab2_context, NULL, &header_height);

  draw_style_common (notebook_context, cr, x, y, width, height, NULL, NULL, NULL, NULL);
  draw_style_common (header_context, cr, x, y, width, header_height, NULL, NULL, NULL, NULL);
  draw_style_common (tabs_context, cr, x, y, width, header_height, NULL, NULL, NULL, NULL);
  draw_style_common (tab1_context, cr, x, y, width / 2, header_height,
                     &contents_x, &contents_y, &contents_width, &contents_height);
  draw_style_common (tab2_context, cr, x + width / 2, y, width / 2, header_height,
                     NULL, NULL, NULL, NULL);
  draw_style_common (stack_context, cr, x, y + header_height, width,height - header_height,
                     NULL, NULL, NULL, NULL);

  g_object_unref (stack_context);
  g_object_unref (tabs_context);
  g_object_unref (tab1_context);
  g_object_unref (tab2_context);
  g_object_unref (header_context);
  g_object_unref (notebook_context);
}

static void
draw_horizontal_scrollbar (CtkWidget     *widget G_GNUC_UNUSED,
                           cairo_t       *cr,
                           gint           x,
                           gint           y,
                           gint           width,
                           gint           position,
                           CtkStateFlags  state,
                           gint          *height)
{
  CtkStyleContext *scrollbar_context;
  CtkStyleContext *contents_context;
  CtkStyleContext *trough_context;
  CtkStyleContext *slider_context;
  gint slider_width;

  /* This information is taken from the CtkScrollbar docs, see "CSS nodes" */
  scrollbar_context = get_style (NULL, "scrollbar.horizontal.bottom");
  contents_context = get_style (scrollbar_context, "contents");
  trough_context = get_style (contents_context, "trough");
  slider_context = get_style (trough_context, "slider");

  ctk_style_context_set_state (scrollbar_context, state);
  ctk_style_context_set_state (contents_context, state);
  ctk_style_context_set_state (trough_context, state);
  ctk_style_context_set_state (slider_context, state);

  *height = 0;
  query_size (scrollbar_context, NULL, height);
  query_size (contents_context, NULL, height);
  query_size (trough_context, NULL, height);
  query_size (slider_context, NULL, height);

  ctk_style_context_get (slider_context, ctk_style_context_get_state (slider_context),
                         "min-width", &slider_width, NULL);

  draw_style_common (scrollbar_context, cr, x, y, width, *height, NULL, NULL, NULL, NULL);
  draw_style_common (contents_context, cr, x, y, width, *height, NULL, NULL, NULL, NULL);
  draw_style_common (trough_context, cr, x, y, width, *height, NULL, NULL, NULL, NULL);
  draw_style_common (slider_context, cr, x + position, y, slider_width, *height, NULL, NULL, NULL, NULL);

  g_object_unref (slider_context);
  g_object_unref (trough_context);
  g_object_unref (contents_context);
  g_object_unref (scrollbar_context);
}

static void
draw_text (CtkWidget     *widget,
           cairo_t       *cr,
           gint           x,
           gint           y,
           gint           width,
           gint           height,
           const gchar   *text,
           CtkStateFlags  state)
{
  CtkStyleContext *label_context;
  CtkStyleContext *selection_context;
  CtkStyleContext *context;
  PangoLayout *layout;

  /* This information is taken from the CtkLabel docs, see "CSS nodes" */
  label_context = get_style (NULL, "label.view");
  selection_context = get_style (label_context, "selection");

  ctk_style_context_set_state (label_context, state);

  if (state & CTK_STATE_FLAG_SELECTED)
    context = selection_context;
  else
    context = label_context;

  layout = ctk_widget_create_pango_layout (widget, text);

  ctk_render_background (context, cr, x, y, width, height);
  ctk_render_frame (context, cr, x, y, width, height);
  ctk_render_layout (context, cr, x, y, layout);

  g_object_unref (layout);

  g_object_unref (selection_context);
  g_object_unref (label_context);
}

static void
draw_check (CtkWidget     *widget G_GNUC_UNUSED,
            cairo_t       *cr,
            gint           x,
            gint           y,
            CtkStateFlags  state,
            gint          *width,
            gint          *height)
{
  CtkStyleContext *button_context;
  CtkStyleContext *check_context;
  gint contents_x, contents_y, contents_width, contents_height;

  /* This information is taken from the CtkCheckButton docs, see "CSS nodes" */
  button_context = get_style (NULL, "checkbutton");
  check_context = get_style (button_context, "check");

  ctk_style_context_set_state (check_context, state);

  *width = *height = 0;
  query_size (button_context, width, height);
  query_size (check_context, width, height);

  draw_style_common (button_context, cr, x, y, *width, *height, NULL, NULL, NULL, NULL);
  draw_style_common (check_context, cr, x, y, *width, *height,
                     &contents_x, &contents_y, &contents_width, &contents_height);
  ctk_render_check (check_context, cr, contents_x, contents_y, contents_width, contents_height);

  g_object_unref (check_context);
  g_object_unref (button_context);

}

static void
draw_radio (CtkWidget     *widget G_GNUC_UNUSED,
            cairo_t       *cr,
            gint           x,
            gint           y,
            CtkStateFlags  state,
            gint          *width,
            gint          *height)
{
  CtkStyleContext *button_context;
  CtkStyleContext *check_context;
  gint contents_x, contents_y, contents_width, contents_height;

  /* This information is taken from the CtkRadioButton docs, see "CSS nodes" */
  button_context = get_style (NULL, "radiobutton");
  check_context = get_style (button_context, "radio");

  ctk_style_context_set_state (check_context, state);

  *width = *height = 0;
  query_size (button_context, width, height);
  query_size (check_context, width, height);

  draw_style_common (button_context, cr, x, y, *width, *height, NULL, NULL, NULL, NULL);
  draw_style_common (check_context, cr, x, y, *width, *height,
                     &contents_x, &contents_y, &contents_width, &contents_height);
  ctk_render_check (check_context, cr, contents_x, contents_y, contents_width, contents_height);

  g_object_unref (check_context);
  g_object_unref (button_context);

}

static void
draw_progress (CtkWidget *widget G_GNUC_UNUSED,
               cairo_t   *cr,
               gint       x,
               gint       y,
               gint       width,
               gint       position,
               gint      *height)
{
  CtkStyleContext *bar_context;
  CtkStyleContext *trough_context;
  CtkStyleContext *progress_context;

  /* This information is taken from the CtkProgressBar docs, see "CSS nodes" */
  bar_context = get_style (NULL, "progressbar.horizontal");
  trough_context = get_style (bar_context, "trough");
  progress_context = get_style (trough_context, "progress.left");

  *height = 0;
  query_size (bar_context, NULL, height);
  query_size (trough_context, NULL, height);
  query_size (progress_context, NULL, height);

  draw_style_common (bar_context, cr, x, y, width, *height, NULL, NULL, NULL, NULL);
  draw_style_common (trough_context, cr, x, y, width, *height, NULL, NULL, NULL, NULL);
  draw_style_common (progress_context, cr, x, y, position, *height, NULL, NULL, NULL, NULL);

  g_object_unref (progress_context);
  g_object_unref (trough_context);
  g_object_unref (bar_context);
}

static void
draw_scale (CtkWidget *widget G_GNUC_UNUSED,
            cairo_t   *cr,
            gint       x,
            gint       y,
            gint       width,
            gint       position,
            gint      *height)
{
  CtkStyleContext *scale_context;
  CtkStyleContext *contents_context;
  CtkStyleContext *trough_context;
  CtkStyleContext *slider_context;
  CtkStyleContext *highlight_context;
  gint contents_x, contents_y, contents_width, contents_height;
  gint trough_height, slider_height;

  scale_context = get_style (NULL, "scale.horizontal");
  contents_context = get_style (scale_context, "contents");
  trough_context = get_style (contents_context, "trough");
  slider_context = get_style (trough_context, "slider");
  highlight_context = get_style (trough_context, "highlight.top");

  *height = 0;
  query_size (scale_context, NULL, height);
  query_size (contents_context, NULL, height);
  query_size (trough_context, NULL, height);
  query_size (slider_context, NULL, height);
  query_size (highlight_context, NULL, height);

  draw_style_common (scale_context, cr, x, y, width, *height,
                     &contents_x, &contents_y, &contents_width, &contents_height);
  draw_style_common (contents_context, cr, contents_x, contents_y, contents_width, contents_height,
                     &contents_x, &contents_y, &contents_width, &contents_height);
  /* Scale trough defines its size querying slider and highlight */
  trough_height = 0;
  query_size (trough_context, NULL, &trough_height);
  slider_height = 0;
  query_size (slider_context, NULL, &slider_height);
  query_size (highlight_context, NULL, &slider_height);
  trough_height += slider_height;
  draw_style_common (trough_context, cr, contents_x, contents_y, contents_width, trough_height,
                     &contents_x, &contents_y, &contents_width, &contents_height);
  draw_style_common (highlight_context, cr, contents_x, contents_y,
                     contents_width / 2, contents_height,
                     NULL, NULL, NULL, NULL);
  draw_style_common (slider_context, cr, contents_x + position, contents_y, contents_height, contents_height,
                     NULL, NULL, NULL, NULL);

  g_object_unref (scale_context);
  g_object_unref (contents_context);
  g_object_unref (trough_context);
  g_object_unref (slider_context);
  g_object_unref (highlight_context);
}

static void
draw_combobox (CtkWidget *widget G_GNUC_UNUSED,
               cairo_t   *cr,
               gint       x,
               gint       y,
               gint       width,
               gboolean   has_entry,
               gint      *height)
{
  CtkStyleContext *combo_context;
  CtkStyleContext *box_context;
  CtkStyleContext *button_context;
  CtkStyleContext *button_box_context;
  CtkStyleContext *entry_context;
  CtkStyleContext *arrow_context;
  gint contents_x, contents_y, contents_width, contents_height;
  gint button_width;
  gint arrow_width, arrow_height, arrow_size;

  /* This information is taken from the CtkComboBox docs, see "CSS nodes" */
  combo_context = get_style (NULL, "combobox:focus");
  box_context = get_style (combo_context, "box.horizontal.linked");
  if (has_entry)
    {
      const char *siblings[3] = { "entry.combo:focus", "button.combo" , NULL };

      entry_context = get_style_with_siblings (box_context, "entry.combo:focus", siblings, 0);
      button_context = get_style_with_siblings (box_context, "button.combo", siblings, 1);
    }
  else
    {
      const char *siblings[2] = { "button.combo" , NULL };

      button_context = get_style_with_siblings (box_context, "button.combo", siblings, 0);
    }
  button_box_context = get_style (button_context, "box.horizontal");
  arrow_context = get_style (button_box_context, "arrow");

  *height = 0;
  query_size (combo_context, NULL, height);
  query_size (box_context, NULL, height);
  if (has_entry)
    query_size (entry_context, NULL, height);
  query_size (button_context, NULL, height);
  query_size (button_box_context, NULL, height);
  query_size (arrow_context, NULL, height);

  ctk_style_context_get (arrow_context, ctk_style_context_get_state (arrow_context),
                         "min-width", &arrow_width, "min-height", &arrow_height, NULL);
  arrow_size = MIN (arrow_width, arrow_height);

  draw_style_common (combo_context, cr, x, y, width, *height, NULL, NULL, NULL, NULL);
  draw_style_common (box_context, cr, x, y, width, *height, NULL, NULL, NULL, NULL);
  if (has_entry)
    {
      button_width = *height;
      draw_style_common (entry_context, cr, x, y, width - button_width, *height, NULL, NULL, NULL, NULL);
      draw_style_common (button_context, cr, x + width - button_width, y, button_width, *height,
                         &contents_x, &contents_y, &contents_width, &contents_height);
    }
  else
    {
      button_width = width;
      draw_style_common (button_context, cr, x, y, width, *height,
                         &contents_x, &contents_y, &contents_width, &contents_height);
    }

  draw_style_common (button_box_context, cr, contents_x, contents_y, contents_width, contents_height,
                     NULL, NULL, NULL, NULL);
  draw_style_common (arrow_context, cr, contents_x, contents_y, contents_width, contents_height,
                     NULL, NULL, NULL, NULL);
  ctk_render_arrow (arrow_context, cr, G_PI / 2,
                    contents_x + contents_width - arrow_size,
                    contents_y + (contents_height - arrow_size) / 2, arrow_size);

  g_object_unref (arrow_context);
  if (has_entry)
    g_object_unref (entry_context);
  g_object_unref (button_context);
  g_object_unref (combo_context);
}

static void
draw_spinbutton (CtkWidget *widget,
                 cairo_t   *cr,
                 gint       x,
                 gint       y,
                 gint       width,
                 gint      *height)
{
  CtkStyleContext *spin_context;
  CtkStyleContext *entry_context;
  CtkStyleContext *up_context;
  CtkStyleContext *down_context;
  CtkIconTheme *icon_theme;
  CtkIconInfo *icon_info;
  GdkPixbuf *pixbuf;
  gint icon_width, icon_height, icon_size;
  gint button_width;
  gint contents_x, contents_y, contents_width, contents_height;

  /* This information is taken from the CtkSpinButton docs, see "CSS nodes" */
  spin_context = get_style (NULL, "spinbutton.horizontal:focus");
  entry_context = get_style (spin_context, "entry:focus");
  up_context = get_style (spin_context, "button.up:focus:active");
  down_context = get_style (spin_context, "button.down:focus");

  *height = 0;
  query_size (spin_context, NULL, height);
  query_size (entry_context, NULL, height);
  query_size (up_context, NULL, height);
  query_size (down_context, NULL, height);
  button_width = *height;

  draw_style_common (spin_context, cr, x, y, width, *height, NULL, NULL, NULL, NULL);
  draw_style_common (entry_context, cr, x, y, width, *height, NULL, NULL, NULL, NULL);

  icon_theme = ctk_icon_theme_get_for_screen (ctk_widget_get_screen (widget));

  ctk_style_context_get (up_context, ctk_style_context_get_state (up_context),
                         "min-width", &icon_width, "min-height", &icon_height, NULL);
  icon_size = MIN (icon_width, icon_height);
  icon_info = ctk_icon_theme_lookup_icon (icon_theme, "list-add-symbolic", icon_size, 0);
  pixbuf = ctk_icon_info_load_symbolic_for_context (icon_info, up_context, NULL, NULL);
  g_object_unref (icon_info);
  draw_style_common (up_context, cr, x + width - button_width, y, button_width, *height,
                     &contents_x, &contents_y, &contents_width, &contents_height);
  ctk_render_icon (up_context, cr, pixbuf, contents_x, contents_y + (contents_height - icon_size) / 2);
  g_object_unref (pixbuf);


  ctk_style_context_get (down_context, ctk_style_context_get_state (down_context),
                         "min-width", &icon_width, "min-height", &icon_height, NULL);
  icon_size = MIN (icon_width, icon_height);
  icon_info = ctk_icon_theme_lookup_icon (icon_theme, "list-remove-symbolic", icon_size, 0);
  pixbuf = ctk_icon_info_load_symbolic_for_context (icon_info, down_context, NULL, NULL);
  g_object_unref (icon_info);
  draw_style_common (down_context, cr, x + width - 2 * button_width, y, button_width, *height,
                     &contents_x, &contents_y, &contents_width, &contents_height);
  ctk_render_icon (down_context, cr, pixbuf, contents_x, contents_y + (contents_height - icon_size) / 2);
  g_object_unref (pixbuf);

  g_object_unref (down_context);
  g_object_unref (up_context);
  g_object_unref (entry_context);
  g_object_unref (spin_context);
}

static gboolean
draw_cb (CtkWidget *widget,
         cairo_t   *cr)
{
  gint panewidth, width, height;
  gint x, y;

  width = ctk_widget_get_allocated_width (widget);
  panewidth = width / 2;
  height = ctk_widget_get_allocated_height (widget);

  cairo_rectangle (cr, 0, 0, width, height);
  cairo_set_source_rgb (cr, 0.9, 0.9, 0.9);
  cairo_fill (cr);

  x = y = 10;
  draw_horizontal_scrollbar (widget, cr, x, y, panewidth - 20, 30, CTK_STATE_FLAG_NORMAL, &height);
  y += height + 8;
  draw_horizontal_scrollbar (widget, cr, x, y, panewidth - 20, 40, CTK_STATE_FLAG_PRELIGHT, &height);
  y += height + 8;
  draw_horizontal_scrollbar (widget, cr, x, y, panewidth - 20, 50, CTK_STATE_FLAG_ACTIVE|CTK_STATE_FLAG_PRELIGHT, &height);

  y += height + 8;
  draw_text (widget, cr, x,  y, panewidth - 20, 20, "Not selected", CTK_STATE_FLAG_NORMAL);
  y += 20 + 10;
  draw_text (widget, cr, x, y, panewidth - 20, 20, "Selected", CTK_STATE_FLAG_SELECTED);

  x = 10;
  y += 20 + 10;
  draw_check (widget, cr,  x, y, CTK_STATE_FLAG_NORMAL, &width, &height);
  x += width + 10;
  draw_check (widget, cr,  x, y, CTK_STATE_FLAG_CHECKED, &width, &height);
  x += width + 10;
  draw_radio (widget, cr,  x, y, CTK_STATE_FLAG_NORMAL, &width, &height);
  x += width + 10;
  draw_radio (widget, cr, x, y, CTK_STATE_FLAG_CHECKED, &width, &height);
  x = 10;

  y += height + 10;
  draw_progress (widget, cr, x, y, panewidth - 20, 50, &height);

  y += height + 10;
  draw_scale (widget, cr, x, y, panewidth - 20, 75, &height);

  y += height + 20;
  draw_notebook (widget, cr, x, y, panewidth - 20, 160);

  /* Second column */
  x += panewidth;
  y = 10;
  draw_menu (widget, cr, x, y, panewidth - 20, &height);

  y += height + 10;
  draw_menubar (widget, cr, x, y, panewidth - 20, &height);

  y += height + 20;
  draw_spinbutton (widget, cr, x, y, panewidth - 20, &height);

  y += height + 30;
  draw_combobox (widget, cr, x, y, panewidth - 20, FALSE, &height);

  y += height + 10;
  draw_combobox (widget, cr, 10 + panewidth, y, panewidth - 20, TRUE, &height);

  return FALSE;
}

CtkWidget *
do_foreigndrawing (CtkWidget *do_widget)
{
  static CtkWidget *window = NULL;

  if (!window)
    {
      CtkWidget *box;
      CtkWidget *da;

      window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
      ctk_window_set_title (CTK_WINDOW (window), "Foreign drawing");
      ctk_window_set_screen (CTK_WINDOW (window),
                             ctk_widget_get_screen (do_widget));
      g_signal_connect (window, "destroy",
                        G_CALLBACK (ctk_widget_destroyed), &window);

      box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 10);
      ctk_container_add (CTK_CONTAINER (window), box);
      da = ctk_drawing_area_new ();
      ctk_widget_set_size_request (da, 400, 400);
      ctk_widget_set_hexpand (da, TRUE);
      ctk_widget_set_vexpand (da, TRUE);
      ctk_widget_set_app_paintable (da, TRUE);
      ctk_container_add (CTK_CONTAINER (box), da);

      g_signal_connect (da, "draw", G_CALLBACK (draw_cb), NULL);
    }

  if (!ctk_widget_get_visible (window))
    ctk_widget_show_all (window);
  else
    ctk_widget_destroy (window);

  return window;
}
