// ts.h
//

#include "sat_tools.h"
#include "babel.h"

#ifndef TS_H
#define TS_H

int  ts_solve(babel_env *be, st_state *bs);
var_state ts_rand_assign(void);
int  ts_ucb_solve(babel_env *be, st_state *bs);
int  ts_ucb_choice(float reward_0, float reward_1, float num_attempts_0, float num_attempts_1);
void ts_ucb_update_stats(st_state *st, var_state curr_assignment, int curr_var);


#endif // TS_H


// Clayton Bauman 2018

