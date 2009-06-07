<?xml version="1.0"?>

<!-- Use xmlstarlet tr songlist-to-xhtml.xml songlist.xml > songlist.tex -->
<!-- Then compile the tex into pdf with pdflatex songlist.tex -->

<xsl:stylesheet  version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:output method="text" encoding="UTF-8" use-character-maps="tex"/>

<!-- LaTex escaping, doesn't work with xmlstarlet :/
<xsl:character-map name="tex">
  <xsl:output-character character="&lt;" string="\textless{}"/>
  <xsl:output-character character="&gt;" string="\textgreater{}"/>
  <xsl:output-character character="~" string="\textasciitilde{}"/>
  <xsl:output-character character="^" string="\textasciicircum{}"/>
  <xsl:output-character character="&amp;" string="\&amp;"/>
  <xsl:output-character character="#" string="\#"/>
  <xsl:output-character character="_" string="\_"/>
  <xsl:output-character character="$" string="\$"/>
  <xsl:output-character character="%" string="\%"/>
  <xsl:output-character character="|" string="\docbooktolatexpipe{}"/>
  <xsl:output-character character="{" string="\{"/>
  <xsl:output-character character="}" string="\}"/>
  <xsl:output-character character="\" string="\textbackslash "/>
</xsl:character-map>
-->

<xsl:template match="/songlist">

\documentclass[a4paper,6pt]{report}
\usepackage[utf8]{inputenc}
\usepackage{graphicx}
\usepackage{multicol}
\oddsidemargin -0.5in
\textwidth 185mm
\begin{document}
\tiny
\begin{multicols}{4}
<xsl:for-each select="song">
\begin{minipage}{45 mm}
<xsl:choose>
<xsl:when test="cover">\includegraphics[width=1cm]{<xsl:value-of select="cover"/>}</xsl:when>
<xsl:otherwise>\hspace{1cm}</xsl:otherwise>
</xsl:choose>
\begin{minipage}{35 cm}
\textbf{<xsl:value-of select="title"/>}
\newline
<xsl:value-of select="artist"/>
\end{minipage}
\end{minipage}
</xsl:for-each>
\end{multicols}
\end{document}

</xsl:template>

</xsl:stylesheet>

