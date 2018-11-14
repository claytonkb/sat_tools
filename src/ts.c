// ts.c

#include "ts.h"
#include "cnf.h"
#include "sls.h"


// Iterative modified DPLL w/UCB
//
int ts_ucb_solve(babel_env *be, st_state *bs){

    int curr_var = 1;
    int result = 0;
    int returning = 0;

    char *solver_stack = (char*)bs->solver_stack;
    char *assignment_stack = (char*)bs->assignment_stack;

    var_state curr_assignment;

    solver_cont sc;

    while(1){

bs->dev_ctr++;
if((bs->dev_ctr % 100000) == 0){
    _prn(".");
    if((bs->dev_ctr % 100000000) == 0){
        _say("");
        _dd(bs->dev_ctr);
    }
}

        if(curr_var == 1 && result == 1) // SAT
            break;

        if(returning){
            returning = 0;
            sc = solver_stack[curr_var];
            switch(sc){
                case CONT_A_SC:
                    goto cont_A;
                case CONT_B_SC:
                    goto cont_B;
            }
        }

        if(curr_var > bs->cl->num_variables){
            if(cnf_clause_all_sat(bs)){
                result = 1;
                returning = 1;
            }
            curr_var--;
            continue;
        }

// call ts_ucb_choice() to choose assignment and store anti-assignment in 
//      assignment_stack

        curr_assignment = ts_rand_assign();

        if(curr_assignment == DEC_ASSIGN0_VS)
            assignment_stack[curr_var] = DEC_ASSIGN1_VS;
        else
            assignment_stack[curr_var] = DEC_ASSIGN0_VS;

//        assignment_stack[curr_var] = DEC_ASSIGN0_VS;

//        if(cnf_var_assign(bs, curr_var, DEC_ASSIGN1_VS)){
        if(cnf_var_assign(bs, curr_var, curr_assignment)){
            if(!cnf_var_unsat(bs, curr_var)){
                solver_stack[curr_var] = CONT_A_SC;
                curr_var++;
                continue;
            }
        }

cont_A:

        if(result == 1){
            returning = 1;
            curr_var--;
            continue;
        }
        // else 

// update rewards/num_attempts arrays for first leg
//void ts_ucb_update_stats(st_state *st, var_state curr_assignment, int curr_var){

        cnf_var_unassign(bs, curr_var);
//        if(cnf_var_assign(bs, curr_var, DEC_ASSIGN0_VS)){
        if(cnf_var_assign(bs, curr_var, assignment_stack[curr_var])){

            if(cnf_var_unsat(bs, curr_var)){
                result = 0;
                goto cont_B;
            }
            else{
                solver_stack[curr_var] = CONT_B_SC; 
                curr_var++;
                continue;
            }

        }

cont_B:

        if(result == 0){
            if(curr_var == 1) // UNSAT
                break;
            cnf_var_unassign(bs, curr_var);
        }

// update rewards/num_attempts arrays for second leg
//void ts_ucb_update_stats(st_state *st, var_state curr_assignment, int curr_var){

        returning = 1;
        curr_var--;
        continue;


    }

    return result;

}


//
//
var_state ts_rand_assign(void){

    if(sls_mt_rand()%2)
        return DEC_ASSIGN1_VS;
    else
        return DEC_ASSIGN0_VS;

}



//
//
int ts_ucb_choice(float reward_0, float reward_1, float num_attempts_0, float num_attempts_1){

    float num_attempts = num_attempts_0 + num_attempts_1;

//    float partial_0 = (reward_0 + sqrt((2 * log(num_attempts)) / num_attempts_0));
//    float partial_1 = (reward_1 + sqrt((2 * log(num_attempts)) / num_attempts_1));
//    float choice = partial_0 - partial_1;

    float choice = 
            (reward_0 + sqrt((2 * log(num_attempts)) / num_attempts_0))
            -
            (reward_1 + sqrt((2 * log(num_attempts)) / num_attempts_1));

    return (choice < 0.0f);

}


//
//
void ts_ucb_update_stats(st_state *st, var_state curr_assignment, int curr_var){

// update rewards/num_attempts arrays for first leg
//      table_select = branch_history[curr_var]
//      if(curr_assignment == DEC_ASSIGN0_VS)
//          attempts = bs->attempts_0
//          rewards = bs->rewards_0
//      else
//          attempts = bs->attempts_1
//          rewards = bs->rewards_1
//      attempts_table = rdp(attempts, (256*curr_var))
//      rewards_table  = rdp(rewards,  (256*curr_var))
//      attempts = rdp(attempts_table, table_select)
//      rewards  = rdp(rewards_table,  table_select)

}


// Iterative modified DPLL
//
int ts_solve(babel_env *be, st_state *bs){

    int curr_var = 1;
    int result = 0;
    int returning = 0;

    char *solver_stack = (char*)bs->solver_stack;
    solver_cont sc;

    while(1){

bs->dev_ctr++;
if((bs->dev_ctr % 100000) == 0){
    _prn(".");
    if((bs->dev_ctr % 100000000) == 0){
        _say("");
        _dd(bs->dev_ctr);
    }
}

        if(curr_var == 1 && result == 1) // SAT
            break;

        if(returning){
            returning = 0;
            sc = solver_stack[curr_var];
            switch(sc){
                case CONT_A_SC:
                    goto cont_A;
                case CONT_B_SC:
                    goto cont_B;
             }
        }

        if(curr_var > bs->cl->num_variables){
            if(cnf_clause_all_sat(bs)){
                result = 1;
                returning = 1;
            }
            curr_var--;
            continue;
        }

        if(cnf_var_assign(bs, curr_var, DEC_ASSIGN1_VS)){
            if(!cnf_var_unsat(bs, curr_var)){
                solver_stack[curr_var] = CONT_A_SC;
                curr_var++;
                continue;
            }
        }

cont_A:

        if(result == 1){
            returning = 1;
            curr_var--;
            continue;
        }
        // else 

        cnf_var_unassign(bs, curr_var);
        if(cnf_var_assign(bs, curr_var, DEC_ASSIGN0_VS)){

            if(cnf_var_unsat(bs, curr_var)){
                result = 0;
                goto cont_B;
            }
            else{
                solver_stack[curr_var] = CONT_B_SC; 
                curr_var++;
                continue;
            }

        }

cont_B:

        if(result == 0){
            if(curr_var == 1) // UNSAT
                break;
            cnf_var_unassign(bs, curr_var);
        }

        returning = 1;
        curr_var--;
        continue;


    }

    return result;

}

// Clayton Bauman 2018

