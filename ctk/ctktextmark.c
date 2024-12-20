/* ctktextmark.c - mark segments
 *
 * Copyright (c) 1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 2000      Red Hat, Inc.
 * Tk -> Ctk port by Havoc Pennington <hp@redhat.com>
 *
 * This software is copyrighted by the Regents of the University of
 * California, Sun Microsystems, Inc., and other parties.  The
 * following terms apply to all files associated with the software
 * unless explicitly disclaimed in individual files.
 *
 * The authors hereby grant permission to use, copy, modify,
 * distribute, and license this software and its documentation for any
 * purpose, provided that existing copyright notices are retained in
 * all copies and that this notice is included verbatim in any
 * distributions. No written agreement, license, or royalty fee is
 * required for any of the authorized uses.  Modifications to this
 * software may be copyrighted by their authors and need not follow
 * the licensing terms described here, provided that the new terms are
 * clearly indicated on the first page of each file where they apply.
 *
 * IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY
 * PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION,
 * OR ANY DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 * NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS,
 * AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 * MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * GOVERNMENT USE: If you are acquiring this software on behalf of the
 * U.S. government, the Government shall have only "Restricted Rights"
 * in the software and related documentation as defined in the Federal
 * Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 * are acquiring the software on behalf of the Department of Defense,
 * the software shall be classified as "Commercial Computer Software"
 * and the Government shall have only "Restricted Rights" as defined
 * in Clause 252.227-7013 (c) (1) of DFARs.  Notwithstanding the
 * foregoing, the authors grant the U.S. Government and others acting
 * in its behalf permission to use and distribute the software in
 * accordance with the terms specified in this license.
 *
 */

#define CTK_TEXT_USE_INTERNAL_UNSUPPORTED_API
#include "config.h"
#include "ctktextbtree.h"
#include "ctkprivate.h"
#include "ctkintl.h"


/**
 * SECTION:ctktextmark
 * @Short_description: A position in the buffer preserved across buffer modifications
 * @Title: CtkTextMark
 *
 * You may wish to begin by reading the
 * [text widget conceptual overview][TextWidget]
 * which gives an overview of all the objects and data
 * types related to the text widget and how they work together.
 *
 * A #CtkTextMark is like a bookmark in a text buffer; it preserves a position in
 * the text. You can convert the mark to an iterator using
 * ctk_text_buffer_get_iter_at_mark(). Unlike iterators, marks remain valid across
 * buffer mutations, because their behavior is defined when text is inserted or
 * deleted. When text containing a mark is deleted, the mark remains in the
 * position originally occupied by the deleted text. When text is inserted at a
 * mark, a mark with “left gravity” will be moved to the
 * beginning of the newly-inserted text, and a mark with “right
 * gravity” will be moved to the end.
 *
 * Note that “left” and “right” here refer to logical direction (left
 * is the toward the start of the buffer); in some languages such as
 * Hebrew the logically-leftmost text is not actually on the left when
 * displayed.
 *
 * Marks are reference counted, but the reference count only controls the validity
 * of the memory; marks can be deleted from the buffer at any time with
 * ctk_text_buffer_delete_mark(). Once deleted from the buffer, a mark is
 * essentially useless.
 *
 * Marks optionally have names; these can be convenient to avoid passing the
 * #CtkTextMark object around.
 *
 * Marks are typically created using the ctk_text_buffer_create_mark() function.
 */

/*
 * Macro that determines the size of a mark segment:
 */
#define MSEG_SIZE ((unsigned) (G_STRUCT_OFFSET (CtkTextLineSegment, body) \
        + sizeof (CtkTextMarkBody)))

static void ctk_text_mark_set_property (GObject         *object,
				        guint            prop_id,
					const GValue    *value,
					GParamSpec      *pspec);
static void ctk_text_mark_get_property (GObject         *object,
					guint            prop_id,
					GValue          *value,
					GParamSpec      *pspec);
static void ctk_text_mark_finalize     (GObject         *object);

static CtkTextLineSegment *ctk_mark_segment_new (CtkTextMark *mark_obj);

G_DEFINE_TYPE (CtkTextMark, ctk_text_mark, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_NAME,
  PROP_LEFT_GRAVITY
};

static void
ctk_text_mark_class_init (CtkTextMarkClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ctk_text_mark_finalize;
  object_class->set_property = ctk_text_mark_set_property;
  object_class->get_property = ctk_text_mark_get_property;

  /**
   * CtkTextMark:name:
   *
   * The name of the mark or %NULL if the mark is anonymous.
   */
  g_object_class_install_property (object_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        P_("Name"),
                                                        P_("Mark name"),
                                                        NULL,
                                                        CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  /**
   * CtkTextMark:left-gravity:
   *
   * Whether the mark has left gravity. When text is inserted at the mark’s
   * current location, if the mark has left gravity it will be moved
   * to the left of the newly-inserted text, otherwise to the right.
   */
  g_object_class_install_property (object_class,
                                   PROP_LEFT_GRAVITY,
                                   g_param_spec_boolean ("left-gravity",
                                                         P_("Left gravity"),
                                                         P_("Whether the mark has left gravity"),
                                                         FALSE,
                                                         CTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
ctk_text_mark_init (CtkTextMark *mark)
{
  mark->segment = ctk_mark_segment_new (mark);
}

static void
ctk_text_mark_finalize (GObject *obj)
{
  CtkTextMark *mark;
  CtkTextLineSegment *seg;

  mark = CTK_TEXT_MARK (obj);

  seg = mark->segment;

  if (seg)
    {
      if (seg->body.mark.tree != NULL)
        g_warning ("CtkTextMark being finalized while still in the buffer; "
                   "someone removed a reference they didn't own! Crash "
                   "impending");

      g_free (seg->body.mark.name);
      g_slice_free1 (MSEG_SIZE, seg);

      mark->segment = NULL;
    }

  /* chain parent_class' handler */
  G_OBJECT_CLASS (ctk_text_mark_parent_class)->finalize (obj);
}

static void
ctk_text_mark_set_property (GObject      *object,
			    guint         prop_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
  gchar *tmp;
  CtkTextMark *mark = CTK_TEXT_MARK (object);
  CtkTextLineSegment *seg = mark->segment;

  switch (prop_id)
    {
    case PROP_NAME:
      tmp = seg->body.mark.name;
      seg->body.mark.name = g_value_dup_string (value);
      g_free (tmp);
      break;

    case PROP_LEFT_GRAVITY:
      if (g_value_get_boolean (value))
	seg->type = &ctk_text_left_mark_type;
      else
	seg->type = &ctk_text_right_mark_type;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ctk_text_mark_get_property (GObject    *object,
			    guint       prop_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
  CtkTextMark *mark = CTK_TEXT_MARK (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, ctk_text_mark_get_name (mark));
      break;

    case PROP_LEFT_GRAVITY:
      g_value_set_boolean (value, ctk_text_mark_get_left_gravity (mark));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

/**
 * ctk_text_mark_new:
 * @name: (allow-none): mark name or %NULL
 * @left_gravity: whether the mark should have left gravity
 *
 * Creates a text mark. Add it to a buffer using ctk_text_buffer_add_mark().
 * If @name is %NULL, the mark is anonymous; otherwise, the mark can be 
 * retrieved by name using ctk_text_buffer_get_mark(). If a mark has left 
 * gravity, and text is inserted at the mark’s current location, the mark 
 * will be moved to the left of the newly-inserted text. If the mark has 
 * right gravity (@left_gravity = %FALSE), the mark will end up on the 
 * right of newly-inserted text. The standard left-to-right cursor is a 
 * mark with right gravity (when you type, the cursor stays on the right
 * side of the text you’re typing).
 *
 * Returns: new #CtkTextMark
 *
 * Since: 2.12
 **/
CtkTextMark *
ctk_text_mark_new (const gchar *name,
		   gboolean     left_gravity)
{
  return g_object_new (CTK_TYPE_TEXT_MARK,
		       "name", name,
		       "left-gravity", left_gravity,
		       NULL);
}

/**
 * ctk_text_mark_get_visible:
 * @mark: a #CtkTextMark
 * 
 * Returns %TRUE if the mark is visible (i.e. a cursor is displayed
 * for it).
 * 
 * Returns: %TRUE if visible
 **/
gboolean
ctk_text_mark_get_visible (CtkTextMark *mark)
{
  CtkTextLineSegment *seg;

  seg = mark->segment;

  return seg->body.mark.visible;
}

/**
 * ctk_text_mark_get_name:
 * @mark: a #CtkTextMark
 * 
 * Returns the mark name; returns NULL for anonymous marks.
 * 
 * Returns: (nullable): mark name
 **/
const char *
ctk_text_mark_get_name (CtkTextMark *mark)
{
  CtkTextLineSegment *seg;

  seg = mark->segment;

  return seg->body.mark.name;
}

/**
 * ctk_text_mark_get_deleted:
 * @mark: a #CtkTextMark
 * 
 * Returns %TRUE if the mark has been removed from its buffer
 * with ctk_text_buffer_delete_mark(). See ctk_text_buffer_add_mark()
 * for a way to add it to a buffer again.
 * 
 * Returns: whether the mark is deleted
 **/
gboolean
ctk_text_mark_get_deleted (CtkTextMark *mark)
{
  CtkTextLineSegment *seg;

  g_return_val_if_fail (CTK_IS_TEXT_MARK (mark), FALSE);

  seg = mark->segment;

  if (seg == NULL)
    return TRUE;

  return seg->body.mark.tree == NULL;
}

/**
 * ctk_text_mark_get_buffer:
 * @mark: a #CtkTextMark
 * 
 * Gets the buffer this mark is located inside,
 * or %NULL if the mark is deleted.
 *
 * Returns: (transfer none): the mark’s #CtkTextBuffer
 **/
CtkTextBuffer*
ctk_text_mark_get_buffer (CtkTextMark *mark)
{
  CtkTextLineSegment *seg;

  g_return_val_if_fail (CTK_IS_TEXT_MARK (mark), NULL);

  seg = mark->segment;

  if (seg->body.mark.tree == NULL)
    return NULL;
  else
    return _ctk_text_btree_get_buffer (seg->body.mark.tree);
}

/**
 * ctk_text_mark_get_left_gravity:
 * @mark: a #CtkTextMark
 * 
 * Determines whether the mark has left gravity.
 * 
 * Returns: %TRUE if the mark has left gravity, %FALSE otherwise
 **/
gboolean
ctk_text_mark_get_left_gravity (CtkTextMark *mark)
{
  CtkTextLineSegment *seg;

  g_return_val_if_fail (CTK_IS_TEXT_MARK (mark), FALSE);
  
  seg = mark->segment;

  return seg->type == &ctk_text_left_mark_type;
}

static CtkTextLineSegment *
ctk_mark_segment_new (CtkTextMark *mark_obj)
{
  CtkTextLineSegment *mark;

  mark = g_slice_alloc0 (MSEG_SIZE);
  mark->body.mark.name = NULL;
  mark->type = &ctk_text_right_mark_type;

  mark->byte_count = 0;
  mark->char_count = 0;

  mark->body.mark.obj = mark_obj;
  mark_obj->segment = mark;

  mark->body.mark.tree = NULL;
  mark->body.mark.line = NULL;
  mark->next = NULL;

  mark->body.mark.visible = FALSE;
  mark->body.mark.not_deleteable = FALSE;

  return mark;
}

void
_ctk_mark_segment_set_tree (CtkTextLineSegment *mark,
			    CtkTextBTree       *tree)
{
  g_assert (mark->body.mark.tree == NULL);
  g_assert (mark->body.mark.obj != NULL);

  mark->byte_count = 0;
  mark->char_count = 0;

  mark->body.mark.tree = tree;
  mark->body.mark.line = NULL;
  mark->next = NULL;

  mark->body.mark.not_deleteable = FALSE;
}

static int                 mark_segment_delete_func  (CtkTextLineSegment *segPtr,
                                                      CtkTextLine        *line,
                                                      int                 treeGone);
static CtkTextLineSegment *mark_segment_cleanup_func (CtkTextLineSegment *segPtr,
                                                      CtkTextLine        *line);
static void                mark_segment_check_func   (CtkTextLineSegment *segPtr,
                                                      CtkTextLine        *line);


/*
 * The following structures declare the "mark" segment types.
 * There are actually two types for marks, one with left gravity
 * and one with right gravity.  They are identical except for
 * their gravity property.
 */

const CtkTextLineSegmentClass ctk_text_right_mark_type = {
  "mark",                                               /* name */
  FALSE,                                                /* leftGravity */
  NULL,                                         /* splitFunc */
  mark_segment_delete_func,                             /* deleteFunc */
  mark_segment_cleanup_func,                            /* cleanupFunc */
  NULL,                                         /* lineChangeFunc */
  mark_segment_check_func                               /* checkFunc */
};

const CtkTextLineSegmentClass ctk_text_left_mark_type = {
  "mark",                                               /* name */
  TRUE,                                         /* leftGravity */
  NULL,                                         /* splitFunc */
  mark_segment_delete_func,                             /* deleteFunc */
  mark_segment_cleanup_func,                            /* cleanupFunc */
  NULL,                                         /* lineChangeFunc */
  mark_segment_check_func                               /* checkFunc */
};

/*
 *--------------------------------------------------------------
 *
 * mark_segment_delete_func --
 *
 *      This procedure is invoked by the text B-tree code whenever
 *      a mark lies in a range of characters being deleted.
 *
 * Results:
 *      Returns 1 to indicate that deletion has been rejected,
 *      or 0 otherwise
 *
 * Side effects:
 *      Frees mark if tree is going away
 *
 *--------------------------------------------------------------
 */

static gboolean
mark_segment_delete_func (CtkTextLineSegment *seg,
                          CtkTextLine        *line G_GNUC_UNUSED,
                          gboolean            tree_gone)
{
  if (tree_gone)
    {
      _ctk_text_btree_release_mark_segment (seg->body.mark.tree, seg);
      return FALSE;
    }
  else
    return TRUE;
}

/*
 *--------------------------------------------------------------
 *
 * mark_segment_cleanup_func --
 *
 *      This procedure is invoked by the B-tree code whenever a
 *      mark segment is moved from one line to another.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The line field of the segment gets updated.
 *
 *--------------------------------------------------------------
 */

static CtkTextLineSegment *
mark_segment_cleanup_func (CtkTextLineSegment *seg,
                           CtkTextLine        *line)
{
  /* not sure why Tk did this here and not in LineChangeFunc */
  seg->body.mark.line = line;
  return seg;
}

/*
 *--------------------------------------------------------------
 *
 * mark_segment_check_func --
 *
 *      This procedure is invoked by the B-tree code to perform
 *      consistency checks on mark segments.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The procedure panics if it detects anything wrong with
 *      the mark.
 *
 *--------------------------------------------------------------
 */

static void
mark_segment_check_func (CtkTextLineSegment *seg,
                         CtkTextLine        *line)
{
  if (seg->body.mark.line != line)
    g_error ("mark_segment_check_func: seg->body.mark.line bogus");
}
