#ifndef UNIQUE_JSON_HPP
#define UNIQUE_JSON_HPP

#include "jansson.h"
#include <memory>

struct json_deleter
{
  void operator () (json_t *J)
  {
    json_decref(J);
  }
};

using unique_json = std::unique_ptr<json_t, json_deleter>;

#endif
