#include <math.h>
#include <stdlib.h>
#include <ctk/ctk.h>

#include <epoxy/gl.h>

enum {
  X_AXIS,
  Y_AXIS,
  Z_AXIS,

  N_AXIS
};

static float rotation_angles[N_AXIS] = { 0.0 };

static CtkWidget *gl_area;

static const GLfloat vertex_data[] = {
 0.f, 0.5f, 0.f, 1.f,
 0.5f, -0.366f, 0.f, 1.f,
 -0.5f, -0.366f, 0.f, 1.f,
};

static void
init_buffers (GLuint *vao_out,
              GLuint *buffer_out)
{
  GLuint vao, buffer;

  /* we only use one VAO, so we always keep it bound */
  glGenVertexArrays (1, &vao);
  glBindVertexArray (vao);

  glGenBuffers (1, &buffer);

  glBindBuffer (GL_ARRAY_BUFFER, buffer);
  glBufferData (GL_ARRAY_BUFFER, sizeof (vertex_data), vertex_data, GL_STATIC_DRAW);
  glBindBuffer (GL_ARRAY_BUFFER, 0);

  if (vao_out != NULL)
    *vao_out = vao;

  if (buffer_out != NULL)
    *buffer_out = buffer;
}

static GLuint
create_shader (int type, const char *src)
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

static const char *vertex_shader_code_gles =
"attribute vec4 position;\n" \
"uniform mat4 mvp;\n" \
"void main() {\n" \
"  gl_Position = mvp * position;\n" \
"}";

static const char *fragment_shader_code_gles =
"precision mediump float;\n" \
"void main() {\n" \
"  float lerpVal = gl_FragCoord.y / 400.0;\n" \
"  gl_FragColor = mix(vec4(1.0, 0.85, 0.35, 1.0), vec4(0.2, 0.2, 0.2, 1.0), lerpVal);\n" \
"}";

static const char *vertex_shader_code_330 =
"#version 330\n" \
"\n" \
"layout(location = 0) in vec4 position;\n" \
"uniform mat4 mvp;\n"
"void main() {\n" \
"  gl_Position = mvp * position;\n" \
"}";

static const char *vertex_shader_code_legacy =
"#version 130\n" \
"\n" \
"attribute vec4 position;\n" \
"uniform mat4 mvp;\n" \
"void main() {\n" \
"  gl_Position = mvp * position;\n" \
"}";

static const char *fragment_shader_code_330 =
"#version 330\n" \
"\n" \
"out vec4 outputColor;\n" \
"void main() {\n" \
"  float lerpVal = gl_FragCoord.y / 400.0f;\n" \
"  outputColor = mix(vec4(1.0f, 0.85f, 0.35f, 1.0f), vec4(0.2f, 0.2f, 0.2f, 1.0f), lerpVal);\n" \
"}";

static const char *fragment_shader_code_legacy =
"#version 130\n" \
"\n" \
"void main() {\n" \
"  float lerpVal = gl_FragCoord.y / 400.0f;\n" \
"  gl_FragColor = mix(vec4(1.0f, 0.85f, 0.35f, 1.0f), vec4(0.2f, 0.2, 0.2f, 1.0f), lerpVal);\n" \
"}";

static void
init_shaders (const char *vertex_shader_code,
              const char *fragment_shader_code,
              GLuint *program_out,
              GLuint *mvp_out)
{
  GLuint vertex, fragment;
  GLuint program = 0;
  GLuint mvp = 0;
  int status;

  vertex = create_shader (GL_VERTEX_SHADER, vertex_shader_code);
  if (vertex == 0)
    {
      *program_out = 0;
      return;
    }

  fragment = create_shader (GL_FRAGMENT_SHADER, fragment_shader_code);
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

static void
realize (CtkWidget *widget)
{
  const char *fragment, *vertex;
  GdkGLContext *context;

  ctk_gl_area_make_current (CTK_GL_AREA (widget));

  if (ctk_gl_area_get_error (CTK_GL_AREA (widget)) != NULL)
    return;

  context = ctk_gl_area_get_context (CTK_GL_AREA (widget));
  if (gdk_gl_context_get_use_es (context))
    {
      vertex = vertex_shader_code_gles;
      fragment = fragment_shader_code_gles;
    }
  else
    {
      if (!gdk_gl_context_is_legacy (context))
        {
          vertex = vertex_shader_code_330;
          fragment = fragment_shader_code_330;
        }
      else
        {
          vertex = vertex_shader_code_legacy;
          fragment = fragment_shader_code_legacy;
        }
    }

  init_buffers (&position_buffer, NULL);
  init_shaders (vertex, fragment, &program, &mvp_location);
}

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

  g_assert (position_buffer != 0);
  g_assert (program != 0);

  compute_mvp (mvp,
               rotation_angles[X_AXIS],
               rotation_angles[Y_AXIS],
               rotation_angles[Z_AXIS]);

  glUseProgram (program);
  glUniformMatrix4fv (mvp_location, 1, GL_FALSE, &mvp[0]);

  glBindBuffer (GL_ARRAY_BUFFER, position_buffer);
  glEnableVertexAttribArray (0);
  glVertexAttribPointer (0, 4, GL_FLOAT, GL_FALSE, 0, 0);

  glDrawArrays (GL_TRIANGLES, 0, 3);

  glDisableVertexAttribArray (0);
  glUseProgram (0);
}

static gboolean
render (CtkGLArea    *area,
        GdkGLContext *context)
{
  glClearColor (0.5, 0.5, 0.5, 1.0);
  glClear (GL_COLOR_BUFFER_BIT);

  draw_triangle ();

  glFlush ();

  return TRUE;
}

static void
on_axis_value_change (CtkAdjustment *adjustment,
                      gpointer       data)
{
  int axis = GPOINTER_TO_INT (data);

  if (axis < 0 || axis >= N_AXIS)
    return;

  rotation_angles[axis] = ctk_adjustment_get_value (adjustment);

  ctk_widget_queue_draw (gl_area);
}

static CtkWidget *
create_axis_slider (int axis)
{
  CtkWidget *box, *label, *slider;
  CtkAdjustment *adj;
  const char *text;

  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, FALSE);

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

int
main (int argc, char *argv[])
{
  CtkWidget *window, *box, *button, *controls;
  int i;

  ctk_init (&argc, &argv);

  /* create a new pixel format; we use this to configure the
   * GL context, and to check for features
   */

  window = ctk_window_new (CTK_WINDOW_TOPLEVEL);
  ctk_window_set_title (CTK_WINDOW (window), "CtkGLArea - Triangle");
  ctk_window_set_default_size (CTK_WINDOW (window), 400, 600);
  ctk_container_set_border_width (CTK_CONTAINER (window), 12);
  g_signal_connect (window, "destroy", G_CALLBACK (ctk_main_quit), NULL);

  box = ctk_box_new (CTK_ORIENTATION_VERTICAL, FALSE);
  ctk_box_set_spacing (CTK_BOX (box), 6);
  ctk_container_add (CTK_CONTAINER (window), box);
  ctk_widget_show (box);

  gl_area = ctk_gl_area_new ();
  ctk_widget_set_hexpand (gl_area, TRUE);
  ctk_widget_set_vexpand (gl_area, TRUE);
  ctk_container_add (CTK_CONTAINER (box), gl_area);
  g_signal_connect (gl_area, "realize", G_CALLBACK (realize), NULL);
  g_signal_connect (gl_area, "unrealize", G_CALLBACK (unrealize), NULL);
  g_signal_connect (gl_area, "render", G_CALLBACK (render), NULL);
  ctk_widget_show (gl_area);

  controls = ctk_box_new (CTK_ORIENTATION_VERTICAL, FALSE);
  ctk_container_add (CTK_CONTAINER (box), controls);
  ctk_widget_set_hexpand (controls, TRUE);
  ctk_widget_show (controls);

  for (i = 0; i < N_AXIS; i++)
    ctk_container_add (CTK_CONTAINER (controls), create_axis_slider (i));

  button = ctk_button_new_with_label ("Quit");
  ctk_widget_set_hexpand (button, TRUE);
  ctk_container_add (CTK_CONTAINER (box), button);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (ctk_widget_destroy), window);
  ctk_widget_show (button);

  ctk_widget_show (window);

  ctk_main ();

  return EXIT_SUCCESS;
}
