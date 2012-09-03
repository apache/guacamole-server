<?xml version="1.0" encoding="UTF-8"?>

<!-- Stylesheet for transforming Doxygen output to DocBook 5.0 -->
<xsl:stylesheet
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl"
    version="1.0">

    <xsl:output method="xml" indent="yes"
        doctype-public="-//OASIS//DTD DocBook V5.0//EN"
        doctype-system="http://docbook.org/xml/5.0/dtd/docbook.dtd"/>

    <xsl:template match="/doxygen">
        <reference>
            <xsl:apply-templates select="//memberdef[@kind='function']"/>
        </reference>
    </xsl:template>

    <xsl:template match="memberdef[@kind='function']">

        <xsl:variable name="name" select="name"/>

        <refentry id="{$name}">

            <indexterm>
                <primary><xsl:value-of select="name"/></primary>
            </indexterm>

            <refmeta>
                <refentrytitle><xsl:value-of select="name"/></refentrytitle>
            </refmeta>

            <refsynopsisdiv>
                <funcsynopsis>
                    <funcprototype>
                        <funcdef>
                            <xsl:value-of select="type"/>
                            <xsl:text> </xsl:text>
                            <function>
                                <xsl:value-of select="name"/>
                            </function>
                        </funcdef>
                        <xsl:for-each select="param">
                            <paramdef>
                                <xsl:value-of select="type"/>
                                <parameter>
                                    <xsl:value-of select="declname"/>
                                </parameter>
                            </paramdef>
                        </xsl:for-each>
                    </funcprototype>
                </funcsynopsis>
            </refsynopsisdiv>

            <refnamediv>
                <refname><xsl:value-of select="name"/></refname>
                <refpurpose>
                    <xsl:value-of select="briefdescription/para"/>
                </refpurpose>
            </refnamediv>

            <refsection>
                <title>Description</title>
                <para>
                    <xsl:value-of select="briefdescription/para"/>
                    <xsl:for-each select="detaileddescription/para[not(parameterlist)]">
                        <xsl:value-of select="."/>
                    </xsl:for-each>
                </para>
            </refsection>

            <refsection>
                <title>Parameters</title>

                <variablelist>
                    <xsl:for-each select=".//parameterlist/parameteritem">
                        <varlistentry>
                            <term>
                                <parameter>
                                    <xsl:value-of select="parameternamelist/parametername"/>
                                </parameter>
                            </term>
                            <listitem>
                                <xsl:for-each select="parameterdescription/para">
                                    <para><xsl:value-of select="."/></para>
                                </xsl:for-each>
                            </listitem>
                        </varlistentry>
                     </xsl:for-each>
                 </variablelist>

            </refsection>


        </refentry>
    </xsl:template>

</xsl:stylesheet>
