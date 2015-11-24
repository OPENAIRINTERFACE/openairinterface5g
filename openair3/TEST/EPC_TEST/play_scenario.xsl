<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://www.w3.org/TR/xhtml1/strict">
                
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

  <xsl:template match="ip.src[parent::packet]/@value">
    <xsl:choose>
      <xsl:when test=".='enb0_s1c'"><xsl:value-of select="$enb0_s1c"/></xsl:when>
      <xsl:when test=".='enb1_s1c'"><xsl:value-of select="$enb1_s1c"/></xsl:when>
      <xsl:when test=".='enb2_s1c'"><xsl:value-of select="$enb2_s1c"/></xsl:when>
      <xsl:when test=".='enb3_s1c'"><xsl:value-of select="$enb3_s1c"/></xsl:when>
      <xsl:when test=".='enb4_s1c'"><xsl:value-of select="$enb4_s1c"/></xsl:when>
      <xsl:when test=".='enb5_s1c'"><xsl:value-of select="$enb5_s1c"/></xsl:when>
      <xsl:when test=".='enb6_s1c'"><xsl:value-of select="$enb6_s1c"/></xsl:when>
      <xsl:when test=".='enb7_s1c'"><xsl:value-of select="$enb7_s1c"/></xsl:when>
      <xsl:when test=".='mme0_s1c_0'"><xsl:value-of select="$mme0_s1c_0"/></xsl:when>
      <xsl:when test=".='mme0_s1c_1'"><xsl:value-of select="$mme0_s1c_1"/></xsl:when>
      <xsl:when test=".='mme0_s1c_2'"><xsl:value-of select="$mme0_s1c_2"/></xsl:when>
      <xsl:when test=".='mme0_s1c_3'"><xsl:value-of select="$mme0_s1c_3"/></xsl:when>
      <xsl:when test=".='mme1_s1c_0'"><xsl:value-of select="$mme1_s1c_0"/></xsl:when>
      <xsl:when test=".='mme1_s1c_1'"><xsl:value-of select="$mme1_s1c_1"/></xsl:when>
      <xsl:when test=".='mme1_s1c_2'"><xsl:value-of select="$mme1_s1c_2"/></xsl:when>
      <xsl:when test=".='mme1_s1c_3'"><xsl:value-of select="$mme1_s1c_3"/></xsl:when>
      <xsl:when test=".='mme2_s1c_0'"><xsl:value-of select="$mme2_s1c_0"/></xsl:when>
      <xsl:when test=".='mme2_s1c_1'"><xsl:value-of select="$mme2_s1c_1"/></xsl:when>
      <xsl:when test=".='mme2_s1c_2'"><xsl:value-of select="$mme2_s1c_2"/></xsl:when>
      <xsl:when test=".='mme2_s1c_3'"><xsl:value-of select="$mme2_s1c_3"/></xsl:when>
      <xsl:when test=".='mme3_s1c_0'"><xsl:value-of select="$mme3_s1c_0"/></xsl:when>
      <xsl:when test=".='mme3_s1c_1'"><xsl:value-of select="$mme3_s1c_1"/></xsl:when>
      <xsl:when test=".='mme3_s1c_2'"><xsl:value-of select="$mme3_s1c_2"/></xsl:when>
      <xsl:when test=".='mme3_s1c_3'"><xsl:value-of select="$mme3_s1c_3"/></xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">ERROR: Cannot resolv IP <xsl:value-of select="."/> !
        </xsl:message>
      </xsl:otherwise>
    </xsl:choose> 
  </xsl:template>
  
  <xsl:template match="ip.dst[parent::packet]/@value">
    <xsl:choose>
      <xsl:when test=".='enb0_s1c'"><xsl:value-of select="$enb0_s1c"/></xsl:when>
      <xsl:when test=".='enb1_s1c'"><xsl:value-of select="$enb1_s1c"/></xsl:when>
      <xsl:when test=".='enb2_s1c'"><xsl:value-of select="$enb2_s1c"/></xsl:when>
      <xsl:when test=".='enb3_s1c'"><xsl:value-of select="$enb3_s1c"/></xsl:when>
      <xsl:when test=".='enb4_s1c'"><xsl:value-of select="$enb4_s1c"/></xsl:when>
      <xsl:when test=".='enb5_s1c'"><xsl:value-of select="$enb5_s1c"/></xsl:when>
      <xsl:when test=".='enb6_s1c'"><xsl:value-of select="$enb6_s1c"/></xsl:when>
      <xsl:when test=".='enb7_s1c'"><xsl:value-of select="$enb7_s1c"/></xsl:when>
      <xsl:when test=".='mme0_s1c_0'"><xsl:value-of select="$mme0_s1c_0"/></xsl:when>
      <xsl:when test=".='mme0_s1c_1'"><xsl:value-of select="$mme0_s1c_1"/></xsl:when>
      <xsl:when test=".='mme0_s1c_2'"><xsl:value-of select="$mme0_s1c_2"/></xsl:when>
      <xsl:when test=".='mme0_s1c_3'"><xsl:value-of select="$mme0_s1c_3"/></xsl:when>
      <xsl:when test=".='mme1_s1c_0'"><xsl:value-of select="$mme1_s1c_0"/></xsl:when>
      <xsl:when test=".='mme1_s1c_1'"><xsl:value-of select="$mme1_s1c_1"/></xsl:when>
      <xsl:when test=".='mme1_s1c_2'"><xsl:value-of select="$mme1_s1c_2"/></xsl:when>
      <xsl:when test=".='mme1_s1c_3'"><xsl:value-of select="$mme1_s1c_3"/></xsl:when>
      <xsl:when test=".='mme2_s1c_0'"><xsl:value-of select="$mme2_s1c_0"/></xsl:when>
      <xsl:when test=".='mme2_s1c_1'"><xsl:value-of select="$mme2_s1c_1"/></xsl:when>
      <xsl:when test=".='mme2_s1c_2'"><xsl:value-of select="$mme2_s1c_2"/></xsl:when>
      <xsl:when test=".='mme2_s1c_3'"><xsl:value-of select="$mme2_s1c_3"/></xsl:when>
      <xsl:when test=".='mme3_s1c_0'"><xsl:value-of select="$mme3_s1c_0"/></xsl:when>
      <xsl:when test=".='mme3_s1c_1'"><xsl:value-of select="$mme3_s1c_1"/></xsl:when>
      <xsl:when test=".='mme3_s1c_2'"><xsl:value-of select="$mme3_s1c_2"/></xsl:when>
      <xsl:when test=".='mme3_s1c_3'"><xsl:value-of select="$mme3_s1c_3"/></xsl:when>
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
