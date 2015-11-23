<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
                
  <xsl:output
    method="xml"
    indent="yes"
    encoding="iso-8859-1"
  />

  <!-- Ugly but no time to find a better way in XSLT 1.0 (map/list)-->
  <xsl:param name="enb0_s1c"   select="'0.0.0.0'"/>
  <xsl:param name="enb1_s1c"   select="'0.0.0.0'"/>
  <xsl:param name="enb2_s1c"   select="'0.0.0.0'"/>
  <xsl:param name="enb3_s1c"   select="'0.0.0.0'"/>
  <xsl:param name="enb4_s1c"   select="'0.0.0.0'"/>
  <xsl:param name="enb5_s1c"   select="'0.0.0.0'"/>
  <xsl:param name="enb6_s1c"   select="'0.0.0.0'"/>
  <xsl:param name="enb7_s1c"   select="'0.0.0.0'"/>
  <xsl:param name="mme0_s1c_0" select="'0.0.0.0'"/>
  <xsl:param name="mme0_s1c_1" select="'0.0.0.0'"/>
  <xsl:param name="mme0_s1c_2" select="'0.0.0.0'"/>
  <xsl:param name="mme0_s1c_3" select="'0.0.0.0'"/>
  <xsl:param name="mme1_s1c_0" select="'0.0.0.0'"/>
  <xsl:param name="mme1_s1c_1" select="'0.0.0.0'"/>
  <xsl:param name="mme1_s1c_2" select="'0.0.0.0'"/>
  <xsl:param name="mme1_s1c_3" select="'0.0.0.0'"/>
  <xsl:param name="mme2_s1c_0" select="'0.0.0.0'"/>
  <xsl:param name="mme2_s1c_1" select="'0.0.0.0'"/>
  <xsl:param name="mme2_s1c_2" select="'0.0.0.0'"/>
  <xsl:param name="mme2_s1c_3" select="'0.0.0.0'"/>
  <xsl:param name="mme3_s1c_0" select="'0.0.0.0'"/>
  <xsl:param name="mme3_s1c_1" select="'0.0.0.0'"/>
  <xsl:param name="mme3_s1c_2" select="'0.0.0.0'"/>
  <xsl:param name="mme3_s1c_3" select="'0.0.0.0'"/>
  <xsl:param name="ip_address" select="'0.0.0.0'"/>



  <xsl:template name="reverse_ip">
    <xsl:param name="ip_address"/>
      <xsl:choose>
        <xsl:when test="$ip_address=$enb0_s1c">enb0_s1c</xsl:when>
        <xsl:when test="$ip_address=$enb1_s1c">enb1_s1c</xsl:when>
        <xsl:when test="$ip_address=$enb2_s1c">enb2_s1c</xsl:when>
        <xsl:when test="$ip_address=$enb3_s1c">enb3_s1c</xsl:when>
        <xsl:when test="$ip_address=$enb4_s1c">enb4_s1c</xsl:when>
        <xsl:when test="$ip_address=$enb5_s1c">enb5_s1c</xsl:when>
        <xsl:when test="$ip_address=$enb6_s1c">enb6_s1c</xsl:when>
        <xsl:when test="$ip_address=$enb7_s1c">enb7_s1c</xsl:when>
        <xsl:when test="$ip_address=$mme0_s1c_0">mme0_s1c_0</xsl:when>
        <xsl:when test="$ip_address=$mme0_s1c_1">mme0_s1c_1</xsl:when>
        <xsl:when test="$ip_address=$mme0_s1c_2">mme0_s1c_2</xsl:when>
        <xsl:when test="$ip_address=$mme0_s1c_3">mme0_s1c_3</xsl:when>
        <xsl:when test="$ip_address=$mme1_s1c_0">mme1_s1c_0</xsl:when>
        <xsl:when test="$ip_address=$mme1_s1c_1">mme1_s1c_1</xsl:when>
        <xsl:when test="$ip_address=$mme1_s1c_2">mme1_s1c_2</xsl:when>
        <xsl:when test="$ip_address=$mme1_s1c_3">mme1_s1c_3</xsl:when>
        <xsl:when test="$ip_address=$mme2_s1c_0">mme2_s1c_0</xsl:when>
        <xsl:when test="$ip_address=$mme2_s1c_1">mme2_s1c_1</xsl:when>
        <xsl:when test="$ip_address=$mme2_s1c_2">mme2_s1c_2</xsl:when>
        <xsl:when test="$ip_address=$mme2_s1c_3">mme2_s1c_3</xsl:when>
        <xsl:when test="$ip_address=$mme3_s1c_0">mme3_s1c_0</xsl:when>
        <xsl:when test="$ip_address=$mme3_s1c_1">mme3_s1c_1</xsl:when>
        <xsl:when test="$ip_address=$mme3_s1c_2">mme3_s1c_2</xsl:when>
        <xsl:when test="$ip_address=$mme3_s1c_3">mme3_s1c_3</xsl:when>
        <xsl:otherwise>
          <xsl:message terminate="yes">ERROR: Cannot reverse resolv IP <xsl:value-of select="."/> !
          </xsl:message>
        </xsl:otherwise>
      </xsl:choose> 
  </xsl:template>

  <xsl:template name="enb_ip_2_enb_instance">
    <xsl:param name="ip_address"/>
      <xsl:choose>
        <xsl:when test="$ip_address=$enb0_s1c">0</xsl:when>
        <xsl:when test="$ip_address=$enb1_s1c">1</xsl:when>
        <xsl:when test="$ip_address=$enb2_s1c">2</xsl:when>
        <xsl:when test="$ip_address=$enb3_s1c">3</xsl:when>
        <xsl:when test="$ip_address=$enb4_s1c">4</xsl:when>
        <xsl:when test="$ip_address=$enb5_s1c">5</xsl:when>
        <xsl:when test="$ip_address=$enb6_s1c">6</xsl:when>
        <xsl:when test="$ip_address=$enb7_s1c">7</xsl:when>
        <xsl:otherwise>
          <xsl:message terminate="yes">ERROR: Cannot set eNB instance <xsl:value-of select="."/> !
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
        <xsl:when test="starts-with($ip_src,'enb')">SEND</xsl:when>
        <xsl:when test="starts-with($ip_src,'mme')">RECEIVE</xsl:when>
        <xsl:otherwise>
          <xsl:message terminate="yes">ERROR: UNKNOWN ACTION <xsl:value-of select="."/> !
          </xsl:message>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="enb_instance">
      <xsl:choose>
        <xsl:when test="starts-with($ip_src,'enb')">
          <xsl:call-template name="enb_ip_2_enb_instance">
            <xsl:with-param name="ip_address" select="$ip/field[@name='ip.src']/@show"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="starts-with($ip_dst,'enb')">
          <xsl:call-template name="enb_ip_2_enb_instance">
            <xsl:with-param name="ip_address" select="$ip/field[@name='ip.dst']/@show"/>
          </xsl:call-template>
        </xsl:when>
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
              <eNB.instance               value="{$enb_instance}"/>
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
            <eNB.instance               value="{$enb_instance}"/>
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
            <eNB.instance               value="{$enb_instance}"/>
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
            <eNB.instance               value="{$enb_instance}"/>
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
            <eNB.instance               value="{$enb_instance}"/>
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
            <eNB.instance               value="{$enb_instance}"/>
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
