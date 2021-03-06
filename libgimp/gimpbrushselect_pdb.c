/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpbrushselect_pdb.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

/* NOTE: This file is auto-generated by pdbgen.pl */

#include "config.h"

#include "gimp.h"


/**
 * SECTION: gimpbrushselect
 * @title: gimpbrushselect
 * @short_description: Functions providing a brush selection dialog.
 *
 * Functions providing a brush selection dialog.
 **/


/**
 * gimp_brushes_popup:
 * @brush_callback: The callback PDB proc to call when brush selection is made.
 * @popup_title: Title of the brush selection dialog.
 * @initial_brush: The name of the brush to set as the first selected.
 * @opacity: The initial opacity of the brush.
 * @spacing: The initial spacing of the brush (if < 0 then use brush default spacing).
 * @paint_mode: The initial paint mode.
 *
 * Invokes the Gimp brush selection.
 *
 * This procedure opens the brush selection dialog.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_brushes_popup (const gchar   *brush_callback,
                    const gchar   *popup_title,
                    const gchar   *initial_brush,
                    gdouble        opacity,
                    gint           spacing,
                    GimpLayerMode  paint_mode)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gboolean success = TRUE;

  args = gimp_value_array_new_from_types (G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_DOUBLE,
                                          GIMP_TYPE_INT32,
                                          G_TYPE_ENUM,
                                          G_TYPE_NONE);
  g_value_set_string (gimp_value_array_index (args, 0), brush_callback);
  g_value_set_string (gimp_value_array_index (args, 1), popup_title);
  g_value_set_string (gimp_value_array_index (args, 2), initial_brush);
  g_value_set_double (gimp_value_array_index (args, 3), opacity);
  g_value_set_int (gimp_value_array_index (args, 4), spacing);
  g_value_set_enum (gimp_value_array_index (args, 5), paint_mode);

  return_vals = gimp_run_procedure_with_array ("gimp-brushes-popup",
                                               args);
  gimp_value_array_unref (args);

  success = g_value_get_enum (gimp_value_array_index (return_vals, 0)) == GIMP_PDB_SUCCESS;

  gimp_value_array_unref (return_vals);

  return success;
}

/**
 * gimp_brushes_close_popup:
 * @brush_callback: The name of the callback registered for this pop-up.
 *
 * Close the brush selection dialog.
 *
 * This procedure closes an opened brush selection dialog.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_brushes_close_popup (const gchar *brush_callback)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gboolean success = TRUE;

  args = gimp_value_array_new_from_types (G_TYPE_STRING,
                                          G_TYPE_NONE);
  g_value_set_string (gimp_value_array_index (args, 0), brush_callback);

  return_vals = gimp_run_procedure_with_array ("gimp-brushes-close-popup",
                                               args);
  gimp_value_array_unref (args);

  success = g_value_get_enum (gimp_value_array_index (return_vals, 0)) == GIMP_PDB_SUCCESS;

  gimp_value_array_unref (return_vals);

  return success;
}

/**
 * gimp_brushes_set_popup:
 * @brush_callback: The name of the callback registered for this pop-up.
 * @brush_name: The name of the brush to set as selected.
 * @opacity: The initial opacity of the brush.
 * @spacing: The initial spacing of the brush (if < 0 then use brush default spacing).
 * @paint_mode: The initial paint mode.
 *
 * Sets the current brush in a brush selection dialog.
 *
 * Sets the current brush in a brush selection dialog.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_brushes_set_popup (const gchar   *brush_callback,
                        const gchar   *brush_name,
                        gdouble        opacity,
                        gint           spacing,
                        GimpLayerMode  paint_mode)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gboolean success = TRUE;

  args = gimp_value_array_new_from_types (G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_DOUBLE,
                                          GIMP_TYPE_INT32,
                                          G_TYPE_ENUM,
                                          G_TYPE_NONE);
  g_value_set_string (gimp_value_array_index (args, 0), brush_callback);
  g_value_set_string (gimp_value_array_index (args, 1), brush_name);
  g_value_set_double (gimp_value_array_index (args, 2), opacity);
  g_value_set_int (gimp_value_array_index (args, 3), spacing);
  g_value_set_enum (gimp_value_array_index (args, 4), paint_mode);

  return_vals = gimp_run_procedure_with_array ("gimp-brushes-set-popup",
                                               args);
  gimp_value_array_unref (args);

  success = g_value_get_enum (gimp_value_array_index (return_vals, 0)) == GIMP_PDB_SUCCESS;

  gimp_value_array_unref (return_vals);

  return success;
}
