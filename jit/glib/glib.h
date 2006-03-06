/*
 * Copyright (C) 2006  Jim Huang
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains drop-in GLib utility functions for monoburg.
 */

#ifndef JATO_GLIB_H
#define JATO_GLIB_H
#include <sys/types.h>
#include <stdio.h>
#include <assert.h>

/* Avoid C++ naming */
#ifdef  __cplusplus
#  define G_BEGIN_DECLS \
	extern "C" {
#  define G_END_DECLS \
	}
#else
#  define G_BEGIN_DECLS
#  define G_END_DECLS
#endif

/* Type definitions for commonly used types. */
typedef char   gchar;
typedef short  gshort;
typedef long   glong;
typedef int    gint;
typedef gint   gboolean;

typedef unsigned char   guchar;
typedef unsigned short  gushort;
typedef unsigned long   gulong;
typedef unsigned int    guint;

typedef float   gfloat;
typedef double  gdouble;

typedef u_int8_t	guint8;
typedef u_int16_t	guint16;
typedef int16_t		gint16;
typedef int32_t		gint32;

typedef void * gpointer;

#define g_new(struct_type, n_structs) \
	(malloc (((size_t) sizeof (struct_type)) * ((size_t) (n_structs))))
#define g_free(mem) \
	free(mem);
#define g_assert(expr) \
	assert(expr)
#define g_assert_not_reached() \
	assert(NULL)
#define g_error(...)
#define g_return_val_if_fail(expr,val)

#endif /* JATO_GLIB_H */

