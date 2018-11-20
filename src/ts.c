// ts.c

#include "ts.h"
#include "cnf.h"
#include "sls.h"


// Iterative modified DPLL w/UCB
//
int ts_ucb_solve(babel_env *be, st_state *st){

    int i;

    unsigned char *solver_stack     = (unsigned char*)st->solver_stack;
    unsigned char *branch_history   = (unsigned char*)st->branch_history;
    unsigned char *assignment_stack = (unsigned char*)st->assignment_stack;
    mword         *reward_stack     = st->reward_stack;
    mword         *attempts_stack   = st->attempts_stack;

    int curr_var;
    int result;
    int returning;

    int max_var=1;
    int local_max_var=1;
    int last_local_max_var=1;

    mword reward;
    mword attempts;
    mword attempts_A, attempts_B;

    unsigned char table_branch_select;

    var_state curr_assignment;

    solver_cont sc;

    float fuzz = 0.1f;

restart:

    for(i=1; i<=st->cl->num_variables; i++){
        solver_stack[i] = 0;
        branch_history[i] = 0;
        assignment_stack[i] = UNASSIGNED_VS;
        cnf_var_write(st, i, UNASSIGNED_VS);
    }

    curr_var = 1;
    result = 0;
    returning = 0;
    reward=1;
    attempts=0;
    table_branch_select=0;

    while(1){

        max_var = MAX(curr_var,max_var);
        local_max_var = MAX(curr_var,local_max_var);

st->dev_ctr++;
if((st->dev_ctr % 100000) == 0){

    _prn(".");

//    if(st->dev_ctr > 10000000)
//        return 0;

    if((st->dev_ctr % 10000000) == 0){
        _say("");
        _dd(st->dev_ctr);
    }

    if((st->dev_ctr % 1000000) == 0){
        fprintf(stderr, "%d:%d", max_var, local_max_var);
        local_max_var=1;
    }

    if((st->dev_ctr % 100000) == 0){
        attempts=0;
        for(i=curr_var; i>0; i--){ // unwind the stats stacks
            attempts += attempts_stack[i];
            reward = MAX(reward,reward_stack[i]);
            ts_ucb_update_stats(st, 
                i, 
                branch_history[i], 
                reward,
                attempts);
        }

        goto restart;
    }

}

        if(curr_var == 1 && result == 1) // SAT
            break;

        if(returning){
            returning = 0;
            sc = solver_stack[curr_var];
            table_branch_select = branch_history[curr_var];
            switch(sc){
                case CONT_A_SC:
                    goto cont_A;
                case CONT_B_SC:
                    goto cont_B;
            }
        }

        reward = curr_var;
        attempts = 1;

        if(curr_var > st->cl->num_variables){
            if(cnf_clause_all_sat(st)){
                result = 1;
                returning = 1;
            }
            curr_var--;
            continue;
        }

//        curr_assignment = ts_rand_assign();
        curr_assignment = ts_ucb_assign(st, curr_var, table_branch_select, fuzz);

        if(curr_assignment == DEC_ASSIGN0_VS){
            assignment_stack[curr_var] = DEC_ASSIGN1_VS;
        }
        else{
            assignment_stack[curr_var] = DEC_ASSIGN0_VS;
        }

        if(cnf_var_assign(st, curr_var, curr_assignment)){
            if(!cnf_var_unsat(st, curr_var)){
                solver_stack[curr_var] = CONT_A_SC;
                branch_history[curr_var] = table_branch_select;
                table_branch_select <<= 1;
                if(curr_assignment == DEC_ASSIGN1_VS)
                    table_branch_select |= 1;
                curr_var++;
                continue;
            }
        }

cont_A:

        reward_stack[curr_var]   = reward;
        attempts_stack[curr_var] = attempts+1;

        if(result == 1){
            returning = 1;
            curr_var--;
            continue;
        }
//        else{
//            ts_ucb_update_stats(st, curr_var, table_branch_select, reward, attempts);
//        }

        cnf_var_unassign(st, curr_var);
        if(cnf_var_assign(st, curr_var, assignment_stack[curr_var])){

            if(cnf_var_unsat(st, curr_var)){
                result = 0;
                goto cont_B;
            }
            else{
                solver_stack[curr_var] = CONT_B_SC; 
                branch_history[curr_var] = table_branch_select;
                table_branch_select <<= 1;
                if(assignment_stack[curr_var] == DEC_ASSIGN1_VS)
                    table_branch_select |= 1;
                curr_var++;
                continue;
            }

        }

cont_B:

        reward = MAX(reward, reward_stack[curr_var]);
        attempts = attempts + attempts_stack[curr_var];

        if(result == 0){
            if(curr_var == 1) // UNSAT
                goto restart;
            cnf_var_unassign(st, curr_var);
            ts_ucb_update_stats(st, curr_var, table_branch_select, reward, attempts);
        }

        returning = 1;
        curr_var--;

        continue;

    }

    return result;

}

//        if((st->dev_ctr % 100000) == 0){ // restart
//            for(i=1; i<=curr_var; i++){
//                assignment_stack[i] = UNASSIGNED_VS;
//                solver_stack[i] = 0;
//            }
//            curr_var = 1;
//            goto restart;
//        }
//        else{
//            if(curr_var > 1)
//                curr_var--;
//        }


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
var_state ts_ucb_assign(st_state *st, int curr_var, int table_branch_select, float fuzz){

    if(rand_bent_coin(fuzz))
        return ts_rand_assign();

    unsigned char select = (unsigned char)table_branch_select;
    select <<= 1;

    int reward_0;
    int reward_1;

    int attempts_0 = ldv(st->num_attempts,(256*curr_var)+(select & ~0x1));
    int attempts_1 = ldv(st->num_attempts,(256*curr_var)+(select |  0x1));

    if((attempts_1 == 0) && (attempts_0 == 0))
        return ts_rand_assign();
    else if(attempts_1 == 0)
        return DEC_ASSIGN1_VS;
    else if(attempts_0 == 0)
        return DEC_ASSIGN0_VS;
    else{
        reward_0 = ldv(st->reward,(256*curr_var)+(select & ~0x1));
        reward_1 = ldv(st->reward,(256*curr_var)+(select |  0x1));
        // XXX REVERSED? XXX //
        if(ts_ucb_choice(reward_0, reward_1, attempts_0, attempts_1))
            return DEC_ASSIGN1_VS;
        else
            return DEC_ASSIGN0_VS;
    }

}


//
//
int ts_ucb_choice(float reward_0, float reward_1, float num_attempts_0, float num_attempts_1){

    float num_attempts = num_attempts_0 + num_attempts_1;

//    float choice = 
//            (reward_0 + sqrt((2 * log(num_attempts)) / num_attempts_0))
//            -
//            (reward_1 + sqrt((2 * log(num_attempts)) / num_attempts_1));

    float choice = 
            (reward_1 + sqrt((2 * log(num_attempts)) / num_attempts_1))
            -
            (reward_0 + sqrt((2 * log(num_attempts)) / num_attempts_0));

    return (choice > 0.0f);

}


//
//
void ts_ucb_update_stats(st_state *st, int curr_var, int table_branch_select, int reward, int attempts){

    // max-reward
    // attempts sum
    unsigned char branch_select = (unsigned char)table_branch_select;

    int curr_reward   = rdv(st->reward,(256*curr_var)+branch_select);
    int curr_attempts = rdv(st->num_attempts,(256*curr_var)+branch_select);

    ldv(st->reward,(256*curr_var)+branch_select)       = MAX(curr_reward,reward);
    ldv(st->num_attempts,(256*curr_var)+branch_select) = curr_attempts + attempts;

}


// Iterative modified DPLL
//
int ts_solve(babel_env *be, st_state *st){

    int curr_var = 1;
    int result = 0;
    int returning = 0;

    char *solver_stack = (char*)st->solver_stack;
    solver_cont sc;

    while(1){

st->dev_ctr++;
if((st->dev_ctr % 100000) == 0){
    _prn(".");
    if((st->dev_ctr % 100000000) == 0){
        return 0;
        _say("");
        _dd(st->dev_ctr);
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

        if(curr_var > st->cl->num_variables){
            if(cnf_clause_all_sat(st)){
                result = 1;
                returning = 1;
            }
            curr_var--;
            continue;
        }

        if(cnf_var_assign(st, curr_var, DEC_ASSIGN1_VS)){
            if(!cnf_var_unsat(st, curr_var)){
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

        cnf_var_unassign(st, curr_var);
        if(cnf_var_assign(st, curr_var, DEC_ASSIGN0_VS)){

            if(cnf_var_unsat(st, curr_var)){
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
            cnf_var_unassign(st, curr_var);
        }

        returning = 1;
        curr_var--;
        continue;


    }

    return result;

}

// Clayton Bauman 2018

