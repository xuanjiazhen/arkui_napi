'use strict';

// This test makes no assertions. It tests that calling OH_JSVM_RemoveWrap and
// OH_JSVM_DeleteReference consecutively doesn't crash the process.

const { buildType } = require('../../common');

const addon = require(`./build/${buildType}/test_reference_double_free`);

addon.deleteImmediately({});
