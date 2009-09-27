/* Copyright © 2009 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include "i18n.hh"
#include "version.hh"
#include "xmp.hh"

#ifdef HAVE_LIBXSLT

#include <sstream>
#include <memory>

#include <libxml/xmlIO.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlsave.h>
#include <libxslt/transform.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xsltutils.h>

static void throw_xml_error()
{
  throw xmp::XmlError(xmlGetLastError()->message);
}

class Xsl;

class Xml
{
protected:
  xmlDocPtr c_xml;
  Xml(xmlDocPtr c_xml)
  : c_xml(c_xml)
  { }
public:
  explicit Xml(const std::string &data)
  {
    this->c_xml = xmlReadMemory(data.c_str(), data.length(), NULL, NULL, XML_PARSE_NONET);
    if (this->c_xml == NULL)
      throw_xml_error();
  }

  virtual ~Xml()
  {
    xmlFreeDoc(this->c_xml);
  }

  std::string serialize()
  {
    xmlBufferPtr buffer = xmlBufferCreate();
    if (buffer == NULL)
      throw_xml_error();
    xmlSaveCtxtPtr context = xmlSaveToBuffer(buffer, "ASCII", XML_SAVE_NO_DECL);
    if (context == NULL)
    {
      xmlBufferFree(buffer);
      throw_xml_error();
    }
    xmlSaveDoc(context, this->c_xml);
    xmlSaveClose(context);
    std::string result = reinterpret_cast<const char*>(buffer->content);
    xmlBufferFree(buffer);
    return result;
  }

  friend class Xsl;
};

class Xsl : public Xml
{
protected:
  xsltStylesheetPtr c_xsl;
public:
  explicit Xsl(const std::string &s)
  : Xml(s)
  {
    this->c_xsl = xsltParseStylesheetDoc(this->c_xml);
    if (this->c_xsl == NULL)
      throw_xml_error();
  }

  virtual ~Xsl()
  {
    this->c_xml = NULL;
    xsltFreeStylesheet(this->c_xsl);
  }

  Xml *transform(const Xml &xml, const char **params)
  {
    xmlDocPtr c_document;
    c_document = xsltApplyStylesheet(this->c_xsl, xml.c_xml, params);
    if (c_document == NULL)
      throw_xml_error();
    return new Xml(c_document);
  }
};

class XmlEnvironment
{
public:
  XmlEnvironment()
  {
    LIBXML_TEST_VERSION
  }

  ~XmlEnvironment()
  {
    xsltCleanupGlobals();
    xmlCleanupParser();
  }
};

static void string_as_xpath(const std::string &string, std::ostream &stream)
{
  char quote = '"';
  bool first = true;
  std::string::const_iterator left = string.begin();
  std::string::const_iterator right = string.begin();
  while (right != string.end())
  {
    if (*right != quote)
      right++;
    else
    {
      if (right > left)
      {
        if (first)
        {
          stream << "concat(";
          first = false;
        }
        else
          stream << ',';
        stream << quote;
        stream << string.substr(left - string.begin(), right - left);
        stream << quote;
        left = right;
      }
      quote ^= ('"' ^ '\'');
    }
  }
  if (!first)
    stream << ',';
  stream << quote;
  stream << string.substr(left - string.begin(), right - left);
  stream << quote;
  right++;
  left = right;
  if (!first)
    stream << ')';
}

static std::string string_as_xpath(const std::string &string)
{
  std::ostringstream stream;
  string_as_xpath(string, stream);
  return stream.str();
}

#include "xmp-xslt.hh"

std::string xmp::transform(const std::string &data)
{
  std::string result;
  XmlEnvironment xml_environment;
  {
    static const std::string xpath_producer = string_as_xpath(PACKAGE_STRING);
    static const char *params[2] = { "djvu-producer", xpath_producer.c_str() };
    Xml xmp(data);
    Xsl xsl(xmp::xslt);
    std::auto_ptr<Xml> transformed_data;
    transformed_data.reset(xsl.transform(xmp, params));
    result = transformed_data->serialize();
  }
  return result;
}

#else

std::string xmp::transform(const std::string &data)
{
  throw XmlError(_("XML transformations are not supported"));
}

#endif

// vim:ts=2 sw=2 et
