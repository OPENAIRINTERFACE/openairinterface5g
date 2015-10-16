<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://www.w3.org/TR/xhtml1/strict">
                
<xsl:output
   method="xml"
   indent="yes"
   encoding="iso-8859-1"
/>

<xsl:strip-space elements="proto field"/>

<scenario name="{$test_name}">

<xsl:template match="proto[@name='frame']">
    <DEBUG_FRAME>
    <xsl:variable name="time_relative" select="field[@name='frame.time_relative']/@show"/>
    <xsl:variable name="ip" select="proto[@name='ip']"/>
    <xsl:variable name="ip.src" select="$ip/field[@name='ip.src']/@show"/>
    <xsl:variable name="ip.dst" select="$ip/field[@name='ip.dst']/@show"/>
    
    <xsl:for-each select="$ip/proto[@name='sctp']">
        <xsl:variable name="sctp.data_sid"               select="./field[@name='sctp.data_sid']/@show"/>
        <xsl:variable name="sctp.srcport"                select="./field[@name='sctp.srcport']/@show"/>
        <xsl:variable name="sctp.dstport"                select="./field[@name='sctp.dstport']/@show"/>
        <xsl:variable name="sctp.data_ssn"               select="./field[@name='sctp.data_ssn']/@show"/>
        <xsl:variable name="sctp.data_payload_proto_id"  select="./field[@name='sctp.data_payload_proto_id']/@show"/>
    
        <xsl:for-each select="./proto[@name='s1ap']">
            <payload name="{ip_dst}">
               <frame.time_relative        value="{$time_relative}"/>
               <ip.dst                     value="{$ip.dst}"/>
               <ip.src                     value="{$ip.src}"/>
               <sctp.data_sid              value="{$sctp.data_sid}"/>
               <sctp.srcport               value="{$sctp.srcport}"/>
               <sctp.dstport               value="{$sctp.dstport}"/>
               <sctp.data_ssn              value="{$sctp.data_ssn}"/>
               <sctp.data_payload_proto_id value="{$sctp.data_payload_proto_id}"/>
               <xsl:copy-of select="node()"/>
            </payload>
        </xsl:for-each>
    </xsl:for-each>
    </DEBUG_FRAME>
</xsl:template>
</scenario>
</xsl:stylesheet>
