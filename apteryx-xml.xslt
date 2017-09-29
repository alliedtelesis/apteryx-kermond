<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output omit-xml-declaration="yes" method="xml" encoding="UTF-8" indent="yes"/>
	<xsl:strip-space elements="*"/>

	<xsl:template match="*[name()='module']">
		<MODULE xmlns:apteryx="https://github.com/alliedtelesis/apteryx" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
		xsi:schemaLocation="https://github.com/alliedtelesis/apteryx
		https://github.com/alliedtelesis/apteryx/releases/download/v2.10/apteryx.xsd">
		<xsl:apply-templates select="node()|@*"/>
		</MODULE>
	</xsl:template>

	<xsl:template match="*[name()='augment' and @target-node = '/if:interfaces/if:interface']">
		<NODE name="interfaces" help="Interface List">
			<NODE name="*" help="Interface Name">
				<xsl:apply-templates select="node()|@*"/>
			</NODE>
		</NODE>
	</xsl:template>

	<xsl:template match="*[name()='augment' and @target-node = '/if:interfaces-state/if:interface']">
		<NODE name="interfaces-state" help="Interface List">
			<NODE name="*" help="Interface Name">
				<xsl:apply-templates select="node()|@*"/>
			</NODE>
		</NODE>
	</xsl:template>

	<xsl:template match="*[name()='container' or name()='leaf' or name()='list']">
		<NODE>
		<xsl:attribute name="name"><xsl:value-of select="@name"/></xsl:attribute>
		<xsl:for-each select="child::*">
			<xsl:if test="name(parent::*) = 'leaf'">
				<xsl:choose>
					<xsl:when test="preceding::*[name() = 'config']/@value = 'false'">
						<xsl:attribute name="mode">r</xsl:attribute>
					</xsl:when>
					<xsl:otherwise>
						<xsl:attribute name="mode">rw</xsl:attribute>
					</xsl:otherwise>
				</xsl:choose>
			</xsl:if>
			<xsl:if test="name() = 'description'">
				<xsl:attribute name="help"><xsl:value-of select="."/></xsl:attribute>
			</xsl:if>
			<xsl:if test="name() = 'default'">
				<xsl:attribute name="default"><xsl:value-of select="@value"/></xsl:attribute>
			</xsl:if>
		</xsl:for-each>
		<xsl:if test="name() = 'list'">
			<NODE name="*">
			<xsl:attribute name="help">
				<xsl:if test="child::*/@value != ''">
					<xsl:value-of select="concat('The ', @name, ' entry with key ', child::*/@value, '.')"/>
				</xsl:if>
				<xsl:if test="child::*/@value = ''">
					<xsl:value-of select="concat('The ', @name, ' entry', '.')"/>
				</xsl:if>
			</xsl:attribute>
			<xsl:apply-templates select="node()|@*"/>
			</NODE>
		</xsl:if>
		<xsl:if test="name() != 'list'">
			<xsl:apply-templates select="node()|@*"/>
		</xsl:if>
		</NODE>
	</xsl:template>

	<xsl:template match="*[name(parent::*) = 'leaf' and name()='type' and @name='enumeration']">
		<xsl:for-each select="child::*">
			<xsl:if test="name() = 'enum'">
				<VALUE>
				<xsl:attribute name="name"><xsl:value-of select="@name"/></xsl:attribute>
				<xsl:attribute name="value"><xsl:value-of select="@name"/></xsl:attribute>
				<xsl:if test="name(child::*) = 'description'">
					<xsl:attribute name="help"><xsl:value-of select="."/></xsl:attribute>
				</xsl:if>
				</VALUE>
			</xsl:if>
		</xsl:for-each>
	</xsl:template>

	<xsl:template match="node()|@*">
		<xsl:apply-templates select="node()|@*"/>
	</xsl:template>

	<xsl:template match="text()|@*">
	</xsl:template>

</xsl:stylesheet>
