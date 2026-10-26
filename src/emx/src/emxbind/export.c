/* export.c -- Export symbols
   Copyright (c) 1991-1995 Eberhard Mattes

This file is part of emxbind.

emxbind is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emxbind is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emxbind; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/moddef.h>
#include "defs.h"
#include "emxbind.h"


/* Export definitions from EXPORTS statements. */

static struct export *export_data = NULL;
static int export_size = 0;
static int export_len = 0;


/* Add an export entry to the exports table.  Adding an export
   (external name or ordinal) which already exists is a fatal error.
   Exporting a symbol with different external names triggers a warning
   message. */

void add_export (const struct export *exp, int from_def)
{
  int i;

  for (i = 0; i < export_len; ++i)
    {
      if (strcmp (exp->entryname, export_data[i].entryname) == 0)
        {
          if (from_def)
            error ("export multiply defined: %s", exp->entryname);
          else
            {
              /* Ignore export multiply defined by __declspec(dllexport). */
              free (exp->entryname);
              free (exp->internalname);
              return;
            }
        }

      if ((verbosity >= 2) &&
          (strcmp (exp->internalname, export_data[i].internalname) == 0))
        printf ("emxbind: %s multiply exported (warning)\n",
                exp->internalname);
      if (exp->ord != 0 && export_data[i].ord == exp->ord)
        error ("ordinal %u multiply defined", (unsigned)exp->ord);
    }
  
  if (   !(exp->flags & _MDEP_NONAME)
      && strlen (exp->entryname) > 255)
      error ("exported name is too long (%d chars): %s", 
             strlen (exp->entryname), exp->entryname);
  
  if (export_len >= export_size)
    {
      export_size += 32;
      export_data = xrealloc (export_data,
                              export_size * sizeof (struct export));
    }
  export_data[export_len++] = *exp;
}


/* Add an entry name to the resident name table or non-resident name
   table TABLE (resnames or nonresnames).  NAME is the name of the
   entrypoint, ORD is the ordinal number. */

void entry_name (struct grow *table, const char *name, int ord)
{
  int name_len;
  byte blen;
  word ord16;

  name_len = strlen (name);
  if (name_len > 255)
    {
      warning("truncating export '%s' to 255 chars (ord %d)!", name, ord);
      blen = 255;
    }
  else
    blen = (byte)name_len;
  ord16 = (word)ord;
  put_grow (table, &blen, 1);
  put_grow (table, name, name_len);
  put_grow (table, &ord16, 2);
}

/* Compare two entries of an int array for qsort().  This function is
   used for sorting the exported ordinal numbers. */

static int int_compare (const void *x1, const void *x2)
{
  int a1, a2;

  a1 = *(int *)x1;
  a2 = *(int *)x2;
  if (a1 < a2)
    return -1;
  else if (a1 > a2)
    return 1;
  else
    return 0;
}


/* Compare two entries of the export table for qsort().  This function
   is used for sorting the export table by ordinal number. */

static int export_compare (const void *x1, const void *x2)
{
  int a1, a2;

  a1 = ((struct export *)x1)->ord;
  a2 = ((struct export *)x2)->ord;
  if (a1 < a2)
    return -1;
  else if (a1 > a2)
    return 1;
  else
    return 0;
}


/* Process the export definitions: build the entry table and update
   the resident name table and the non-resident name table. */

void exports (void)
{
  int i, j, n, ord, bundle;
  struct export *exp;
  struct ninfo *nl;
  int *used;
  byte count;
  byte flags;
  byte type;
  word object;
  dword offset;

  read_sym ();

  /* Add __declspec(dllexport) symbols */

  for (i = 0; i < sym_count; i++)
    {
      struct export e;
      const char *name;
      const char *symbol;
      const char *ordinal;
      char *ordinal_end;
      const char *sym_type;
      int name_len;
      int symbol_len;

      if( sym_image[ i ].n_type != N_EXP )
        continue;

      /* dllexport: <exportname>,<ordinal>=<asmname>,<code|data> */

      name = (char *)str_image + sym_image[i].n_un.n_strx;

      /* find equal sign first, we'll use this for validating the ordinal. */
      symbol = strchr (name, '=');
      if (!symbol)
          error ("Invalid export record: missing `='. \nstabs: %s", name);

      /* ordinal */
      ordinal = strchr (name, ',');
      if (!ordinal || ordinal >= symbol)
        error ("Invalid export record: missing ordinal.\nstabs: %s", name);
      name_len = ordinal - name;
      ordinal++;
      ord = strtol (ordinal, &ordinal_end, 0);
      if (ord < 0 || ord >= 0x10000)
        error ("Invalid export record: ordinal value is out of range (0-65k):%d\nstabs:%s",
               ord, name);
      if (ordinal_end != symbol)
        error ("Invalid export record: ordinal field doesn't end at`='.\nstabs:%s",
               name);

      /* type and symbol */
      symbol++;
      sym_type = strchr (symbol, ',');
      if (!sym_type)
        error("Invalid export record: Symbol type is missing\nstabs:%s", name);
      symbol_len = sym_type - symbol;
      sym_type++;
      if (strcmp (sym_type, "code") && strcmp (sym_type, "data") &&
          strcmp (sym_type, "bss") && strcmp (sym_type, "common"))
        error("Invalid export record: Invalid symbol type: %s\nstabs:%s",
              sym_type, name);
      if (!name_len)
        error("Invalid export record: No (internal) symbol name.\nstabs:%s",
              name);

      e.ord = ord;
      e.resident = !e.ord;
      e.entryname = xstrdup (name);
      e.entryname[name_len] = '\0';
      e.flags = 0;
      e.internalname = xstrdup (symbol);
      e.internalname[symbol_len] = '\0';
      e.offset = 0;
      e.object = 0;
      add_export (&e, FALSE);
    }

  if (export_len != 0)
    {
      if (a_in_h.a_syms == 0 || a_in_str_size == 0)
        error ("need symbol table for EXPORTS");
    }

  /* Search symbol table */

  for (i = 0; i < export_len; ++i)
    {
      nl = find_symbol (export_data[i].internalname);
      if (nl == NULL)
        error ("symbol %s undefined (EXPORTS)", export_data[i].internalname);
      switch (nl->n_type & ~N_EXT)
	{
	case N_TEXT:
	  export_data[i].offset = nl->n_value - obj_text.virt_base;
	  export_data[i].object = OBJ_TEXT;
	  break;
        case N_BSS:
	case N_DATA:
	  export_data[i].offset = nl->n_value - obj_data.virt_base;
	  export_data[i].object = OBJ_DATA;
	  break;
	default:
	  error ("cannot export symbol %s of type %d",
                 export_data[i].internalname, nl->n_type);
	}
    }

  /* Assign unused ordinal numbers to entries with ord == 0 */

  used = xmalloc (export_len * sizeof (int));
  n = 0;
  for (i = 0; i < export_len; ++i)
    {
      ord = export_data[i].ord;
      if (ord != 0)
	used[n++] = ord;
    }
  qsort (used, n, sizeof (int), int_compare);
  ord = 1; j = 0;
  for (i = 0; i < export_len; ++i)
    if (export_data[i].ord == 0)
      {
	while (j < n && used[j] == ord)
	  {
	    ++ord; ++j;
	  }
	export_data[i].ord = ord++;
      }
  qsort (export_data, export_len, sizeof (struct export), export_compare);
  ord = 1; bundle = 0;
  for (i = 0; i < export_len; ++i)
    {
      exp = &export_data[i];
      if (!(exp->flags & _MDEP_NONAME))
      entry_name ((exp->resident ? &resnames : &nonresnames),
		  exp->entryname, exp->ord);
      if (bundle == 0)
	{
	  while (ord < exp->ord)
	    {
	      if (exp->ord - ord > 255)
		count = 255;
	      else
		count = exp->ord - ord;
	      type = 0;       /* empty bundle */
	      put_grow (&entry_tab, &count, 1);
	      put_grow (&entry_tab, &type, 1);
	      ord += count;
	    }
	  object = (word)exp->object;
	  bundle = 1;
	  while (i+bundle < export_len && bundle < 255 &&
		 export_data[i+bundle].ord == ord + bundle &&
		 export_data[i+bundle].object == object)
	    ++bundle;
	  count = (byte)bundle;
	  type = 3;           /* entry point, 32-bit offset */
	  ++object;
	  put_grow (&entry_tab, &count, 1);
	  put_grow (&entry_tab, &type, 1);
	  put_grow (&entry_tab, &object, 2);
	}
      flags = 3;
      offset = exp->offset;
      put_grow (&entry_tab, &flags, 1);
      put_grow (&entry_tab, &offset, 4);
      ++ord; --bundle;
    }
  count = 0;
  put_grow (&entry_tab, &count, 1);
  if (nonresnames.len != 0)
    put_grow (&nonresnames, &count, 1);
}


/* Retrieve an export entry for writing the .map file. */

const struct export *get_export (int i)
{
  if (i < export_len)
    return &export_data[i];
  else
    return NULL;
}
