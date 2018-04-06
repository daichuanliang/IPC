#include "jsonAPI.h"
#include "curlPost.h"
#include <string>
#include <iostream>

using namespace std;



int main()
{
    Meta_t meta;

    string json = convertMeta2Json_rj(meta);
    string output;
    int ret = postJson2Server(json, output);
    if (!ret)
        cout<<"Fail to do postJson2Server"<<endl;
    else
        cout<<"postJson2Server return "<<output<<endl;

    return 0;
}
