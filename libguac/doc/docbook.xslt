<?xml version="1.0" encoding="UTF-8"?>

<!-- Stylesheet for transforming Doxygen output to DocBook 5.0 -->
<xsl:stylesheet
    xmlns="http://docbook.org/ns/docbook"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl"
    version="1.0">

    <xsl:output method="xml" indent="yes"/>

    <xsl:template match="/doxygen">
        <reference version="5.0" xml:lang="en">
            <title>libguac</title>
            <xsl:apply-templates select="//memberdef[@kind='function']"/>
        </reference>
    </xsl:template>

    <xsl:template match="memberdef[@kind='function']">

        <xsl:variable name="name" select="name"/>

        <!-- Previously had  xml:id="{$name}" -->
        <refentry>

            <indexterm>
                <primary><xsl:value-of select="name"/></primary>
            </indexterm>

            <refmeta>
                <refentrytitle><xsl:value-of select="name"/></refentrytitle>
            </refmeta>

            <refnamediv>
                <refname><xsl:value-of select="name"/></refname>
                <refpurpose>
                    <xsl:value-of select="briefdescription/para"/>
                </refpurpose>
            </refnamediv>

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
                                <xsl:if test="declname">
                                    <xsl:text> </xsl:text>
                                    <parameter>
                                        <xsl:value-of select="declname"/>
                                    </parameter>
                                </xsl:if>
                            </paramdef>
                        </xsl:for-each>
                        <xsl:if test="not(param)">
                            <void/>
                        </xsl:if>
                    </funcprototype>
                </funcsynopsis>
            </refsynopsisdiv>

            <refsection>
                <title>Description</title>
                <para>
                    <xsl:value-of select="briefdescription/para"/>
                </para>
                <xsl:for-each select="detaileddescription/para[not(parameterlist|simplesect)]">
                    <para><xsl:value-of select="."/></para>
                </xsl:for-each>
            </refsection>

            <xsl:if test=".//parameterlist/parameteritem">
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
            </xsl:if>

            <xsl:for-each select="detaileddescription/para/simplesect[@kind='return']">
                <refsection>
                    <title>Return Value</title>
                    <xsl:for-each select="para">
                        <para><xsl:value-of select="."/></para>
                    </xsl:for-each>
                </refsection>
            </xsl:for-each>


        </refentry>
    </xsl:template>

</xsl:stylesheet>
