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
#ifndef IMPL_NAPITEST_H
#define IMPL_NAPITEST_H

#include <string>
#include <memory>
#include <vector>
#include <cmath>
#include <map>
#include <any>


using NUMBER_TYPE_1 = uint32_t;
using NUMBER_TYPE_2 = uint32_t;
using NUMBER_TYPE_3 = uint32_t;
using NUMBER_TYPE_4 = uint32_t;
using NUMBER_TYPE_5 = uint32_t;
using NUMBER_TYPE_6 = uint32_t;
using NUMBER_TYPE_7 = uint32_t;
using NUMBER_TYPE_8 = uint32_t;


namespace napitest {
class Test {
public:
    std::string name;
    NUMBER_TYPE_1 age;
};

class Entry {
public:
    std::vector<NUMBER_TYPE_2> key;
    std::vector<std::string> value;
    std::vector<bool> isExit;
};

class Test1 {
public:
    bool $listInputMethod(std::string& V);
};

class Person {
public:
    static bool $getMaxAge(std::string& v1, NUMBER_TYPE_7& out);
};

bool testArray(std::vector<std::string> &v, std::vector<std::string>& out);
bool testArray1(std::vector<NUMBER_TYPE_3> &v, std::vector<NUMBER_TYPE_4>& out);
bool testArray2(std::vector<bool> &v, std::vector<bool>& out);
bool fun1(std::vector<std::string> &v1, std::vector<Test> &v2, std::vector<NUMBER_TYPE_5>& out);
bool fun2(std::vector<NUMBER_TYPE_6> &v1, std::vector<Test> &v2, std::vector<std::string>& out);
bool fun3(std::vector<bool> &v1, std::vector<Test> &v2, std::vector<bool>& out);
bool fun4(std::string& v, std::vector<Entry> &v1, std::vector<Entry>& out);
bool $fun5(std::string& v, std::string& out);
bool fun6(Test1& v, NUMBER_TYPE_8& out);
}

#endif // IMPL_NAPITEST_H
