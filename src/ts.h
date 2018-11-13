// ts.h
//

#include "sat_tools.h"
#include "babel.h"

#ifndef TS_H
#define TS_H

int ts_solve(babel_env *be, st_state *bs);
int ts_ucb_choice(float reward_0, float reward_1, float num_attempts_0, float num_attempts_1);
var_state ts_rand_assign(void);


#endif // TS_H


// Clayton Bauman 2018

