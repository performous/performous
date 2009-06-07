<?xml version="1.0"?>

<!-- Use xmlstarlet tr songlist-to-xhtml.xml songlist.xml > songlist.xhtml -->

<xsl:stylesheet  version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns="http://www.w3.org/1999/xhtml">

<xsl:output method="xml" indent="yes" encoding="UTF-8"/>

<xsl:template match="/songlist">
  <html>
    <head>
      <title>Karaoke</title>
      <meta http-equiv="Content-type" content="application/xhtml+xml; charset=UTF-8"/>
      <style>
    	body { font-size: xx-small; font-family: sans; }
    	tr { overflow: auto; }
    	th { text-align: left; }
    	td { white-space: pre; overflow: hidden; }
        #layout { border-spacing: 10mm; }
        table { float: left; margin: 3mm; table-layout: fixed; width: 46%; border: 2px solid black; border-collapse: collapse; }
        th:first-child, td:first-child { text-align: right; width: 5ex; }
        th, td { padding: 2px; padding-left: 3mm; padding-right: 3mm; }
        table th { background: black; color: white; border-bottom: 2px solid black; }
        #byArtist tr.fifth { background: #ddf; }
        #byTitle tr.fifth { background: #cfc; }
      </style>
    </head>
    <body>
            <table id="byArtist">
              <thead>
                <tr><th>#</th><th>Artist</th><th>Title</th></tr>
              </thead>
              <tbody>
                <xsl:for-each select="song">
                  <tr>
                    <xsl:if test="position() mod 5 = 0"><xsl:attribute name="class">fifth</xsl:attribute></xsl:if>
                    <td><xsl:value-of select="@num"/></td><td><xsl:value-of select="artist"/></td><td><xsl:value-of select="title"/></td>
                  </tr>
                </xsl:for-each>
              </tbody>
            </table>
            <table id="byTitle">
              <thead>
                <tr><th>#</th><th>Title</th><th>Artist</th></tr>
              </thead>
              <tbody>
                <xsl:for-each select="song">
                  <xsl:sort select="collate/title"/>
                  <tr>
                    <xsl:if test="position() mod 5 = 0"><xsl:attribute name="class">fifth</xsl:attribute></xsl:if>
                    <td><xsl:value-of select="@num"/></td><td><xsl:value-of select="title"/></td><td><xsl:value-of select="artist"/></td>
                  </tr>
                </xsl:for-each>
              </tbody>
            </table>
    </body>
  </html>
</xsl:template>

</xsl:stylesheet>

