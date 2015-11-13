<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
                
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
        <xsl:when test="$ip_address=$enb_s1c6">enb_s1c6</xsl:when>
        <xsl:when test="$ip_address=$enb_s1c7">enb_s1c7</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c0_0">mme_s1c0_0</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c0_1">mme_s1c0_1</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c0_2">mme_s1c0_2</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c0_3">mme_s1c0_3</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c1_0">mme_s1c1_0</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c1_1">mme_s1c1_1</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c1_2">mme_s1c1_2</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c1_3">mme_s1c1_3</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c2_0">mme_s1c2_0</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c2_1">mme_s1c2_1</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c2_2">mme_s1c2_2</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c2_3">mme_s1c2_3</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c3_0">mme_s1c3_0</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c3_1">mme_s1c3_1</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c3_2">mme_s1c3_2</xsl:when>
        <xsl:when test="$ip_address=$mme_s1c3_3">mme_s1c3_3</xsl:when>
        <xsl:otherwise>
          <xsl:message terminate="yes">ERROR: Cannot reverse resolv IP <xsl:value-of select="."/> !
          </xsl:message>
        </xsl:otherwise>
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
      <xsl:otherwise>
        <xsl:message terminate="yes">ERROR: UNKNOWN CHUNK TYPE <xsl:value-of select="."/> !
        </xsl:message>
      </xsl:otherwise>
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
    <xsl:variable name="frame_number" select="field[@name='frame.number']/@show"/>
    <xsl:variable name="ip" select="proto[@name='ip']"/>
    <xsl:variable name="ip_src">
      <xsl:call-template name="reverse_ip">
        <xsl:with-param name="ip_address" select="$ip/field[@name='ip.src']/@show"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="ip_dst">
      <xsl:call-template name="reverse_ip">
        <xsl:with-param name="ip_address" select="$ip/field[@name='ip.dst']/@show"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="action">
      <xsl:choose>
        <xsl:when test="starts-with($ip_src,'enb_s1')">SEND</xsl:when>
        <xsl:when test="starts-with($ip_src,'mme_s1c')">RECEIVE</xsl:when>
        <xsl:otherwise>
          <xsl:message terminate="yes">ERROR: UNKNOWN ACTION <xsl:value-of select="."/> !
          </xsl:message>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    
    <xsl:for-each select="$ip/proto[@name='sctp']">
      <xsl:variable name="sctp_data_sid"               select="./field/field[@name='sctp.data_sid']/@show"/>
      <!-- TODO resolv problem of 2 SCTP packets in 1 IP packet: src and dst ports are not in the 2nd SCTP packet -->
      <!--xsl:variable name="sctp_srcport"                select="./field[@name='sctp.srcport']/@show"/-->
      <!--xsl:variable name="sctp_dstport"                select="./field[@name='sctp.dstport']/@show"/-->
      <!--xsl:variable name="sctp_data_ssn"               select="./field/field[@name='sctp.data_ssn']/@show"/-->
      <!--xsl:variable name="sctp_data_payload_proto_id"  select="./field/field[@name='sctp.data_payload_proto_id']/@show"/-->
      <xsl:variable name="sctp_chunk_type_str">
        <xsl:call-template name="chunktype2str">
          <xsl:with-param name="chunk_type" select="./field/field[@name='sctp.chunk_type']/@value"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:variable name="sctp_pos_offset" select="./@pos"/>
      <xsl:variable name="sctp_node" select="."/>

      <xsl:choose>
        <xsl:when test="$sctp_chunk_type_str='DATA'">
          <xsl:for-each select="./proto[@name='s1ap']">
            <xsl:variable name="s1ap_pos_offset" select="./@pos"/>
            <packet name="{$sctp_chunk_type_str}" action="{$action}">
              <frame.time_relative        value="{$time_relative}"/>
              <frame.number               value="{$frame_number}"/>
               
              <!-- TODO: pos_offset(substract it from all pos_offsets in s1ap, may depend on which test scenario protocol target S1AP/NAS or NAS only...)-->
              <pos_offset                 value="{$s1ap_pos_offset}"/>
              <ip.src                     value="{$ip_src}"/>
              <ip.dst                     value="{$ip_dst}"/>
              <!--sctp.data_sid              value="{$sctp_data_sid}"/-->
              <!--sctp.srcport               value="{$sctp_srcport}"/-->
              <!--sctp.dstport               value="{$sctp_dstport}"/-->
              <!--sctp.data_ssn              value="{$sctp_data_ssn}"/-->
              <!--sctp.data_payload_proto_id value="{$sctp_data_payload_proto_id}"/-->
              <sctp.chunk_type_str        value="{$sctp_chunk_type_str}"/>
              <xsl:copy-of select="$sctp_node"/>
            </packet>
          </xsl:for-each>
        </xsl:when>
        <xsl:when test="$sctp_chunk_type_str='INIT'">
          <!--xsl:variable name="sctp_init_nr_out_streams"  select="./field/field[@name='sctp.init_nr_out_streams']/@show"/-->
          <!--xsl:variable name="sctp_init_nr_in_streams"   select="./field/field[@name='sctp.init_nr_in_streams']/@show"/-->
          <!--xsl:variable name="sctp_init_initial_tsn"     select="./field/field[@name='sctp.init_initial_tsn']/@show"/-->
          <packet name="{$sctp_chunk_type_str}" action="{$action}">
            <frame.time_relative        value="{$time_relative}"/>
            <frame.number               value="{$frame_number}"/>
            <!-- TODO: pos_offset(substract it from all pos_offsets in s1ap, may depend on which test scenario protocol target S1AP/NAS or NAS only...)-->
            <pos_offset                 value="{$sctp_pos_offset}"/>
            <ip.src                     value="{$ip_src}"/>
            <ip.dst                     value="{$ip_dst}"/>
            <!--sctp.srcport               value="{$sctp_srcport}"/-->
            <!--sctp.dstport               value="{$sctp_dstport}"/-->
            <!--sctp.init_nr_in_streams    value="{$sctp_init_nr_in_streams}"/-->
            <!--sctp.init_nr_out_streams   value="{$sctp_init_nr_out_streams}"/-->
            <!--sctp.init_initial_tsn      value="{$sctp_init_initial_tsn}"/-->
            <sctp.chunk_type_str        value="{$sctp_chunk_type_str}"/>
            <xsl:copy-of select="$sctp_node"/>
          </packet>
        </xsl:when>
        <xsl:when test="$sctp_chunk_type_str='INIT_ACK'">
          <!--xsl:variable name="sctp_initack_nr_out_streams"  select="./field/field[@name='sctp.initack_nr_out_streams']/@show"/-->
          <!--xsl:variable name="sctp_initack_nr_in_streams"   select="./field/field[@name='sctp.initack_nr_in_streams']/@show"/-->
          <!--xsl:variable name="sctp_initack_initial_tsn"     select="./field/field[@name='sctp.initack_initial_tsn']/@show"/-->
          <packet name="{$sctp_chunk_type_str}" action="{$action}">
            <frame.time_relative        value="{$time_relative}"/>
            <frame.number               value="{$frame_number}"/>
            <!-- TODO: pos_offset(substract it from all pos_offsets in s1ap, may depend on which test scenario protocol target S1AP/NAS or NAS only...)-->
            <pos_offset                 value="{$sctp_pos_offset}"/>
            <ip.src                     value="{$ip_src}"/>
            <ip.dst                     value="{$ip_dst}"/>
            <!--sctp.data_sid              value="{$sctp_data_sid}"/-->
            <!--sctp.srcport               value="{$sctp_srcport}"/-->
            <!--sctp.dstport               value="{$sctp_dstport}"/-->
            <!--sctp.initack_nr_in_streams  value="{$sctp_initack_nr_in_streams}"/-->
            <!--sctp.initack_nr_out_streams value="{$sctp_initack_nr_out_streams}"/-->
            <!--sctp.initack_initial_tsn   value="{$sctp_initack_initial_tsn}"/-->
            <sctp.chunk_type_str        value="{$sctp_chunk_type_str}"/>
            <xsl:copy-of select="$sctp_node"/>
          </packet>
        </xsl:when>
        <!--xsl:when test="$sctp_chunk_type_str='SACK'">       </xsl:when-->
        <!--xsl:when test="$sctp_chunk_type_str='HEARTBEAT'"></xsl:when-->
        <!--xsl:when test="$sctp_chunk_type_str='HEARTBEAT_ACK'"></xsl:when-->
        <xsl:when test="$sctp_chunk_type_str='ABORT'">
          <packet name="{$sctp_chunk_type_str}" action="{$action}">
            <frame.time_relative        value="{$time_relative}"/>
            <frame.number               value="{$frame_number}"/>
            <!-- TODO: pos_offset(substract it from all pos_offsets in s1ap, may depend on which test scenario protocol target S1AP/NAS or NAS only...)-->
            <pos_offset                 value="{$sctp_pos_offset}"/>
            <ip.src                     value="{$ip_src}"/>
            <ip.dst                     value="{$ip_dst}"/>
            <!--sctp.data_sid              value="{$sctp_data_sid}"/-->
            <!--sctp.srcport               value="{$sctp_srcport}"/-->
            <!--sctp.dstport               value="{$sctp_dstport}"/-->
            <sctp.chunk_type_str        value="{$sctp_chunk_type_str}"/>
            <xsl:copy-of select="$sctp_node"/>
          </packet>
        </xsl:when>
        <xsl:when test="$sctp_chunk_type_str='SHUTDOWN'">
          <packet name="{$sctp_chunk_type_str}" action="{$action}">
            <frame.time_relative        value="{$time_relative}"/>
            <frame.number               value="{$frame_number}"/>
            <!-- TODO: pos_offset(substract it from all pos_offsets in s1ap, may depend on which test scenario protocol target S1AP/NAS or NAS only...)-->
            <pos_offset                 value="{$sctp_pos_offset}"/>
            <ip.src                     value="{$ip_src}"/>
            <ip.dst                     value="{$ip_dst}"/>
            <!--sctp.data_sid              value="{$sctp_data_sid}"/-->
            <!--sctp.srcport               value="{$sctp_srcport}"/-->
            <!--sctp.dstport               value="{$sctp_dstport}"/-->
            <sctp.chunk_type_str        value="{$sctp_chunk_type_str}"/>
            <xsl:copy-of select="$sctp_node"/>
          </packet>
        </xsl:when>
        <!--xsl:when test="$sctp_chunk_type_str='SHUTDOWN_ACK'"></xsl:when-->
        <xsl:when test="$sctp_chunk_type_str='ERROR'">
          <packet name="{$sctp_chunk_type_str}" action="{$action}">
            <frame.time_relative        value="{$time_relative}"/>
            <frame.number               value="{$frame_number}"/>
            <!-- TODO: pos_offset(substract it from all pos_offsets in s1ap, may depend on which test scenario protocol target S1AP/NAS or NAS only...)-->
            <pos_offset                 value="{$sctp_pos_offset}"/>
            <ip.src                     value="{$ip_src}"/>
            <ip.dst                     value="{$ip_dst}"/>
            <!--sctp.data_sid              value="{$sctp_data_sid}"/-->
            <!--sctp.srcport               value="{$sctp_srcport}"/-->
            <!--sctp.dstport               value="{$sctp_dstport}"/-->
            <sctp.chunk_type_str        value="{$sctp_chunk_type_str}"/>
            <xsl:copy-of select="$sctp_node"/>
          </packet>
        </xsl:when>
        <!--xsl:when test="$sctp_chunk_type_str='COOKIE_ECHO'">            </xsl:when-->
        <!--xsl:when test="$sctp_chunk_type_str='COOKIE_ACK'">            </xsl:when-->
        <!--xsl:when test="$sctp_chunk_type_str='ECNE'">            </xsl:when-->
        <!--xsl:when test="$sctp_chunk_type_str='CWR'">            </xsl:when-->
        <!--xsl:when test="$sctp_chunk_type_str='SHUTDOWN_COMPLETE'">            </xsl:when-->
        <xsl:otherwise></xsl:otherwise>
      </xsl:choose> 
    </xsl:for-each>
  </xsl:template>
</xsl:stylesheet>
