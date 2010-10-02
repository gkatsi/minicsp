/****************************************************************[setcons.h]
 *
 * MiniCSP
 *
 * Copyright (c) 2010 George katsirelos
 *************************************************************************/

#ifndef __MINICSP_SETCONS_HPP__
#define __MINICSP_SETCONS_HPP__

/* A \ B = C */
void post_setdiff(Solver &s, setvar a, setvar b, setvar c);

/* A = B */
void post_seteq(Solver &s, setvar a, setvar b);

/* A != B */
void post_setneq(Solver &s, setvar a, setvar b);

#endif
