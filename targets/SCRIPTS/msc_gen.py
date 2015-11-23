#!/usr/bin/python
# -*- coding: utf-8 -*-

# The aim of this script is to collect some traces from oai stack and generate a sequence diagram image (png or jpeg).
#
# It is supposed that a protocol name (MSC_NEW_STR) starts with...its name (RRC, MAC, NAS, S1AP, etc) then is followed by an underscore and whatever (RRC_UE,  RRC_eNB)
#   Like this it is possible to distinguish between PDUS, SDUs or other messages only by reading source ans destination

import sys
import subprocess
import re
import socket
import datetime
from datetime import date
import os, errno
import argparse




parser = argparse.ArgumentParser()
parser.add_argument("--diag_rlc_um", "-u", type=str,help="Try to find RLC protocol diagnostics", default="no")
parser.add_argument("--dir", "-d", type=str,help="Directory where msc logs can be found", default="/tmp")
parser.add_argument("--profile", "-p", type=str,help="E_UTRAN, EPC", default="EPC")
parser.add_argument("--no_message", "-M", type=str,help="Trace PDUs, not messages, SDUs", default="no")
parser.add_argument("--no_pdu", "-P", type=str,help="Trace messages, SDUs, not PDUs", default="no")
parser.add_argument("--no_event", "-E", type=str,help="Do not trace events", default="no")
args = parser.parse_args()

MSCGEN_OUTPUT_TYPE       = "png"
MAX_MESSAGES_PER_PAGE    = 36

MSC_NEW_STR              = '[PROTO]'
MSC_MSG_STR              = '[MESSAGE]'
MSC_BOX_STR              = '[EVENT]'


# This list is filled as follow : g_proto_names[module_id_int] = (proto_name), example RRC_UE
g_proto_names = []
# This list is filled as follow : g_proto_names[module_id_int] = (basename proto_name) example: RRC
g_proto_types = []



# list of messages
g_messages         = {}


# Display color of messages of sending entities
g_display_color  = ['\"teal\"',   # To check in msc.h: MSC_NAS_UE
                    '\"red\"',    # To check in msc.h: MSC_RRC_UE
                    '\"red\"',    # To check in msc.h: MSC_NAS_UE
                    '\"red\"',    # To check in msc.h: MSC_PDCP_UE
                    '\"red\"',    # To check in msc.h: MSC_RLC_UE
                    '\"red\"',    # To check in msc.h: MSC_MAC_UE
                    '\"red\"',    # To check in msc.h: MSC_PHY_UE
                    '\"indigo\"', # To check in msc.h: MSC_PHY_ENB
                    '\"indigo\"', # To check in msc.h: MSC_MAC_ENB
                    '\"indigo\"', # To check in msc.h: MSC_RLC_ENB
                    '\"indigo\"', # To check in msc.h: MSC_PDCP_ENB
                    '\"orange\"', # To check in msc.h: MSC_RRC_ENB
                    '\"black\"',  # To check in msc.h: MSC_IP_ENB
                    '\"black\"',  # To check in msc.h: MSC_S1AP_ENB
                    '\"black\"',  # To check in msc.h: MSC_GTPU_ENB
                    '\"black\"',  # To check in msc.h: MSC_GTPU_SGW
                    '\"black\"',  # To check in msc.h: MSC_S1AP_MME
                    '\"black\"',  # To check in msc.h: MSC_MMEAPP_MME
                    '\"black\"',  # To check in msc.h: MSC_NAS_MME
                    '\"black\"',  # To check in msc.h: MSC_NAS_EMM_MME
                    '\"black\"',  # To check in msc.h: MSC_NAS_ESM_MME
                    '\"black\"',  
                    '\"black\"',  
                    '\"black\"',  
                    '\"black\"',  
                    '\"black\"',  
                    '\"black\"',  
                    '\"black\"',  
                    '\"black\"',  
                    '\"black\"',  
                    '\"black\"',  
                    '\"black\"',  
                    '\"black\"'] 


# helper for diagnostic of RLC
g_diag_rlc_sn = {}

g_sequence_generator = 0


g_filenames = []
if "E_UTRAN" == args.profile.strip():
    g_filenames = [    
        args.dir+'/openair.msc.0.log',
        args.dir+'/openair.msc.1.log']
elif "EPC" == args.profile.strip():
    g_filenames = [        
        args.dir+'/openair.msc.2.log',
        args.dir+'/openair.msc.3.log',
        args.dir+'/openair.msc.4.log']

def sequence_number_generator():
    global g_sequence_generator
    l_seq = g_sequence_generator
    g_sequence_generator = g_sequence_generator + 1
    return l_seq

def file_is_empty(fpath):  
    return False if os.path.isfile(fpath) and os.path.getsize(fpath) > 0 else True

def parse_oai_log_files():
    global g_entities_dic
    global g_entities
    global g_messages
    global g_final_display_order_list
    #open TXT file that contain OAI filtered traces for mscgen

    # we may insert diagnostic events
    event_id_offset = 0
    for filename in g_filenames:
        if file_is_empty(filename):
            continue
        try:
            fhandle  = open(filename, 'r')
            fcontent = fhandle.read()
            fhandle.close()

            # split file content in lines
            lines = fcontent.splitlines()
            for line in lines:
                print ("INPUT LINE:  %s " % line)
                if line.strip() != ""  and not line.strip().startswith('#'):
                    partition = line.split(' ',3)
                    event_id = int(partition[0]) + event_id_offset
                    event_type = partition[1]
                    entity_id = int(partition[2])
                    if MSC_NEW_STR == event_type:
                        entity_name = partition[3]
                        if len(g_proto_names) <= entity_id:
                            for i in range(len(g_proto_names),(entity_id +1)):
                                g_proto_names.append("NotDeclared")
                                g_proto_types.append("NotDeclared")
                        g_proto_names[entity_id] = entity_name
                        partition_name1 = entity_name.split('_',1);
                        partition_name2 = partition_name1[0].split('-',1);
                        partition_name3 = partition_name2[0].split();
                        g_proto_types[entity_id] = partition_name3
                        print ("entity name %s , entity type %s" % (g_proto_names[entity_id],g_proto_types[entity_id]) )
                    # if line is a trace of a message between 2 protocol entities or layers
                    elif MSC_MSG_STR == event_type:
                        #print ("partition[3]:%s" % partition[3])
                        sub_partition = partition[3].split(' ',4)
                        arrow   = sub_partition[0]
                        entity2_id = int(sub_partition[1])
                        mac     = int(sub_partition[2])
                        time    = sub_partition[3]
                        message = sub_partition[4]
                        Message = {}
                        Message['mac'] = mac
                        Message['time'] = time
                        Message['message'] = message
                        Message['line_color'] = g_display_color[entity_id]
                        Message['text_color'] = g_display_color[entity_id]
                        if arrow == '<-':
                            if "yes" == args.diag_rlc_um.strip() and "DATA SN " in message and "RB UM " in message:
                                rlc_key = re.match(r"[^[]*\[([^]]*)\]", message).groups()[0]
                                sn_info = message.partition("SN")[2]
                                if sn_info.strip() != "":
                                    sn_str = sn_info.strip().partition(" ")[0]                                
                                    sn = int(sn_str)
                                    if rlc_key in g_diag_rlc_sn:
                                        previous_sn = g_diag_rlc_sn[rlc_key]
                                        if (previous_sn + 1) % 1024 != sn:
                                            print ("DIAG missing SN:  %s " % line)  
                                            MessageDiag = {}
                                            MessageDiag['type'] = "box"
                                            MessageDiag['tx'] = entity_id
                                            MessageDiag['rx'] = entity_id
                                            MessageDiag['discarded'] = False
                                            MessageDiag['time'] = time
                                            MessageDiag['message'] = "Missing SN "+ sn_str + " detected"
                                            MessageDiag['line_color'] = '\"red\"'
                                            MessageDiag['text_color'] = '\"red\"'
                                            g_messages[event_id + event_id_offset] = MessageDiag
                                            event_id_offset = event_id_offset + 1                       
                                    g_diag_rlc_sn[rlc_key] = int(sn)
                            Message['type'] = "rx"
                            Message['tx'] = entity2_id
                            Message['rx'] = entity_id
                            Message['discarded'] = False
                            g_messages[event_id + event_id_offset] = Message

                        elif arrow == '->':
                            Message['type'] = "tx"
                            Message['tx'] = entity_id
                            Message['rx'] = entity2_id
                            Message['discarded'] = False
                            g_messages[event_id + event_id_offset] = Message
                        elif arrow == 'x-':
                            Message['type'] = "rx"
                            Message['tx'] = entity2_id
                            Message['rx'] = entity_id
                            Message['discarded'] = True
                            g_messages[event_id + event_id_offset] = Message
                        elif arrow == '-x':
                            Message['type'] = "tx"
                            Message['tx'] = entity_id
                            Message['rx'] = entity2_id
                            Message['discarded'] = True
                            g_messages[event_id + event_id_offset] = Message

                    elif MSC_BOX_STR == event_type:
                        sub_partition = partition[3].split(' ',1)
                        time    = sub_partition[0]
                        message = sub_partition[1]
                        Message = {}
                        Message['type'] = "box"
                        Message['tx'] = entity_id
                        Message['rx'] = entity_id
                        Message['discarded'] = False
                        Message['time'] = time
                        Message['message'] = message
                        Message['line_color'] = g_display_color[entity_id]
                        Message['text_color'] = g_display_color[entity_id]
                        g_messages[event_id + event_id_offset] = Message

        except IOError, e:  
            print ("File %s INPUT LINE:  %s " % (filename, line))
            print 'err message'

    #print("------------------------------------")
    #print ("  %s " % ( g_messages ) )

#    for event_id_int in sorted(g_messages.iterkeys()):



def msc_chart_write_header(fileP):
    global g_final_display_order_list
    fileP.write("msc {\n")
    fileP.write("width = \"2048\";\n")

    entity_line_list_str = ''
    #print ("  %s " % ( g_proto_names ) )
    for entity in g_proto_names:
        if entity != 'NotDeclared':
            entity_line_list_str = entity_line_list_str + ' ' + entity + ','

    entity_line_list_str = entity_line_list_str.rstrip().strip(',')
    fileP.write("  %s;\n" % (entity_line_list_str))


def msc_chart_write_footer(fileP):
    fileP.write("\n}\n")

def msc_chart_generate(file_nameP):
    global  MSCGEN_OUTPUT_TYPE
    print "Generating sequence diagram for ",file_nameP
    command_line = "mscgen -T " + MSCGEN_OUTPUT_TYPE + " -i " + file_nameP
    fi,fo,fe=os.popen3(command_line)
    for i in fe.readlines():
        print "error:",i

def get_new_file_descriptor():
    global g_base_file_name
    global g_page_index
    l_file_name = g_base_file_name + str(g_page_index)+'.txt'
    l_file = open(l_file_name, "wb")
    print "Generating mscgen input file ",l_file_name;
    return l_file


###### MAIN STAR HERE #################
parse_oai_log_files()

g_page_index    = 0
g_message_index = 0
g_now = datetime.datetime.now()
g_now_formated = 'mscgen_' + args.profile.strip() + '_'+ g_now.strftime("%Y-%m-%d_%H.%M.%S")
#g_currentdir = os.curdir
g_resultdir = os.path.join(args.dir, g_now_formated)
os.mkdir(g_resultdir)
os.chdir(g_resultdir)

g_base_file_name = args.profile.strip() + '_mscgen_page_'

g_file = get_new_file_descriptor()
msc_chart_write_header(g_file)

for event_id_int in sorted(g_messages.iterkeys()):
    message = g_messages[event_id_int]
    if 'tx' in message['type']:
        if "yes" == args.no_message.strip():
            if g_proto_types[message['tx']] != g_proto_types[message['rx']]:
                continue
        if "yes" == args.no_pdu.strip():
            if g_proto_types[message['tx']] == g_proto_types[message['rx']]:
                continue   
        g_file.write("  %s=>%s [ label = \"(%d|%s) %s\", linecolour=%s , textcolour=%s ] ;\n" % (g_proto_names[message['tx']], g_proto_names[message['rx']], event_id_int, message['time'], message['message'], message['line_color'], message['text_color']))

    elif 'rx' in message['type']:
        if "yes" == args.no_message.strip():
            if g_proto_types[message['tx']] != g_proto_types[message['rx']]:
                continue
        if "yes" == args.no_pdu.strip():
            if g_proto_types[message['tx']] == g_proto_types[message['rx']]:
                continue   
        g_file.write("  %s<=%s [ label = \"(%d|%s) %s\", linecolour=%s , textcolour=%s ] ;\n" % (g_proto_names[message['rx']], g_proto_names[message['tx']], event_id_int, message['time'], message['message'], message['line_color'], message['text_color']))

    elif 'box' in message['type']:
        if "yes" == args.no_event.strip():
            continue  
        g_file.write("  %s note %s [ label = \"%s\", textcolour=%s ] ;\n" % (g_proto_names[message['tx']], g_proto_names[message['rx']], message['message'], message['text_color']))

    g_message_index = g_message_index + 1

    if ((g_message_index % MAX_MESSAGES_PER_PAGE) == 0):
        msc_chart_write_footer(g_file)
        g_file.close()
        msc_chart_generate(g_file.name)
        g_page_index = g_page_index + 1

        g_file = get_new_file_descriptor()
        msc_chart_write_header(g_file)


msc_chart_write_footer(g_file)
g_file.close()
msc_chart_generate(g_file.name)
