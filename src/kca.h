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
//    // kca-hybrid
//    mword *literal_list;
//    mword *candidate_list;
//
//    mword *lit_clause_map;
//
//    mword *var_pos_count_array; // [ptr [val  ] ... ] : size=num_candidates/2 x num_variables
//    mword *var_neg_count_array; // [ptr [val  ] ... ] : size=num_candidates/2 x num_variables
//    mword *lit_avg_array;       // [val  ]            : size=num_assignments
//    mword *sat_count_array;     // [val8 ]            : size=num_candidates/2
//    mword *running_score_array; // [val  ]            : size=num_candidates/2
//
//    mword *candidate_score_map;
//
//} kca_state;

//typedef struct {
//    uint64_t *variables;
//    uint64_t *clauses;
//    uint8_t  *clause_lengths;
//    int       num_variables;
//    int       num_clauses;
//    int       num_assignments;
//} clause_list;

int    kca_solve(kca_state *ks, int num_candidates, int max_gens);
int    kca_solve_init(kca_state *ks, int num_candidates);
int    kca_solve_init_matrices(kca_state *ks, int num_candidates);
int    kca_solve_init_score_map(kca_state *ks, int num_candidates);
int    kca_solve_init_clause_map(kca_state *ks, int num_candidates);
int    kca_solve_init_stats(kca_state *ks, int num_candidates);

int    kca_solve_body(kca_state *ks, int max_gens);
int    kca_solve_generate_new_candidates(kca_state *ks);
int    kca_solve_merge_new_generation(kca_state *ks);
int    kca_solve_update_literals(kca_state *ks);

var_state kca_rand_lit(kca_state *ks, int lit_id);
void   kca_rand_candidate(kca_state *ks, mword *candidate);

int    kca_score_candidates(kca_state *ks, int begin_offset);

int    kca_solution(kca_state *ks, mword *candidate);
float  kca_candidate_score(kca_state *ks, mword *candidate, float sample_rate);
float  kca_variable_score(kca_state *ks, char *candidate_assignment, float sample_rate);
float  kca_var_id_score(kca_state *ks, char *candidate_assignment, int var_id);
float  kca_clause_score(kca_state *ks, char *candidate_assignment, float sample_rate);

int    kca_sort_candidates(kca_state *ks);
int    kca_generate_new_candidates(kca_state *ks);


#endif // KCA_H


// Clayton Bauman 2018

