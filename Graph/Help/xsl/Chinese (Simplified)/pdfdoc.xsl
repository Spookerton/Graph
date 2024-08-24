<xsl:stylesheet
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 xmlns:fo="http://www.w3.org/1999/XSL/Format"
 version="1.0">

  <xsl:import href="../pdfdoc.xsl"/>

  <xsl:param name="l10n.gentext.language" select="'zh_cn'"/>

  <xsl:param name="body.font.family" select="'simhei'"></xsl:param>
  <xsl:param name="monospace.font.family" select="'simhei'"></xsl:param>
  <xsl:param name="title.font.family" select="'simhei'"></xsl:param>

  <xsl:param name="hyphenate">false</xsl:param>

  <xsl:template match="guibutton|guimenu|guisubmenu|guimenuitem">
    <fo:inline font-family="simhei">
      <xsl:call-template name="inline.charseq"/>
    </fo:inline>
  </xsl:template>

  <xsl:template match="menuchoice">
    <fo:inline font-family="simhei">
      <xsl:call-template name="process.menuchoice"/>
    </fo:inline>
  </xsl:template>

  <xsl:template match="guilabel">
    <fo:inline font-family="simhei">
      <xsl:call-template name="inline.charseq"/>
    </fo:inline>
  </xsl:template>

  <xsl:template match="guibutton">
    <fo:inline font-family="simhei">
      <xsl:call-template name="inline.charseq"/>
    </fo:inline>
  </xsl:template>

  <xsl:template match="keysym">
    <fo:inline font-family="Symbol">
      <xsl:call-template name="inline.charseq"/>
    </fo:inline>
  </xsl:template>

 <xsl:template name="itemizedlist.label.markup">
  <xsl:param name="itemsymbol" select="'disc'"/>
  <fo:inline font-family="Helvetica">&#x2022;</fo:inline>
</xsl:template>
 
</xsl:stylesheet>
