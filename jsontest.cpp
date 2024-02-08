//
// Created by Matei Budiu on 1/30/24.
//

#include "json.hpp"
int main()
{
    jobject* j = jparse(R"({ "one" : "yes", "two" : 2, "three":false , "four":true , "five":null, "six":[2], "seven":[1, 2, 3], "eight": {} })");
    jnumber* s = static_cast<jnumber*>((*j)["two"]);
    printf("[%f]\n", s->v);
}