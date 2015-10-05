/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <inttypes.h>

#include <mysql/mysql.h>

#include "hss_config.h"
#include "db_proto.h"

extern void ComputeOPc( const uint8_t const kP[16], const uint8_t const opP[16], uint8_t opcP[16] );


database_t *db_desc;

static void print_buffer(const char *prefix, uint8_t *buffer, int length)
{
  int i;

  fprintf(stdout, "%s", prefix);

  for (i = 0; i < length; i++) {
    fprintf(stdout, "%02x.", buffer[i]);
  }

  fprintf(stdout, "\n");
}

int hss_mysql_connect(const hss_config_t *hss_config_p)
{
  const int mysql_reconnect_val = 1;

  if ((hss_config_p->mysql_server == NULL) ||
      (hss_config_p->mysql_user == NULL) ||
      (hss_config_p->mysql_password == NULL) ||
      (hss_config_p->mysql_database == NULL)) {
    DB_ERROR("An empty name is not allowed\n");
    return EINVAL;
  }

  DB_DEBUG("Initializing db layer\n");

  db_desc = malloc(sizeof(database_t));

  if (db_desc == NULL) {
    DB_DEBUG("An error occured on MALLOC\n");
    return errno;
  }

  pthread_mutex_init(&db_desc->db_cs_mutex, NULL);

  /* Copy database configuration from static hss config */
  db_desc->server   = strdup(hss_config_p->mysql_server);
  db_desc->user     = strdup(hss_config_p->mysql_user);
  db_desc->password = strdup(hss_config_p->mysql_password);
  db_desc->database = strdup(hss_config_p->mysql_database);

  /* Init mySQL client */
  db_desc->db_conn = mysql_init(NULL);

  mysql_options(db_desc->db_conn, MYSQL_OPT_RECONNECT, &mysql_reconnect_val);

  /* Try to connect to database */
  if (!mysql_real_connect(db_desc->db_conn, db_desc->server, db_desc->user,
                          db_desc->password, db_desc->database, 0, NULL, 0)) {
    DB_ERROR("An error occured while connecting to db: %s\n",
             mysql_error(db_desc->db_conn));
    return -1;
  }

  /* Set the multi statement ON */
  mysql_set_server_option(db_desc->db_conn, MYSQL_OPTION_MULTI_STATEMENTS_ON);

  DB_DEBUG("Initializing db layer: DONE\n");

  return 0;
}

void hss_mysql_disconnect(void)
{
  mysql_close(db_desc->db_conn);
}

int hss_mysql_update_loc(const char *imsi, mysql_ul_ans_t *mysql_ul_ans)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  char query[1000];
  int ret = 0;

  if ((db_desc->db_conn == NULL) || (mysql_ul_ans == NULL)) {
    return EINVAL;
  }

  if (strlen(imsi) > 15) {
    return EINVAL;
  }

  sprintf(query, "SELECT `access_restriction`,`mmeidentity_idmmeidentity`,"
          "`msisdn`,`ue_ambr_ul`,`ue_ambr_dl`,`rau_tau_timer` "
          "FROM `users` WHERE `users`.`imsi`='%s' ", imsi);

  memcpy(mysql_ul_ans->imsi, imsi, strlen(imsi) + 1);

  DB_DEBUG("Query: %s\n", query);

  pthread_mutex_lock(&db_desc->db_cs_mutex);

  if (mysql_query(db_desc->db_conn, query)) {
    pthread_mutex_unlock(&db_desc->db_cs_mutex);
    DB_ERROR("Query execution failed: %s\n",
             mysql_error(db_desc->db_conn));
    mysql_thread_end();
    return EINVAL;
  }

  res = mysql_store_result(db_desc->db_conn);

  pthread_mutex_unlock(&db_desc->db_cs_mutex);

  if ((row = mysql_fetch_row(res)) != NULL) {
    int mme_id;
    /* MSISDN may be NULL */
    mysql_ul_ans->access_restriction = atoi(row[0]);

    if ((mme_id = atoi(row[1])) > 0) {
      ret = hss_mysql_query_mmeidentity(mme_id, &mysql_ul_ans->mme_identity);
    } else {
      mysql_ul_ans->mme_identity.mme_host[0] = '\0';
      mysql_ul_ans->mme_identity.mme_realm[0] = '\0';
    }

    if (row[2] != 0) {
      memcpy(mysql_ul_ans->msisdn, row[2], strlen(row[2]));
    }

    mysql_ul_ans->aggr_ul = atoi(row[3]);
    mysql_ul_ans->aggr_dl = atoi(row[4]);
    mysql_ul_ans->rau_tau = atoi(row[5]);

    mysql_free_result(res);
    mysql_thread_end();
    return ret;
  }

  mysql_free_result(res);
  mysql_thread_end();
  return EINVAL;
}

int hss_mysql_purge_ue(mysql_pu_req_t *mysql_pu_req,
                       mysql_pu_ans_t *mysql_pu_ans)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  char query[1000];
  int ret = 0;

  if ((db_desc->db_conn == NULL) ||
      (mysql_pu_req == NULL) ||
      (mysql_pu_ans == NULL)) {
    return EINVAL;
  }

  if (strlen(mysql_pu_req->imsi) > 15) {
    return EINVAL;
  }

  sprintf(query, "UPDATE `users` SET `users`.`ms_ps_status`=\"PURGED\" "
          "WHERE `users`.`imsi`='%s'; "
          "SELECT `users`.`mmeidentity_idmmeidentity` FROM `users` "
          "WHERE `users`.`imsi`=%s ",
          mysql_pu_req->imsi, mysql_pu_req->imsi);

  DB_DEBUG("Query: %s\n", query);

  pthread_mutex_lock(&db_desc->db_cs_mutex);

  if (mysql_query(db_desc->db_conn, query)) {
    pthread_mutex_unlock(&db_desc->db_cs_mutex);
    DB_ERROR("Query execution failed: %s\n",
             mysql_error(db_desc->db_conn));
    mysql_thread_end();
    return EINVAL;
  }

  res = mysql_store_result(db_desc->db_conn);

  pthread_mutex_unlock(&db_desc->db_cs_mutex);

  if ((row = mysql_fetch_row(res)) != NULL) {
    int mme_id;

    if ((mme_id = atoi(row[0])) > 0) {
      ret = hss_mysql_query_mmeidentity(mme_id, mysql_pu_ans);
    } else {
      mysql_pu_ans->mme_host[0] = '\0';
      mysql_pu_ans->mme_realm[0] = '\0';
    }

    mysql_free_result(res);
    mysql_thread_end();
    return ret;
  }

  mysql_free_result(res);
  mysql_thread_end();
  return EINVAL;
}

int hss_mysql_get_user(const char *imsi)
{
  MYSQL_RES *res;
  MYSQL_ROW row;
  char query[1000];

  if (db_desc->db_conn == NULL) {
    return EINVAL;
  }

  sprintf(query, "SELECT `imsi` FROM `users` WHERE `users`.`imsi`=%s ", imsi);

  DB_DEBUG("Query: %s\n", query);

  pthread_mutex_lock(&db_desc->db_cs_mutex);

  if (mysql_query(db_desc->db_conn, query)) {
    pthread_mutex_unlock(&db_desc->db_cs_mutex);
    DB_ERROR("Query execution failed: %s\n",
             mysql_error(db_desc->db_conn));
    mysql_thread_end();
    return EINVAL;
  }

  res = mysql_store_result(db_desc->db_conn);

  pthread_mutex_unlock(&db_desc->db_cs_mutex);

  if ((row = mysql_fetch_row(res)) != NULL) {
    mysql_free_result(res);
    mysql_thread_end();
    return 0;
  }

  mysql_free_result(res);
  mysql_thread_end();

  return EINVAL;
}

int mysql_push_up_loc(mysql_ul_push_t *ul_push_p)
{
  MYSQL_RES *res;
  char query[1000];
  int query_length = 0;
  int status;

  if ((db_desc->db_conn == NULL) || (ul_push_p == NULL)) {
    return EINVAL;
  }

  // TODO: multi-statement check results

  if (ul_push_p->mme_identity_present == MME_IDENTITY_PRESENT) {
    query_length += sprintf(&query[query_length], "INSERT INTO `mmeidentity`"
                            " (`mmehost`,`mmerealm`) SELECT '%s','%s' FROM `mmeidentity` WHERE NOT"
                            " EXISTS (SELECT * FROM `mmeidentity` WHERE `mmehost`='%s'"
                            " AND `mmerealm`='%s') LIMIT 1;", ul_push_p->mme_identity.mme_host,
                            ul_push_p->mme_identity.mme_realm, ul_push_p->mme_identity.mme_host,
                            ul_push_p->mme_identity.mme_realm);
  }

  query_length += sprintf(&query[query_length], "UPDATE `users`%s SET ",
                          ul_push_p->mme_identity_present == MME_IDENTITY_PRESENT ?
                          ",`mmeidentity`" : "");

  if (ul_push_p->imei_present == IMEI_PRESENT) {
    query_length += sprintf(&query[query_length], "`imei`='%s',",
                            ul_push_p->imei);
  }

  if (ul_push_p->sv_present == SV_PRESENT) {
    query_length += sprintf(&query[query_length], "`imeisv`='%*s',", 2,
                            ul_push_p->software_version);
  }

  if (ul_push_p->mme_identity_present == MME_IDENTITY_PRESENT) {
    query_length += sprintf(&query[query_length],
                            "`users`.`mmeidentity_idmmeidentity`=`mmeidentity`.`idmmeidentity`, "
                            "`users`.`ms_ps_status`=\"NOT_PURGED\"");
  }

  query_length += sprintf(&query[query_length], " WHERE `users`.`imsi`='%s'",
                          ul_push_p->imsi);

  if (ul_push_p->mme_identity_present == MME_IDENTITY_PRESENT) {
    query_length += sprintf(&query[query_length],
                            " AND `mmeidentity`.`mmehost`='%s'"
                            " AND `mmeidentity`.`mmerealm`='%s'",
                            ul_push_p->mme_identity.mme_host, ul_push_p->mme_identity.mme_realm);
  }

  DB_DEBUG("Query: %s\n", query);

  pthread_mutex_lock(&db_desc->db_cs_mutex);

  if (mysql_query(db_desc->db_conn, query)) {
    pthread_mutex_unlock(&db_desc->db_cs_mutex);
    fprintf(stderr, "Query execution failed: %s\n",
            mysql_error(db_desc->db_conn));
    mysql_thread_end();
    return EINVAL;
  }

  /* process each statement result */
  do {
    /* did current statement return data? */
    res = mysql_store_result(db_desc->db_conn);

    if (res) {
      /* yes; process rows and free the result set */
      mysql_free_result(res);
    } else {      /* no result set or error */
      if (mysql_field_count(db_desc->db_conn) == 0) {
        DB_ERROR("%lld rows affected\n",
                 mysql_affected_rows(db_desc->db_conn));
      } else { /* some error occurred */
        DB_ERROR("Could not retrieve result set\n");
        break;
      }
    }

    /* more results? -1 = no, >0 = error, 0 = yes (keep looping) */
    if ((status = mysql_next_result(db_desc->db_conn)) > 0)
      DB_ERROR("Could not execute statement\n");
  } while (status == 0);

  pthread_mutex_unlock(&db_desc->db_cs_mutex);

  mysql_free_result(res);
  mysql_thread_end();

  return 0;
}

int hss_mysql_push_rand_sqn(const char *imsi, uint8_t *rand_p, uint8_t *sqn)
{
  int status = 0, i;
  MYSQL_RES *res;
  char query[1000];
  int query_length = 0;
  uint64_t sqn_decimal = 0;

  if (db_desc->db_conn == NULL) {
    return EINVAL;
  }

  if (rand_p == NULL || sqn == NULL) {
    return EINVAL;
  }

  sqn_decimal = ((uint64_t)sqn[0] << 40) | ((uint64_t)sqn[1] << 32) |
                ((uint64_t)sqn[2] << 24) | (sqn[3] << 16) |
                (sqn[4] << 8) | sqn[5];

  query_length = sprintf(query, "UPDATE `users` SET `rand`=UNHEX('");

  for (i = 0; i < RAND_LENGTH; i ++) {
    query_length += sprintf(&query[query_length], "%02x", rand_p[i]);
  }

  query_length += sprintf(&query[query_length], "'),`sqn`=%"PRIu64, sqn_decimal);
  query_length += sprintf(&query[query_length], " WHERE `users`.`imsi`='%s'", imsi);

  DB_DEBUG("Query: %s\n", query);

  pthread_mutex_lock(&db_desc->db_cs_mutex);

  if (mysql_query(db_desc->db_conn, query)) {
    pthread_mutex_unlock(&db_desc->db_cs_mutex);
    DB_ERROR("Query execution failed: %s\n",
             mysql_error(db_desc->db_conn));
    mysql_thread_end();
    return EINVAL;
  }

  /* process each statement result */
  do {
    /* did current statement return data? */
    res = mysql_store_result(db_desc->db_conn);

    if (res) {
      /* yes; process rows and free the result set */
      mysql_free_result(res);
    } else {      /* no result set or error */
      if (mysql_field_count(db_desc->db_conn) == 0) {
        DB_ERROR("%lld rows affected\n",
                 mysql_affected_rows(db_desc->db_conn));
      } else { /* some error occurred */
        DB_ERROR("Could not retrieve result set\n");
        break;
      }
    }

    /* more results? -1 = no, >0 = error, 0 = yes (keep looping) */
    if ((status = mysql_next_result(db_desc->db_conn)) > 0)
      DB_ERROR("Could not execute statement\n");
  } while (status == 0);

  pthread_mutex_unlock(&db_desc->db_cs_mutex);

  mysql_free_result(res);
  mysql_thread_end();

  return 0;
}

int hss_mysql_increment_sqn(const char *imsi)
{
  int status;
  MYSQL_RES *res;
  char query[1000];

  if (db_desc->db_conn == NULL) {
    return EINVAL;
  }

  if (imsi == NULL) {
    return EINVAL;
  }

  /* + 32 = 2 ^ sizeof(IND) (see 3GPP TS. 33.102) */
  sprintf(query, "UPDATE `users` SET `sqn` = `sqn` + 32 WHERE `users`.`imsi`=%s", imsi);

  DB_DEBUG("Query: %s\n", query);

  if (mysql_query(db_desc->db_conn, query)) {
    pthread_mutex_unlock(&db_desc->db_cs_mutex);
    DB_ERROR("Query execution failed: %s\n",
             mysql_error(db_desc->db_conn));
    mysql_thread_end();
    return EINVAL;
  }

  /* process each statement result */
  do {
    /* did current statement return data? */
    res = mysql_store_result(db_desc->db_conn);

    if (res) {
      /* yes; process rows and free the result set */
      mysql_free_result(res);
    } else {      /* no result set or error */
      if (mysql_field_count(db_desc->db_conn) == 0) {
        DB_ERROR("%lld rows affected\n",
                 mysql_affected_rows(db_desc->db_conn));
      } else { /* some error occurred */
        DB_ERROR("Could not retrieve result set\n");
        break;
      }
    }

    /* more results? -1 = no, >0 = error, 0 = yes (keep looping) */
    if ((status = mysql_next_result(db_desc->db_conn)) > 0)
      DB_ERROR("Could not execute statement\n");
  } while (status == 0);

  pthread_mutex_unlock(&db_desc->db_cs_mutex);

  mysql_free_result(res);
  mysql_thread_end();

  return 0;
}

int hss_mysql_auth_info(mysql_auth_info_req_t  *auth_info_req,
                        mysql_auth_info_resp_t *auth_info_resp)
{
  int ret = 0;
  MYSQL_RES *res;
  MYSQL_ROW row;
  char query[1000];

  if (db_desc->db_conn == NULL) {
    return EINVAL;
  }

  if ((auth_info_req == NULL) || (auth_info_resp == NULL)) {
    return EINVAL;
  }

  sprintf(query, "SELECT `key`,`sqn`,`rand`,`OPc` FROM `users` WHERE `users`.`imsi`=%s ",
          auth_info_req->imsi);

  DB_DEBUG("Query: %s\n", query);

  pthread_mutex_lock(&db_desc->db_cs_mutex);

  if (mysql_query(db_desc->db_conn, query)) {
    pthread_mutex_unlock(&db_desc->db_cs_mutex);
    DB_ERROR("Query execution failed: %s\n",
             mysql_error(db_desc->db_conn));
    mysql_thread_end();
    return EINVAL;
  }

  res = mysql_store_result(db_desc->db_conn);

  pthread_mutex_unlock(&db_desc->db_cs_mutex);

  if ((row = mysql_fetch_row(res)) != NULL) {
    if (row[0] == NULL || row[1] == NULL || row[2] == NULL || row[3] == NULL) {
      ret = EINVAL;
    }

    if (row[0] != NULL) {
      print_buffer("Key: ", (uint8_t*)row[0], KEY_LENGTH);
      memcpy(auth_info_resp->key, row[0], KEY_LENGTH);
    }

    if (row[1] != NULL) {
      uint64_t sqn = 0;

      sqn = atoll(row[1]);
      printf("Received SQN %s converted to %"PRIu64"\n", row[1], sqn);

      auth_info_resp->sqn[0] = (sqn & (255UL << 40)) >> 40;
      auth_info_resp->sqn[1] = (sqn & (255UL << 32)) >> 32;
      auth_info_resp->sqn[2] = (sqn & (255UL << 24)) >> 24;
      auth_info_resp->sqn[3] = (sqn & (255UL << 16)) >> 16;
      auth_info_resp->sqn[4] = (sqn & (255UL << 8)) >> 8;
      auth_info_resp->sqn[5] = (sqn & 0xFF);

      print_buffer("SQN: ", auth_info_resp->sqn, SQN_LENGTH);
    }

    if (row[2] != NULL) {
      print_buffer("RAND: ", (uint8_t*)row[2], RAND_LENGTH);
      memcpy(auth_info_resp->rand, row[2], RAND_LENGTH);
    }

    if (row[3] != NULL) {
        print_buffer("OPc: ", (uint8_t*)row[3], KEY_LENGTH);
        memcpy(auth_info_resp->opc, row[3], KEY_LENGTH);
    }

    mysql_free_result(res);
    mysql_thread_end();
    return ret;
  }

  mysql_free_result(res);
  mysql_thread_end();

  return EINVAL;
}

int hss_mysql_check_opc_keys(const uint8_t const opP[16])
{
  int        ret   = 0;
  MYSQL_RES *res   = NULL;
  MYSQL_RES *res2  = NULL;
  MYSQL_ROW  row;
  char       query[1000];
  char       update[1000];
  uint8_t    k[16];
  uint8_t    opc[16];
  int        update_length = 0;
  int        status        = 0;
  int        i;

  if (db_desc->db_conn == NULL) {
	return EINVAL;
  }

  sprintf(query, "SELECT `imsi`,`key`,`OPc` FROM `users` ");

  DB_DEBUG("Query: %s\n", query);

  pthread_mutex_lock(&db_desc->db_cs_mutex);

  if (mysql_query(db_desc->db_conn, query)) {
	pthread_mutex_unlock(&db_desc->db_cs_mutex);
	DB_ERROR("Query execution failed: %s\n",
			 mysql_error(db_desc->db_conn));
	mysql_thread_end();
	return EINVAL;
  }

  res = mysql_store_result(db_desc->db_conn);

  pthread_mutex_unlock(&db_desc->db_cs_mutex);


  while ((row = mysql_fetch_row(res))) {
    if (row[0] == NULL || row[1] == NULL ) {
      DB_ERROR("Query execution failed: %s\n",
    				 mysql_error(db_desc->db_conn));
      ret = EINVAL;
    } else {
	  if (row[0] != NULL) {
	    printf("IMSI: %s", (uint8_t*)row[0]);
	  }
	  if (row[1] != NULL) {
	    print_buffer("Key: ", (uint8_t*)row[1], KEY_LENGTH);
        memcpy(k, row[1], KEY_LENGTH);
      }
	  //if (row[3] != NULL)
	  {
	    print_buffer("OPc: ", (uint8_t*)row[2], KEY_LENGTH);
	  //} else {
	    ComputeOPc( k, opP, opc);
	    update_length = sprintf(update, "UPDATE `users` SET `OPc`=UNHEX('");
	    for (i = 0; i < KEY_LENGTH; i ++) {
	    	update_length += sprintf(&update[update_length], "%02x", opc[i]);
	    }
	    update_length += sprintf(&update[update_length], "') WHERE `users`.`imsi`='%s'", (uint8_t*)row[0]);
	    DB_DEBUG("Query: %s\n", update);

	    if (mysql_query(db_desc->db_conn, update)) {
	      DB_ERROR("Query execution failed: %s\n",
	               mysql_error(db_desc->db_conn));
	    } else {
	        printf("IMSI %s Updated OPc ", (uint8_t*)row[0]);
		    for (i = 0; i < KEY_LENGTH; i ++) {
		    	printf("%02x", (uint8_t)(row[2][i]));
		    }
	        printf(" -> ");
		    for (i = 0; i < KEY_LENGTH; i ++) {
		    	printf("%02x", opc[i]);
		    }
	        printf("\n");

	      /* process each statement result */
	      do {
	        /* did current statement return data? */
	        res2 = mysql_store_result(db_desc->db_conn);

	        if (res2) {
	          /* yes; process rows and free the result set */
	          mysql_free_result(res2);
	        } else {      /* no result set or error */
	          if (mysql_field_count(db_desc->db_conn) == 0) {
	            DB_ERROR("%lld rows affected\n",
	                   mysql_affected_rows(db_desc->db_conn));
	          } else { /* some error occurred */
	            DB_ERROR("Could not retrieve result set\n");
	            break;
	          }
	        }

	        /* more results? -1 = no, >0 = error, 0 = yes (keep looping) */
	        if ((status = mysql_next_result(db_desc->db_conn)) > 0)
	          DB_ERROR("Could not execute statement\n");
	      } while (status == 0);
	    }
	  }
    }
  }

  mysql_free_result(res);
  mysql_thread_end();

  return ret;

}


