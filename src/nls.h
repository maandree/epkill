/*
 * This header was copied from util-linux at fall 2011.
 */

#ifndef EPKILL_NLS_H
#define EPKILL_NLS_H


/* programs issuing bindtextdomain() also need LOCALEDIR string */
#ifndef LOCALEDIR
# define LOCALEDIR "/usr/share/locale"
#endif

#ifdef HAVE_LOCALE_H
# include <locale.h>
#else
# undef setlocale
# define setlocale(category, locale)  /* empty */
#endif

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(text)  gettext(text)
#else
# undef bindtextdomain
# define bindtextdomain(domain, directory)  /* empty */
# undef textdomain
# define textdomain(domain)  /* empty */
# define _(text)  (text)
#endif


#endif

