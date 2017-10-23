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

/* This message asks for task initialization */
MESSAGE_DEF(INITIALIZE_MESSAGE, MESSAGE_PRIORITY_MED, IttiMsgEmpty, initialize_message)

/* This message asks for task activation */
MESSAGE_DEF(ACTIVATE_MESSAGE,   MESSAGE_PRIORITY_MED, IttiMsgEmpty, activate_message)

/* This message asks for task deactivation */
MESSAGE_DEF(DEACTIVATE_MESSAGE, MESSAGE_PRIORITY_MED, IttiMsgEmpty, deactivate_message)

/* This message asks for task termination */
MESSAGE_DEF(TERMINATE_MESSAGE,  MESSAGE_PRIORITY_MAX, IttiMsgEmpty, terminate_message)

/* Test message used for debug */
MESSAGE_DEF(MESSAGE_TEST,       MESSAGE_PRIORITY_MED, IttiMsgEmpty, message_test)

/* Error message  */
MESSAGE_DEF(ERROR_LOG,          MESSAGE_PRIORITY_MAX, IttiMsgEmpty, error_log)
/* Warning message  */
MESSAGE_DEF(WARNING_LOG,        MESSAGE_PRIORITY_MAX, IttiMsgEmpty, warning_log)
/* Notice message  */
MESSAGE_DEF(NOTICE_LOG,         MESSAGE_PRIORITY_MED, IttiMsgEmpty, notice_log)
/* Info message  */
MESSAGE_DEF(INFO_LOG,           MESSAGE_PRIORITY_MED, IttiMsgEmpty, info_log)
/* Debug message  */
MESSAGE_DEF(DEBUG_LOG,          MESSAGE_PRIORITY_MED, IttiMsgEmpty, debug_log)

/* Generic log message for text */
MESSAGE_DEF(GENERIC_LOG,        MESSAGE_PRIORITY_MED, IttiMsgEmpty, generic_log)
