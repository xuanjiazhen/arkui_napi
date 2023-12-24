'use strict';

// Test that `OH_JSVM_CallFunction()` returns `JSVM_CANNOT_RUN_JS` in experimental
// mode and `JSVM_PENDING_EXCEPTION` otherwise. This test calls the add-on's
// `createRef()` method, which creates a strong reference to a JS function. When
// the process exits, it calls all reference finalizers. The finalizer for the
// strong reference created herein will attempt to call `OH_JSVM_GetProperty()` on
// a property of the global object and will abort the process if the API doesn't
// return the correct status.

const { buildType, mustNotCall } = require('../../common');
const addon_v8 = require(`./build/${buildType}/test_pending_exception`);
const addon_new = require(`./build/${buildType}/test_cannot_run_js`);

function runTests(addon, isVersion8) {
  addon.createRef(mustNotCall());
}

function runAllTests() {
  runTests(addon_v8, /* isVersion8 */ true);
  runTests(addon_new, /* isVersion8 */ false);
}

runAllTests();
