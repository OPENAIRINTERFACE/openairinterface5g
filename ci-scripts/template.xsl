<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:output method="html" version="4" encoding="UTF-8" indent="no" omit-xml-declaration="yes"/>
    <!-- main body -->
    <xsl:template match="/">
        <html>
          <body>
            <h3>TEMPLATE Results Summary</h3>
            <table border="1">
              <tr bgcolor="lightcyan">
                <!--Header only so select first row to get headers-->
                  <th>Hostname</th>
                  <th>Nb Tests</th>
                  <th>Failures</th>
                  <th>Timestamp</th>
              </tr>
              <!--Get all the other rows-->
              <xsl:for-each select="testsuites/testsuite">
                <tr>
                    <td>
                      <xsl:value-of select="@hostname"/>
                    </td>
                    <td>
                      <xsl:value-of select="@tests"/>
                    </td>
                    <td>
                      <xsl:value-of select="@failures"/>
                    </td>
                    <td>
                      <xsl:value-of select="@timestamp"/>
                    </td>
                </tr>
              </xsl:for-each>
              </table>
            <h4>Details</h4>
            <table border="1">
              <tr bgcolor="lightcyan">
                <!--Header only so select first row to get headers-->
                  <th>Test Name</th>
                  <th>Result</th>
                  <th>Time</th>
                  <th>Description</th>
              </tr>
              <!--Get all the other rows-->
              <xsl:for-each select="testsuites/testsuite/testcase">
                <tr>
                    <td>
                      <xsl:value-of select="@name"/>
                    </td>
                    <td>
                      <xsl:value-of select="@RESULT"/>
                    </td>
                    <td>
                      <xsl:value-of select="@time"/>
                    </td>
                    <td>
                      <xsl:value-of select="@description"/>
                    </td>
                </tr>
              </xsl:for-each>
              </table>
          </body>
        </html>
    </xsl:template>
</xsl:stylesheet>
