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

    int num_candidates;

    mword *lit_clause_map;

    mword *lh_matrix; // [ptr [val  ] ... ] : size=num_assignments x num_candidates/2
    mword *rh_matrix; // [ptr [ptr [val <score> ] [val <candidate> ] ] ... ] : size=num_candidates/2 x 2

    // XXX PERF: make var_pos/neg_count_arrays into multi-dim var arrays;
    //              makes resetting faster (memset) and might speed up main
    //              loop, as well
    mword *var_pos_count_array; // [ptr [val  ] ... ] : size=num_candidates/2 x num_variables
    mword *var_neg_count_array; // [ptr [val  ] ... ] : size=num_candidates/2 x num_variables
    mword *lit_avg_array;       // [val  ]            : size=num_assignments
    mword *sat_count_array;     // [val8 ]            : size=num_candidates/2
    mword *lh_score_map;        // [ptr [ptr [val <score> ] [val <cand_id> ] ] ... ] : size=num_candidates/2 x 2

    // XXX GET RID OF (rh scores are stored in rh_matrix):
    mword *rh_score_array;      // [val  ]            : size=num_candidates/2

    int last_sat_clause;

    // XXX DEPRECATED XXX //
    mword *candidate_score_map;
    mword *clause_sat_array;    // [ptr [val8 ] ... ] : size=num_clauses x num_candidates/2

    //sls_kca*:
    mword *literal_list;
    mword *candidate_list;

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

