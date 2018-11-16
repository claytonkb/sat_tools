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
//          update cand score:
//              update sat_count (use lit_clause_map[lit_id])
//              update pos/neg_counts (use ks->st->cl->variables[lit_id])
//      rh_score_array[cand_id] = cand score
//
//  qsort the right-hand matrix by candidate score
//  find the copy threshold between the matrices
//  copy over the new candidates that will survive this round, if any
//      for each new candidate to be copied:
//          next_index = index of next-lowest-scoring candidate in lh_matrix
//          copy new candidate into lh_matrix at index
//          copy new candidate's score into candidate_lh_score_map at the appropriate index
//  qsort the left-hand matrix (that is, qsort candidate_lh_score_map by score)
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
    kca_solve_init_lh_score_map(ks, num_candidates);
    kca_solve_init_clause_map(ks, num_candidates);
    kca_solve_init_stats(ks, num_candidates);

}


// precondition:
//      literal_list populated
//
int kca_solve_body(kca_state *ks, int max_gens){

    int num_firstfruits;

    while(max_gens--){

        // generate next batch of candidates, scoring is performed during generation
        kca_solve_generate_new_candidates(ks);

        // sort new candidates and merge firstfruits, if any, into lh_matrix
        num_firstfruits = kca_solve_merge_new_generation(ks);

        // recalculate lit_avg of new entries in lh_matrix
        kca_solve_update_lit_avg(ks, num_firstfruits);

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
    mword *new_candidate;

    // create lh_matrix rows...
    for(i=0; i < ks->st->cl->num_assignments; i++){
        new_literal = mem_new_str(ks->be, num_candidates/2, '\0');
        kca_rand_literal(ks, new_literal);
        ldp(lh_matrix, i) = new_literal;
    }

    mword *score;

    // create rh_matrix rows...
    for(i=0; i < num_candidates/2; i++){
        score = _val(ks->be, 0);
        new_candidate = mem_new_str(ks->be, ks->st->cl->num_assignments, '\0');
        ldp(rh_matrix, i) = list_cons(ks->be, score, new_candidate);
    }

    ks->rh_matrix      = rh_matrix;
    ks->lh_matrix      = lh_matrix;
    ks->num_candidates = num_candidates;

}


//
//
int kca_solve_init_lh_score_map(kca_state *ks, int num_candidates){

    mword *lh_score_map = mem_new_ptr(ks->be, num_candidates/2);

    int i;
    for(i=0; i<num_candidates; i++){
        ldp(lh_score_map,i) 
            = list_cons(ks->be, _val(ks->be, 0), _val(ks->be, i));
    }

    ks->lh_score_map = lh_score_map;

}


//
//
int kca_solve_init_stats(kca_state *ks, int num_candidates){

    int i;

    ks->sat_count_array  = mem_new_val(ks->be, num_candidates/2, 0);
    ks->lit_avg_array    = mem_new_val(ks->be, ks->st->cl->num_assignments, 0);
    ks->rh_score_array   = mem_new_val(ks->be, ks->num_candidates/2, 0);

    ks->var_pos_count_array = mem_new_ptr(ks->be, num_candidates/2);
    ks->var_neg_count_array = mem_new_ptr(ks->be, num_candidates/2);

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

    // FIXME: Is this needed?:
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

    qsort(ks->rh_matrix, ks->num_candidates/2, sizeof(mword), cmp_kca_score);

}


// FIXME: This function is useless!!!
//
int kca_solve_reset_stats(kca_state *ks){

    int i, j;
    mword *curr_cand_pos_counts;
    mword *curr_cand_neg_counts;

    for(i=0; i < ks->num_candidates/2; i++){
        ks->sat_count_array[i] = 0;
        ks->rh_score_array[i] = 0;
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

    if(((int)rdv(ks->st->cl->variables,lit_id) > 0)
        && (lit_choice == DEC_ASSIGN1_VS)){
            ks->var_pos_count_array[cand_id]++;
            if(ks->last_sat_clause < ks->lit_clause_map[lit_id]){
                ks->sat_count_array[cand_id]++;
                ks->last_sat_clause = ks->lit_clause_map[lit_id];
            }
    }
    else if(((int)rdv(ks->st->cl->variables,lit_id) < 0)
        && (lit_choice == DEC_ASSIGN0_VS)){
            ks->var_neg_count_array[cand_id]++;
            if(ks->last_sat_clause < ks->lit_clause_map[lit_id]){
                ks->sat_count_array[cand_id]++;
                ks->last_sat_clause = ks->lit_clause_map[lit_id];
            }
    }
    //else do nothing

}


//
//
int kca_solve_score_candidates(kca_state *ks){

    // for cand_id = 0 to M/2
    //      rh_score_array[cand_id] = sls_kca_score(sat_count, pos_count_array, neg_count_array)
    int i;
    mword *cand_score;

    for(i=0; i < ks->num_candidates/2; i++){
        cand_score = (ks->rh_score_array+i);
        *(float*)cand_score = kca_candidate_score(ks, i);
    }

}


//
//
float  kca_candidate_score(kca_state *ks, int cand_id){

    float var_score=0.0f;
    float clause_score = kca_clause_score(ks, ks->sat_count_array[cand_id]);
    int i;

    mword *cand_pos_count_array = rdp(ks->var_pos_count_array,cand_id);
    mword *cand_neg_count_array = rdp(ks->var_neg_count_array,cand_id);

    for(i=1; i <= ks->st->cl->num_variables; i++){
        var_score += kca_var_id_score(ks, i, cand_pos_count_array[i], cand_neg_count_array[i]);
    }

    return (var_score / ks->st->cl->num_variables) * clause_score;

}


//
//
float kca_var_id_score(kca_state *ks, int var_id, int var_pos_count, int var_neg_count){

    // pos_avg = var_pos_count/num_lits
    //
    // neg_avg = var_neg_count/num_lits
    //
    // return (1 - abs(pos_avg - neg_avg))

    float pos_avg, neg_avg;
    mword  num_lits;
    mword *lit_clause_map;

    lit_clause_map = rdp(ks->st->lit_pos_clause_map, var_id);
    num_lits = size(lit_clause_map);

    pos_avg = (float)var_pos_count / num_lits;

    lit_clause_map = rdp(ks->st->lit_neg_clause_map,var_id);
    num_lits = size(lit_clause_map);

    neg_avg = (float)var_neg_count / num_lits;

    return (1 - fabs(pos_avg - neg_avg));

}


//
//
float kca_clause_score(kca_state *ks, int sat_count){

    return (float)sat_count / ks->st->cl->num_clauses;

}


//
//
var_state kca_rand_lit(kca_state *ks, int lit_id){

    float p = (float)ks->lit_avg_array[0] / (ks->num_candidates/2);

    if(rand_bent_coin(p))
        return DEC_ASSIGN1_VS;
    else
        return DEC_ASSIGN0_VS;

}


// FIXME: Get rid of rh_score_array and use rh_matrix instead!!!
//
int kca_solve_merge_new_generation(kca_state *ks){

    int num_firstfruits=0;
    float curr_lh_score;
    float curr_rh_score;

    // calculate num_firstfruits
    int lh_index = (ks->num_candidates/2)-1; // consider sorting in major order
    int rh_index = 0;
    int lh_cand_id;

    curr_lh_score = *(float*)pcar(rdp(ks->lh_score_map, lh_index));
    curr_rh_score = *(float*)(ks->rh_score_array+rh_index);

    while(curr_lh_score < curr_rh_score){
        num_firstfruits++;
        if((lh_index == 0) || (rh_index == ks->num_candidates/2))
            break;
        lh_index--;
        rh_index++;
        curr_lh_score = *(float*)pcar(rdp(ks->lh_score_map, lh_index));
        curr_rh_score = *(float*)(ks->rh_score_array+rh_index);
    }

    char *lh_lit;
    char *rh_lit;

    // overwrite worst lh_matrix candidates with best rh_matrix candidates
    // XXX THIS IS THE SLOW LOOP XXX
    int i,j;
    lh_index = (ks->num_candidates/2)-1; // consider sorting in major order
    for(i=0; i<num_firstfruits; i++){
        lh_cand_id = rdv(pcdr(rdp(ks->lh_score_map, lh_index)),0);
        rh_lit = (char*)pcdr(rdp(ks->rh_matrix, i));
        for(j=0; j < ks->st->cl->num_assignments; j++){
            lh_lit = (char*)rdp(ks->lh_matrix,j);
            lh_lit[lh_cand_id] = rh_lit[j]; // Ouch! :(
        }
        ldv(pcar(rdp(ks->lh_score_map, lh_index)),0) = rdv(ks->rh_score_array,i);
        lh_index--;
    }

    return num_firstfruits;

}


//
//
int kca_solve_update_lit_avg(kca_state *ks, int num_firstfruits){



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

