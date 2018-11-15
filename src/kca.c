// kca.c

#include "kca.h"
#include "sls.h"
#include "pearson.h"
#include "cnf.h"
#include "array.h"
#include "mem.h"
#include <math.h>

// kca-hybrid

//            c c       c             c
//            a a       a             a
//            n n . . . n    . . .    n
//            d d       d             d
//            0 1       M/2           M
//
//     lit0   - - - ... ->o | | | ... |
//     lit1   - - - ... ->o | | | ... |
//     lit2   - - - ... ->o | | | ... |
//      .       .         o   .      
//      .          .      o      .   
//      .             .   o         .
//     litN   - - - ... ->o | | | ... |
//                        V v v v     v
//                        V  scores
//                  lit_avg
//
//  Given M/2 initialized, sorted candidates (left-hand matrix) and lit_avg_array:
//
//  for cand_id = M/2+1 to M:
//      curr_cand = candidate_list[cand_id]
//      for lit_id = lit0 to litN:
//          set curr_cand[lit_id] = rand_bent_coin(lit_avg_array[lit_id] / whatever)
//          update running score:
//              update sat_count (use lit_clause_map[lit_id])
//              update pos/neg_counts (use ks->st->cl->variables[lit_id])
//      running_score_array[cand_id] = running score
//
//  qsort the right-hand matrix by candidate score
//  find the copy threshold between the matrices
//  copy over the new candidates that will survive this round, if any
//      for each new candidate to be copied:
//          next_index = index of next-lowest-scoring candidate in lh_matrix
//          copy new candidate into lh_matrix at index
//          copy new candidate's score into candidate_score_map at the appropriate index
//  qsort the left-hand matrix (that is, qsort candidate_score_map by score)
//
//  for each literal (lit_id):
//      for 0 to M/2:
//          sum positive settings
//      update lit_avg_array[lit_id] from sum

//
//
int kca_solve(kca_state *ks, int num_candidates, int max_gens){

    kca_solve_init(ks, num_candidates);
    return kca_solve_body(ks, max_gens);

}


//
//
int kca_solve_init(kca_state *ks, int num_candidates){

    kca_solve_init_matrices(ks, num_candidates);
    kca_solve_init_score_map(ks, num_candidates);
    kca_solve_init_clause_map(ks, num_candidates);
    kca_solve_init_stats(ks, num_candidates);

}


// precondition:
//      literal_list populated
//
int kca_solve_body(kca_state *ks, int max_gens){

    while(max_gens--){

        // generate next batch of candidates, scoring is performed during generation
        kca_solve_generate_new_candidates(ks);

        // score the new candidates
        kca_solve_merge_new_generation(ks);

        // sort the candidate_score_map
        kca_solve_update_literals(ks);

    }

    return 0;

}


//
//
int kca_solve_init_matrices(kca_state *ks, int num_candidates){

    mword *lh_matrix = mem_new_ptr(ks->be, ks->st->cl->num_assignments);
    mword *rh_matrix = mem_new_ptr(ks->be, num_candidates/2);

    int i;
    mword *new_literal;

    // create lh_matrix rows...
    for(i=0; i < ks->st->cl->num_assignments; i++){
        new_literal = mem_new_str(ks->be, num_candidates/2, '\0');
        kca_rand_literal(ks, new_literal);
        ldp(lh_matrix, i) = new_literal;
    }

    // create rh_matrix rows...
    for(i=0; i < num_candidates/2; i++)
        ldp(rh_matrix, i) = mem_new_str(ks->be, ks->st->cl->num_assignments, '\0');

    ks->rh_matrix      = rh_matrix;
    ks->lh_matrix      = lh_matrix;
    ks->num_candidates = num_candidates;

}


//
//
int kca_solve_init_score_map(kca_state *ks, int num_candidates){

    mword *score_map = mem_new_ptr(ks->be, num_candidates/2);

    int i;
    for(i=0; i<num_candidates; i++){
        ldp(score_map,i) 
            = list_cons(ks->be, _val(ks->be, 0), _val(ks->be, i));
    }

    ks->score_map = score_map;

}


//
//
int kca_solve_init_stats(kca_state *ks, int num_candidates){

    ks->sat_count_array     = mem_new_str(ks->be, num_candidates/2, '\0');
    ks->lit_avg_array       = mem_new_str(ks->be, ks->st->cl->num_assignments, '\0');
    ks->running_score_array = mem_new_str(ks->be, ks->st->cl->num_assignments, '\0');

    ks->var_pos_count_array = mem_new_ptr(ks->be, num_candidates/2);
    ks->var_neg_count_array = mem_new_ptr(ks->be, num_candidates/2);

    int i;

    for(i=0; i < num_candidates/2; i++){
        ldp(ks->var_pos_count_array,i) = mem_new_str(ks->be, ks->st->cl->num_variables, '\0');
        ldp(ks->var_neg_count_array,i) = mem_new_str(ks->be, ks->st->cl->num_variables, '\0');
    }

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



//
//
int kca_solve_generate_new_candidates(kca_state *ks){

    int first_cand_id = ks->num_candidates/2 + 1;
    unsigned char *curr_cand;
    var_state lit_choice;

    int i,j;

    kca_solve_reset_stats(ks);

    for(i = first_cand_id; i < ks->num_candidates; i++){

        curr_cand = (unsigned char*)rdp(ks->rh_matrix, i);

        for(j=0; j < ks->st->cl->num_assignments; j++){
            lit_choice = kca_rand_lit(ks, j);
            curr_cand[j] = (unsigned char)lit_choice;
            kca_solve_update_counts(ks, i, j, lit_choice);
        }
    }

    kca_solve_score_candidates(ks);

}


//
//
int kca_solve_reset_stats(kca_state *ks){

    int i, j;
    mword *curr_cand_pos_counts;
    mword *curr_cand_neg_counts;

    for(i=0; i < ks->num_candidates/2; i++){
        ldv(ks->sat_count_array,i) = 0;
        ldv(ks->running_score_array,i) = 0;
    }

    for(i=0; i < ks->num_candidates/2; i++){
        curr_cand_pos_counts = rdp(ks->var_pos_count_array,i);
        curr_cand_neg_counts = rdp(ks->var_neg_count_array,i);
        for(j=0; j < ks->st->cl->num_variables; j++){
            ldv(curr_cand_pos_counts,j) = 0;
            ldv(curr_cand_neg_counts,j) = 0;
        }
    }

    ks->last_sat_clause = -1;

}


//
//
int kca_solve_update_counts(kca_state *ks, int cand_id, int lit_id, var_state lit_choice){

    // update sat_count_array[cand_id]
    //      if(ks->last_sat_clause < lit_clause_map[lit_id])
    //          sat_count_array[cand_id]++
    //          ks->last_sat_clause = lit_clause_map[lit_id]
    //
    // if((int)rdv(ks->st->cl->variables,lit_id) > 0)
    //      if(lit_choice == DEC_ASSIGN1_VS)
    //          ks->var_pos_count_array[cand_id] ++
    // else
    //      if(lit_choice == DEC_ASSIGN0_VS)
    //          ks->var_neg_count_array[cand_id] ++

}


//
//
int kca_solve_score_candidates(kca_state *ks){

    // for cand_id = 0 to M/2
    //      running_score_array[cand_id] = sls_kca_score(sat_count, pos_count_array, neg_count_array)

}


//
//
var_state kca_rand_lit(kca_state *ks, int lit_id){
//            var_id = abs( ks->st->cl->variables[j] );
//(float)rdp(ks->lit_avg_array, j) / 
}

//
//
int kca_solve_merge_new_generation(kca_state *ks){



}


//
//
int kca_solve_update_literals(kca_state *ks){



}


// synonym: kca_rand_literal
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


// Clayton Bauman 2018

