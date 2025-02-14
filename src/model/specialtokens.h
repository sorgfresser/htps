//
// Created by simon on 13.02.25.
//

#ifndef HTPS_SPECIALTOKENS_H
#define HTPS_SPECIALTOKENS_H

#include <string>
#include <array>


namespace htps {
    constexpr char BOS_WORD[] = "<s>";
    constexpr char EOS_WORD[] = "</s>";
    constexpr char PAD_WORD[] = "<pad>";
    constexpr char UNK_WORD[] = "<unk>";
    constexpr char MASK_WORD[] = "<MASK>";
    constexpr char B_CMD_WORD[] = "<COMMAND>";
    constexpr char E_CMD_WORD[] = "</COMMAND>";
    constexpr char B_GOAL_WORD[] = "<GOAL>";
    constexpr char E_GOAL_WORD[] = "</GOAL>";
    constexpr char B_HYP_WORD[] = "<HYP>";
    constexpr char M_HYP_WORD[] = "<HYP_NAME>";
    constexpr char E_HYP_WORD[] = "</HYP>";
    constexpr char EXCEPTION_WORD[] = "<EXCEPTION>";
    constexpr char EMPTY_GOAL_WORD[] = "<EMPTY_GOAL>";
    constexpr char UNAFFECTED_GOAL_WORD[] = "<UNAFFECTED_GOAL>";
    constexpr char B_SUBST_WORD[] = "<SUBST>";
    constexpr char M_SUBST_WORD[] = "<SUBST_NAME>";
    constexpr char E_SUBST_WORD[] = "</SUBST>";
    constexpr char B_STACK_WORD[] = "<STACK>";
    constexpr char M_STACK_WORD[] = "<STACK_SEP>";
    constexpr char E_STACK_WORD[] = "</STACK>";
    constexpr char EMB_WORD[] = "<EMB>";
    constexpr char EMPTY_NODE_WORD[] = "<EMPTY_NODE>";
    constexpr char B_THEOREM_WORD[] = "<THEOREM>";
    constexpr char E_THEOREM_WORD[] = "</THEOREM>";
    constexpr char EOU_WORD[] = "<END_OF_USEFUL>";
    constexpr char SOLVABLE_WORD[] = "<SOLVABLE>";
    constexpr char NON_SOLVABLE_WORD[] = "<NON_SOLVABLE>";
    constexpr char CRITIC_WORD[] = "<CRITIC>";
    constexpr char B_NODE_WORD[] = "<NODE>";
    constexpr char E_NODE_WORD[] = "</NODE>";
    constexpr char B_HEAD_WORD[] = "<HEAD>";
    constexpr char E_HEAD_WORD[] = "</HEAD>";
    constexpr char STOP_WORD[] = "<STOP>";
    constexpr char NEWLINE_WORD[] = "<NEWLINE>";
    constexpr char REPEAT_GOAL_WORD[] = "<REPEAT_GOAL>";
    constexpr char GOAL_STATEMENT_WORD[] = "<GOAL_STATEMENT>";
    constexpr char SUCCESS_WORD[] = "<SUCCESS>";
    constexpr char NO_EFFECT_WORD[] = "<NO_EFFECT>";
    constexpr char UNK_ERROR_WORD[] = "UNK_ERROR";
    constexpr char PROVED_WORD[] = "<PROVED>";
    constexpr char UNPROVED_WORD[] = "<UNPROVED>";
    constexpr char GEN_REAL[] = "<GEN_REAL>";
    constexpr char GEN_FAKE[] = "<GEN_FAKE>";
    constexpr char B_NS_WORD[] = "<NS>";
    constexpr char E_NS_WORD[] = "</NS>";
    constexpr char BWD_WORD[] = "<BWD>";
    constexpr char TARGET_IS_GOAL_WORD[] = "<TARGET_IS_GOAL>";
    constexpr char TACTIC_FILL_WORD[] = "<TACTIC_FILL>";
    constexpr char TACTIC_PARSE_ERROR_WORD[] = "<TACTIC_PARSE_ERROR>";
    constexpr char TACTIC_ENV_ERROR_WORD[] = "<TACTIC_ENV_ERROR>";

    constexpr size_t N_SPECIAL_TOKENS = 51;

    constexpr const char *SPECIAL_TOKENS[] = {
            BOS_WORD, EOS_WORD, PAD_WORD, UNK_WORD, MASK_WORD, B_CMD_WORD, E_CMD_WORD, B_GOAL_WORD, E_GOAL_WORD,
            B_HYP_WORD, M_HYP_WORD, E_HYP_WORD, EXCEPTION_WORD, EMPTY_GOAL_WORD, UNAFFECTED_GOAL_WORD, B_SUBST_WORD,
            M_SUBST_WORD, E_SUBST_WORD, B_STACK_WORD, M_STACK_WORD, E_STACK_WORD, EMB_WORD, EMPTY_NODE_WORD,
            B_THEOREM_WORD, E_THEOREM_WORD, EOU_WORD, SOLVABLE_WORD, NON_SOLVABLE_WORD, CRITIC_WORD, B_NODE_WORD,
            E_NODE_WORD, B_HEAD_WORD, E_HEAD_WORD, STOP_WORD, NEWLINE_WORD, REPEAT_GOAL_WORD, GOAL_STATEMENT_WORD,
            SUCCESS_WORD, NO_EFFECT_WORD, UNK_ERROR_WORD, PROVED_WORD, UNPROVED_WORD, GEN_REAL, GEN_FAKE, B_NS_WORD,
            E_NS_WORD, BWD_WORD, TARGET_IS_GOAL_WORD, TACTIC_FILL_WORD, TACTIC_PARSE_ERROR_WORD, TACTIC_ENV_ERROR_WORD
    };
}

#endif //HTPS_SPECIALTOKENS_H
