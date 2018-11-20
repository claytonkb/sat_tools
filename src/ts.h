// ts.h
//

#include "sat_tools.h"
#include "babel.h"

#ifndef TS_H
#define TS_H

int  ts_solve(babel_env *be, st_state *st);
var_state ts_rand_assign(void);
int  ts_ucb_solve(babel_env *be, st_state *st);
int  ts_ucb_choice(float reward_0, float reward_1, float num_attempts_0, float num_attempts_1);
void ts_ucb_update_stats(st_state *st, int curr_var, int table_branch_select, int reward, int attempts);
var_state ts_ucb_assign(st_state *st, int curr_var, int table_branch_select, float p_annealing);


#endif // TS_H


// Clayton Bauman 2018

