/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "test.h"
#include "quickjs_native_engine.h"

static NativeEngine* g_nativeEngine = nullptr;
static NativeEngine* g_moduleNameNativeEngine = nullptr;

NativeEngineTest::NativeEngineTest()
{
    engine_ = g_nativeEngine;
    moduleNameEngine_ = g_moduleNameNativeEngine;
}

NativeEngineTest::~NativeEngineTest()
{
}

int main(int argc, char** argv)
{
    testing::GTEST_FLAG(output) = "xml:./";
    testing::InitGoogleTest(&argc, argv);

    JSRuntime* rt = JS_NewRuntime();
    if (rt == nullptr) {
        return 0;
    }

    JSContext* ctx = JS_NewContext(rt);
    if (ctx == nullptr) {
        return 0;
    }

    g_nativeEngine = new QuickJSNativeEngine(rt, ctx, 0);
    g_moduleNameNativeEngine = new QuickJSNativeEngine(g_nativeEngine->GetNativeEngineImpl(), 0, false);

    const char* moduleName1 = "ability.featureAbility";
    const char* moduleName2 = "ability.particleAbility";
    std::string strModuleName1(moduleName1);
    std::string strModuleName2(moduleName2);

    g_nativeEngine->SetModuleFileName(strModuleName1);
    g_moduleNameNativeEngine->SetModuleFileName(strModuleName2);

    int rev = RUN_ALL_TESTS();

    if (g_moduleNameNativeEngine) {
        delete g_moduleNameNativeEngine;
        g_moduleNameNativeEngine = nullptr;
    }

    if (g_nativeEngine) {
        g_nativeEngine->DeleteEngine();
        delete g_nativeEngine;
        g_nativeEngine = nullptr;
    }
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return rev;
}
