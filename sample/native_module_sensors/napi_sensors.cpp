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

namespace {

enum SensorType {
    SENSOR_TYPE_ACCELEROMETER = 1,
    SENSOR_TYPE_GYROSCOPE = 2,
    SENSOR_TYPE_MAGNETOMETER = 3,
    SENSOR_TYPE_LIGHT = 4,
    SENSOR_TYPE_TEMPERATURE = 5,
    SENSOR_TYPE_HUMIDITY = 6,
    SENSOR_TYPE_PRESSURE = 7,
    SENSOR_TYPE_PROXIMITY = 8,
    SENSOR_TYPE_HEART_RATE = 9,
    SENSOR_TYPE_STEP_COUNTER = 10
};

struct AccelerometerData {
    float x;
    float y;
    float z;
    int64_t timestamp;
};

struct GyroscopeData {
    float x;
    float y;
    float z;
    int64_t timestamp;
};

struct MagnetometerData {
    float x;
    float y;
    float z;
    int64_t timestamp;
};

struct LightData {
    float intensity;
    int64_t timestamp;
};

struct TemperatureData {
    float temperature;
    int64_t timestamp;
};

struct HumidityData {
    float humidity;
    int64_t timestamp;
};

struct PressureData {
    float pressure;
    int64_t timestamp;
};

struct ProximityData {
    float distance;
    int64_t timestamp;
};

struct HeartRateData {
    int rate;
    int64_t timestamp;
};

struct StepCounterData {
    int steps;
    int64_t timestamp;
};

constexpr float DEFAULT_ACCEL_RANGE = 10.0f;
constexpr float DEFAULT_GYRO_RANGE = 5.0f;
constexpr float DEFAULT_MAG_RANGE = 100.0f;
constexpr float MAX_LIGHT_INTENSITY = 10000.0f;
constexpr float MIN_TEMP = 10.0f;
constexpr float MAX_TEMP = 40.0f;
constexpr float MIN_HUMIDITY = 20.0f;
constexpr float MAX_HUMIDITY = 80.0f;
constexpr float MIN_PRESSURE = 980.0f;
constexpr float MAX_PRESSURE = 1020.0f;
constexpr float MAX_PROXIMITY = 10.0f;
constexpr int MIN_HEART_RATE = 60;
constexpr int MAX_HEART_RATE = 120;
constexpr int MAX_STEPS = 10000;
constexpr double DEFAULT_SAMPLING_RATE = 100.0;

class SensorManager {
public:
    SensorManager()
    {
        srand(static_cast<unsigned int>(time(nullptr)));
    }

    AccelerometerData getAccelerometerData()
    {
        AccelerometerData data;
        data.x = generateRandomValue(-DEFAULT_ACCEL_RANGE, DEFAULT_ACCEL_RANGE);
        data.y = generateRandomValue(-DEFAULT_ACCEL_RANGE, DEFAULT_ACCEL_RANGE);
        data.z = generateRandomValue(-DEFAULT_ACCEL_RANGE, DEFAULT_ACCEL_RANGE);
        data.timestamp = getCurrentTimestamp();
        return data;
    }

    GyroscopeData getGyroscopeData()
    {
        GyroscopeData data;
        data.x = generateRandomValue(-DEFAULT_GYRO_RANGE, DEFAULT_GYRO_RANGE);
        data.y = generateRandomValue(-DEFAULT_GYRO_RANGE, DEFAULT_GYRO_RANGE);
        data.z = generateRandomValue(-DEFAULT_GYRO_RANGE, DEFAULT_GYRO_RANGE);
        data.timestamp = getCurrentTimestamp();
        return data;
    }

    MagnetometerData getMagnetometerData()
    {
        MagnetometerData data;
        data.x = generateRandomValue(-DEFAULT_MAG_RANGE, DEFAULT_MAG_RANGE);
        data.y = generateRandomValue(-DEFAULT_MAG_RANGE, DEFAULT_MAG_RANGE);
        data.z = generateRandomValue(-DEFAULT_MAG_RANGE, DEFAULT_MAG_RANGE);
        data.timestamp = getCurrentTimestamp();
        return data;
    }

    LightData getLightData()
    {
        LightData data;
        data.intensity = generateRandomValue(0.0f, MAX_LIGHT_INTENSITY);
        data.timestamp = getCurrentTimestamp();
        return data;
    }

    TemperatureData getTemperatureData()
    {
        TemperatureData data;
        data.temperature = generateRandomValue(MIN_TEMP, MAX_TEMP);
        data.timestamp = getCurrentTimestamp();
        return data;
    }

    HumidityData getHumidityData()
    {
        HumidityData data;
        data.humidity = generateRandomValue(MIN_HUMIDITY, MAX_HUMIDITY);
        data.timestamp = getCurrentTimestamp();
        return data;
    }

    PressureData getPressureData()
    {
        PressureData data;
        data.pressure = generateRandomValue(MIN_PRESSURE, MAX_PRESSURE);
        data.timestamp = getCurrentTimestamp();
        return data;
    }

    ProximityData getProximityData()
    {
        ProximityData data;
        data.distance = generateRandomValue(0.0f, MAX_PROXIMITY);
        data.timestamp = getCurrentTimestamp();
        return data;
    }

    HeartRateData getHeartRateData()
    {
        HeartRateData data;
        data.rate = generateRandomInt(MIN_HEART_RATE, MAX_HEART_RATE);
        data.timestamp = getCurrentTimestamp();
        return data;
    }

    StepCounterData getStepCounterData()
    {
        StepCounterData data;
        data.steps = generateRandomInt(0, MAX_STEPS);
        data.timestamp = getCurrentTimestamp();
        return data;
    }

    bool isSensorAvailable(SensorType type)
    {
        return true;
    }

    bool calibrateSensor(SensorType type)
    {
        return true;
    }

private:
    float generateRandomValue(float min, float max)
    {
        return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
    }

    int generateRandomInt(int min, int max)
    {
        return min + rand() % (max - min + 1);
    }

    int64_t getCurrentTimestamp()
    {
        return static_cast<int64_t>(time(nullptr) * 1000);
    }
};

SensorManager sensorManager;

napi_value GetAccelerometerData(napi_env env, napi_callback_info info)
{
    AccelerometerData data = sensorManager.getAccelerometerData();

    napi_value result;
    napi_create_object(env, &result);

    napi_value x;
    napi_value y;
    napi_value z;
    napi_value timestamp;
    napi_create_number(env, data.x, &x);
    napi_create_number(env, data.y, &y);
    napi_create_number(env, data.z, &z);
    napi_create_number(env, data.timestamp, &timestamp);

    napi_set_named_property(env, result, "x", x);
    napi_set_named_property(env, result, "y", y);
    napi_set_named_property(env, result, "z", z);
    napi_set_named_property(env, result, "timestamp", timestamp);

    return result;
}

napi_value GetGyroscopeData(napi_env env, napi_callback_info info)
{
    GyroscopeData data = sensorManager.getGyroscopeData();

    napi_value result;
    napi_create_object(env, &result);

    napi_value x;
    napi_value y;
    napi_value z;
    napi_value timestamp;
    napi_create_number(env, data.x, &x);
    napi_create_number(env, data.y, &y);
    napi_create_number(env, data.z, &z);
    napi_create_number(env, data.timestamp, &timestamp);

    napi_set_named_property(env, result, "x", x);
    napi_set_named_property(env, result, "y", y);
    napi_set_named_property(env, result, "z", z);
    napi_set_named_property(env, result, "timestamp", timestamp);

    return result;
}

napi_value GetMagnetometerData(napi_env env, napi_callback_info info)
{
    MagnetometerData data = sensorManager.getMagnetometerData();

    napi_value result;
    napi_create_object(env, &result);

    napi_value x;
    napi_value y;
    napi_value z;
    napi_value timestamp;
    napi_create_number(env, data.x, &x);
    napi_create_number(env, data.y, &y);
    napi_create_number(env, data.z, &z);
    napi_create_number(env, data.timestamp, &timestamp);

    napi_set_named_property(env, result, "x", x);
    napi_set_named_property(env, result, "y", y);
    napi_set_named_property(env, result, "z", z);
    napi_set_named_property(env, result, "timestamp", timestamp);

    return result;
}

napi_value GetLightData(napi_env env, napi_callback_info info)
{
    LightData data = sensorManager.getLightData();

    napi_value result;
    napi_create_object(env, &result);

    napi_value intensity;
    napi_value timestamp;
    napi_create_number(env, data.intensity, &intensity);
    napi_create_number(env, data.timestamp, &timestamp);

    napi_set_named_property(env, result, "intensity", intensity);
    napi_set_named_property(env, result, "timestamp", timestamp);

    return result;
}

napi_value GetTemperatureData(napi_env env, napi_callback_info info)
{
    TemperatureData data = sensorManager.getTemperatureData();

    napi_value result;
    napi_create_object(env, &result);

    napi_value temperature;
    napi_value timestamp;
    napi_create_number(env, data.temperature, &temperature);
    napi_create_number(env, data.timestamp, &timestamp);

    napi_set_named_property(env, result, "temperature", temperature);
    napi_set_named_property(env, result, "timestamp", timestamp);

    return result;
}

napi_value GetHumidityData(napi_env env, napi_callback_info info)
{
    HumidityData data = sensorManager.getHumidityData();

    napi_value result;
    napi_create_object(env, &result);

    napi_value humidity;
    napi_value timestamp;
    napi_create_number(env, data.humidity, &humidity);
    napi_create_number(env, data.timestamp, &timestamp);

    napi_set_named_property(env, result, "humidity", humidity);
    napi_set_named_property(env, result, "timestamp", timestamp);

    return result;
}

napi_value GetPressureData(napi_env env, napi_callback_info info)
{
    PressureData data = sensorManager.getPressureData();

    napi_value result;
    napi_create_object(env, &result);

    napi_value pressure;
    napi_value timestamp;
    napi_create_number(env, data.pressure, &pressure);
    napi_create_number(env, data.timestamp, &timestamp);

    napi_set_named_property(env, result, "pressure", pressure);
    napi_set_named_property(env, result, "timestamp", timestamp);

    return result;
}

napi_value GetProximityData(napi_env env, napi_callback_info info)
{
    ProximityData data = sensorManager.getProximityData();

    napi_value result;
    napi_create_object(env, &result);

    napi_value distance;
    napi_value timestamp;
    napi_create_number(env, data.distance, &distance);
    napi_create_number(env, data.timestamp, &timestamp);

    napi_set_named_property(env, result, "distance", distance);
    napi_set_named_property(env, result, "timestamp", timestamp);

    return result;
}

napi_value GetHeartRateData(napi_env env, napi_callback_info info)
{
    HeartRateData data = sensorManager.getHeartRateData();

    napi_value result;
    napi_create_object(env, &result);

    napi_value rate;
    napi_value timestamp;
    napi_create_number(env, data.rate, &rate);
    napi_create_number(env, data.timestamp, &timestamp);

    napi_set_named_property(env, result, "rate", rate);
    napi_set_named_property(env, result, "timestamp", timestamp);

    return result;
}

napi_value GetStepCounterData(napi_env env, napi_callback_info info)
{
    StepCounterData data = sensorManager.getStepCounterData();

    napi_value result;
    napi_create_object(env, &result);

    napi_value steps;
    napi_value timestamp;
    napi_create_number(env, data.steps, &steps);
    napi_create_number(env, data.timestamp, &timestamp);

    napi_set_named_property(env, result, "steps", steps);
    napi_set_named_property(env, result, "timestamp", timestamp);

    return result;
}

napi_value IsSensorAvailable(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing sensor type");
        return nullptr;
    }

    int sensorType = 0;
    napi_get_value_int32(env, args[0], &sensorType);

    bool available = sensorManager.isSensorAvailable(static_cast<SensorType>(sensorType));
    napi_value result;
    napi_create_boolean(env, available, &result);
    return result;
}

napi_value CalibrateSensor(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing sensor type");
        return nullptr;
    }

    int sensorType = 0;
    napi_get_value_int32(env, args[0], &sensorType);

    bool success = sensorManager.calibrateSensor(static_cast<SensorType>(sensorType));
    napi_value result;
    napi_create_boolean(env, success, &result);
    return result;
}

napi_value GetSensorTypes(napi_env env, napi_callback_info info)
{
    napi_value result;
    napi_create_object(env, &result);

    napi_value accelerometer;
    napi_value gyroscope;
    napi_value magnetometer;
    napi_value light;
    napi_value temperature;
    napi_value humidity;
    napi_value pressure;
    napi_value proximity;
    napi_value heartRate;
    napi_value stepCounter;

    napi_create_number(env, SENSOR_TYPE_ACCELEROMETER, &accelerometer);
    napi_create_number(env, SENSOR_TYPE_GYROSCOPE, &gyroscope);
    napi_create_number(env, SENSOR_TYPE_MAGNETOMETER, &magnetometer);
    napi_create_number(env, SENSOR_TYPE_LIGHT, &light);
    napi_create_number(env, SENSOR_TYPE_TEMPERATURE, &temperature);
    napi_create_number(env, SENSOR_TYPE_HUMIDITY, &humidity);
    napi_create_number(env, SENSOR_TYPE_PRESSURE, &pressure);
    napi_create_number(env, SENSOR_TYPE_PROXIMITY, &proximity);
    napi_create_number(env, SENSOR_TYPE_HEART_RATE, &heartRate);
    napi_create_number(env, SENSOR_TYPE_STEP_COUNTER, &stepCounter);

    napi_set_named_property(env, result, "ACCELEROMETER", accelerometer);
    napi_set_named_property(env, result, "GYROSCOPE", gyroscope);
    napi_set_named_property(env, result, "MAGNETOMETER", magnetometer);
    napi_set_named_property(env, result, "LIGHT", light);
    napi_set_named_property(env, result, "TEMPERATURE", temperature);
    napi_set_named_property(env, result, "HUMIDITY", humidity);
    napi_set_named_property(env, result, "PRESSURE", pressure);
    napi_set_named_property(env, result, "PROXIMITY", proximity);
    napi_set_named_property(env, result, "HEART_RATE", heartRate);
    napi_set_named_property(env, result, "STEP_COUNTER", stepCounter);

    return result;
}

napi_value GetOrientation(napi_env env, napi_callback_info info)
{
    napi_value result;
    napi_create_object(env, &result);

    float yaw = sensorManager.getGyroscopeData().z;
    float pitch = sensorManager.getAccelerometerData().x;
    float roll = sensorManager.getAccelerometerData().y;

    napi_value yawValue;
    napi_value pitchValue;
    napi_value rollValue;
    napi_value timestamp;
    napi_create_number(env, yaw, &yawValue);
    napi_create_number(env, pitch, &pitchValue);
    napi_create_number(env, roll, &rollValue);
    napi_create_number(env, static_cast<int64_t>(time(nullptr) * 1000), &timestamp);

    napi_set_named_property(env, result, "yaw", yawValue);
    napi_set_named_property(env, result, "pitch", pitchValue);
    napi_set_named_property(env, result, "roll", rollValue);
    napi_set_named_property(env, result, "timestamp", timestamp);

    return result;
}

napi_value GetCompassHeading(napi_env env, napi_callback_info info)
{
    MagnetometerData magData = sensorManager.getMagnetometerData();
    float heading = atan2(magData.y, magData.x) * 180.0f / M_PI;
    if (heading < 0) {
        heading += 360.0f;
    }

    napi_value result;
    napi_create_object(env, &result);

    napi_value headingValue;
    napi_value timestamp;
    napi_create_number(env, heading, &headingValue);
    napi_create_number(env, magData.timestamp, &timestamp);

    napi_set_named_property(env, result, "heading", headingValue);
    napi_set_named_property(env, result, "timestamp", timestamp);

    return result;
}

napi_value GetBatchSensorData(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing sensor type");
        return nullptr;
    }

    int sensorType = 0;
    napi_get_value_int32(env, args[0], &sensorType);

    napi_value result;
    napi_create_array(env, &result);

    for (int i = 0; i < 10; i++) {
        napi_value sample;
        napi_create_object(env, &sample);

        switch (static_cast<SensorType>(sensorType)) {
            case SENSOR_TYPE_ACCELEROMETER: {
                AccelerometerData data = sensorManager.getAccelerometerData();
                napi_value x;
                napi_value y;
                napi_value z;
                napi_value timestamp;
                napi_create_number(env, data.x, &x);
                napi_create_number(env, data.y, &y);
                napi_create_number(env, data.z, &z);
                napi_create_number(env, data.timestamp, &timestamp);
                napi_set_named_property(env, sample, "x", x);
                napi_set_named_property(env, sample, "y", y);
                napi_set_named_property(env, sample, "z", z);
                napi_set_named_property(env, sample, "timestamp", timestamp);
                break;
            }
            case SENSOR_TYPE_GYROSCOPE: {
                GyroscopeData data = sensorManager.getGyroscopeData();
                napi_value x;
                napi_value y;
                napi_value z;
                napi_value timestamp;
                napi_create_number(env, data.x, &x);
                napi_create_number(env, data.y, &y);
                napi_create_number(env, data.z, &z);
                napi_create_number(env, data.timestamp, &timestamp);
                napi_set_named_property(env, sample, "x", x);
                napi_set_named_property(env, sample, "y", y);
                napi_set_named_property(env, sample, "z", z);
                napi_set_named_property(env, sample, "timestamp", timestamp);
                break;
            }
            case SENSOR_TYPE_MAGNETOMETER: {
                MagnetometerData data = sensorManager.getMagnetometerData();
                napi_value x;
                napi_value y;
                napi_value z;
                napi_value timestamp;
                napi_create_number(env, data.x, &x);
                napi_create_number(env, data.y, &y);
                napi_create_number(env, data.z, &z);
                napi_create_number(env, data.timestamp, &timestamp);
                napi_set_named_property(env, sample, "x", x);
                napi_set_named_property(env, sample, "y", y);
                napi_set_named_property(env, sample, "z", z);
                napi_set_named_property(env, sample, "timestamp", timestamp);
                break;
            }
            default:
                napi_set_named_property(env, sample, "error", napi_get_undefined(env, nullptr));
                break;
        }

        napi_set_element(env, result, i, sample);
    }

    return result;
}

napi_value SetSensorSamplingRate(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 2) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing sensor type and sampling rate");
        return nullptr;
    }

    int sensorType = 0;
    double samplingRate = 0.0;
    napi_get_value_int32(env, args[0], &sensorType);
    napi_get_value_double(env, args[1], &samplingRate);

    napi_value result;
    napi_create_boolean(env, true, &result);
    return result;
}

napi_value GetSensorSamplingRate(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing sensor type");
        return nullptr;
    }

    int sensorType = 0;
    napi_get_value_int32(env, args[0], &sensorType);

    napi_value result;
    napi_create_number(env, DEFAULT_SAMPLING_RATE, &result);
    return result;
}

napi_value SetSensorPowerState(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 2) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing sensor type and power state");
        return nullptr;
    }

    int sensorType = 0;
    bool enabled = false;
    napi_get_value_int32(env, args[0], &sensorType);
    napi_get_value_bool(env, args[1], &enabled);

    napi_value result;
    napi_create_boolean(env, true, &result);
    return result;
}

napi_value GetSensorPowerState(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing sensor type");
        return nullptr;
    }

    int sensorType = 0;
    napi_get_value_int32(env, args[0], &sensorType);

    napi_value result;
    napi_create_boolean(env, true, &result);
    return result;
}

}

extern "C" {

napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor descriptors[] = {
        { "getAccelerometerData", nullptr, GetAccelerometerData, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "getGyroscopeData", nullptr, GetGyroscopeData, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "getMagnetometerData", nullptr, GetMagnetometerData, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "getLightData", nullptr, GetLightData, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "getTemperatureData", nullptr, GetTemperatureData, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "getHumidityData", nullptr, GetHumidityData, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "getPressureData", nullptr, GetPressureData, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "getProximityData", nullptr, GetProximityData, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "getHeartRateData", nullptr, GetHeartRateData, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "getStepCounterData", nullptr, GetStepCounterData, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "isSensorAvailable", nullptr, IsSensorAvailable, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "calibrateSensor", nullptr, CalibrateSensor, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "getSensorTypes", nullptr, GetSensorTypes, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "getOrientation", nullptr, GetOrientation, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "getCompassHeading", nullptr, GetCompassHeading, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "getBatchSensorData", nullptr, GetBatchSensorData, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "setSensorSamplingRate", nullptr, SetSensorSamplingRate, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "getSensorSamplingRate", nullptr, GetSensorSamplingRate, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "setSensorPowerState", nullptr, SetSensorPowerState, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "getSensorPowerState", nullptr, GetSensorPowerState, nullptr, nullptr, nullptr, napi_default, nullptr },
    };

    napi_define_properties(env, exports, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
    return exports;
}

NAPI_MODULE(native_module_sensors, Init)

}
