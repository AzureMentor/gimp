/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Depth Merge -- Combine two image layers via corresponding depth maps
 * Copyright (C) 1997, 1998 Sean Cier (scier@PostHorizon.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/* Version 1.0.0: (14 August 1998)
 *   Math optimizations, miscellaneous speedups
 *
 * Version 0.1: (6 July 1997)
 *   Initial Release
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define DEBUG

#ifndef LERP
#define LERP(frac,a,b) ((frac)*(b) + (1-(frac))*(a))
#endif

#define MUL255(i) ((i)*256 - (i))
#define DIV255(i) (((i) + (i)/256 + 1) / 256)

#define PLUG_IN_PROC    "plug-in-depth-merge"
#define PLUG_IN_VERSION "August 1998"
#define PLUG_IN_BINARY  "depth-merge"
#define PLUG_IN_ROLE    "gimp-depth-merge"

#define PREVIEW_SIZE    256

/* ----- DepthMerge ----- */

struct _DepthMerge;

typedef struct _DepthMergeInterface
{
  gboolean   active;

  GtkWidget *dialog;

  GtkWidget *preview;
  gint       previewWidth;
  gint       previewHeight;

  guchar    *previewSource1;
  guchar    *previewSource2;
  guchar    *previewDepthMap1;
  guchar    *previewDepthMap2;
} DepthMergeInterface;

typedef struct _DepthMergeParams
{
  gint32  result;
  gint32  source1;
  gint32  source2;
  gint32  depthMap1;
  gint32  depthMap2;
  gfloat  overlap;
  gfloat  offset;
  gfloat  scale1;
  gfloat  scale2;
} DepthMergeParams;

typedef struct _DepthMerge
{
  DepthMergeInterface *interface;
  DepthMergeParams     params;

  gint32               resultDrawable_id;
  gint32               source1Drawable_id;
  gint32               source2Drawable_id;
  gint32               depthMap1Drawable_id;
  gint32               depthMap2Drawable_id;
  gint                 selectionX;
  gint                 selectionY;
  gint                 selectionWidth;
  gint                 selectionHeight;
  gint                 resultHasAlpha;
} DepthMerge;

static void      DepthMerge_initParams              (DepthMerge *dm);
static gboolean  DepthMerge_construct               (DepthMerge *dm);
static void      DepthMerge_destroy                 (DepthMerge *dm);
static gint32    DepthMerge_execute                 (DepthMerge *dm);
static void      DepthMerge_executeRegion           (DepthMerge *dm,
                                                     guchar     *source1Row,
                                                     guchar     *source2Row,
                                                     guchar     *depthMap1Row,
                                                     guchar     *depthMap2Row,
                                                     guchar     *resultRow,
                                                     gint        length);
static gboolean  DepthMerge_dialog                  (DepthMerge *dm);
static void      DepthMerge_buildPreviewSourceImage (DepthMerge *dm);
static void      DepthMerge_updatePreview           (DepthMerge *dm);


static gboolean  dm_constraint                      (gint32      imageId,
                                                     gint32      drawableId,
                                                     gpointer    data);

static void dialogSource1ChangedCallback   (GtkWidget *widget, DepthMerge *dm);
static void dialogSource2ChangedCallback   (GtkWidget *widget, DepthMerge *dm);
static void dialogDepthMap1ChangedCallback (GtkWidget *widget, DepthMerge *dm);
static void dialogDepthMap2ChangedCallback (GtkWidget *widget, DepthMerge *dm);

static void dialogValueScaleUpdateCallback (GtkAdjustment *adjustment,
                                            gpointer       data);

static void util_fillReducedBuffer (guchar       *dest,
                                    const Babl   *dest_format,
                                    gint          destWidth,
                                    gint          destHeight,
                                    gint32        sourceDrawable_id,
                                    gint          x0,
                                    gint          y0,
                                    gint          sourceWidth,
                                    gint          sourceHeight);


/* ----- plug-in entry points ----- */

static void query (void);
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",  "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",     "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "result",    "Result" },
    { GIMP_PDB_DRAWABLE, "source1",   "Source 1" },
    { GIMP_PDB_DRAWABLE, "source2",   "Source 2" },
    { GIMP_PDB_DRAWABLE, "depthMap1", "Depth map 1" },
    { GIMP_PDB_DRAWABLE, "depthMap2", "Depth map 2" },
    { GIMP_PDB_FLOAT,    "overlap",   "Overlap" },
    { GIMP_PDB_FLOAT,    "offset",    "Depth relative offset" },
    { GIMP_PDB_FLOAT,    "scale1",    "Depth relative scale 1" },
    { GIMP_PDB_FLOAT,    "scale2",    "Depth relative scale 2" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Combine two images using depth maps (z-buffers)"),
                          "Taking as input two full-color, full-alpha "
                            "images and two corresponding grayscale depth "
                            "maps, this plug-in combines the images based "
                            "on which is closer (has a lower depth map value) "
                            "at each point.",
                          "Sean Cier",
                          "Sean Cier",
                          PLUG_IN_VERSION,
                          N_("_Depth Merge..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Combine");
}

static void
run (const gchar      *name,
     gint              numParams,
     const GimpParam  *param,
     gint             *numReturnVals,
     GimpParam       **returnVals)
{
  static GimpParam  values[1];
  GimpRunMode       runMode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  DepthMerge        dm;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  runMode = param[0].data.d_int32;

  *numReturnVals = 1;
  *returnVals    = values;

  switch (runMode)
    {
    case GIMP_RUN_INTERACTIVE:
      DepthMerge_initParams (&dm);
      gimp_get_data (PLUG_IN_PROC, &(dm.params));
      dm.params.result = param[2].data.d_drawable;
      if (!DepthMerge_construct (&dm))
        return;

      if (!DepthMerge_dialog (&dm))
        {
          values[0].type = GIMP_PDB_STATUS;
          values[0].data.d_status = GIMP_PDB_SUCCESS;
          return;
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      DepthMerge_initParams (&dm);
      if (numParams != 11)
        status = GIMP_PDB_CALLING_ERROR;
      else
        {
          dm.params.result    = param[ 2].data.d_drawable;
          dm.params.source1   = param[ 3].data.d_drawable;
          dm.params.source2   = param[ 4].data.d_drawable;
          dm.params.depthMap1 = param[ 5].data.d_drawable;
          dm.params.depthMap2 = param[ 6].data.d_drawable;
          dm.params.overlap   = param[ 7].data.d_float;
          dm.params.offset    = param[ 8].data.d_float;
          dm.params.scale1    = param[ 9].data.d_float;
          dm.params.scale2    = param[10].data.d_float;
        }
      if (!DepthMerge_construct (&dm))
        return;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      DepthMerge_initParams (&dm);
      gimp_get_data (PLUG_IN_PROC, &(dm.params));
      if (!DepthMerge_construct (&dm))
        return;
      break;

    default:
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (!DepthMerge_execute (&dm))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
      else
        {
          if (runMode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          if (runMode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC,
                           &(dm.params), sizeof (DepthMergeParams));
        }
    }

  DepthMerge_destroy (&dm);

  values[0].data.d_status = status;
}

/* ----- DepthMerge ----- */

static void
DepthMerge_initParams (DepthMerge *dm)
{
  dm->params.result    = -1;
  dm->params.source1   = -1;
  dm->params.source2   = -1;
  dm->params.depthMap1 = -1;
  dm->params.depthMap2 = -1;
  dm->params.overlap   =  0;
  dm->params.offset    =  0;
  dm->params.scale1    =  1;
  dm->params.scale2    =  1;
}

static gboolean
DepthMerge_construct (DepthMerge *dm)
{
  dm->interface = NULL;

  dm->resultDrawable_id = dm->params.result;

  if (! gimp_drawable_mask_intersect (dm->resultDrawable_id,
                                      &(dm->selectionX), &(dm->selectionY),
                                      &(dm->selectionWidth),
                                      &(dm->selectionHeight)))
    {
      return FALSE;
    }

  dm->resultHasAlpha = gimp_drawable_has_alpha (dm->resultDrawable_id);

  dm->source1Drawable_id   = dm->params.source1;
  dm->source2Drawable_id   = dm->params.source2;
  dm->depthMap1Drawable_id = dm->params.depthMap1;
  dm->depthMap2Drawable_id = dm->params.depthMap2;

  dm->params.overlap = CLAMP (dm->params.overlap, 0, 2);
  dm->params.offset  = CLAMP (dm->params.offset, -1, 1);
  dm->params.scale1  = CLAMP (dm->params.scale1, -1, 1);
  dm->params.scale2  = CLAMP (dm->params.scale2, -1, 1);

  return TRUE;
}

static void
DepthMerge_destroy (DepthMerge *dm)
{
  if (dm->interface != NULL)
    {
      g_free (dm->interface->previewSource1);
      g_free (dm->interface->previewSource2);
      g_free (dm->interface->previewDepthMap1);
      g_free (dm->interface->previewDepthMap2);
      g_free (dm->interface);
    }
}

static gint32
DepthMerge_execute (DepthMerge *dm)
{
  int           x, y;
  GeglBuffer   *source1_buffer   = NULL;
  GeglBuffer   *source2_buffer   = NULL;
  GeglBuffer   *depthMap1_buffer = NULL;
  GeglBuffer   *depthMap2_buffer = NULL;
  GeglBuffer   *result_buffer;
  guchar       *source1Row,   *source2Row;
  guchar       *depthMap1Row, *depthMap2Row;
  guchar       *resultRow;
  guchar       *tempRow;

  gimp_progress_init (_("Depth-merging"));

  resultRow    = g_new (guchar, dm->selectionWidth * 4);
  source1Row   = g_new (guchar, dm->selectionWidth * 4);
  source2Row   = g_new (guchar, dm->selectionWidth * 4);
  depthMap1Row = g_new (guchar, dm->selectionWidth    );
  depthMap2Row = g_new (guchar, dm->selectionWidth    );
  tempRow      = g_new (guchar, dm->selectionWidth * 4);

  if (dm->source1Drawable_id > 0)
    {
      source1_buffer = gimp_drawable_get_buffer (dm->source1Drawable_id);
    }
  else
    {
      for (x = 0; x < dm->selectionWidth; x++)
        {
          source1Row[4 * x    ] = 0;
          source1Row[4 * x + 1] = 0;
          source1Row[4 * x + 2] = 0;
          source1Row[4 * x + 3] = 255;
        }
    }

  if (dm->source2Drawable_id > 0)
    {
      source2_buffer = gimp_drawable_get_buffer (dm->source2Drawable_id);
    }
  else
    {
      for (x = 0; x < dm->selectionWidth; x++)
        {
          source2Row[4 * x    ] = 0;
          source2Row[4 * x + 1] = 0;
          source2Row[4 * x + 2] = 0;
          source2Row[4 * x + 3] = 255;
        }
    }

  if (dm->depthMap1Drawable_id > 0)
    {
      depthMap1_buffer = gimp_drawable_get_buffer (dm->depthMap1Drawable_id);
    }
  else
    {
      for (x = 0; x < dm->selectionWidth; x++)
        {
          depthMap1Row[x] = 0;
        }
    }

  if (dm->depthMap2Drawable_id > 0)
    {
      depthMap2_buffer = gimp_drawable_get_buffer (dm->depthMap2Drawable_id);
    }
  else
    {
      for (x = 0; x < dm->selectionWidth; x++)
        {
          depthMap2Row[x] = 0;
        }
    }

  result_buffer = gimp_drawable_get_shadow_buffer (dm->resultDrawable_id);

  for (y = dm->selectionY; y < (dm->selectionY + dm->selectionHeight); y++)
    {
      if (dm->source1Drawable_id > 0)
        {
          gegl_buffer_get (source1_buffer,
                           GEGL_RECTANGLE (dm->selectionX, y,
                                           dm->selectionWidth, 1), 1.0,
                           babl_format ("R'G'B'A u8"), source1Row,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
        }

      if (dm->source2Drawable_id > 0)
        {
          gegl_buffer_get (source2_buffer,
                           GEGL_RECTANGLE (dm->selectionX, y,
                                           dm->selectionWidth, 1), 1.0,
                           babl_format ("R'G'B'A u8"), source2Row,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
        }

      if (dm->depthMap1Drawable_id > 0)
        {
          gegl_buffer_get (depthMap1_buffer,
                           GEGL_RECTANGLE (dm->selectionX, y,
                                           dm->selectionWidth, 1), 1.0,
                           babl_format ("Y' u8"), depthMap1Row,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
        }

      if (dm->depthMap2Drawable_id > 0)
        {
          gegl_buffer_get (depthMap2_buffer,
                           GEGL_RECTANGLE (dm->selectionX, y,
                                           dm->selectionWidth, 1), 1.0,
                           babl_format ("Y' u8"), depthMap2Row,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
        }

      DepthMerge_executeRegion (dm,
                                source1Row, source2Row, depthMap1Row, depthMap2Row,
                                resultRow,
                                dm->selectionWidth);

      gegl_buffer_set (result_buffer,
                       GEGL_RECTANGLE (dm->selectionX, y,
                                       dm->selectionWidth, 1), 0,
                       babl_format ("R'G'B'A u8"), resultRow,
                       GEGL_AUTO_ROWSTRIDE);

      gimp_progress_update ((double)(y-dm->selectionY) /
                            (double)(dm->selectionHeight - 1));
    }

  g_free (resultRow);
  g_free (source1Row);
  g_free (source2Row);
  g_free (depthMap1Row);
  g_free (depthMap2Row);
  g_free (tempRow);

  gimp_progress_update (1.0);

  if (source1_buffer)
    g_object_unref (source1_buffer);

  if (source2_buffer)
    g_object_unref (source2_buffer);

  if (depthMap1_buffer)
    g_object_unref (depthMap1_buffer);

  if (depthMap2_buffer)
    g_object_unref (depthMap2_buffer);

  g_object_unref (result_buffer);

  gimp_drawable_merge_shadow (dm->resultDrawable_id, TRUE);
  gimp_drawable_update (dm->resultDrawable_id,
                        dm->selectionX, dm->selectionY,
                        dm->selectionWidth, dm->selectionHeight);

  return TRUE;
}

static void
DepthMerge_executeRegion (DepthMerge *dm,
                          guchar     *source1Row,
                          guchar     *source2Row,
                          guchar     *depthMap1Row,
                          guchar     *depthMap2Row,
                          guchar     *resultRow,
                          gint        length)
{
  gfloat  scale1, scale2, offset255, invOverlap255;
  gfloat  frac, depth1, depth2;
  gushort c1[4], c2[4];
  gushort cR1[4] = { 0, 0, 0, 0 }, cR2[4] = { 0, 0, 0, 0 };
  gushort cR[4], temp;
  gint    i, tempInt;

  invOverlap255 = 1.0 / (MAX (dm->params.overlap, 0.001) * 255);
  offset255     = dm->params.offset * 255;
  scale1        = dm->params.scale1;
  scale2        = dm->params.scale2;

  for (i = 0; i < length; i++)
    {
      depth1 = (gfloat) depthMap1Row[i];
      depth2 = (gfloat) depthMap2Row[i];

      frac = (depth2 * scale2 - (depth1 * scale1 + offset255)) * invOverlap255;
      frac = 0.5 * (frac + 1.0);
      frac = CLAMP(frac, 0.0, 1.0);

      /* c1 -> color corresponding to source1 */
      c1[0] = source1Row[4 * i    ];
      c1[1] = source1Row[4 * i + 1];
      c1[2] = source1Row[4 * i + 2];
      c1[3] = source1Row[4 * i + 3];

      /* c2 -> color corresponding to source2 */
      c2[0] = source2Row[4 * i    ];
      c2[1] = source2Row[4 * i + 1];
      c2[2] = source2Row[4 * i + 2];
      c2[3] = source2Row[4 * i + 3];

      if (frac != 0)
        {
          /* cR1 -> result if c1 is completely on top */
          cR1[0] = c1[3] * c1[0]  + (255 - c1[3]) * c2[0];
          cR1[1] = c1[3] * c1[1]  + (255 - c1[3]) * c2[1];
          cR1[2] = c1[3] * c1[2]  + (255 - c1[3]) * c2[2];
          cR1[3] = MUL255 (c1[3]) + (255 - c1[3]) * c2[3];
        }

      if (frac != 1)
        {
          /* cR2 -> result if c2 is completely on top */
          cR2[0] = c2[3] * c2[0]  + (255 - c2[3]) * c1[0];
          cR2[1] = c2[3] * c2[1]  + (255 - c2[3]) * c1[1];
          cR2[2] = c2[3] * c2[2]  + (255 - c2[3]) * c1[2];
          cR2[3] = MUL255 (c2[3]) + (255 - c2[3]) * c1[3];
        }

      if (frac == 1)
        {
          cR[0] = cR1[0];
          cR[1] = cR1[1];
          cR[2] = cR1[2];
          cR[3] = cR1[3];
        }
      else if (frac == 0)
        {
          cR[0] = cR2[0];
          cR[1] = cR2[1];
          cR[2] = cR2[2];
          cR[3] = cR2[3];
        }
      else
        {
          tempInt = LERP (frac, cR2[0], cR1[0]);
          cR[0] = CLAMP (tempInt,0,255 * 255);
          tempInt = LERP (frac, cR2[1], cR1[1]);
          cR[1] = CLAMP (tempInt,0,255 * 255);
          tempInt = LERP (frac, cR2[2], cR1[2]);
          cR[2] = CLAMP (tempInt,0,255 * 255);
          tempInt = LERP (frac, cR2[3], cR1[3]);
          cR[3] = CLAMP (tempInt,0,255 * 255);
        }

      temp = DIV255 (cR[0]); resultRow[4 * i    ] = MIN (temp, 255);
      temp = DIV255 (cR[1]); resultRow[4 * i + 1] = MIN (temp, 255);
      temp = DIV255 (cR[2]); resultRow[4 * i + 2] = MIN (temp, 255);
      temp = DIV255 (cR[3]); resultRow[4 * i + 3] = MIN (temp, 255);
    }
}

static gboolean
DepthMerge_dialog (DepthMerge *dm)
{
  GtkWidget     *dialog;
  GtkWidget     *vbox;
  GtkWidget     *frame;
  GtkWidget     *grid;
  GtkWidget     *hbox;
  GtkWidget     *label;
  GtkWidget     *combo;
  GtkAdjustment *adj;
  gboolean       run;

  dm->interface = g_new0 (DepthMergeInterface, 1);

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dm->interface->dialog =
    dialog = gimp_dialog_new (_("Depth Merge"), PLUG_IN_ROLE,
                              NULL, 0,
                              gimp_standard_help_func, PLUG_IN_PROC,

                              _("_Cancel"), GTK_RESPONSE_CANCEL,
                              _("_OK"),     GTK_RESPONSE_OK,

                              NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  /* Preview */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  dm->interface->previewWidth  = MIN (dm->selectionWidth,  PREVIEW_SIZE);
  dm->interface->previewHeight = MIN (dm->selectionHeight, PREVIEW_SIZE);
  dm->interface->preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (dm->interface->preview,
                               dm->interface->previewWidth,
                               dm->interface->previewHeight);
  gtk_container_add (GTK_CONTAINER (frame), dm->interface->preview);
  gtk_widget_show (dm->interface->preview);

  DepthMerge_buildPreviewSourceImage (dm);

  /* Source and Depth Map selection */
  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  label = gtk_label_new (_("Source 1:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_widget_show (label);

  combo = gimp_drawable_combo_box_new (dm_constraint, dm);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), dm->params.source1,
                              G_CALLBACK (dialogSource1ChangedCallback),
                              dm);

  gtk_grid_attach (GTK_GRID (grid), combo, 1, 0, 2, 1);
  gtk_widget_show (combo);

  label = gtk_label_new(_("Depth map:"));
  gtk_widget_set_margin_bottom (label, 6);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  gtk_widget_show (label);

  combo = gimp_drawable_combo_box_new (dm_constraint, dm);
  gtk_widget_set_margin_bottom (combo, 6);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), dm->params.depthMap1,
                              G_CALLBACK (dialogDepthMap1ChangedCallback),
                              dm);

  gtk_grid_attach (GTK_GRID (grid), combo, 1, 1, 2, 1);
  gtk_widget_show (combo);

  label = gtk_label_new (_("Source 2:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
  gtk_widget_show (label);

  combo = gimp_drawable_combo_box_new (dm_constraint, dm);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), dm->params.source2,
                              G_CALLBACK (dialogSource2ChangedCallback),
                              dm);

  gtk_grid_attach (GTK_GRID (grid), combo, 1, 2, 2, 1);
  gtk_widget_show (combo);

  label = gtk_label_new (_("Depth map:"));
  gtk_widget_set_margin_bottom (label, 6);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 3, 1, 1);
  gtk_widget_show (label);

  combo = gimp_drawable_combo_box_new (dm_constraint, dm);
  gtk_widget_set_margin_bottom (combo, 6);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), dm->params.depthMap2,
                              G_CALLBACK (dialogDepthMap2ChangedCallback),
                              dm);

  gtk_grid_attach (GTK_GRID (grid), combo, 1, 3, 2, 1);
  gtk_widget_show (combo);

  /* Numeric parameters */
  adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 4,
                              _("O_verlap:"), 0, 6,
                              dm->params.overlap, 0, 2, 0.001, 0.01, 3,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (dialogValueScaleUpdateCallback),
                    &(dm->params.overlap));
  g_object_set_data (G_OBJECT (adj), "dm", dm);

  adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 5,
                              _("O_ffset:"), 0, 6,
                              dm->params.offset, -1, 1, 0.001, 0.01, 3,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (dialogValueScaleUpdateCallback),
                    &(dm->params.offset));
  g_object_set_data (G_OBJECT (adj), "dm", dm);

  adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 6,
                              _("Sc_ale 1:"), 0, 6,
                              dm->params.scale1, -1, 1, 0.001, 0.01, 3,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (dialogValueScaleUpdateCallback),
                    &(dm->params.scale1));
  g_object_set_data (G_OBJECT (adj), "dm", dm);

  adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 7,
                              _("Sca_le 2:"), 0, 6,
                              dm->params.scale2, -1, 1, 0.001, 0.01, 3,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (dialogValueScaleUpdateCallback),
                    &(dm->params.scale2));
  g_object_set_data (G_OBJECT (adj), "dm", dm);

  dm->interface->active = TRUE;

  gtk_widget_show (dm->interface->dialog);
  DepthMerge_updatePreview (dm);

  run = (gimp_dialog_run (GIMP_DIALOG (dm->interface->dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dm->interface->dialog);
  dm->interface->dialog = NULL;

  return run;
}

static void
DepthMerge_buildPreviewSourceImage (DepthMerge *dm)
{
  dm->interface->previewSource1   =
    g_new (guchar, dm->interface->previewWidth *
                   dm->interface->previewHeight * 4);
  util_fillReducedBuffer (dm->interface->previewSource1,
                          babl_format ("R'G'B'A u8"),
                          dm->interface->previewWidth,
                          dm->interface->previewHeight,
                          dm->source1Drawable_id,
                          dm->selectionX, dm->selectionY,
                          dm->selectionWidth, dm->selectionHeight);

  dm->interface->previewSource2   =
    g_new (guchar, dm->interface->previewWidth *
                   dm->interface->previewHeight * 4);
  util_fillReducedBuffer (dm->interface->previewSource2,
                          babl_format ("R'G'B'A u8"),
                          dm->interface->previewWidth,
                          dm->interface->previewHeight,
                          dm->source2Drawable_id,
                          dm->selectionX, dm->selectionY,
                          dm->selectionWidth, dm->selectionHeight);

  dm->interface->previewDepthMap1 =
    g_new (guchar, dm->interface->previewWidth *
                   dm->interface->previewHeight * 1);
  util_fillReducedBuffer (dm->interface->previewDepthMap1,
                          babl_format ("Y' u8"),
                          dm->interface->previewWidth,
                          dm->interface->previewHeight,
                          dm->depthMap1Drawable_id,
                          dm->selectionX, dm->selectionY,
                          dm->selectionWidth, dm->selectionHeight);

  dm->interface->previewDepthMap2 =
    g_new (guchar, dm->interface->previewWidth *
                   dm->interface->previewHeight * 1);
  util_fillReducedBuffer (dm->interface->previewDepthMap2,
                          babl_format ("Y' u8"),
                          dm->interface->previewWidth,
                          dm->interface->previewHeight,
                          dm->depthMap2Drawable_id,
                          dm->selectionX, dm->selectionY,
                          dm->selectionWidth, dm->selectionHeight);
}

static void
DepthMerge_updatePreview (DepthMerge *dm)
{
  gint    y;
  guchar *source1Row, *source2Row;
  guchar *depthMap1Row, *depthMap2Row;
  guchar *resultRGBA;

  if (!dm->interface->active)
    return;

  resultRGBA = g_new (guchar, 4 * dm->interface->previewWidth *
                                  dm->interface->previewHeight);

  for (y = 0; y < dm->interface->previewHeight; y++)
    {
      source1Row =
        &(dm->interface->previewSource1[  y * dm->interface->previewWidth * 4]);
      source2Row =
        &(dm->interface->previewSource2[  y * dm->interface->previewWidth * 4]);
      depthMap1Row =
        &(dm->interface->previewDepthMap1[y * dm->interface->previewWidth    ]);
      depthMap2Row =
        &(dm->interface->previewDepthMap2[y * dm->interface->previewWidth    ]);

      DepthMerge_executeRegion(dm,
                               source1Row, source2Row, depthMap1Row, depthMap2Row,
                               resultRGBA + 4 * y * dm->interface->previewWidth,
                               dm->interface->previewWidth);
    }

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (dm->interface->preview),
                          0, 0,
                          dm->interface->previewWidth,
                          dm->interface->previewHeight,
                          GIMP_RGBA_IMAGE,
                          resultRGBA,
                          dm->interface->previewWidth * 4);
  g_free(resultRGBA);
}

/* ----- Callbacks ----- */

static gboolean
dm_constraint (gint32   imageId,
               gint32   drawableId,
               gpointer data)
{
  DepthMerge *dm = (DepthMerge *)data;

  return ((drawableId == -1) ||
          ((gimp_drawable_width (drawableId) ==
            gimp_drawable_width (dm->params.result)) &&
           (gimp_drawable_height (drawableId) ==
            gimp_drawable_height (dm->params.result)) &&
           ((gimp_drawable_is_rgb (drawableId) &&
             (gimp_drawable_is_rgb (dm->params.result))) ||
            gimp_drawable_is_gray (drawableId))));
}

static void
dialogSource1ChangedCallback (GtkWidget  *widget,
                              DepthMerge *dm)
{
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget),
                                 &dm->params.source1);

  dm->source1Drawable_id = dm->params.source1;

  util_fillReducedBuffer (dm->interface->previewSource1,
                          babl_format ("R'G'B'A u8"),
                          dm->interface->previewWidth,
                          dm->interface->previewHeight,
                          dm->source1Drawable_id,
                          dm->selectionX, dm->selectionY,
                          dm->selectionWidth, dm->selectionHeight);

  DepthMerge_updatePreview (dm);
}

static void
dialogSource2ChangedCallback (GtkWidget  *widget,
                              DepthMerge *dm)
{
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget),
                                 &dm->params.source2);

  dm->source2Drawable_id = dm->params.source2;

  util_fillReducedBuffer (dm->interface->previewSource2,
                          babl_format ("R'G'B'A u8"),
                          dm->interface->previewWidth,
                          dm->interface->previewHeight,
                          dm->source2Drawable_id,
                          dm->selectionX, dm->selectionY,
                          dm->selectionWidth, dm->selectionHeight);

  DepthMerge_updatePreview (dm);
}

static void
dialogDepthMap1ChangedCallback (GtkWidget  *widget,
                                DepthMerge *dm)
{
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget),
                                 &dm->params.depthMap1);

  dm->depthMap1Drawable_id = dm->params.depthMap1;

  util_fillReducedBuffer (dm->interface->previewDepthMap1,
                          babl_format ("Y' u8"),
                          dm->interface->previewWidth,
                          dm->interface->previewHeight,
                          dm->depthMap1Drawable_id,
                          dm->selectionX, dm->selectionY,
                          dm->selectionWidth, dm->selectionHeight);

  DepthMerge_updatePreview (dm);
}

static void
dialogDepthMap2ChangedCallback (GtkWidget  *widget,
                                DepthMerge *dm)
{
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget),
                                 &dm->params.depthMap2);

  dm->depthMap2Drawable_id = dm->params.depthMap2;;

  util_fillReducedBuffer (dm->interface->previewDepthMap2,
                          babl_format ("Y' u8"),
                          dm->interface->previewWidth,
                          dm->interface->previewHeight,
                          dm->depthMap2Drawable_id,
                          dm->selectionX, dm->selectionY,
                          dm->selectionWidth, dm->selectionHeight);

  DepthMerge_updatePreview (dm);
}

static void
dialogValueScaleUpdateCallback (GtkAdjustment *adjustment,
                                gpointer       data)
{
  DepthMerge *dm = g_object_get_data (G_OBJECT (adjustment), "dm");

  gimp_float_adjustment_update (adjustment, data);

  DepthMerge_updatePreview (dm);
}

/* ----- Utility routines ----- */

static void
util_fillReducedBuffer (guchar     *dest,
                        const Babl *dest_format,
                        gint        destWidth,
                        gint        destHeight,
                        gint32      sourceDrawable_id,
                        gint        x0,
                        gint        y0,
                        gint        sourceWidth,
                        gint        sourceHeight)
{
  GeglBuffer *buffer;
  guchar     *sourceBuffer,    *reducedRowBuffer;
  guchar     *sourceBufferPos, *reducedRowBufferPos;
  guchar     *sourceBufferRow;
  gint        x, y, i, yPrime;
  gint        destBPP;
  gint       *sourceRowOffsetLookup;

  destBPP = babl_format_get_bytes_per_pixel (dest_format);

  if ((sourceDrawable_id < 1) || (sourceWidth == 0) || (sourceHeight == 0))
    {
      for (x = 0; x < destWidth * destHeight * destBPP; x++)
        dest[x] = 0;

      return;
    }

  sourceBuffer          = g_new (guchar, sourceWidth * sourceHeight * destBPP);
  reducedRowBuffer      = g_new (guchar, destWidth   * destBPP);
  sourceRowOffsetLookup = g_new (int, destWidth);

  buffer = gimp_drawable_get_buffer (sourceDrawable_id);

  for (x = 0; x < destWidth; x++)
    sourceRowOffsetLookup[x] = (x * (sourceWidth - 1) / (destWidth - 1)) * destBPP;

  gegl_buffer_get (buffer,
                   GEGL_RECTANGLE (x0, y0, sourceWidth, sourceHeight), 1.0,
                   dest_format, sourceBuffer,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  for (y = 0; y < destHeight; y++)
    {
      yPrime = y * (sourceHeight - 1) / (destHeight - 1);
      sourceBufferRow = &(sourceBuffer[yPrime * sourceWidth * destBPP]);
      reducedRowBufferPos = reducedRowBuffer;

      for (x = 0; x < destWidth; x++)
        {
          sourceBufferPos = sourceBufferRow + sourceRowOffsetLookup[x];
          for (i = 0; i < destBPP; i++)
            reducedRowBufferPos[i] = sourceBufferPos[i];
          reducedRowBufferPos += destBPP;
        }

      memcpy (&(dest[y * destWidth * destBPP]), reducedRowBuffer,
              destWidth * destBPP);
    }

  g_object_unref (buffer);

  g_free (sourceBuffer);
  g_free (reducedRowBuffer);
  g_free (sourceRowOffsetLookup);
}
