/* ctktexttag.c - text tag object
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
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

#ifndef __CTK_TEXT_TAG_H__
#define __CTK_TEXT_TAG_H__


#if !defined (__CTK_H_INSIDE__) && !defined (CTK_COMPILATION)
#error "Only <ctk/ctk.h> can be included directly."
#endif

#include <cdk/cdk.h>
#include <ctk/ctkenums.h>


G_BEGIN_DECLS

typedef struct _CtkTextIter CtkTextIter;
typedef struct _CtkTextTagTable CtkTextTagTable;

#define CTK_TYPE_TEXT_TAG            (ctk_text_tag_get_type ())
#define CTK_TEXT_TAG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_TEXT_TAG, CtkTextTag))
#define CTK_TEXT_TAG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_TEXT_TAG, CtkTextTagClass))
#define CTK_IS_TEXT_TAG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_TEXT_TAG))
#define CTK_IS_TEXT_TAG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_TEXT_TAG))
#define CTK_TEXT_TAG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_TEXT_TAG, CtkTextTagClass))

typedef struct _CtkTextTag             CtkTextTag;
typedef struct _CtkTextTagPrivate      CtkTextTagPrivate;
typedef struct _CtkTextTagClass        CtkTextTagClass;

struct _CtkTextTag
{
  GObject parent_instance;

  CtkTextTagPrivate *priv;
};

struct _CtkTextTagClass
{
  GObjectClass parent_class;

  gboolean (* event) (CtkTextTag        *tag,
                      GObject           *event_object, /* widget, canvas item, whatever */
                      CdkEvent          *event,        /* the event itself */
                      const CtkTextIter *iter);        /* location of event in buffer */

  /* Padding for future expansion */
  void (*_ctk_reserved1) (void);
  void (*_ctk_reserved2) (void);
  void (*_ctk_reserved3) (void);
  void (*_ctk_reserved4) (void);
};

CDK_AVAILABLE_IN_ALL
GType        ctk_text_tag_get_type     (void) G_GNUC_CONST;
CDK_AVAILABLE_IN_ALL
CtkTextTag  *ctk_text_tag_new          (const gchar       *name);
CDK_AVAILABLE_IN_ALL
gint         ctk_text_tag_get_priority (CtkTextTag        *tag);
CDK_AVAILABLE_IN_ALL
void         ctk_text_tag_set_priority (CtkTextTag        *tag,
                                        gint               priority);
CDK_AVAILABLE_IN_ALL
gboolean     ctk_text_tag_event        (CtkTextTag        *tag,
                                        GObject           *event_object,
                                        CdkEvent          *event,
                                        const CtkTextIter *iter);
CDK_AVAILABLE_IN_3_20
void         ctk_text_tag_changed      (CtkTextTag        *tag,
                                        gboolean           size_changed);

G_END_DECLS

#endif

