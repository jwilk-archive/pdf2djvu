<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
    <xsl:import href="http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl"/>
    <xsl:param name="man.charmap.use.subset" select="0"/>
    <xsl:param name="man.authors.section.enabled" select="0"/>
    <xsl:param name="local.l10n.xml" select="document('')"/>
    <l:i18n xmlns:l="http://docbook.sourceforge.net/xmlns/l10n/1.0">
        <l:l10n language="en">
            <l:context name="datetime"><l:template name="format" text="Y-m-d"/></l:context>
        </l:l10n>
        <l:l10n language="pl">
            <l:context name="datetime"><l:template name="format" text="d.m.Y"/></l:context>
        </l:l10n>
        <l:l10n language="ru">
            <l:context name="datetime"><l:template name="format" text="d.m.Y"/></l:context>
        </l:l10n>
    </l:i18n>
</xsl:stylesheet>

<!-- vim:set ts=4 sts=4 sw=4 tw=120 et: -->
