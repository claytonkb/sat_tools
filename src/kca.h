// kca.h
//

#include "sat_tools.h"
#include "sls.h"
#include "cnf_parse.h"
#include "babel.h"

#ifndef KCA_H
#define KCA_H

#define kca_rand_literal kca_rand_candidate

//typedef struct{
//
//    babel_env *be;
//    st_state  *st;
//
//    int num_candidates;
//
//    mword *lit_clause_map;
//
//    mword *lh_matrix; // [ptr [val  ] ... ] : size=num_assignments x num_candidates/2
//    mword *rh_matrix; // [ptr [ptr [val <score> ] [val <candidate> ] ] ... ] : size=num_candidates/2 x 2
//
//    // XXX PERF: make var_pos/neg_count_arrays into multi-dim var arrays;
//    //              makes resetting faster (memset) and might speed up main
//    //              loop, as well
//    mword *var_pos_count_array; // [ptr [val  ] ... ] : size=num_candidates/2 x num_variables
//    mword *var_neg_count_array; // [ptr [val  ] ... ] : size=num_candidates/2 x num_variables
//    mword *lit_count_array;       // [val  ]            : size=num_assignments
//    mword *sat_count_array;     // [val8 ]            : size=num_candidates/2
//    mword *lh_score_map;        // [ptr [ptr [val <score> ] [val <cand_id> ] ] ... ] : size=num_candidates/2 x 2
//    mword *rh_score_array;      // [val  ]            : size=num_candidates/2
//
//    int last_sat_clause;
//
//    // XXX DEPRECATED XXX //
//    mword *candidate_score_map;
//    mword *clause_sat_array;    // [ptr [val8 ] ... ] : size=num_clauses x num_candidates/2
//
//    //sls_kca*:
//    mword *literal_list;
//    mword *candidate_list;
//
//} kca_state;

typedef enum score_sel_enum 
    { CLAUSE_VAR_SCORE, CLAUSE_SCORE, VAR_SCORE } score_sel;

int    kca_solve(kca_state *ks, int num_candidates, int max_gens);
int    kca_solve_init(kca_state *ks, int num_candidates);
int    kca_solve_init_matrices(kca_state *ks, int num_candidates);
int    kca_solve_init_lh_score_map(kca_state *ks, int num_candidates);
int    kca_solve_init_clause_map(kca_state *ks, int num_candidates);
int    kca_solve_init_stats(kca_state *ks, int num_candidates);

int    kca_solve_body(kca_state *ks, int max_gens);
int    kca_solve_generate_new_candidates(kca_state *ks, score_sel sel);
int    kca_solve_merge_new_generation(kca_state *ks);
int    kca_solve_update_lit_count(kca_state *ks);
int    kca_solve_reset_stats(kca_state *ks);
int    kca_solve_reset_sat_counts(kca_state *ks);
int    kca_solve_update_counts(kca_state *ks, int cand_id, int lit_id, var_state lit_choice);
int    kca_solve_score_candidates(kca_state *ks, score_sel sel);

var_state kca_rand_lit(kca_state *ks, int lit_id);
void   kca_rand_candidate(kca_state *ks, mword *candidate);

int    kca_score_candidates(kca_state *ks, int begin_offset);
float  kca_var_id_score(kca_state *ks, int var_id, int var_pos_count, int var_neg_count);

int    kca_solution(kca_state *ks, mword *candidate);
float  kca_candidate_score(kca_state *ks, int cand_id, score_sel sel);
float  kca_variable_score(kca_state *ks, char *candidate_assignment, float sample_rate);
float  kca_clause_score(kca_state *ks, int sat_count);

int    kca_sort_candidates(kca_state *ks);
int    kca_generate_new_candidates(kca_state *ks);


#endif // KCA_H


// Clayton Bauman 2018

