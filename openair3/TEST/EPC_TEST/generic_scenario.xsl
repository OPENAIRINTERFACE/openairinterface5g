<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://www.w3.org/TR/xhtml1/strict">
                
<xsl:output
   method="xml"
   indent="yes"
   encoding="iso-8859-1"
/>

<!-- Ugly but no time to find a better way in XSLT 1.0 (map/list)-->
 <xsl:param name="enb_s1c0"   select="'0.0.0.0'"/>
 <xsl:param name="enb_s1c1"   select="'0.0.0.0'"/>
 <xsl:param name="enb_s1c2"   select="'0.0.0.0'"/>
 <xsl:param name="enb_s1c3"   select="'0.0.0.0'"/>
 <xsl:param name="enb_s1c4"   select="'0.0.0.0'"/>
 <xsl:param name="enb_s1c5"   select="'0.0.0.0'"/>
 <xsl:param name="enb_s1c6"   select="'0.0.0.0'"/>
 <xsl:param name="enb_s1c7"   select="'0.0.0.0'"/>
 <xsl:param name="mme_s1c0_0" select="'0.0.0.0'"/>
 <xsl:param name="mme_s1c0_1" select="'0.0.0.0'"/>
 <xsl:param name="mme_s1c0_2" select="'0.0.0.0'"/>
 <xsl:param name="mme_s1c0_3" select="'0.0.0.0'"/>
 <xsl:param name="mme_s1c1_0" select="'0.0.0.0'"/>
 <xsl:param name="mme_s1c1_1" select="'0.0.0.0'"/>
 <xsl:param name="mme_s1c1_2" select="'0.0.0.0'"/>
 <xsl:param name="mme_s1c1_3" select="'0.0.0.0'"/>
 <xsl:param name="mme_s1c2_0" select="'0.0.0.0'"/>
 <xsl:param name="mme_s1c2_1" select="'0.0.0.0'"/>
 <xsl:param name="mme_s1c2_2" select="'0.0.0.0'"/>
 <xsl:param name="mme_s1c2_3" select="'0.0.0.0'"/>
 <xsl:param name="mme_s1c3_0" select="'0.0.0.0'"/>
 <xsl:param name="mme_s1c3_1" select="'0.0.0.0'"/>
 <xsl:param name="mme_s1c3_2" select="'0.0.0.0'"/>
 <xsl:param name="mme_s1c3_3" select="'0.0.0.0'"/>
 <xsl:param name="ip_address" select="'0.0.0.0'"/>



<xsl:template name="reverse_ip">
  <xsl:param name="ip_address"/>
    <xsl:choose>
        <xsl:when test="$ip_address=$enb_s1c0">enb_s1c0</xsl:when>
        <xsl:when test="$ip_address=$enb_s1c1">enb_s1c1</xsl:when>
        <xsl:when test="$ip_address=$enb_s1c2">enb_s1c2</xsl:when>
        <xsl:when test="$ip_address=$enb_s1c3">enb_s1c3</xsl:when>
        <xsl:when test="$ip_address=$enb_s1c4">enb_s1c4</xsl:when>
        <xsl:when test="$ip_address=$enb_s1c5">enb_s1c5</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c0_0">mme_s1c0_0</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c0_1">mme_s1c0_1</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c0_2">mme_s1c0_2</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c0_3">mme_s1c0_3</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c1_0">mme_s1c1_0</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c1_1">mme_s1c1_1</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c1_2">mme_s1c1_2"</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c1_3">mme_s1c1_3</xsl:when>
        <xsl:otherwise>reverse_ip_yourself</xsl:otherwise>
    </xsl:choose> 
</xsl:template>

<xsl:template name="chunktype2str">
  <xsl:param name="chunk_type"/>
    <xsl:choose>
        <xsl:when test="$chunk_type='00'">DATA</xsl:when>
        <xsl:when test="$chunk_type='01'">INIT</xsl:when>
        <xsl:when test="$chunk_type='02'">INIT_ACK</xsl:when>
        <xsl:when test="$chunk_type='03'">SACK</xsl:when>
        <xsl:when test="$chunk_type='04'">HEARTBEAT</xsl:when>
        <xsl:when test="$chunk_type='05'">HEARTBEAT_ACK</xsl:when>
        <xsl:when test="$chunk_type='06'">ABORT</xsl:when>
        <xsl:when test="$chunk_type='07'">SHUTDOWN</xsl:when>
        <xsl:when test="$chunk_type='08'">SHUTDOWN_ACK</xsl:when>
        <xsl:when test="$chunk_type='09'">ERROR</xsl:when>
        <xsl:when test="$chunk_type='0a'">COOKIE_ECHO</xsl:when>
        <xsl:when test="$chunk_type='0b'">COOKIE_ACK</xsl:when>
        <xsl:when test="$chunk_type='0c'">ECNE</xsl:when>
        <xsl:when test="$chunk_type='0d'">CWR</xsl:when>
        <xsl:when test="$chunk_type='0e'">SHUTDOWN_COMPLETE</xsl:when>
        <xsl:otherwise>UNKNOWN_CHUNK_TYPE</xsl:otherwise>
    </xsl:choose> 
</xsl:template>




<xsl:strip-space elements="pdml packet proto field"/>

<xsl:template match="/">
    <scenario name="{$test_name}">
        <xsl:apply-templates/>
    </scenario>
</xsl:template>

<xsl:template match="proto[@name='frame']">
    <xsl:variable name="time_relative" select="field[@name='frame.time_relative']/@show"/>
    <xsl:variable name="ip" select="proto[@name='ip']"/>
    <xsl:variable name="ip.src">
        <xsl:call-template name="reverse_ip">
            <xsl:with-param name="ip_address" select="$ip/field[@name='ip.src']/@show"/>
        </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="ip.dst">
        <xsl:call-template name="reverse_ip">
            <xsl:with-param name="ip_address" select="$ip/field[@name='ip.dst']/@show"/>
        </xsl:call-template>
    </xsl:variable>
    
    
    <xsl:for-each select="$ip/proto[@name='sctp']">
        <xsl:variable name="sctp.data_sid"               select="./field[@name='sctp.data_sid']/@show"/>
        <xsl:variable name="sctp.srcport"                select="./field[@name='sctp.srcport']/@show"/>
        <xsl:variable name="sctp.dstport"                select="./field[@name='sctp.dstport']/@show"/>
        <xsl:variable name="sctp.data_ssn"               select="./field[@name='sctp.data_ssn']/@show"/>
        <xsl:variable name="sctp.data_payload_proto_id"  select="./field[@name='sctp.data_payload_proto_id']/@show"/>
        <xsl:variable name="sctp.chunk_type_str">
            <xsl:call-template name="chunktype2str">
                <xsl:with-param name="chunk_type" select="./field/field[@name='sctp.chunk_type']/@value"/>
            </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="sctp_pos_offset" select="./@pos"/>

        <xsl:choose>
            <xsl:when test="$sctp.chunk_type_str='DATA'">
        <xsl:for-each select="./proto[@name='s1ap']">
            <xsl:variable name="s1ap_pos_offset" select="./@pos"/>
            <payload name="{$sctp.chunk_type_str}">
               <frame.time_relative        value="{$time_relative}"/>
               
               <!-- TODO: pos_offset(substract it from all pos_offsets in s1ap, may depend on which test scenario protocol target S1AP/NAS or NAS only...)-->
               <pos_offset                 value="{$s1ap_pos_offset}"/>
               <ip.src                     value="{$ip.src}"/>
               <ip.dst                     value="{$ip.dst}"/>
               <sctp.data_sid              value="{$sctp.data_sid}"/>
               <sctp.srcport               value="{$sctp.srcport}"/>
               <sctp.dstport               value="{$sctp.dstport}"/>
               <sctp.data_ssn              value="{$sctp.data_ssn}"/>
               <sctp.data_payload_proto_id value="{$sctp.data_payload_proto_id}"/>
               <sctp.chunk_type_str        value="{$sctp.chunk_type_str}"/>
               <xsl:copy-of select="node()"/>
            </payload>
        </xsl:for-each>
            </xsl:when>
            <xsl:when test="$sctp.chunk_type_str='INIT'">
                <xsl:variable name="sctp.init_nr_out_streams"  select="./field/field[@name='sctp.init_nr_out_streams']/@show"/>
                <xsl:variable name="sctp.init_nr_in_streams"   select="./field/field[@name='sctp.init_nr_in_streams']/@show"/>
                <xsl:variable name="sctp.init_initial_tsn"     select="./field/field[@name='sctp.init_initial_tsn']/@show"/>
                <payload name="{$sctp.chunk_type_str}">
                   <frame.time_relative        value="{$time_relative}"/>
                   <!-- TODO: pos_offset(substract it from all pos_offsets in s1ap, may depend on which test scenario protocol target S1AP/NAS or NAS only...)-->
                   <pos_offset                 value="{$sctp_pos_offset}"/>
                   <ip.src                     value="{$ip.src}"/>
                   <ip.dst                     value="{$ip.dst}"/>
                   <sctp.data_sid              value="{$sctp.data_sid}"/>
                   <sctp.srcport               value="{$sctp.srcport}"/>
                   <sctp.dstport               value="{$sctp.dstport}"/>
                   <sctp.init_nr_in_streams    value="{$sctp.init_nr_in_streams}"/>
                   <sctp.init_nr_out_streams   value="{$sctp.init_nr_out_streams}"/>
                   <sctp.init_initial_tsn      value="{$sctp.init_initial_tsn}"/>
                   <sctp.chunk_type_str        value="{$sctp.chunk_type_str}"/>
                   <!--xsl:copy-of select="node()"/-->
                </payload>
            </xsl:when>
            <xsl:when test="$sctp.chunk_type_str='INIT_ACK'">
                <xsl:variable name="sctp.initack_nr_out_streams"  select="./field/field[@name='sctp.initack_nr_out_streams']/@show"/>
                <xsl:variable name="sctp.initack_nr_in_streams"   select="./field/field[@name='sctp.initack_nr_in_streams']/@show"/>
                <xsl:variable name="sctp.initack_initial_tsn"     select="./field/field[@name='sctp.initack_initial_tsn']/@show"/>
                <payload name="{$sctp.chunk_type_str}">
                   <frame.time_relative        value="{$time_relative}"/>
                   <!-- TODO: pos_offset(substract it from all pos_offsets in s1ap, may depend on which test scenario protocol target S1AP/NAS or NAS only...)-->
                   <pos_offset                 value="{$sctp_pos_offset}"/>
                   <ip.src                     value="{$ip.src}"/>
                   <ip.dst                     value="{$ip.dst}"/>
                   <sctp.data_sid              value="{$sctp.data_sid}"/>
                   <sctp.srcport               value="{$sctp.srcport}"/>
                   <sctp.dstport               value="{$sctp.dstport}"/>
                   <sctp.initack_nr_in_streams  value="{$sctp.initack_nr_in_streams}"/>
                   <sctp.initack_nr_out_streams value="{$sctp.initack_nr_out_streams}"/>
                   <sctp.initack_initial_tsn   value="{$sctp.initack_initial_tsn}"/>
                   <sctp.chunk_type_str        value="{$sctp.chunk_type_str}"/>
                   <!--xsl:copy-of select="node()"/-->
                </payload>
            </xsl:when>
            <!--xsl:when test="$sctp.chunk_type_str='SACK'">       </xsl:when-->
            <!--xsl:when test="$sctp.chunk_type_str='HEARTBEAT'"></xsl:when-->
            <!--xsl:when test="$sctp.chunk_type_str='HEARTBEAT_ACK'"></xsl:when-->
            <xsl:when test="$sctp.chunk_type_str='ABORT'">
                <payload name="{$sctp.chunk_type_str}">
                   <frame.time_relative        value="{$time_relative}"/>
                   <!-- TODO: pos_offset(substract it from all pos_offsets in s1ap, may depend on which test scenario protocol target S1AP/NAS or NAS only...)-->
                   <pos_offset                 value="{$sctp_pos_offset}"/>
                   <ip.src                     value="{$ip.src}"/>
                   <ip.dst                     value="{$ip.dst}"/>
                   <sctp.data_sid              value="{$sctp.data_sid}"/>
                   <sctp.srcport               value="{$sctp.srcport}"/>
                   <sctp.dstport               value="{$sctp.dstport}"/>
                   <sctp.chunk_type_str        value="{$sctp.chunk_type_str}"/>
                   <xsl:copy-of select="node()"/>
                </payload>
            </xsl:when>
            <xsl:when test="$sctp.chunk_type_str='SHUTDOWN'">
                <payload name="{$sctp.chunk_type_str}">
                   <frame.time_relative        value="{$time_relative}"/>
                   <!-- TODO: pos_offset(substract it from all pos_offsets in s1ap, may depend on which test scenario protocol target S1AP/NAS or NAS only...)-->
                   <pos_offset                 value="{$sctp_pos_offset}"/>
                   <ip.src                     value="{$ip.src}"/>
                   <ip.dst                     value="{$ip.dst}"/>
                   <sctp.data_sid              value="{$sctp.data_sid}"/>
                   <sctp.srcport               value="{$sctp.srcport}"/>
                   <sctp.dstport               value="{$sctp.dstport}"/>
                   <sctp.chunk_type_str        value="{$sctp.chunk_type_str}"/>
                   <xsl:copy-of select="node()"/>
                </payload>
            </xsl:when>
            <!--xsl:when test="$sctp.chunk_type_str='SHUTDOWN_ACK'"></xsl:when-->
            <xsl:when test="$sctp.chunk_type_str='ERROR'">
                <payload name="{$sctp.chunk_type_str}">
                   <frame.time_relative        value="{$time_relative}"/>
                   <!-- TODO: pos_offset(substract it from all pos_offsets in s1ap, may depend on which test scenario protocol target S1AP/NAS or NAS only...)-->
                   <pos_offset                 value="{$sctp_pos_offset}"/>
                   <ip.src                     value="{$ip.src}"/>
                   <ip.dst                     value="{$ip.dst}"/>
                   <sctp.data_sid              value="{$sctp.data_sid}"/>
                   <sctp.srcport               value="{$sctp.srcport}"/>
                   <sctp.dstport               value="{$sctp.dstport}"/>
                   <sctp.chunk_type_str        value="{$sctp.chunk_type_str}"/>
                   <xsl:copy-of select="node()"/>
                </payload>
            </xsl:when>
            <!--xsl:when test="$sctp.chunk_type_str='COOKIE_ECHO'">            </xsl:when-->
            <!--xsl:when test="$sctp.chunk_type_str='COOKIE_ACK'">            </xsl:when-->
            <!--xsl:when test="$sctp.chunk_type_str='ECNE'">            </xsl:when-->
            <!--xsl:when test="$sctp.chunk_type_str='CWR'">            </xsl:when-->
            <!--xsl:when test="$sctp.chunk_type_str='SHUTDOWN_COMPLETE'">            </xsl:when-->
            <xsl:otherwise></xsl:otherwise>
        </xsl:choose> 

        
    </xsl:for-each>
</xsl:template>
</xsl:stylesheet>
