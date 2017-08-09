#ifndef JSONUTIL_H
#define JSONUTIL_H

#include "cJSON.h"
#include "json_type.h"
#include <stdlib.h>

namespace JsonUtil {

    string objectToString(const JsonType &obj);

    JsonType stringToObject(const string &json);

	JsonType json2obj(const cJSON *pJsonRoot);
}

#endif // JSONUTIL_H
