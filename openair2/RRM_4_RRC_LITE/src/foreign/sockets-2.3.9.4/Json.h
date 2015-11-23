#ifndef _SOCKETS_Json_H
#define _SOCKETS_Json_H

#include "Exception.h"
#include <list>
#include <map>

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE
{
#endif

class Json
{
public:
  typedef std::list<Json> json_list_t;
  typedef std::map<std::string, Json> json_map_t;

  typedef enum {
    TYPE_UNKNOWN = 0,
    TYPE_INTEGER,
    TYPE_REAL,
    TYPE_STRING,
    TYPE_BOOLEAN,
    TYPE_ARRAY,
    TYPE_OBJECT
  } json_type_t;

public:
  /** Default constructor */
  Json(); //m_type(TYPE_UNKNOWN);

  /** Basic type constructors */
  Json(char value);
  Json(short value);
  Json(long value); //m_type(TYPE_INTEGER), m_i_value(value);
  Json(long long value);
  // LG Json(int64_t value);
  Json(double value); //m_type(TYPE_REAL), m_d_value(value);
  Json(const char *value); //m_type(TYPE_STRING), m_str_value(value);
  Json(const std::string& value); //m_type(TYPE_STRING), m_str_value(value);
  Json(bool value); //m_type(TYPE_BOOLEAN), m_b_value(value);

  /** Complex type constructor (array, object) */
  Json(json_type_t t); //m_type(t);

  virtual ~Json();

  char Parse(const char *buffer, size_t& index);

  json_type_t Type() const;

  bool HasValue(const std::string& name) const;

  operator char() const;
  operator short() const;
  operator long() const;
  //operator int64_t() const;
  operator long long() const;
  operator double() const;
  operator std::string() const;
  operator bool() const;

  const Json& operator[](const char *name) const;
  Json& operator[](const char *name);

  const Json& operator[](const std::string& name) const;
  Json& operator[](const std::string& name);

  void Add(Json data);

  const std::string& GetString() const;

  const json_list_t& GetArray() const;
  json_list_t& GetArray();

  const json_map_t& GetObject() const;
  json_map_t& GetObject();

  std::string ToString(bool quote = true) const;

  static Json Parse(const std::string& data);

  void encode(std::string& src) const;
  void decode(std::string& src) const;
  std::string encode(const std::string&) const;

private:
  json_type_t m_type;
  long m_i_value;
  double m_d_value;
  std::string m_str_value;
  bool m_b_value;
  json_list_t m_array;
  json_map_t m_object;
};

#ifdef SOCKETS_NAMESPACE
} // namespace SOCKETS_NAMESPACE {
#endif

#endif // _SOCKETS_Json_H
