// Test script for native_module_sensors

const sensors = require('./build/Release/native_module_sensors');

// Get sensor types
const sensorTypes = sensors.getSensorTypes();
console.log('Sensor Types:', sensorTypes);

// Test accelerometer
console.log('\n=== Accelerometer Test ===');
const accelData = sensors.getAccelerometerData();
console.log('Accelerometer:', accelData);

// Test gyroscope
console.log('\n=== Gyroscope Test ===');
const gyroData = sensors.getGyroscopeData();
console.log('Gyroscope:', gyroData);

// Test magnetometer
console.log('\n=== Magnetometer Test ===');
const magData = sensors.getMagnetometerData();
console.log('Magnetometer:', magData);

// Test light sensor
console.log('\n=== Light Sensor Test ===');
const lightData = sensors.getLightData();
console.log('Light:', lightData);

// Test temperature sensor
console.log('\n=== Temperature Sensor Test ===');
const tempData = sensors.getTemperatureData();
console.log('Temperature:', tempData);

// Test humidity sensor
console.log('\n=== Humidity Sensor Test ===');
const humidityData = sensors.getHumidityData();
console.log('Humidity:', humidityData);

// Test pressure sensor
console.log('\n=== Pressure Sensor Test ===');
const pressureData = sensors.getPressureData();
console.log('Pressure:', pressureData);

// Test proximity sensor
console.log('\n=== Proximity Sensor Test ===');
const proximityData = sensors.getProximityData();
console.log('Proximity:', proximityData);

// Test heart rate sensor
console.log('\n=== Heart Rate Sensor Test ===');
const heartRateData = sensors.getHeartRateData();
console.log('Heart Rate:', heartRateData);

// Test step counter
console.log('\n=== Step Counter Test ===');
const stepData = sensors.getStepCounterData();
console.log('Step Counter:', stepData);

// Test sensor availability
console.log('\n=== Sensor Availability Test ===');
const accelAvailable = sensors.isSensorAvailable(sensorTypes.ACCELEROMETER);
console.log('Accelerometer available:', accelAvailable);

// Test sensor calibration
console.log('\n=== Sensor Calibration Test ===');
const calibrated = sensors.calibrateSensor(sensorTypes.ACCELEROMETER);
console.log('Accelerometer calibrated:', calibrated);

// Test sensor fusion
console.log('\n=== Sensor Fusion Test ===');
const orientation = sensors.getOrientation();
console.log('Orientation:', orientation);

const compassHeading = sensors.getCompassHeading();
console.log('Compass Heading:', compassHeading);

// Test batch sensor data
console.log('\n=== Batch Sensor Data Test ===');
const batchData = sensors.getBatchSensorData(sensorTypes.ACCELEROMETER);
console.log('Batch Accelerometer Data:', batchData.length, 'samples');

// Test sensor configuration
console.log('\n=== Sensor Configuration Test ===');
const setRateResult = sensors.setSensorSamplingRate(sensorTypes.ACCELEROMETER, 50);
console.log('Set sampling rate:', setRateResult);

const getRateResult = sensors.getSensorSamplingRate(sensorTypes.ACCELEROMETER);
console.log('Get sampling rate:', getRateResult);

// Test sensor power management
console.log('\n=== Sensor Power Management Test ===');
const setPowerResult = sensors.setSensorPowerState(sensorTypes.ACCELEROMETER, true);
console.log('Set power state:', setPowerResult);

const getPowerResult = sensors.getSensorPowerState(sensorTypes.ACCELEROMETER);
console.log('Get power state:', getPowerResult);

console.log('\n=== All tests completed ===');
