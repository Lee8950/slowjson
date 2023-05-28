#include <bits/stdc++.h>
#include "../include/slowjson.hpp"

int main()
{
    ecl::json_storage js;
    js.read(std::string("{\
        \"test\":{\
            \"noitem\":true\
        }\
    }"));
    js.tokenize();
    js.read_token_stream();
    return 0;
}