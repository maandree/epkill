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
# define setlocale(Category, Locale) /* empty */
#endif

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# undef bindtextdomain
# define bindtextdomain(Domain, Directory) /* empty */
# undef textdomain
# define textdomain(Domain) /* empty */
# define _(Text) (Text)
#endif /* ENABLE_NLS */


#endif

