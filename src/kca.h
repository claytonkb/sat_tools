// kca.h
//

#include "sat_tools.h"
#include "sls.h"
#include "cnf_parse.h"
#include "babel.h"

#ifndef KCA_H
#define KCA_H

typedef struct{

    babel_env *be;
    st_state  *st;

    mword *literal_list; // transpose of kca_state->candidate_list
    int num_candidates;

    // each candidate has the following form:
    //      [ptr [val <score>] [val <array_index>] ]
    mword *candidate_score_map;

    // use these to accumulate score data while sweeping literals, that is,
    //      combine the generation & scoring phases to save the cache
    mword lit_clause_map;
    mword lit_var_map;

} kca_trans_state;



int    kca_solve(kca_state *ks, int num_candidates, int max_gens);
int    kca_solve_init(kca_state *ks, int num_candidates);
int    kca_solve_body(kca_state *ks, int max_gens);

void   kca_rand_candidate(kca_state *ks, mword *candidate);

int    kca_solution(kca_state *ks, mword *candidate);

float  kca_candidate_score(kca_state *ks, mword *candidate, float sample_rate);
float  kca_variable_score(kca_state *ks, char *candidate_assignment, float sample_rate);
float  kca_var_id_score(kca_state *ks, char *candidate_assignment, int var_id);
float  kca_clause_score(kca_state *ks, char *candidate_assignment, float sample_rate);


#endif // KCA_H


// Clayton Bauman 2018

