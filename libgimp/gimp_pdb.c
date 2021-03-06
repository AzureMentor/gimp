/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimp_pdb.c
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
 * SECTION: gimp
 * @title: gimp
 * @short_description: Miscellaneous procedures
 *
 * Miscellaneous procedures not fitting in any category.
 **/


/**
 * gimp_version:
 *
 * Returns the host GIMP version.
 *
 * This procedure returns the version number of the currently running
 * GIMP.
 *
 * Returns: GIMP version number.
 **/
gchar *
gimp_version (void)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gchar *version = NULL;

  args = gimp_value_array_new_from_types (G_TYPE_NONE);

  return_vals = gimp_run_procedure_with_array ("gimp-version",
                                               args);
  gimp_value_array_unref (args);

  if (g_value_get_enum (gimp_value_array_index (return_vals, 0)) == GIMP_PDB_SUCCESS)
    version = g_value_dup_string (gimp_value_array_index (return_vals, 1));

  gimp_value_array_unref (return_vals);

  return version;
}

/**
 * gimp_getpid:
 *
 * Returns the PID of the host GIMP process.
 *
 * This procedure returns the process ID of the currently running GIMP.
 *
 * Returns: The PID.
 *
 * Since: 2.4
 **/
gint
gimp_getpid (void)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gint pid = 0;

  args = gimp_value_array_new_from_types (G_TYPE_NONE);

  return_vals = gimp_run_procedure_with_array ("gimp-getpid",
                                               args);
  gimp_value_array_unref (args);

  if (g_value_get_enum (gimp_value_array_index (return_vals, 0)) == GIMP_PDB_SUCCESS)
    pid = g_value_get_int (gimp_value_array_index (return_vals, 1));

  gimp_value_array_unref (return_vals);

  return pid;
}

/**
 * gimp_attach_parasite:
 * @parasite: The parasite to attach.
 *
 * Add a global parasite.
 *
 * This procedure attaches a global parasite. It has no return values.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.8
 **/
gboolean
gimp_attach_parasite (const GimpParasite *parasite)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gboolean success = TRUE;

  args = gimp_value_array_new_from_types (GIMP_TYPE_PARASITE,
                                          G_TYPE_NONE);
  g_value_set_boxed (gimp_value_array_index (args, 0), parasite);

  return_vals = gimp_run_procedure_with_array ("gimp-attach-parasite",
                                               args);
  gimp_value_array_unref (args);

  success = g_value_get_enum (gimp_value_array_index (return_vals, 0)) == GIMP_PDB_SUCCESS;

  gimp_value_array_unref (return_vals);

  return success;
}

/**
 * gimp_detach_parasite:
 * @name: The name of the parasite to detach.
 *
 * Removes a global parasite.
 *
 * This procedure detaches a global parasite from. It has no return
 * values.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.8
 **/
gboolean
gimp_detach_parasite (const gchar *name)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gboolean success = TRUE;

  args = gimp_value_array_new_from_types (G_TYPE_STRING,
                                          G_TYPE_NONE);
  g_value_set_string (gimp_value_array_index (args, 0), name);

  return_vals = gimp_run_procedure_with_array ("gimp-detach-parasite",
                                               args);
  gimp_value_array_unref (args);

  success = g_value_get_enum (gimp_value_array_index (return_vals, 0)) == GIMP_PDB_SUCCESS;

  gimp_value_array_unref (return_vals);

  return success;
}

/**
 * gimp_get_parasite:
 * @name: The name of the parasite to find.
 *
 * Look up a global parasite.
 *
 * Finds and returns the global parasite that was previously attached.
 *
 * Returns: The found parasite.
 *
 * Since: 2.8
 **/
GimpParasite *
gimp_get_parasite (const gchar *name)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  GimpParasite *parasite = NULL;

  args = gimp_value_array_new_from_types (G_TYPE_STRING,
                                          G_TYPE_NONE);
  g_value_set_string (gimp_value_array_index (args, 0), name);

  return_vals = gimp_run_procedure_with_array ("gimp-get-parasite",
                                               args);
  gimp_value_array_unref (args);

  if (g_value_get_enum (gimp_value_array_index (return_vals, 0)) == GIMP_PDB_SUCCESS)
    parasite = g_value_dup_boxed (gimp_value_array_index (return_vals, 1));

  gimp_value_array_unref (return_vals);

  return parasite;
}

/**
 * gimp_get_parasite_list:
 * @num_parasites: The number of attached parasites.
 *
 * List all parasites.
 *
 * Returns a list of all currently attached global parasites.
 *
 * Returns: The names of currently attached parasites. The returned
 * value must be freed with g_strfreev().
 *
 * Since: 2.8
 **/
gchar **
gimp_get_parasite_list (gint *num_parasites)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gchar **parasites = NULL;

  args = gimp_value_array_new_from_types (G_TYPE_NONE);

  return_vals = gimp_run_procedure_with_array ("gimp-get-parasite-list",
                                               args);
  gimp_value_array_unref (args);

  *num_parasites = 0;

  if (g_value_get_enum (gimp_value_array_index (return_vals, 0)) == GIMP_PDB_SUCCESS)
    {
      *num_parasites = g_value_get_int (gimp_value_array_index (return_vals, 1));
      parasites = gimp_value_dup_string_array (gimp_value_array_index (return_vals, 2));
    }

  gimp_value_array_unref (return_vals);

  return parasites;
}

/**
 * gimp_temp_name:
 * @extension: The extension the file will have.
 *
 * Generates a unique filename.
 *
 * Generates a unique filename using the temp path supplied in the
 * user's gimprc.
 *
 * Returns: The new temp filename.
 **/
gchar *
gimp_temp_name (const gchar *extension)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gchar *name = NULL;

  args = gimp_value_array_new_from_types (G_TYPE_STRING,
                                          G_TYPE_NONE);
  g_value_set_string (gimp_value_array_index (args, 0), extension);

  return_vals = gimp_run_procedure_with_array ("gimp-temp-name",
                                               args);
  gimp_value_array_unref (args);

  if (g_value_get_enum (gimp_value_array_index (return_vals, 0)) == GIMP_PDB_SUCCESS)
    name = g_value_dup_string (gimp_value_array_index (return_vals, 1));

  gimp_value_array_unref (return_vals);

  return name;
}
