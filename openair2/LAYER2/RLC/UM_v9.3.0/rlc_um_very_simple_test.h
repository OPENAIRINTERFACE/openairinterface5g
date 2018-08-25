/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file rlc_um_very_simple_test.h
* \brief This file defines the prototypes of the functions dealing with the sending of self generated packet for very basic test or debug of RLC or lower layers.
* \author GAUTHIER Lionel
* \date 2010-2011
* \version
* \note
* \bug
* \warning
*/
#    ifndef __RLC_UM_VERY_SIMPLE_TEST_H__
#        define __RLC_UM_VERY_SIMPLE_TEST_H__
//-----------------------------------------------------------------------------
#        include "rlc_um_entity.h"
#        include "mem_block.h"
#        include "rrm_config_structs.h"
#        include "rlc_um_structs.h"
#        include "rlc_um_constants.h"
//-----------------------------------------------------------------------------
#define RLC_UM_TEST_SDU_TYPE_TCPIP 0
#define RLC_UM_TEST_SDU_TYPE_VOIP  1
#define RLC_UM_TEST_SDU_TYPE_SMALL 2

#define tcip_sdu  "NOS TESTS MONTRENT QUE LE NOUVEAU TOSHIBA MK7559GSXP, UN DISQUE DUR DE 750 GO FONCTIONNANT AVEC DES SECTEURS DE 4 KO, EST AU MOINS AUSSI RAPIDE QUE SON PRÉDÉCESSEUR, LE MK6465GSX 640 GO, DANS TOUS LES BENCHMARKS THÉORIQUES. SES PERFORMANCES EN E/S ET SON TEMPS D’ACCÈS SONT COMPARABLES ET SON DÉBIT RÉEL EST MÊME NETTEMENT PLUS ÉLEVÉ. SES RÉSULTATS SOUS PCMARK VANTAGE, PAR CONTRE, SONT QUELQUE PEU MOINS BONS. DANS CE CAS, LEQUEL CHOISIR ? LES SCORES OBTENUS DANS LES TESTS THÉORIQUES NOUS CONFIRMENT QUE LE NOUVEAU MODÈLE SE COMPORTE CONVENABLEMENT « MALGRÉ » SES SECTEURS DE 4 KO ET QUE LA RAISON DU LÉGER RECUL DE SES PERFORMANCES SOUS PCMARK VANTAGE SE TROUVE AILLEURS. L’ALIGNEMENT DES SECTEURS N’EST PAS NON PLUS EN CAUSE, ÉTANT DONNÉ QUE WINDOWS VISTA (NOTRE OS DE TEST) ET WINDOWS 7 EN TIENNENT COMPTE LORS DE LA CRÉATION DES PARTITIONS — CE QUE NOUS AVONS BIEN ENTENDU VÉRIFIÉ INDÉPENDAMMENT.IL NOUS EST TOUTEFOIS IMPOSSIBLE DE CONTRÔLER L’EXÉCUTION ET L’ORGANISATION DE L’ÉCRITURE DES DONNÉES. PCMARK VANTAGE N’A EN EFFET JAMAIS ÉTÉ OPTIMISÉ POUR L’ÉCRITURE DE BLOCS DE DONNÉES DE GRANDE TAILLE ; DANS LA VIE RÉELLE, SI VOUS ÉCRIVEZ SURTOUT DE GROS FICHIERS, LE NOUVEAU DISQUE DUR DE 750 GO VA S’AVÉRER PLUS RAPIDE QUE LE 640 GO ET SURTOUT QUE LES RÉSULTATS ENREGISTRÉS DANS NOTRE BENCHMARK PCMARK, CAR SES SECTEURS DE 4 KO SERONT TOUJOURS PLUS PETITS QUE LES DONNÉES À ÉCRIRE. LE PROBLÈME EST QU’AUSSI LONGTEMPS QUE LES APPLICATIONS CONTINUERONT À EFFECTUER DES DEMANDES D’ÉCRITURE EN MODE  512 OCTETS"

#define voip_sdu  "Nos tests montrent que le nouveau Toshiba MK7559GSXP, un disque dur de 750 Go"
#define very_small_sdu "NoS tEsTs MoNtReNt"

void rlc_um_test_send_sdu (rlc_um_entity_t* rlcP,  uint32_t frame, unsigned int sdu_typeP);
#    endif
