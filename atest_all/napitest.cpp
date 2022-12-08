/*
* Copyright (c) 2021 Huawei Device Co., Ltd. 
* Licensed under the Apache License, Version 2.0 (the "License"); 
* you may not use this file except in compliance with the License. 
* You may obtain a copy of the License at 
*
* http://www.apache.org/licenses/LICENSE-2.0 
*
* Unless required by applicable law or agreed to in writing, software 
* distributed under the License is distributed on an "AS IS" BASIS, 
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
* See the License for the specific language governing permissions and 
* limitations under the License. 
*/
#include "napitest.h"

namespace napitest {
bool Test1::$listInputMethod(std::string& V)
{
    return true;
}

bool Person::$getMaxAge(std::string& v1, NUMBER_TYPE_7& out)
{
    return true;
}

bool testArray(std::vector<std::string> &v, std::vector<std::string>& out)
{
    return true;
}

bool testArray1(std::vector<NUMBER_TYPE_3> &v, std::vector<NUMBER_TYPE_4>& out)
{
    return true;
}

bool testArray2(std::vector<bool> &v, std::vector<bool>& out)
{
    return true;
}

bool fun1(std::vector<std::string> &v1, std::vector<Test> &v2, std::vector<NUMBER_TYPE_5>& out)
{
    return true;
}

bool fun2(std::vector<NUMBER_TYPE_6> &v1, std::vector<Test> &v2, std::vector<std::string>& out)
{
    return true;
}

bool fun3(std::vector<bool> &v1, std::vector<Test> &v2, std::vector<bool>& out)
{
    return true;
}

bool fun4(std::string& v, std::vector<Entry> &v1, std::vector<Entry>& out)
{
    return true;
}

bool $fun5(std::string& v, std::string& out)
{
    return true;
}

bool fun6(Test1& v, NUMBER_TYPE_8& out)
{
    return true;
}
}
