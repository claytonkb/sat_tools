// mt.h
//

#ifndef MT_H
#define MT_H

#include "cnf_parse.h"
#include "backtrack.h"

int mt_srand(int seed);
int mt_rand(void);

int  mt_solve(backtrack_state *bs, int max_tries);
void mt_rand_assignment(backtrack_state *bs);
void mt_clause_kick_sa(backtrack_state *bs, int clause_id);
void mt_clause_kick(backtrack_state *bs, int clause_id);
mword mt_choose_unsat_clause(backtrack_state *bs);
int  mt_update_clause_sat(backtrack_state *bs);
int  mt_clause_sat_count(backtrack_state *bs);
void mt_update_var_sat(backtrack_state *bs, int curr_var);
int  cnf_update_clause_sat(backtrack_state *bs, int clause_id);

int mt_var_sat_count_fast(backtrack_state *bs, int var_id);
int mt_var_sat_count(backtrack_state *bs, int var_id);


float mt_logistic(float x);
int rand_bent_coin(float p);


#endif // MT_H


// Clayton Bauman 2018

