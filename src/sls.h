// sls.h
//

#include "sat_tools.h"
#include "cnf_parse.h"
#include "babel.h"

#ifndef MT_H
#define MT_H

int cmp_kca_score(const void *a, const void *b);

int    sls_mt_srand(int seed);
int    sls_mt_rand(void);

int    sls_mt_solve(st_state *st, int max_tries);
void   sls_mt_rand_assignment(st_state *st);
void   sls_mt_clause_kick_sa(st_state *st, int clause_id);
void   sls_mt_clause_kick(st_state *st, int clause_id);
mword  sls_mt_choose_unsat_clause(st_state *st);
int    sls_mt_update_clause_sat(st_state *st);
int    sls_mt_clause_sat_count(st_state *st);
void   sls_mt_update_var_sat(st_state *st, int curr_var);

int    cnf_update_clause_sat(st_state *st, int clause_id);

int    sls_mt_var_sat_count_fast(st_state *st, int var_id);
int    sls_mt_var_sat_count(st_state *st, int var_id);

float  sls_mt_logistic(float x);
int    rand_bent_coin(float p);

int    sls_gsat_solve(st_state *st, int max_tries, int max_flips);
int    sls_gsat_var_make_break(st_state *st, int var);
int    sls_gsat_var_choose(st_state *st);

typedef struct{

    babel_env *be;
    st_state  *st;

    // each candidate has the following form:
    //      [ptr [val <score>] [val8 <assignment>] ]
    mword *candidate_list;
    int num_candidates;

    // kca-transpose:
    mword *literal_list; // transpose of kca_state->candidate_list

    // each candidate has the following form:
    //      [ptr [val <score>] [val <array_index>] ]
    mword *candidate_score_map;

    // use these to accumulate score data while sweeping literals, that is,
    //      combine the generation & scoring phases to save the cache
    mword *lit_clause_map;
    //mword *lit_var_map; --> use ks->st->cl->variables

} kca_state;

int    sls_kca_solve(kca_state *ks, int num_candidates, int max_gens);
int    sls_kca_solve_init(kca_state *ks, int num_candidates);
int    sls_kca_solve_body(kca_state *ks, int max_gens);

void   sls_kca_rand_candidate(kca_state *ks, mword *candidate);

int    sls_kca_solution(kca_state *ks, mword *candidate);

float  sls_kca_candidate_score(kca_state *ks, mword *candidate, float sample_rate);
float  sls_kca_variable_score(kca_state *ks, char *candidate_assignment, float sample_rate);
float  sls_kca_var_id_score(kca_state *ks, char *candidate_assignment, int var_id);
float  sls_kca_clause_score(kca_state *ks, char *candidate_assignment, float sample_rate);
//float  sls_kca_clause_score(kca_state *ks, int clause_id, char *clause, int clause_size);
//mword *sls_kca_make_candidate(babel_env *be, mword *score, mword *assignment);


#endif // MT_H


// Clayton Bauman 2018

