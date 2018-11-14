// kca.c

#include "kca.h"
#include "sls.h"
#include "pearson.h"
#include "cnf.h"
#include "array.h"
#include "mem.h"
#include <math.h>

// kca-transpose...

//
//
int kca_solve(kca_state *ks, int num_candidates, int max_gens){

    kca_solve_init(ks, num_candidates);
    return kca_solve_body(ks, max_gens);

}


//
//
int kca_solve_init(kca_state *ks, int num_candidates){

    kca_solve_init_literals(ks, num_candidates);
    kca_solve_init_score_map(ks, num_candidates);
    kca_solve_init_clause_map(ks, num_candidates);
    kca_solve_init_stats(ks, num_candidates);
    kca_solve_init_scores(ks, num_candidates);

}


//
//
int kca_solve_init_literals(kca_state *ks, int num_candidates){

    mword *literal_list  = mem_new_ptr(ks->be, ks->st->cl->num_assignments);

    int i;
    mword *new_literal;
    float candidate_score=0;
    mword *candidate_score_val;

    // create ks->literal_list
    for(i=0; i < ks->st->cl->num_assignments; i++){

        new_literal = mem_new_str(ks->be, num_candidates, '\0');
        kca_rand_literal(ks, new_literal);
        ldp(literal_list,i) = new_literal;

    }

    ks->literal_list   = literal_list;
    ks->num_candidates = num_candidates;

}


//
//
int kca_solve_init_score_map(kca_state *ks, int num_candidates){

    mword *candidate_score_map = mem_new_ptr(ks->be, num_candidates);

    int i;
    for(i=0; i<num_candidates; i++){
        ldp(candidate_score_map,i) 
            = list_cons(ks->be, _val(ks->be, 0), _val(ks->be, i));
    }

    ks->candidate_score_map = candidate_score_map;

}


//
//
int kca_solve_init_stats(kca_state *ks, int num_candidates){

    ks->clause_sat_array    = mem_new_str(ks->be, ks->st->cl->num_clauses, '\0');
    ks->var_pos_count_array = mem_new_val(ks->be, ks->st->cl->num_clauses, 0);
    ks->var_neg_count_array = mem_new_val(ks->be, ks->st->cl->num_clauses, 0);

}


//
//
int kca_solve_init_scores(kca_state *ks, int num_candidates){

    kca_score_candidates(ks, 0);

}



//
//
int kca_solve_init_clause_map(kca_state *ks, int num_candidates){

    mword *lit_clause_map = mem_new_val(ks->be, ks->st->cl->num_assignments, 0);

    int i,j;
    int clause_length;
    int lit_id=0;

    // create ks->literal_list
    for(i=0; i < ks->st->cl->num_clauses; i++){

        clause_length = ks->st->cl->clause_lengths[i];

        for(j=0; j < clause_length; j++){
            ldv(lit_clause_map, lit_id) = i;
            lit_id++;
        }

    }

    ks->lit_clause_map = lit_clause_map;

}


// precondition:
//      literal_list populated
//
int kca_solve_body(kca_state *ks, int max_gens){

    while(max_gens--){

        // sort the candidate_score_map
        kca_sort_candidates(ks);

        // generate next batch of candidates
        kca_generate_new_candidates(ks);

        // score the new candidates
        kca_score_candidates(ks, (ks->num_candidates/2));

    }

    return 0;

}


//
//
int kca_score_candidates(kca_state *ks, int begin_offset){

    int i,j;

    unsigned char *curr_assignment;
    unsigned char *clause_sat;
    mword *var_pos_count;
    mword *var_neg_count;
    mword *curr_literal;

    // reset ks->clause_sat_array (all sub-arrays from begin_offset)
    // reset ks->var_pos_count_array (all sub-arrays from begin_offset)
    // reset ks->var_neg_count_array (all sub-arrays from begin_offset)

    for(i = 0; i < ks->st->cl->num_assignments; i++){

        curr_assignment = (char*)rdp(ks->literal_list,i);
        curr_literal    = rdp(ks->st->cl->variables, i);

        if(curr_literal > 0){

            for(j = begin_offset; j < ks->num_candidates; j++){

                clause_sat = (char*)rdp(ks->clause_sat_array, j);
                var_pos_count = rdp(ks->var_pos_count_array, j);

                if((var_state)curr_assignment[j] == DEC_ASSIGN1_VS){
                    clause_sat[ks->lit_clause_map[i]] = 1;
                    ldv(var_pos_count,i) = ldv(var_pos_count,i) + 1;
                }
            }
        }
        else{ // curr_literal < 0

            for(j = begin_offset; j < ks->num_candidates; j++){

                clause_sat = (char*)rdp(ks->clause_sat_array, j);
                var_neg_count = rdp(ks->var_neg_count_array, j);

                if((var_state)curr_assignment[j] == DEC_ASSIGN0_VS){
                    clause_sat[ks->lit_clause_map[i]] = 1;
                    ldv(var_neg_count,i) = ldv(var_neg_count,i) + 1;
                }
            }
        }
    }

}


//
//
int kca_sort_candidates(kca_state *ks){

    // qsort candidate_score_map by candidate_id

    // update score for each candidate:
//    for(j = begin_offset; j < ks->num_candidates; j++){
//
//    }

    // qsort candidate_score_map by score

}


//
//
int kca_generate_new_candidates(kca_state *ks){
}


int kca_solution(kca_state *ks, mword *candidate){
//          convert each literal to a variable setting and 
//              call cnf_var_write() to set the variable
//
//        st->clause_sat_count = sls_mt_update_clause_sat(st);
//
//        best_sat_count = (st->clause_sat_count > best_sat_count)
//                ?
//                st->clause_sat_count 
//                : 
//                best_sat_count;
//
//        if( (st->clause_sat_count == st->cl->num_clauses)
//                && cnf_clause_all_sat(st))
//                return best_sat_count;
}


//
//
void kca_rand_candidate(kca_state *ks, mword *candidate){

    mword num_literals = array8_size(candidate);
    int i;
    int rand_val;

    char *var_array = (char*)candidate;

    for(i=1; i<=num_literals; i++){
        rand_val = (mword)sls_mt_rand();
        if(rand_val % 2)
            var_array[i] = DEC_ASSIGN0_VS;
        else
            var_array[i] = DEC_ASSIGN1_VS;
    }

}


//
//
float kca_candidate_score(kca_state *ks, mword *candidate, float sample_rate){

    float result=1.0;
    float variable_score;
    float clause_score;
    int i=0;

    while(result == 1.0){
        if(i++ > 10)
            break;
        variable_score = kca_variable_score(ks, (char*)candidate, sample_rate);
        clause_score   = kca_clause_score(ks, (char*)candidate,   sample_rate);
        result = variable_score * clause_score;
    }

    return result;

}


//
//
float kca_variable_score(kca_state *ks, char *candidate_assignment, float sample_rate){

    int i;
    int num_samples;
    mword var_id;
    float sum=0;

    if(sample_rate == 1.0f){
        for(i=1; i <= ks->st->cl->num_variables; i++){
            sum += kca_var_id_score(ks, candidate_assignment, i);
        }
        return sum / ks->st->cl->num_variables;
    }
    else{
        num_samples = ks->st->cl->num_variables * sample_rate;
        for(i=0; i < num_samples; i++){
            var_id = ((unsigned)sls_mt_rand() % (unsigned)ks->st->cl->num_variables) + 1;
            sum += kca_var_id_score(ks, candidate_assignment, var_id);
        }
        return sum / num_samples;
    }

}


// input: mu_vk and mu_~vk
//
float kca_var_id_score(kca_state *ks, char *candidate_assignment, int var_id){

    // sum = 0
    //
    // for each pos_lit:
    //      sum += candidate_assignment[lit_clause_map[i]]
    //
    // pos_avg = sum/num_lits
    //
    // for each neg_lit:
    //      sum += cnf_var_xor_score()
    //
    // neg_avg = sum/num_lits
    //
    // return (1 - abs(pos_avg - neg_avg))

    float pos_avg, neg_avg;
    mword  num_lits;
    mword *lit_clause_map;

    int i;
    int sum,lit_is_true;

    lit_clause_map = rdp(ks->st->lit_pos_clause_map, var_id);
    num_lits = size(lit_clause_map);
    sum=0;

    for(i=0; i < num_lits; i++){
        lit_is_true = ( candidate_assignment[lit_clause_map[i]] == DEC_ASSIGN1_VS );
        sum += lit_is_true;
    }

    pos_avg = (float)sum / num_lits;

    lit_clause_map = rdp(ks->st->lit_neg_clause_map,var_id);
    num_lits = size(lit_clause_map);
    sum=0;

    for(i=0; i < num_lits; i++){
        lit_is_true = ( candidate_assignment[lit_clause_map[i]] == DEC_ASSIGN0_VS );
        sum += lit_is_true;
    }

    neg_avg = (float)sum / num_lits;

    return (1 - fabs(pos_avg - neg_avg));

}


// input: mu_vk and mu_~vk
//
float kca_clause_score(kca_state *ks, char *candidate_assignment, float sample_rate){

    // for each clause: 
        // sum += cnf_clause_sat_lit(st_state *bs, int clause_id, char *candidate_assignment);
    //
    // return sum/num_clauses
    int i;
    int sum = 0;
    int num_samples;
    mword clause_id;
    int sat;

    if(sample_rate == 1.0f){
        for(i=0; i < ks->st->cl->num_clauses; i++){
            sat = cnf_clause_sat_lit(ks->st, i, candidate_assignment);
            sum += sat;
        }

        return ((float)sum) / ks->st->cl->num_clauses;
    }
    else{
        num_samples = ks->st->cl->num_clauses * sample_rate;

        for(i=0; i < num_samples; i++){
            clause_id = ((unsigned)sls_mt_rand() % (unsigned)ks->st->cl->num_clauses);
            sat = cnf_clause_sat_lit(ks->st, clause_id, candidate_assignment);
            sum += sat;
        }

        return ((float)sum) / num_samples;
    }

}


// Clayton Bauman 2018

