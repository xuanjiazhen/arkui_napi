#include <napi.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>
#include <algorithm>

namespace {

// Basic arithmetic functions
napi_value Add(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 2) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing arguments");
    return nullptr;
  }

  double a, b;
  napi_get_value_double(env, args[0], &a);
  napi_get_value_double(env, args[1], &b);

  napi_value result;
  napi_create_number(env, a + b, &result);
  return result;
}

napi_value Subtract(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 2) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing arguments");
    return nullptr;
  }

  double a, b;
  napi_get_value_double(env, args[0], &a);
  napi_get_value_double(env, args[1], &b);

  napi_value result;
  napi_create_number(env, a - b, &result);
  return result;
}

napi_value Multiply(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 2) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing arguments");
    return nullptr;
  }

  double a, b;
  napi_get_value_double(env, args[0], &a);
  napi_get_value_double(env, args[1], &b);

  napi_value result;
  napi_create_number(env, a * b, &result);
  return result;
}

napi_value Divide(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 2) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing arguments");
    return nullptr;
  }

  double a, b;
  napi_get_value_double(env, args[0], &a);
  napi_get_value_double(env, args[1], &b);

  if (b == 0) {
    napi_throw_error(env, "ERR_DIVISION_BY_ZERO", "Division by zero");
    return nullptr;
  }

  napi_value result;
  napi_create_number(env, a / b, &result);
  return result;
}

napi_value Modulo(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 2) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing arguments");
    return nullptr;
  }

  double a, b;
  napi_get_value_double(env, args[0], &a);
  napi_get_value_double(env, args[1], &b);

  if (b == 0) {
    napi_throw_error(env, "ERR_DIVISION_BY_ZERO", "Division by zero");
    return nullptr;
  }

  napi_value result;
  napi_create_number(env, fmod(a, b), &result);
  return result;
}

napi_value Power(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 2) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing arguments");
    return nullptr;
  }

  double base, exponent;
  napi_get_value_double(env, args[0], &base);
  napi_get_value_double(env, args[1], &exponent);

  napi_value result;
  napi_create_number(env, pow(base, exponent), &result);
  return result;
}

napi_value SquareRoot(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing argument");
    return nullptr;
  }

  double value;
  napi_get_value_double(env, args[0], &value);

  if (value < 0) {
    napi_throw_error(env, "ERR_INVALID_ARGUMENT", "Cannot take square root of negative number");
    return nullptr;
  }

  napi_value result;
  napi_create_number(env, sqrt(value), &result);
  return result;
}

// Trigonometric functions
napi_value Sin(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing argument");
    return nullptr;
  }

  double value;
  napi_get_value_double(env, args[0], &value);

  napi_value result;
  napi_create_number(env, sin(value), &result);
  return result;
}

napi_value Cos(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing argument");
    return nullptr;
  }

  double value;
  napi_get_value_double(env, args[0], &value);

  napi_value result;
  napi_create_number(env, cos(value), &result);
  return result;
}

napi_value Tan(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing argument");
    return nullptr;
  }

  double value;
  napi_get_value_double(env, args[0], &value);

  napi_value result;
  napi_create_number(env, tan(value), &result);
  return result;
}

napi_value Asin(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing argument");
    return nullptr;
  }

  double value;
  napi_get_value_double(env, args[0], &value);

  if (value < -1 || value > 1) {
    napi_throw_error(env, "ERR_INVALID_ARGUMENT", "Value must be between -1 and 1");
    return nullptr;
  }

  napi_value result;
  napi_create_number(env, asin(value), &result);
  return result;
}

napi_value Acos(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing argument");
    return nullptr;
  }

  double value;
  napi_get_value_double(env, args[0], &value);

  if (value < -1 || value > 1) {
    napi_throw_error(env, "ERR_INVALID_ARGUMENT", "Value must be between -1 and 1");
    return nullptr;
  }

  napi_value result;
  napi_create_number(env, acos(value), &result);
  return result;
}

napi_value Atan(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing argument");
    return nullptr;
  }

  double value;
  napi_get_value_double(env, args[0], &value);

  napi_value result;
  napi_create_number(env, atan(value), &result);
  return result;
}

napi_value Atan2(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 2) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing arguments");
    return nullptr;
  }

  double y, x;
  napi_get_value_double(env, args[0], &y);
  napi_get_value_double(env, args[1], &x);

  napi_value result;
  napi_create_number(env, atan2(y, x), &result);
  return result;
}

// Exponential and logarithmic functions
napi_value Exp(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing argument");
    return nullptr;
  }

  double value;
  napi_get_value_double(env, args[0], &value);

  napi_value result;
  napi_create_number(env, exp(value), &result);
  return result;
}

napi_value Log(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing argument");
    return nullptr;
  }

  double value;
  napi_get_value_double(env, args[0], &value);

  if (value <= 0) {
    napi_throw_error(env, "ERR_INVALID_ARGUMENT", "Value must be positive");
    return nullptr;
  }

  napi_value result;
  napi_create_number(env, log(value), &result);
  return result;
}

napi_value Log10(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing argument");
    return nullptr;
  }

  double value;
  napi_get_value_double(env, args[0], &value);

  if (value <= 0) {
    napi_throw_error(env, "ERR_INVALID_ARGUMENT", "Value must be positive");
    return nullptr;
  }

  napi_value result;
  napi_create_number(env, log10(value), &result);
  return result;
}

napi_value Log2(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing argument");
    return nullptr;
  }

  double value;
  napi_get_value_double(env, args[0], &value);

  if (value <= 0) {
    napi_throw_error(env, "ERR_INVALID_ARGUMENT", "Value must be positive");
    return nullptr;
  }

  napi_value result;
  napi_create_number(env, log2(value), &result);
  return result;
}

// Statistical functions
napi_value Min(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing array");
    return nullptr;
  }

  bool isArray;
  napi_is_array(env, args[0], &isArray);
  if (!isArray) {
    napi_throw_error(env, "ERR_INVALID_TYPE", "Argument must be an array");
    return nullptr;
  }

  uint32_t length;
  napi_get_array_length(env, args[0], &length);

  if (length == 0) {
    napi_throw_error(env, "ERR_INVALID_ARGUMENT", "Array cannot be empty");
    return nullptr;
  }

  double minValue;
  napi_value element;
  napi_get_element(env, args[0], 0, &element);
  napi_get_value_double(env, element, &minValue);

  for (uint32_t i = 1; i < length; i++) {
    napi_get_element(env, args[0], i, &element);
    double value;
    napi_get_value_double(env, element, &value);
    if (value < minValue) {
      minValue = value;
    }
  }

  napi_value result;
  napi_create_number(env, minValue, &result);
  return result;
}

napi_value Max(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing array");
    return nullptr;
  }

  bool isArray;
  napi_is_array(env, args[0], &isArray);
  if (!isArray) {
    napi_throw_error(env, "ERR_INVALID_TYPE", "Argument must be an array");
    return nullptr;
  }

  uint32_t length;
  napi_get_array_length(env, args[0], &length);

  if (length == 0) {
    napi_throw_error(env, "ERR_INVALID_ARGUMENT", "Array cannot be empty");
    return nullptr;
  }

  double maxValue;
  napi_value element;
  napi_get_element(env, args[0], 0, &element);
  napi_get_value_double(env, element, &maxValue);

  for (uint32_t i = 1; i < length; i++) {
    napi_get_element(env, args[0], i, &element);
    double value;
    napi_get_value_double(env, element, &value);
    if (value > maxValue) {
      maxValue = value;
    }
  }

  napi_value result;
  napi_create_number(env, maxValue, &result);
  return result;
}

napi_value Sum(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing array");
    return nullptr;
  }

  bool isArray;
  napi_is_array(env, args[0], &isArray);
  if (!isArray) {
    napi_throw_error(env, "ERR_INVALID_TYPE", "Argument must be an array");
    return nullptr;
  }

  uint32_t length;
  napi_get_array_length(env, args[0], &length);

  double sum = 0;
  for (uint32_t i = 0; i < length; i++) {
    napi_value element;
    napi_get_element(env, args[0], i, &element);
    double value;
    napi_get_value_double(env, element, &value);
    sum += value;
  }

  napi_value result;
  napi_create_number(env, sum, &result);
  return result;
}

napi_value Average(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing array");
    return nullptr;
  }

  bool isArray;
  napi_is_array(env, args[0], &isArray);
  if (!isArray) {
    napi_throw_error(env, "ERR_INVALID_TYPE", "Argument must be an array");
    return nullptr;
  }

  uint32_t length;
  napi_get_array_length(env, args[0], &length);

  if (length == 0) {
    napi_throw_error(env, "ERR_INVALID_ARGUMENT", "Array cannot be empty");
    return nullptr;
  }

  double sum = 0;
  for (uint32_t i = 0; i < length; i++) {
    napi_value element;
    napi_get_element(env, args[0], i, &element);
    double value;
    napi_get_value_double(env, element, &value);
    sum += value;
  }

  napi_value result;
  napi_create_number(env, sum / length, &result);
  return result;
}

napi_value Median(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing array");
    return nullptr;
  }

  bool isArray;
  napi_is_array(env, args[0], &isArray);
  if (!isArray) {
    napi_throw_error(env, "ERR_INVALID_TYPE", "Argument must be an array");
    return nullptr;
  }

  uint32_t length;
  napi_get_array_length(env, args[0], &length);

  if (length == 0) {
    napi_throw_error(env, "ERR_INVALID_ARGUMENT", "Array cannot be empty");
    return nullptr;
  }

  std::vector<double> values;
  for (uint32_t i = 0; i < length; i++) {
    napi_value element;
    napi_get_element(env, args[0], i, &element);
    double value;
    napi_get_value_double(env, element, &value);
    values.push_back(value);
  }

  std::sort(values.begin(), values.end());

  double median;
  if (length % 2 == 0) {
    median = (values[length / 2 - 1] + values[length / 2]) / 2;
  } else {
    median = values[length / 2];
  }

  napi_value result;
  napi_create_number(env, median, &result);
  return result;
}

napi_value StandardDeviation(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing array");
    return nullptr;
  }

  bool isArray;
  napi_is_array(env, args[0], &isArray);
  if (!isArray) {
    napi_throw_error(env, "ERR_INVALID_TYPE", "Argument must be an array");
    return nullptr;
  }

  uint32_t length;
  napi_get_array_length(env, args[0], &length);

  if (length < 2) {
    napi_throw_error(env, "ERR_INVALID_ARGUMENT", "Array must have at least 2 elements");
    return nullptr;
  }

  double sum = 0;
  for (uint32_t i = 0; i < length; i++) {
    napi_value element;
    napi_get_element(env, args[0], i, &element);
    double value;
    napi_get_value_double(env, element, &value);
    sum += value;
  }
  double mean = sum / length;

  double variance = 0;
  for (uint32_t i = 0; i < length; i++) {
    napi_value element;
    napi_get_element(env, args[0], i, &element);
    double value;
    napi_get_value_double(env, element, &value);
    variance += pow(value - mean, 2);
  }
  variance /= (length - 1);

  napi_value result;
  napi_create_number(env, sqrt(variance), &result);
  return result;
}

// Random number generation
napi_value Random(napi_env env, napi_callback_info info) {
  napi_value result;
  napi_create_number(env, static_cast<double>(rand()) / RAND_MAX, &result);
  return result;
}

napi_value RandomInt(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 2) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing arguments");
    return nullptr;
  }

  int min, max;
  napi_get_value_int32(env, args[0], &min);
  napi_get_value_int32(env, args[1], &max);

  if (min >= max) {
    napi_throw_error(env, "ERR_INVALID_ARGUMENT", "Min must be less than max");
    return nullptr;
  }

  int randomValue = min + (rand() % (max - min + 1));
  napi_value result;
  napi_create_number(env, randomValue, &result);
  return result;
}

napi_value SeedRandom(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    // Seed with current time if no seed provided
    srand(static_cast<unsigned int>(time(nullptr)));
  } else {
    int seed;
    napi_get_value_int32(env, args[0], &seed);
    srand(static_cast<unsigned int>(seed));
  }

  napi_value result;
  napi_get_undefined(env, &result);
  return result;
}

// Linear algebra functions
napi_value DotProduct(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 2) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing arrays");
    return nullptr;
  }

  bool isArray1, isArray2;
  napi_is_array(env, args[0], &isArray1);
  napi_is_array(env, args[1], &isArray2);
  if (!isArray1 || !isArray2) {
    napi_throw_error(env, "ERR_INVALID_TYPE", "Both arguments must be arrays");
    return nullptr;
  }

  uint32_t length1, length2;
  napi_get_array_length(env, args[0], &length1);
  napi_get_array_length(env, args[1], &length2);

  if (length1 != length2) {
    napi_throw_error(env, "ERR_INVALID_ARGUMENT", "Arrays must have the same length");
    return nullptr;
  }

  double product = 0;
  for (uint32_t i = 0; i < length1; i++) {
    napi_value element1, element2;
    napi_get_element(env, args[0], i, &element1);
    napi_get_element(env, args[1], i, &element2);
    double value1, value2;
    napi_get_value_double(env, element1, &value1);
    napi_get_value_double(env, element2, &value2);
    product += value1 * value2;
  }

  napi_value result;
  napi_create_number(env, product, &result);
  return result;
}

// Geometry functions
napi_value Distance(napi_env env, napi_callback_info info) {
  size_t argc = 4;
  napi_value args[4];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 4) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing coordinates");
    return nullptr;
  }

  double x1, y1, x2, y2;
  napi_get_value_double(env, args[0], &x1);
  napi_get_value_double(env, args[1], &y1);
  napi_get_value_double(env, args[2], &x2);
  napi_get_value_double(env, args[3], &y2);

  double distance = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
  napi_value result;
  napi_create_number(env, distance, &result);
  return result;
}

napi_value AreaOfCircle(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing radius");
    return nullptr;
  }

  double radius;
  napi_get_value_double(env, args[0], &radius);

  if (radius < 0) {
    napi_throw_error(env, "ERR_INVALID_ARGUMENT", "Radius must be non-negative");
    return nullptr;
  }

  double area = M_PI * pow(radius, 2);
  napi_value result;
  napi_create_number(env, area, &result);
  return result;
}

napi_value VolumeOfSphere(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing radius");
    return nullptr;
  }

  double radius;
  napi_get_value_double(env, args[0], &radius);

  if (radius < 0) {
    napi_throw_error(env, "ERR_INVALID_ARGUMENT", "Radius must be non-negative");
    return nullptr;
  }

  double volume = (4.0 / 3.0) * M_PI * pow(radius, 3);
  napi_value result;
  napi_create_number(env, volume, &result);
  return result;
}

// Number theory functions
napi_value IsPrime(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing number");
    return nullptr;
  }

  int n;
  napi_get_value_int32(env, args[0], &n);

  if (n <= 1) {
    napi_value result;
    napi_create_boolean(env, false, &result);
    return result;
  }

  for (int i = 2; i * i <= n; i++) {
    if (n % i == 0) {
      napi_value result;
      napi_create_boolean(env, false, &result);
      return result;
    }
  }

  napi_value result;
  napi_create_boolean(env, true, &result);
  return result;
}

napi_value GCD(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 2) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing arguments");
    return nullptr;
  }

  int a, b;
  napi_get_value_int32(env, args[0], &a);
  napi_get_value_int32(env, args[1], &b);

  a = abs(a);
  b = abs(b);

  while (b != 0) {
    int temp = b;
    b = a % b;
    a = temp;
  }

  napi_value result;
  napi_create_number(env, a, &result);
  return result;
}

napi_value LCM(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 2) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing arguments");
    return nullptr;
  }

  int a, b;
  napi_get_value_int32(env, args[0], &a);
  napi_get_value_int32(env, args[1], &b);

  int gcd = 0;
  int tempA = abs(a);
  int tempB = abs(b);
  while (tempB != 0) {
    int temp = tempB;
    tempB = tempA % tempB;
    tempA = temp;
  }
  gcd = tempA;

  int lcm = (abs(a) / gcd) * abs(b);
  napi_value result;
  napi_create_number(env, lcm, &result);
  return result;
}

// Financial functions
napi_value PresentValue(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value args[3];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 3) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing arguments");
    return nullptr;
  }

  double rate, payment, periods;
  napi_get_value_double(env, args[0], &rate);
  napi_get_value_double(env, args[1], &payment);
  napi_get_value_double(env, args[2], &periods);

  if (rate == 0) {
    napi_value result;
    napi_create_number(env, payment * periods, &result);
    return result;
  }

  double pv = payment * (1 - pow(1 + rate, -periods)) / rate;
  napi_value result;
  napi_create_number(env, pv, &result);
  return result;
}

napi_value FutureValue(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value args[3];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 3) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing arguments");
    return nullptr;
  }

  double rate, payment, periods;
  napi_get_value_double(env, args[0], &rate);
  napi_get_value_double(env, args[1], &payment);
  napi_get_value_double(env, args[2], &periods);

  double fv = payment * (pow(1 + rate, periods) - 1) / rate;
  napi_value result;
  napi_create_number(env, fv, &result);
  return result;
}

// Utility functions
napi_value Abs(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing argument");
    return nullptr;
  }

  double value;
  napi_get_value_double(env, args[0], &value);

  napi_value result;
  napi_create_number(env, fabs(value), &result);
  return result;
}

napi_value Round(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing argument");
    return nullptr;
  }

  double value;
  napi_get_value_double(env, args[0], &value);

  napi_value result;
  napi_create_number(env, round(value), &result);
  return result;
}

napi_value Floor(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing argument");
    return nullptr;
  }

  double value;
  napi_get_value_double(env, args[0], &value);

  napi_value result;
  napi_create_number(env, floor(value), &result);
  return result;
}

napi_value Ceil(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing argument");
    return nullptr;
  }

  double value;
  napi_get_value_double(env, args[0], &value);

  napi_value result;
  napi_create_number(env, ceil(value), &result);
  return result;
}

napi_value Trunc(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing argument");
    return nullptr;
  }

  double value;
  napi_get_value_double(env, args[0], &value);

  napi_value result;
  napi_create_number(env, trunc(value), &result);
  return result;
}

napi_value Sign(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing argument");
    return nullptr;
  }

  double value;
  napi_get_value_double(env, args[0], &value);

  double signValue;
  if (value > 0) {
    signValue = 1;
  } else if (value < 0) {
    signValue = -1;
  } else {
    signValue = 0;
  }

  napi_value result;
  napi_create_number(env, signValue, &result);
  return result;
}

} // namespace

extern "C" {

napi_value Init(napi_env env, napi_value exports) {
  // Seed random number generator
  srand(static_cast<unsigned int>(time(nullptr)));

  napi_property_descriptor descriptors[] = {
    // Basic arithmetic
    { "add", nullptr, Add, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "subtract", nullptr, Subtract, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "multiply", nullptr, Multiply, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "divide", nullptr, Divide, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "modulo", nullptr, Modulo, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "power", nullptr, Power, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "sqrt", nullptr, SquareRoot, nullptr, nullptr, nullptr, napi_default, nullptr },
    
    // Trigonometric
    { "sin", nullptr, Sin, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "cos", nullptr, Cos, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "tan", nullptr, Tan, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "asin", nullptr, Asin, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "acos", nullptr, Acos, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "atan", nullptr, Atan, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "atan2", nullptr, Atan2, nullptr, nullptr, nullptr, napi_default, nullptr },
    
    // Exponential and logarithmic
    { "exp", nullptr, Exp, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "log", nullptr, Log, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "log10", nullptr, Log10, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "log2", nullptr, Log2, nullptr, nullptr, nullptr, napi_default, nullptr },
    
    // Statistical
    { "min", nullptr, Min, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "max", nullptr, Max, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "sum", nullptr, Sum, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "average", nullptr, Average, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "median", nullptr, Median, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "stdDev", nullptr, StandardDeviation, nullptr, nullptr, nullptr, napi_default, nullptr },
    
    // Random
    { "random", nullptr, Random, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "randomInt", nullptr, RandomInt, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "seedRandom", nullptr, SeedRandom, nullptr, nullptr, nullptr, napi_default, nullptr },
    
    // Linear algebra
    { "dotProduct", nullptr, DotProduct, nullptr, nullptr, nullptr, napi_default, nullptr },
    
    // Geometry
    { "distance", nullptr, Distance, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "areaOfCircle", nullptr, AreaOfCircle, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "volumeOfSphere", nullptr, VolumeOfSphere, nullptr, nullptr, nullptr, napi_default, nullptr },
    
    // Number theory
    { "isPrime", nullptr, IsPrime, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "gcd", nullptr, GCD, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "lcm", nullptr, LCM, nullptr, nullptr, nullptr, napi_default, nullptr },
    
    // Financial
    { "presentValue", nullptr, PresentValue, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "futureValue", nullptr, FutureValue, nullptr, nullptr, nullptr, napi_default, nullptr },
    
    // Utility
    { "abs", nullptr, Abs, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "round", nullptr, Round, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "floor", nullptr, Floor, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "ceil", nullptr, Ceil, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "trunc", nullptr, Trunc, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "sign", nullptr, Sign, nullptr, nullptr, nullptr, napi_default, nullptr },
  };

  napi_define_properties(env, exports, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
  return exports;
}

NAPI_MODULE(native_module_math, Init)

}
