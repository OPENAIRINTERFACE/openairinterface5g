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

  <xsl:template match="ip.src[parent::payload]/@value">
    <xsl:choose>
      <xsl:when test=".='enb_s1c0'"><xsl:value-of select="$enb_s1c0"/></xsl:when>
      <xsl:when test=".='enb_s1c1'"><xsl:value-of select="$enb_s1c1"/></xsl:when>
      <xsl:when test=".='enb_s1c2'"><xsl:value-of select="$enb_s1c2"/></xsl:when>
      <xsl:when test=".='enb_s1c3'"><xsl:value-of select="$enb_s1c3"/></xsl:when>
      <xsl:when test=".='enb_s1c4'"><xsl:value-of select="$enb_s1c4"/></xsl:when>
      <xsl:when test=".='enb_s1c5'"><xsl:value-of select="$enb_s1c5"/></xsl:when>
      <xsl:when test=".='enb_s1c6'"><xsl:value-of select="$enb_s1c6"/></xsl:when>
      <xsl:when test=".='enb_s1c7'"><xsl:value-of select="$enb_s1c7"/></xsl:when>
      <xsl:when test=".='mme_s1c0_0'"><xsl:value-of select="$mme_s1c0_0"/></xsl:when>
      <xsl:when test=".='mme_s1c0_1'"><xsl:value-of select="$mme_s1c0_1"/></xsl:when>
      <xsl:when test=".='mme_s1c0_2'"><xsl:value-of select="$mme_s1c0_2"/></xsl:when>
      <xsl:when test=".='mme_s1c0_3'"><xsl:value-of select="$mme_s1c0_3"/></xsl:when>
      <xsl:when test=".='mme_s1c1_0'"><xsl:value-of select="$mme_s1c1_0"/></xsl:when>
      <xsl:when test=".='mme_s1c1_1'"><xsl:value-of select="$mme_s1c1_1"/></xsl:when>
      <xsl:when test=".='mme_s1c1_2'"><xsl:value-of select="$mme_s1c1_2"/></xsl:when>
      <xsl:when test=".='mme_s1c1_3'"><xsl:value-of select="$mme_s1c1_3"/></xsl:when>
      <xsl:when test=".='mme_s1c2_0'"><xsl:value-of select="$mme_s1c2_0"/></xsl:when>
      <xsl:when test=".='mme_s1c2_1'"><xsl:value-of select="$mme_s1c2_1"/></xsl:when>
      <xsl:when test=".='mme_s1c2_2'"><xsl:value-of select="$mme_s1c2_2"/></xsl:when>
      <xsl:when test=".='mme_s1c2_3'"><xsl:value-of select="$mme_s1c2_3"/></xsl:when>
      <xsl:when test=".='mme_s1c3_0'"><xsl:value-of select="$mme_s1c3_0"/></xsl:when>
      <xsl:when test=".='mme_s1c3_1'"><xsl:value-of select="$mme_s1c3_1"/></xsl:when>
      <xsl:when test=".='mme_s1c3_2'"><xsl:value-of select="$mme_s1c3_2"/></xsl:when>
      <xsl:when test=".='mme_s1c3_3'"><xsl:value-of select="$mme_s1c3_3"/></xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">ERROR: Cannot resolv IP <xsl:value-of select="."/> !
        </xsl:message>
      </xsl:otherwise>
    </xsl:choose> 
  </xsl:template>
  
  <xsl:template match="ip.dst[parent::payload]/@value">
    <xsl:choose>
      <xsl:when test=".='enb_s1c0'"><xsl:value-of select="$enb_s1c0"/></xsl:when>
      <xsl:when test=".='enb_s1c1'"><xsl:value-of select="$enb_s1c1"/></xsl:when>
      <xsl:when test=".='enb_s1c2'"><xsl:value-of select="$enb_s1c2"/></xsl:when>
      <xsl:when test=".='enb_s1c3'"><xsl:value-of select="$enb_s1c3"/></xsl:when>
      <xsl:when test=".='enb_s1c4'"><xsl:value-of select="$enb_s1c4"/></xsl:when>
      <xsl:when test=".='enb_s1c5'"><xsl:value-of select="$enb_s1c5"/></xsl:when>
      <xsl:when test=".='enb_s1c6'"><xsl:value-of select="$enb_s1c6"/></xsl:when>
      <xsl:when test=".='enb_s1c7'"><xsl:value-of select="$enb_s1c7"/></xsl:when>
      <xsl:when test=".='mme_s1c0_0'"><xsl:value-of select="$mme_s1c0_0"/></xsl:when>
      <xsl:when test=".='mme_s1c0_1'"><xsl:value-of select="$mme_s1c0_1"/></xsl:when>
      <xsl:when test=".='mme_s1c0_2'"><xsl:value-of select="$mme_s1c0_2"/></xsl:when>
      <xsl:when test=".='mme_s1c0_3'"><xsl:value-of select="$mme_s1c0_3"/></xsl:when>
      <xsl:when test=".='mme_s1c1_0'"><xsl:value-of select="$mme_s1c1_0"/></xsl:when>
      <xsl:when test=".='mme_s1c1_1'"><xsl:value-of select="$mme_s1c1_1"/></xsl:when>
      <xsl:when test=".='mme_s1c1_2'"><xsl:value-of select="$mme_s1c1_2"/></xsl:when>
      <xsl:when test=".='mme_s1c1_3'"><xsl:value-of select="$mme_s1c1_3"/></xsl:when>
      <xsl:when test=".='mme_s1c2_0'"><xsl:value-of select="$mme_s1c2_0"/></xsl:when>
      <xsl:when test=".='mme_s1c2_1'"><xsl:value-of select="$mme_s1c2_1"/></xsl:when>
      <xsl:when test=".='mme_s1c2_2'"><xsl:value-of select="$mme_s1c2_2"/></xsl:when>
      <xsl:when test=".='mme_s1c2_3'"><xsl:value-of select="$mme_s1c2_3"/></xsl:when>
      <xsl:when test=".='mme_s1c3_0'"><xsl:value-of select="$mme_s1c3_0"/></xsl:when>
      <xsl:when test=".='mme_s1c3_1'"><xsl:value-of select="$mme_s1c3_1"/></xsl:when>
      <xsl:when test=".='mme_s1c3_2'"><xsl:value-of select="$mme_s1c3_2"/></xsl:when>
      <xsl:when test=".='mme_s1c3_3'"><xsl:value-of select="$mme_s1c3_3"/></xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">ERROR: Cannot resolv IP <xsl:value-of select="."/> !
        </xsl:message>
      </xsl:otherwise>
    </xsl:choose> 
  </xsl:template>
  
  <xsl:template match="node()|@*">
    <xsl:copy>
      <xsl:apply-templates select="node()|@*"/>
    </xsl:copy>
  </xsl:template>
  
  <xsl:template match="/">
      <xsl:apply-templates/>
  </xsl:template>

</xsl:stylesheet>
