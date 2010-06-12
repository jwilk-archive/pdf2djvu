/* Copyright © 2009, 2010 Jakub Wilk
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991.
 */

#include "i18n.hh"
#include "version.hh"
#include "xmp.hh"

#if HAVE_LIBXSLT

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
  xmlErrorPtr error = xmlGetLastError();
  if (error != NULL)
    throw xmp::XmlError(error->message);
  else
    throw xmp::XmlError(_("Unknown error"));
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

  Xml *transform(const Xml &xml, const std::vector<std::string> &params)
  {
    xmlDocPtr c_document;
    const char **c_params = new const char*[params.size() + 1];
    for (size_t i = 0; i < params.size(); i++)
      c_params[i] = params[i].c_str();
    c_params[params.size()] = NULL;
    c_document = xsltApplyStylesheet(this->c_xsl, xml.c_xml, c_params);
    delete[] c_params;
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

static inline void char_as_xpath(char c, std::ostream &stream)
{
  if (c < 0 || c >= ' ' || c == '\r' || c == '\n' || c == '\t')
    stream << c;
  else
  {
    /* These are invalid characters for XML documents.
     * Replace them with U+FFFD (replacement character).
     *
     * See
     * http://www.w3.org/TR/REC-xml/#charsets
     * for details.
     */
    stream << "\xef\xbf\xbd";
  }
}

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
        while (left < right)
          char_as_xpath(*left++, stream);
        stream << quote;
        left = right;
      }
      quote ^= ('"' ^ '\'');
    }
  }
  if (!first)
    stream << ',';
  stream << quote;
  while (left < right)
    char_as_xpath(*left++, stream);
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
#include "xmp-dummy.hh"

static std::string pdf_key_to_xslt_key(const std::string &string)
{
  std::string result = "pdf";
  for (std::string::const_iterator it = string.begin(); it != string.end(); it++)
  {
    if (*it >= 'A' && *it <= 'Z')
    {
      result += '-';
      result += *it - 'A' + 'a';
    }
    else
      result += *it;
  }
  return result;
}

static void add_meta_string(const char *key, const std::string &value, std::vector<std::string> &params)
{
  params.push_back(pdf_key_to_xslt_key(key));
  params.push_back(string_as_xpath(value));
}

static void add_meta_date(const char *key, const pdf::Timestamp &value, std::vector<std::string> &params)
{
  std::string string_value;
  try
  {
    string_value = value.format('T');
  }
  catch (pdf::Timestamp::Invalid)
  {
    /* Ignore the error. User should be warned somewhere else anyway. */
    return;
  }
  params.push_back(pdf_key_to_xslt_key(key));
  params.push_back(string_as_xpath(string_value));
}

std::string xmp::transform(const std::string &data, const pdf::Metadata &metadata)
{
  std::string result;
  XmlEnvironment xml_environment;
  {
    std::vector<std::string> params;
    params.push_back("djvu-producer");
    params.push_back(string_as_xpath(PACKAGE_STRING));
    params.push_back("now");
    params.push_back(string_as_xpath(pdf::Timestamp::now().format('T')));
    metadata.iterate<std::vector<std::string> >(add_meta_string, add_meta_date, params);
    Xml xmp(data.length() > 0 ? data : dummy_xmp);
    Xsl xsl(xmp::xslt);
    std::auto_ptr<Xml> transformed_data;
    transformed_data.reset(xsl.transform(xmp, params));
    result = transformed_data->serialize();
  }
  return result;
}

#else

std::string xmp::transform(const std::string &data, const pdf::Metadata &metadata)
{
  throw XmlError(_("pdf2djvu was built without GNOME XSLT; XML transformations are disabled."));
}

#endif

// vim:ts=2 sw=2 et
