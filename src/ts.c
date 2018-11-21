// ts.c

#include "ts.h"
#include "cnf.h"
#include "sls.h"


// Iterative modified DPLL + restarts
//
int ts_ucb_solve(babel_env *be, st_state *st){

    int max_tries = -1;

    int i;

    unsigned char *branch_stack   = (unsigned char*)st->branch_history;
    unsigned char *assignment_stack = (unsigned char*)st->assignment_stack;
    unsigned char *solver_stack = (unsigned char*)st->solver_stack;
    solver_cont sc;

    var_state curr_assignment;

    mword *reward_stack   = st->reward_stack;
    mword *attempts_stack = st->attempts_stack;

    mword reward=1;
    mword attempts=1;

    int curr_var = 1;
    int max_var = 1;
    int local_max_var = 1;
    int result = 0;
    int returning = 0;
    int restarting = 0;
    int restart_period = 100000;
    int major_restarting = 0;
    int major_restart_period = 10000000;
    int branch_select = 0;

    while(max_tries--){

        max_var = MAX(curr_var,max_var);
        local_max_var = MAX(curr_var,local_max_var);

st->dev_ctr++;
if((st->dev_ctr % 100000) == 0){
    if(local_max_var==max_var)
        fprintf(stderr,"[");
    fprintf(stderr,"%d",local_max_var);
    if(local_max_var==max_var)
        fprintf(stderr,"]");
    fprintf(stderr," ");
}
if((st->dev_ctr % restart_period) == 0) restarting=1;
if((st->dev_ctr % major_restart_period) == 0){
    restarting=1;
    major_restarting=1;
}

        if(restarting){

            reward = MAX(reward, reward_stack[curr_var]);
            attempts += attempts_stack[curr_var];

            ts_ucb_update_stats(st, 
                curr_var, 
                branch_stack[curr_var], 
                reward,
                attempts);

            solver_stack[curr_var] = 0;
            reward_stack[curr_var] = 0;
            attempts_stack[curr_var] = 0;
            branch_stack[curr_var] = 0;
            assignment_stack[curr_var] = UNASSIGNED_VS;

            if(curr_var > 1)
                curr_var--;
            else{

                for(i=0;i<st->cl->num_variables;i++)
                    cnf_var_write(st, i, UNASSIGNED_VS);

                if(major_restarting){
                    major_restarting = 0;
                    fprintf(stderr,"MR ");
                    for(i=0;i<size(st->num_attempts);i++){
                        st->num_attempts[i]=(st->num_attempts[i]>0);
                        st->reward[i]=st->reward[i]>>1;
                    }
                }

                reward = 1;
                attempts = 1;
                curr_var = 1;
                local_max_var = 1;
                result = 0;
                returning = 0;
                restarting = 0;
                branch_select = 0;

            }

            continue;

        }

        // XXX SECTION PURPLE XXX //
        if(curr_var > st->cl->num_variables){
            if(cnf_clause_all_sat(st)){
                result = 1;
                break;
            }
            returning = 1;
            curr_var--;
            reward = curr_var;
            continue;
        }

        // XXX SECTION PURPLE FALL-THROUGH XXX //
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

        // XXX SECTION BLUE XXX //
        //curr_assignment = ts_rand_assign();
        curr_assignment = ts_ucb_assign(st, curr_var, (branch_select<<1), 0.1);

        if(curr_assignment == DEC_ASSIGN0_VS){
            assignment_stack[curr_var] = DEC_ASSIGN1_VS;
        }
        else{
            assignment_stack[curr_var] = DEC_ASSIGN0_VS;
        }

        solver_stack[curr_var] = CONT_A_SC;

        reward = curr_var;
        attempts = 1;

        branch_stack[curr_var] = branch_select;
        branch_select = (branch_select<<1) | (curr_assignment == DEC_ASSIGN1_VS);

        if(cnf_var_assign(st, curr_var, curr_assignment)
            && !cnf_var_unsat(st, curr_var)){
            curr_var++;
        }
        else{
            goto cont_A;
        }

        continue;

cont_A:

        // XXX SECTION GREEN XXX //
        reward_stack[curr_var] = reward;
        attempts_stack[curr_var] = attempts;

        branch_select = branch_stack[curr_var];
        branch_select = (branch_select<<1) | (assignment_stack[curr_var] == DEC_ASSIGN1_VS);

        cnf_var_unassign(st, curr_var);

        if(cnf_var_assign(st, curr_var, assignment_stack[curr_var])){

            if(cnf_var_unsat(st, curr_var)){
                goto cont_B;
            }
            else{
                solver_stack[curr_var] = CONT_B_SC; 
                curr_var++;
            }

        }
        else{
            goto cont_B;
        }

        continue;

cont_B:

        // XXX SECTION RED XXX //

        if(curr_var == 1) // went the wrong way :(
            restarting=1;

        branch_select = branch_stack[curr_var]; // redundant?
        reward   = MAX(reward, reward_stack[curr_var]);
        attempts = attempts + attempts_stack[curr_var];

        ts_ucb_update_stats(st, 
            curr_var, 
            branch_select,
            reward,
            attempts);
        
        cnf_var_unassign(st, curr_var);
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
var_state ts_ucb_assign(st_state *st, int curr_var, int table_branch_select, float fuzz){

    if(rand_bent_coin(fuzz))
        return ts_rand_assign();

    unsigned char select_0 = ((unsigned char)table_branch_select) & ~0x1;
    unsigned char select_1 = ((unsigned char)table_branch_select) |  0x1;

    int reward_0;
    int reward_1;

    int attempts_0 = ldv(st->num_attempts,(256*curr_var)+select_0);
    int attempts_1 = ldv(st->num_attempts,(256*curr_var)+select_1);

    if((attempts_1 == 0) && (attempts_0 == 0)){
        return ts_rand_assign();
//        if(ts_rand_assign()==DEC_ASSIGN1_VS){
//            rdv(st->num_attempts,(256*curr_var)+select_1)=1;
//            return DEC_ASSIGN1_VS;
//        }
//        else{
//            rdv(st->num_attempts,(256*curr_var)+select_0)=1;
//            return DEC_ASSIGN0_VS;
//        }
    }
    else if(attempts_1 == 0){
//        rdv(st->num_attempts,(256*curr_var)+select_1)=1;
        return DEC_ASSIGN1_VS;
    }
    else if(attempts_0 == 0){
//        rdv(st->num_attempts,(256*curr_var)+select_0)=1;
        return DEC_ASSIGN0_VS;
    }
    else{
        reward_0 = ldv(st->reward,(256*curr_var)+select_0);
        reward_1 = ldv(st->reward,(256*curr_var)+select_1);
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
//
//    return (choice < 0.0f);

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
    int i;

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

