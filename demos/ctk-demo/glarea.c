/* OpenGL Area
 *
 * CtkGLArea is a widget that allows custom drawing using OpenGL calls.
 */

#include <math.h>
#include <ctk/ctk.h>
#include <epoxy/gl.h>

static CtkWidget *demo_window = NULL;

/* the CtkGLArea widget */
static CtkWidget *gl_area = NULL;

enum {
  X_AXIS,
  Y_AXIS,
  Z_AXIS,

  N_AXIS
};

/* Rotation angles on each axis */
static float rotation_angles[N_AXIS] = { 0.0 };

/* The object we are drawing */
static const GLfloat vertex_data[] = {
  0.f,   0.5f,   0.f, 1.f,
  0.5f, -0.366f, 0.f, 1.f,
 -0.5f, -0.366f, 0.f, 1.f,
};

/* Initialize the GL buffers */
static void
init_buffers (GLuint *vao_out,
              GLuint *buffer_out)
{
  GLuint vao, buffer;

  /* We only use one VAO, so we always keep it bound */
  glGenVertexArrays (1, &vao);
  glBindVertexArray (vao);

  /* This is the buffer that holds the vertices */
  glGenBuffers (1, &buffer);
  glBindBuffer (GL_ARRAY_BUFFER, buffer);
  glBufferData (GL_ARRAY_BUFFER, sizeof (vertex_data), vertex_data, GL_STATIC_DRAW);
  glBindBuffer (GL_ARRAY_BUFFER, 0);

  if (vao_out != NULL)
    *vao_out = vao;

  if (buffer_out != NULL)
    *buffer_out = buffer;
}

/* Create and compile a shader */
static GLuint
create_shader (int         type,
               const char *src)
{
  GLuint shader;
  int status;

  shader = glCreateShader (type);
  glShaderSource (shader, 1, &src, NULL);
  glCompileShader (shader);

  glGetShaderiv (shader, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE)
    {
      int log_len;
      char *buffer;

      glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &log_len);

      buffer = g_malloc (log_len + 1);
      glGetShaderInfoLog (shader, log_len, NULL, buffer);

      g_warning ("Compile failure in %s shader:\n%s",
                 type == GL_VERTEX_SHADER ? "vertex" : "fragment",
                 buffer);

      g_free (buffer);

      glDeleteShader (shader);

      return 0;
    }

  return shader;
}

/* Initialize the shaders and link them into a program */
static void
init_shaders (const char *vertex_path,
              const char *fragment_path,
              GLuint *program_out,
              GLuint *mvp_out)
{
  GLuint vertex, fragment;
  GLuint program = 0;
  GLuint mvp = 0;
  int status;
  GBytes *source;

  source = g_resources_lookup_data (vertex_path, 0, NULL);
  vertex = create_shader (GL_VERTEX_SHADER, g_bytes_get_data (source, NULL));
  g_bytes_unref (source);

  if (vertex == 0)
    {
      *program_out = 0;
      return;
    }

  source = g_resources_lookup_data (fragment_path, 0, NULL);
  fragment = create_shader (GL_FRAGMENT_SHADER, g_bytes_get_data (source, NULL));
  g_bytes_unref (source);

  if (fragment == 0)
    {
      glDeleteShader (vertex);
      *program_out = 0;
      return;
    }

  program = glCreateProgram ();
  glAttachShader (program, vertex);
  glAttachShader (program, fragment);

  glLinkProgram (program);

  glGetProgramiv (program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE)
    {
      int log_len;
      char *buffer;

      glGetProgramiv (program, GL_INFO_LOG_LENGTH, &log_len);

      buffer = g_malloc (log_len + 1);
      glGetProgramInfoLog (program, log_len, NULL, buffer);

      g_warning ("Linking failure:\n%s", buffer);

      g_free (buffer);

      glDeleteProgram (program);
      program = 0;

      goto out;
    }

  /* Get the location of the "mvp" uniform */
  mvp = glGetUniformLocation (program, "mvp");

  glDetachShader (program, vertex);
  glDetachShader (program, fragment);

out:
  glDeleteShader (vertex);
  glDeleteShader (fragment);

  if (program_out != NULL)
    *program_out = program;

  if (mvp_out != NULL)
    *mvp_out = mvp;
}

static void
compute_mvp (float *res,
             float  phi,
             float  theta,
             float  psi)
{
  float x = phi * (G_PI / 180.f);
  float y = theta * (G_PI / 180.f);
  float z = psi * (G_PI / 180.f);
  float c1 = cosf (x), s1 = sinf (x);
  float c2 = cosf (y), s2 = sinf (y);
  float c3 = cosf (z), s3 = sinf (z);
  float c3c2 = c3 * c2;
  float s3c1 = s3 * c1;
  float c3s2s1 = c3 * s2 * s1;
  float s3s1 = s3 * s1;
  float c3s2c1 = c3 * s2 * c1;
  float s3c2 = s3 * c2;
  float c3c1 = c3 * c1;
  float s3s2s1 = s3 * s2 * s1;
  float c3s1 = c3 * s1;
  float s3s2c1 = s3 * s2 * c1;
  float c2s1 = c2 * s1;
  float c2c1 = c2 * c1;

  /* initialize to the identity matrix */
  res[0] = 1.f; res[4] = 0.f;  res[8] = 0.f; res[12] = 0.f;
  res[1] = 0.f; res[5] = 1.f;  res[9] = 0.f; res[13] = 0.f;
  res[2] = 0.f; res[6] = 0.f; res[10] = 1.f; res[14] = 0.f;
  res[3] = 0.f; res[7] = 0.f; res[11] = 0.f; res[15] = 1.f;

  /* apply all three rotations using the three matrices:
   *
   * ⎡  c3 s3 0 ⎤ ⎡ c2  0 -s2 ⎤ ⎡ 1   0  0 ⎤
   * ⎢ -s3 c3 0 ⎥ ⎢  0  1   0 ⎥ ⎢ 0  c1 s1 ⎥
   * ⎣   0  0 1 ⎦ ⎣ s2  0  c2 ⎦ ⎣ 0 -s1 c1 ⎦
   */
  res[0] = c3c2;  res[4] = s3c1 + c3s2s1;  res[8] = s3s1 - c3s2c1; res[12] = 0.f;
  res[1] = -s3c2; res[5] = c3c1 - s3s2s1;  res[9] = c3s1 + s3s2c1; res[13] = 0.f;
  res[2] = s2;    res[6] = -c2s1;         res[10] = c2c1;          res[14] = 0.f;
  res[3] = 0.f;   res[7] = 0.f;           res[11] = 0.f;           res[15] = 1.f;
}

static GLuint position_buffer;
static GLuint program;
static GLuint mvp_location;

/* We need to set up our state when we realize the CtkGLArea widget */
static void
realize (CtkWidget *widget)
{
  const char *vertex_path, *fragment_path;
  CdkGLContext *context;

  ctk_gl_area_make_current (CTK_GL_AREA (widget));

  if (ctk_gl_area_get_error (CTK_GL_AREA (widget)) != NULL)
    return;

  context = ctk_gl_area_get_context (CTK_GL_AREA (widget));

  if (cdk_gl_context_get_use_es (context))
    {
      vertex_path = "/glarea/glarea-gles.vs.glsl";
      fragment_path = "/glarea/glarea-gles.fs.glsl";
    }
  else
    {
      vertex_path = "/glarea/glarea-gl.vs.glsl";
      fragment_path = "/glarea/glarea-gl.fs.glsl";
    }

  init_buffers (&position_buffer, NULL);
  init_shaders (vertex_path, fragment_path, &program, &mvp_location);
}

/* We should tear down the state when unrealizing */
static void
unrealize (CtkWidget *widget)
{
  ctk_gl_area_make_current (CTK_GL_AREA (widget));

  if (ctk_gl_area_get_error (CTK_GL_AREA (widget)) != NULL)
    return;

  glDeleteBuffers (1, &position_buffer);
  glDeleteProgram (program);
}

static void
draw_triangle (void)
{
  float mvp[16];

  /* Compute the model view projection matrix using the
   * rotation angles specified through the CtkRange widgets
   */
  compute_mvp (mvp,
               rotation_angles[X_AXIS],
               rotation_angles[Y_AXIS],
               rotation_angles[Z_AXIS]);

  /* Use our shaders */
  glUseProgram (program);

  /* Update the "mvp" matrix we use in the shader */
  glUniformMatrix4fv (mvp_location, 1, GL_FALSE, &mvp[0]);

  /* Use the vertices in our buffer */
  glBindBuffer (GL_ARRAY_BUFFER, position_buffer);
  glEnableVertexAttribArray (0);
  glVertexAttribPointer (0, 4, GL_FLOAT, GL_FALSE, 0, 0);

  /* Draw the three vertices as a triangle */
  glDrawArrays (GL_TRIANGLES, 0, 3);

  /* We finished using the buffers and program */
  glDisableVertexAttribArray (0);
  glBindBuffer (GL_ARRAY_BUFFER, 0);
  glUseProgram (0);
}

static gboolean
render (CtkGLArea    *area,
        CdkGLContext *context G_GNUC_UNUSED)
{
  if (ctk_gl_area_get_error (area) != NULL)
    return FALSE;

  /* Clear the viewport */
  glClearColor (0.5, 0.5, 0.5, 1.0);
  glClear (GL_COLOR_BUFFER_BIT);

  /* Draw our object */
  draw_triangle ();

  /* Flush the contents of the pipeline */
  glFlush ();

  return TRUE;
}

static void
on_axis_value_change (CtkAdjustment *adjustment,
                      gpointer       data)
{
  int axis = GPOINTER_TO_INT (data);

  g_assert (axis >= 0 && axis < N_AXIS);

  /* Update the rotation angle */
  rotation_angles[axis] = ctk_adjustment_get_value (adjustment);

  /* Update the contents of the GL drawing area */
  ctk_widget_queue_draw (gl_area);
}

static CtkWidget *
create_axis_slider (int axis)
{
  CtkWidget *box, *label, *slider;
  CtkAdjustment *adj;
  const char *text;

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);

  switch (axis)
    {
    case X_AXIS:
      text = "X axis";
      break;

    case Y_AXIS:
      text = "Y axis";
      break;

    case Z_AXIS:
      text = "Z axis";
      break;

    default:
      g_assert_not_reached ();
    }

  label = ctk_label_new (text);
  ctk_container_add (CTK_CONTAINER (box), label);
  ctk_widget_show (label);

  adj = ctk_adjustment_new (0.0, 0.0, 360.0, 1.0, 12.0, 0.0);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (on_axis_value_change),
                    GINT_TO_POINTER (axis));
  slider = ctk_scale_new (CTK_ORIENTATION_HORIZONTAL, adj);
  ctk_container_add (CTK_CONTAINER (box), slider);
  ctk_widget_set_hexpand (slider, TRUE);
  ctk_widget_show (slider);

  ctk_widget_show (box);

  return box;
}

static void
close_window (CtkWidget *widget G_GNUC_UNUSED)
{
  /* Reset the state */
  demo_window = NULL;
  gl_area = NULL;

  rotation_angles[X_AXIS] = 0.0;
  rotation_angles[Y_AXIS] = 0.0;
  rotation_angles[Z_AXIS] = 0.0;
}

CtkWidget *
create_glarea_window (CtkWidget *do_widget)
{
  CtkWidget *window, *box, *button, *controls;
  int i;

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_screen (CTK_WINDOW (window), ctk_widget_get_screen (do_widget));
  ctk_window_set_title (CTK_WINDOW (window), "OpenGL Area");
  ctk_window_set_default_size (CTK_WINDOW (window), 400, 600);
  ctk_container_set_border_width (CTK_CONTAINER (window), 12);
  g_signal_connect (window, "destroy", G_CALLBACK (close_window), NULL);

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, FALSE);
  ctk_box_set_spacing (CTK_BOX (box), 6);
  ctk_container_add (CTK_CONTAINER (window), box);

  gl_area = ctk_gl_area_new ();
  ctk_widget_set_hexpand (gl_area, TRUE);
  ctk_widget_set_vexpand (gl_area, TRUE);
  ctk_container_add (CTK_CONTAINER (box), gl_area);

  /* We need to initialize and free GL resources, so we use
   * the realize and unrealize signals on the widget
   */
  g_signal_connect (gl_area, "realize", G_CALLBACK (realize), NULL);
  g_signal_connect (gl_area, "unrealize", G_CALLBACK (unrealize), NULL);

  /* The main "draw" call for CtkGLArea */
  g_signal_connect (gl_area, "render", G_CALLBACK (render), NULL);

  controls = ctk_box_new (CTK_ORIENTATION_VERTICAL, FALSE);
  ctk_container_add (CTK_CONTAINER (box), controls);
  ctk_widget_set_hexpand (controls, TRUE);

  for (i = 0; i < N_AXIS; i++)
    ctk_container_add (CTK_CONTAINER (controls), create_axis_slider (i));

  button = ctk_button_new_with_label ("Quit");
  ctk_widget_set_hexpand (button, TRUE);
  ctk_container_add (CTK_CONTAINER (box), button);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (ctk_widget_destroy), window);

  return window;
}

CtkWidget*
do_glarea (CtkWidget *do_widget)
{
  if (demo_window == NULL)
    demo_window = create_glarea_window (do_widget);

  if (!ctk_widget_get_visible (demo_window))
    ctk_widget_show_all (demo_window);
  else
    ctk_widget_destroy (demo_window);

  return demo_window;
}
