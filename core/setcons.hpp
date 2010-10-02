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

/* A = B <=> r */
void post_seteq_re(Solver &s, setvar a, setvar b, cspvar r);
void post_seteq_re(Solver &s, setvar a, setvar b, Lit r);

/* A != B */
void post_setneq(Solver &s, setvar a, setvar b);

/* A != B <=> r */
void post_setneq_re(Solver &s, setvar a, setvar b, cspvar r);
void post_setneq_re(Solver &s, setvar a, setvar b, Lit r);

/* X in A */
void post_setin(Solver &s, cspvar x, setvar a);

/* X in A <=> b */
void post_setin_re(Solver &s, cspvar x, setvar a, Lit b);
void post_setin_re(Solver &s, cspvar x, setvar a, cspvar b);

/* A \cap B = C */
void post_setintersect(Solver &s, setvar a, setvar b, setvar c);

/* A \cup B = C */
void post_setunion(Solver &s, setvar a, setvar b, setvar c);

#endif
