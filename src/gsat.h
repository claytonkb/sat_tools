// gsat.h
//

#ifndef GSAT_H
#define GSAT_H

#include "cnf_parse.h"
#include "backtrack.h"

int  gsat_solve(backtrack_state *bs, int max_tries, int max_flips);
int  gsat_var_make_break(backtrack_state *bs, int var);
int  gsat_var_choose(backtrack_state *bs);


#endif // GSAT_H


// Clayton Bauman 2018

