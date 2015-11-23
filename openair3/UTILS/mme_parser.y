/* For development only : */
%debug
%error-verbose

%parse-param {struct mme_config_s *mme_config_p}

/* Keep track of location */
%locations
%defines
%pure-parser

%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mme_config.h"
#include "mme_parser.h"

void yyerror (YYLTYPE *llocp, struct mme_config_s *mme_config_p, const char *s);

extern int yywrap();
extern int yylex();

/* The Lex parser prototype */
int fddlex(YYSTYPE *lvalp, YYLTYPE *llocp);
%}

/* Values returned by lex for token */
%union {
    char         *string;   /* The string is allocated by strdup in lex.*/
    int           integer;  /* Store integer values */
}

%token LEX_ERROR

%token <string> QSTRING
%token <integer> INTEGER

%token EMERGENCY_ATTACH_SUPPORTED
%token UNAUTHENTICATED_IMSI_SUPPORTED
%token S6A_CONF
%token MAX_UE
%token MAX_ENB
%token ITTI_QUEUE_SIZE
%token SCTP_INSTREAMS
%token SCTP_OUTSTREAMS
%token MME_CODE
%token MME_GID
%token PLMN_Y
%token REALM
%token RELATIVE_CAP
%token S1AP_OUTCOME_TIMER
%token MME_STATISTIC_TIMER
%token SGW_INTERFACE_NAME_FOR_S11
%token SGW_IP_ADDRESS_FOR_S11
%token SGW_IP_NETMASK_FOR_S11
%token SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP
%token SGW_IP_ADDRESS_FOR_S1U_S12_S4_UP
%token SGW_IP_NETMASK_FOR_S1U_S12_S4_UP
%token SGW_INTERFACE_NAME_FOR_S5_S8_UP
%token SGW_IP_ADDRESS_FOR_S5_S8_UP
%token SGW_IP_NETMASK_FOR_S5_S8_UP
%token PGW_INTERFACE_NAME_FOR_S5_S8
%token PGW_IP_ADDRESS_FOR_S5_S8
%token PGW_IP_NETMASK_FOR_S5_S8
%token PGW_INTERFACE_NAME_FOR_SGI
%token PGW_IP_ADDR_FOR_SGI
%token PGW_IP_NETMASK_FOR_SGI
%token MME_INTERFACE_NAME_FOR_S1_MME
%token MME_IP_ADDRESS_FOR_S1_MME
%token MME_IP_NETMASK_FOR_S1_MME
%token MME_INTERFACE_NAME_FOR_S11_MME
%token MME_IP_ADDRESS_FOR_S11_MME
%token MME_IP_NETMASK_FOR_S11_MME

%%
conffile:       /* If options not provided, we will default values */
    | conffile s6aconf
    | conffile emergency_attach_supported
    | conffile unauthenticated_imsi_supported
    | conffile maxenb
    | conffile maxue
    | conffile mmec
    | conffile mmegid
    | conffile plmn
    | conffile realm
    | conffile relative_capacity
    | conffile itti_queue_size
    | conffile s1ap_outcome_timer
    | conffile mme_statistic_timer
    | conffile sgw_interface_name_for_S11
    | conffile sgw_ip_address_for_S11
    | conffile sgw_ip_netmask_for_S11
    | conffile sgw_interface_name_for_S1u_S12_S4_up
    | conffile sgw_ip_address_for_S1u_S12_S4_up
    | conffile sgw_ip_netmask_for_S1u_S12_S4_up
    | conffile sgw_interface_name_for_S5_S8_up
    | conffile sgw_ip_address_for_S5_S8_up
    | conffile sgw_ip_netmask_for_S5_S8_up
    | conffile pgw_interface_name_for_S5_S8
    | conffile pgw_ip_address_for_S5_S8
    | conffile pgw_ip_netmask_for_S5_S8
    | conffile pgw_interface_name_for_SGI
    | conffile pgw_ip_addr_for_SGI
    | conffile pgw_ip_netmask_for_SGI
    | conffile mme_interface_name_for_S1_MME
    | conffile mme_ip_address_for_S1_MME
    | conffile mme_ip_netmask_for_S1_MME
    | conffile mme_interface_name_for_S11_MME
    | conffile mme_ip_address_for_S11_MME
    | conffile mme_ip_netmask_for_S11_MME
    | conffile sctp_instreams
    | conffile sctp_outstreams
    | conffile errors
    {
        yyerror(&yylloc, mme_config_p, "An error occurred while parsing the configuration file");
        return EINVAL;
    }
    ;

emergency_attach_supported: EMERGENCY_ATTACH_SUPPORTED '=' INTEGER ';'
    {
        if ($3 != 0 && $3 != 1) {
            yyerror(&yylloc, mme_config_p, "Invalid value (possible values are 0 or 1");
            return EINVAL;
        }
        mme_config_p->emergency_attach_supported = $3;
    };

unauthenticated_imsi_supported: UNAUTHENTICATED_IMSI_SUPPORTED '=' INTEGER ';'
    {
        if ($3 != 0 && $3 != 1) {
            yyerror(&yylloc, mme_config_p, "Invalid value (possible values are 0 or 1");
            return EINVAL;
        }
        mme_config_p->unauthenticated_imsi_supported = $3;
    };

mme_statistic_timer: MME_STATISTIC_TIMER '=' INTEGER ';'
    {
        if ($3 <= 0) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        }
        mme_config_p->mme_statistic_timer = $3;
    }
    ;

s1ap_outcome_timer: S1AP_OUTCOME_TIMER '=' INTEGER ';'
    {
        /* 0 is not a valid value here */
        if ($3 <= 0 || $3 >= (1 << (8 * sizeof(mme_config_p->s1ap_config.outcome_drop_timer_sec)))) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        }
        mme_config_p->s1ap_config.outcome_drop_timer_sec = $3;
    }
    ;

s6aconf: S6A_CONF '=' QSTRING ';'
    {
        mme_config_p->s6a_config.conf_file = $3;
    }
    ;

maxenb: MAX_ENB '=' INTEGER ';'
    {
        if ($3 <= 0) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        }
        mme_config_p->max_eNBs = (uint32_t)$3;
    }
    ;

maxue: MAX_UE '=' INTEGER ';'
    {
        if ($3 <= 0) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        }
        mme_config_p->max_ues = (uint32_t)$3;
    }
    ;

mmec: MME_CODE '=' mmeclist ';'
    ;

mmeclist:
    {
        yyerror(&yylloc, mme_config_p, "Empty value is not allowed");
        return EINVAL;
    }
    | mmecvalue ',' mmeclist
    | mmecvalue
    ;

mmecvalue: INTEGER
    {
        if (mme_config_p->gummei.nb_mmec == 256) {
            yyerror(&yylloc, mme_config_p, "Reached maximum number of mmec");
            return EINVAL;
        }
        if ($1 >= 256) {
            yyerror(&yylloc, mme_config_p, "Value exceed limit <= 255");
            return EINVAL;
        }
        mme_config_p->gummei.nb_mmec++;
        mme_config_p->gummei.mmec = realloc(mme_config_p->gummei.mmec,
                                            mme_config_p->gummei.nb_mmec * sizeof(uint8_t));
        mme_config_p->gummei.mmec[mme_config_p->gummei.nb_mmec-1] = (uint8_t)$1;
    }
    ;

mmegid: MME_GID '=' mmegidlist ';'
    ;

mmegidlist:
    {
        yyerror(&yylloc, mme_config_p, "Empty value is not allowed");
        return EINVAL;
    }
    | mmegidvalue
    | mmegidvalue ',' mmegidlist
    ;

mmegidvalue: INTEGER
    {
        if (mme_config_p->gummei.nb_mme_gid == 65535) {
            yyerror(&yylloc, mme_config_p, "Reached maximum number of mmegid");
            return EINVAL;
        }
        if ($1 >= 65356) {
            yyerror(&yylloc, mme_config_p, "Value exceed limit <= 65535");
            return EINVAL;
        }
        mme_config_p->gummei.nb_mme_gid++;
        mme_config_p->gummei.mme_gid = realloc(mme_config_p->gummei.mme_gid,
                                               mme_config_p->gummei.nb_mme_gid * sizeof(uint16_t));
        mme_config_p->gummei.mme_gid[mme_config_p->gummei.nb_mme_gid-1] = (uint16_t)$1;
    }
    ;

realm: REALM '=' QSTRING ';'
    {
        mme_config_p->realm = $3;
        mme_config_p->realm_length = strlen(mme_config_p->realm);
    }
    ;

plmn: PLMN_Y '=' plmnlist ';'
    ;

plmnlist:
    {
        yyerror(&yylloc, mme_config_p, "Empty value is not allowed");
        return EINVAL;
    }
    | plmnvalue
    | plmnvalue ',' plmnlist
    ;

plmnvalue: INTEGER '.' INTEGER ':' INTEGER
    {
        if (mme_config_p->gummei.nb_plmns == 32) {
            yyerror(&yylloc, mme_config_p, "Reached maximum number of plmns");
            return EINVAL;
        }
        if ($1 > 999 || $3 > 999) {
            yyerror(&yylloc, mme_config_p, "PLMN value exceed limit <= 999");
            return EINVAL;
        }
        if ($5 > 65535) {
            yyerror(&yylloc, mme_config_p, "TAC value exceed limit <= 65535");
            return EINVAL;
        }
        mme_config_p->gummei.nb_plmns++;
        mme_config_p->gummei.plmn_mcc = realloc(mme_config_p->gummei.plmn_mcc,
                                                mme_config_p->gummei.nb_plmns * sizeof(uint16_t));
        mme_config_p->gummei.plmn_mnc = realloc(mme_config_p->gummei.plmn_mnc,
                                                mme_config_p->gummei.nb_plmns * sizeof(uint16_t));
        mme_config_p->gummei.plmn_tac = realloc(mme_config_p->gummei.plmn_tac,
                                                mme_config_p->gummei.nb_plmns * sizeof(uint16_t));
        mme_config_p->gummei.plmn_mcc[mme_config_p->gummei.nb_plmns-1] = (uint16_t)$1;
        mme_config_p->gummei.plmn_mnc[mme_config_p->gummei.nb_plmns-1] = (uint16_t)$3;
        mme_config_p->gummei.plmn_tac[mme_config_p->gummei.nb_plmns-1] = (uint16_t)$5;
    }
    ;

relative_capacity: RELATIVE_CAP '=' INTEGER ';'
    {
        if ($3 >= 256 || $3 < 0) {
            yyerror(&yylloc, mme_config_p, "Value exceed limit <= 255");
            return EINVAL;
        }
        mme_config_p->relative_capacity = $3;
    }
    ;

itti_queue_size: ITTI_QUEUE_SIZE '=' INTEGER ';'
    {
        if ($3 <= 0) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        }
        mme_config_p->itti_config.queue_size = (uint32_t)$3;
    }
    ;

sctp_instreams: SCTP_INSTREAMS '=' INTEGER ';'
    {
        if ($3 <= 0) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        } else if ($3 > UINT16_MAX) {
            yyerror(&yylloc, mme_config_p, "Value exceed container");
            return EINVAL;
        }
        mme_config_p->sctp_config.in_streams = (uint16_t)$3;
    }
    ;

sctp_outstreams: SCTP_OUTSTREAMS '=' INTEGER ';'
    {
        if ($3 <= 0) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        } else if ($3 > UINT16_MAX) {
            yyerror(&yylloc, mme_config_p, "Value exceed container");
            return EINVAL;
        }
        mme_config_p->sctp_config.out_streams = (uint16_t)$3;
    }
    ;

sgw_interface_name_for_S11: SGW_INTERFACE_NAME_FOR_S11 '=' QSTRING ';'
    {
        mme_config_p->ipv4.sgw_interface_name_for_S11 = $3;
    }
    ;

sgw_ip_address_for_S11: SGW_IP_ADDRESS_FOR_S11 '=' QSTRING ';'
    {
        struct in_addr inp;
        if ( inet_aton($3, &inp ) < 0 ) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        } else {
            mme_config_p->ipv4.sgw_ip_address_for_S11 = inp.s_addr;
        }
    }
    ;

sgw_ip_netmask_for_S11: SGW_IP_NETMASK_FOR_S11 '=' INTEGER ';'
    {
        if ($3 <= 0) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        } else if ($3 > UINT16_MAX) {
            yyerror(&yylloc, mme_config_p, "Value exceed container");
            return EINVAL;
        }
        mme_config_p->ipv4.sgw_ip_netmask_for_S11 = (uint16_t)$3;
    }
    ;

sgw_interface_name_for_S1u_S12_S4_up: SGW_INTERFACE_NAME_FOR_S1U_S12_S4_UP '=' QSTRING ';'
    {
        mme_config_p->ipv4.sgw_interface_name_for_S1u_S12_S4_up = $3;
    }
    ;

sgw_ip_address_for_S1u_S12_S4_up: SGW_IP_ADDRESS_FOR_S1U_S12_S4_UP '=' QSTRING ';'
    {
        struct in_addr inp;
        if ( inet_aton($3, &inp ) < 0 ) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        } else {
            mme_config_p->ipv4.sgw_ip_address_for_S1u_S12_S4_up = inp.s_addr;
        }
    }
    ;

sgw_ip_netmask_for_S1u_S12_S4_up: SGW_IP_NETMASK_FOR_S1U_S12_S4_UP '=' INTEGER ';'
    {
        if ($3 <= 0) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        } else if ($3 > UINT16_MAX) {
            yyerror(&yylloc, mme_config_p, "Value exceed container");
            return EINVAL;
        }
        mme_config_p->ipv4.sgw_ip_netmask_for_S1u_S12_S4_up = (uint16_t)$3;
    }
    ;

sgw_interface_name_for_S5_S8_up: SGW_INTERFACE_NAME_FOR_S5_S8_UP '=' QSTRING ';'
    {
        mme_config_p->ipv4.sgw_interface_name_for_S5_S8_up = $3;
    }
    ;

sgw_ip_address_for_S5_S8_up: SGW_IP_ADDRESS_FOR_S5_S8_UP '=' QSTRING ';'
    {
        struct in_addr inp;
        if ( inet_aton($3, &inp ) < 0 ) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        } else {
            mme_config_p->ipv4.sgw_ip_address_for_S5_S8_up = inp.s_addr;
        }
    }
    ;

sgw_ip_netmask_for_S5_S8_up: SGW_IP_NETMASK_FOR_S5_S8_UP '=' INTEGER ';'
    {
        if ($3 <= 0) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        } else if ($3 > UINT16_MAX) {
            yyerror(&yylloc, mme_config_p, "Value exceed container");
            return EINVAL;
        }
        mme_config_p->ipv4.sgw_ip_netmask_for_S5_S8_up = (uint16_t)$3;
    }
    ;

pgw_interface_name_for_S5_S8: PGW_INTERFACE_NAME_FOR_S5_S8 '=' QSTRING ';'
    {
        mme_config_p->ipv4.pgw_interface_name_for_S5_S8 = $3;
    }
    ;

pgw_ip_address_for_S5_S8: PGW_IP_ADDRESS_FOR_S5_S8 '=' QSTRING ';'
    {
        struct in_addr inp;
        if ( inet_aton($3, &inp ) < 0 ) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        } else {
            mme_config_p->ipv4.pgw_ip_address_for_S5_S8 = inp.s_addr;
        }
    }
    ;

pgw_ip_netmask_for_S5_S8: PGW_IP_NETMASK_FOR_S5_S8 '=' INTEGER ';'
    {
        if ($3 <= 0) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        } else if ($3 > UINT16_MAX) {
            yyerror(&yylloc, mme_config_p, "Value exceed container");
            return EINVAL;
        }
        mme_config_p->ipv4.pgw_ip_netmask_for_S5_S8 = (uint16_t)$3;
    }
    ;

pgw_interface_name_for_SGI: PGW_INTERFACE_NAME_FOR_SGI '=' QSTRING ';'
    {
        mme_config_p->ipv4.pgw_interface_name_for_SGI = $3;
    }
    ;

pgw_ip_addr_for_SGI: PGW_IP_ADDR_FOR_SGI '=' QSTRING ';'
    {
        struct in_addr inp;
        if ( inet_aton($3, &inp ) < 0 ) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        } else {
            mme_config_p->ipv4.pgw_ip_addr_for_SGI = inp.s_addr;
        }
    }
    ;

pgw_ip_netmask_for_SGI: PGW_IP_NETMASK_FOR_SGI '=' INTEGER ';'
    {
        if ($3 <= 0) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        } else if ($3 > UINT16_MAX) {
            yyerror(&yylloc, mme_config_p, "Value exceed container");
            return EINVAL;
        }
        mme_config_p->ipv4.pgw_ip_netmask_for_SGI = (uint16_t)$3;
    }
    ;

mme_interface_name_for_S1_MME: MME_INTERFACE_NAME_FOR_S1_MME '=' QSTRING ';'
    {
        mme_config_p->ipv4.mme_interface_name_for_S1_MME = $3;
    }
    ;

mme_ip_address_for_S1_MME: MME_IP_ADDRESS_FOR_S1_MME '=' QSTRING ';'
    {
        struct in_addr inp;
        if ( inet_aton($3, &inp ) < 0 ) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        } else {
            mme_config_p->ipv4.mme_ip_address_for_S1_MME = inp.s_addr;
        }
    }
    ;

mme_ip_netmask_for_S1_MME: MME_IP_NETMASK_FOR_S1_MME '=' INTEGER ';'
    {
        if ($3 <= 0) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        } else if ($3 > UINT16_MAX) {
            yyerror(&yylloc, mme_config_p, "Value exceed container");
            return EINVAL;
        }
        mme_config_p->ipv4.mme_ip_netmask_for_S1_MME = (uint16_t)$3;
    }
    ;

mme_interface_name_for_S11_MME: MME_INTERFACE_NAME_FOR_S11_MME '=' QSTRING ';'
    {
        mme_config_p->ipv4.mme_interface_name_for_S11 = $3;
    }
    ;

mme_ip_address_for_S11_MME: MME_IP_ADDRESS_FOR_S11_MME '=' QSTRING ';'
    {
        struct in_addr inp;
        if ( inet_aton($3, &inp ) < 0 ) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        } else {
            mme_config_p->ipv4.mme_ip_address_for_S11 = inp.s_addr;
        }
    }
    ;

mme_ip_netmask_for_S11_MME: MME_IP_NETMASK_FOR_S11_MME '=' INTEGER ';'
    {
        if ($3 <= 0) {
            yyerror(&yylloc, mme_config_p, "Invalid value");
            return EINVAL;
        } else if ($3 > UINT16_MAX) {
            yyerror(&yylloc, mme_config_p, "Value exceed container");
            return EINVAL;
        }
        mme_config_p->ipv4.mme_ip_netmask_for_S11 = (uint16_t)$3;
    }
    ;


    /* Lexical or syntax error */
errors: LEX_ERROR
    | error
    ;
%%

void yyerror(YYLTYPE *llocp, struct mme_config_s *mme_config_p, const char *str)
{
    fprintf(stderr, "Error in %s ( on line %i column %i -> line %i column %i) : %s\n",
            mme_config_p->config_file, llocp->first_line, llocp->first_column,
            llocp->last_line, llocp->last_column, str);
}
