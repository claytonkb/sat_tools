// sls.h
//

#include "cnf_parse.h"
#include "sat_tools.h"

#ifndef MT_H
#define MT_H

int sls_mt_srand(int seed);
int sls_mt_rand(void);

int  sls_mt_solve(st_state *bs, int max_tries);
void sls_mt_rand_assignment(st_state *bs);
void sls_mt_clause_kick_sa(st_state *bs, int clause_id);
void sls_mt_clause_kick(st_state *bs, int clause_id);
mword sls_mt_choose_unsat_clause(st_state *bs);
int  sls_mt_update_clause_sat(st_state *bs);
int  sls_mt_clause_sat_count(st_state *bs);
void sls_mt_update_var_sat(st_state *bs, int curr_var);
int  cnf_update_clause_sat(st_state *bs, int clause_id);

int sls_mt_var_sat_count_fast(st_state *bs, int var_id);
int sls_mt_var_sat_count(st_state *bs, int var_id);


float sls_mt_logistic(float x);
int rand_bent_coin(float p);

int  sls_gsat_solve(st_state *bs, int max_tries, int max_flips);
int  sls_gsat_var_make_break(st_state *bs, int var);
int  sls_gsat_var_choose(st_state *bs);


#endif // MT_H


// Clayton Bauman 2018

