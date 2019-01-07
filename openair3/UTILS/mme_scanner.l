
/* Lex configuration parser.
 *
 * This file defines the token for parsing the configuration file
 *
 * Note : This module is NOT thread-safe. All processing must be done from one thread only.
 */
%{
#include <stdio.h>

#include "mme_config.h"
/* Include yacc tokens definitions */
#include "mme_parser.h"

/* Update the column information */
#ifdef DEBUG_LEX
#define YY_USER_ACTION {                        \
    yylloc->first_column = yylloc->last_column + 1;         \
    yylloc->last_column = yylloc->first_column + yyleng - 1;    \
    TRACE_DEBUG(FULL,                       \
        "(%d:%d-%d:%d) matched rule %d, length=%d, txt='%s'\n", \
        yylloc->first_line, yylloc->first_column,       \
        yylloc->last_line, yylloc->last_column,         \
        yy_act, yyleng, yytext);                \
}
#else /* DEBUG_LEX */
#define YY_USER_ACTION {                        \
    yylloc->first_column = yylloc->last_column + 1;         \
    yylloc->last_column = yylloc->first_column + yyleng - 1;    \
}
#endif

#define YY_NO_INPUT
%}

%option bison-bridge bison-locations
%option noyywrap
%option nounput

/* Quoted string. Multilines do not match. */
qstring     \"[^\"\n]*\"

%%

    /* List of patterns and actions */

<*>\n {
    /* Update the line count */
    yylloc->first_line++;
    yylloc->last_line++; 
    yylloc->last_column=0;
}

<*>([[:space:]]{-}[\n])+    ; /* Eat all spaces, not new lines */
<*>#.*$                     ; /* Eat all comments */

{qstring}       {
    /* First copy the string without the quotes for use in the yacc parser */
    if ((yylval->string = strdup(yytext+1)) == NULL) { /* This allocates one useless tail char but... it's easier :D */
        return LEX_ERROR;/* on error, trig an error in yacc parser */
    }
    yylval->string[yyleng-2] = '\0';

    /* the yacc parser will check the string is valid */
    return QSTRING;
}

[[:digit:]]+    {
    /* Convert this to an integer value */
    int ret = sscanf(yytext, "%i", &yylval->integer);
    if (ret != 1) {
        /* No matching: an error occurred */
        fprintf(stderr, "Unable to convert the value '%s' to a valid number: %s\n",
                yytext, strerror(errno));
        return LEX_ERROR; /* trig an error in yacc parser */
        /* Maybe we could REJECT instead of failing here? */
    }
    return INTEGER;
}

    /* Full words tokens (keywords) */
(?i:"EMERGENCY_ATTACH_SUPPORTED")     { return EMERGENCY_ATTACH_SUPPORTED; }
(?i:"UNAUTHENTICATED_IMSI_SUPPORTED") { return UNAUTHENTICATED_IMSI_SUPPORTED; }
(?i:"MAXENB")                   { return MAX_ENB; }
(?i:"MAXUE")                    { return MAX_UE; }
(?i:"S6A_CONF")                 { return S6A_CONF; }
(?i:"ITTI_QUEUE_SIZE")          { return ITTI_QUEUE_SIZE; }
(?i:"SCTP_INSTREAMS")           { return SCTP_INSTREAMS; }
(?i:"SCTP_OUTSTREAMS")          { return SCTP_OUTSTREAMS; }
(?i:"MME_CODE")                 { return MME_CODE; }
(?i:"MME_GID")                  { return MME_GID; }
(?i:"REALM")                    { return REALM; }
(?i:"PLMN")                     { return PLMN_Y; }
(?i:"RELATIVE_CAPACITY")        { return RELATIVE_CAP; }
(?i:"S1AP_OUTCOME_TIMER")       { return S1AP_OUTCOME_TIMER; }
(?i:"MME_STATISTIC_TIMER")      { return MME_STATISTIC_TIMER; }
(?i:"SGW_INTERFACE_NAME_FOR_S11")           { return  SGW_INTERFACE_NAME_FOR_S11; }
(?i:"SGW_IP_ADDRESS_FOR_S11")               { return  SGW_IP_ADDRESS_FOR_S11; }
(?i:"SGW_IP_NETMASK_FOR_S11")               { return  SGW_IP_NETMASK_FOR_S11; }
(?i:"SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP") { return  SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP; }
(?i:"SGW_IP_ADDRESS_FOR_S1U_S12_S4_UP")     { return  SGW_IP_ADDRESS_FOR_S1U_S12_S4_UP; }
(?i:"SGW_IP_NETMASK_FOR_S1U_S12_S4_UP")     { return  SGW_IP_NETMASK_FOR_S1U_S12_S4_UP; }
(?i:"SGW_INTERFACE_NAME_FOR_S5_S8_UP")      { return  SGW_INTERFACE_NAME_FOR_S5_S8_UP; }
(?i:"SGW_IP_ADDRESS_FOR_S5_S8_UP")          { return  SGW_IP_ADDRESS_FOR_S5_S8_UP; }
(?i:"SGW_IP_NETMASK_FOR_S5_S8_UP")          { return  SGW_IP_NETMASK_FOR_S5_S8_UP; }
(?i:"PGW_INTERFACE_NAME_FOR_S5_S8")         { return  PGW_INTERFACE_NAME_FOR_S5_S8; }
(?i:"PGW_IP_ADDRESS_FOR_S5_S8")             { return  PGW_IP_ADDRESS_FOR_S5_S8; }
(?i:"PGW_IP_NETMASK_FOR_S5_S8")             { return  PGW_IP_NETMASK_FOR_S5_S8; }
(?i:"PGW_INTERFACE_NAME_FOR_SGI")           { return  PGW_INTERFACE_NAME_FOR_SGI; }
(?i:"PGW_IP_ADDR_FOR_SGI")                  { return  PGW_IP_ADDR_FOR_SGI; }
(?i:"PGW_IP_NETMASK_FOR_SGI")               { return  PGW_IP_NETMASK_FOR_SGI; }
(?i:"MME_INTERFACE_NAME_FOR_S1_MME")        { return  MME_INTERFACE_NAME_FOR_S1_MME; }
(?i:"MME_IP_ADDRESS_FOR_S1_MME")            { return  MME_IP_ADDRESS_FOR_S1_MME; }
(?i:"MME_IP_NETMASK_FOR_S1_MME")            { return  MME_IP_NETMASK_FOR_S1_MME; }
(?i:"MME_INTERFACE_NAME_FOR_S11_MME")       { return  MME_INTERFACE_NAME_FOR_S11_MME; }
(?i:"MME_IP_ADDRESS_FOR_S11_MME")           { return  MME_IP_ADDRESS_FOR_S11_MME; }
(?i:"MME_IP_NETMASK_FOR_S11_MME")           { return  MME_IP_NETMASK_FOR_S11_MME; }

    /* Valid single characters for yyparse */
<*>[.=,:;{}]     { return yytext[0]; }

<*>[[:alnum:]]+     |   /* This rule is only useful to print a complete token in error messages */
    /* Unrecognized character */
<*>.    {
    fprintf(stderr, "Unrecognized text on line %d col %d: '%s'.\n",
            yylloc->first_line, yylloc->first_column, yytext);
    return LEX_ERROR;
}

%%
