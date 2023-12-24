'use strict';
const common = require('../../common');
const assert = require('assert');

// Test passing NULL to object-related N-APIs.
const { testNull } = require(`./build/${common.buildType}/test_constructor`);
const expectedResult = {
  envIsNull: 'Invalid argument',
  nameIsNull: 'Invalid argument',
  lengthIsZero: 'JSVM_OK',
  nativeSideIsNull: 'Invalid argument',
  dataIsNull: 'JSVM_OK',
  propsLengthIsZero: 'JSVM_OK',
  propsIsNull: 'Invalid argument',
  resultIsNull: 'Invalid argument',
};

assert.deepStrictEqual(testNull.testDefineClass(), expectedResult);
