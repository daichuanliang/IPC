
#ifndef META_H
#define META_H

#include <string>
using std::string;

typedef struct Meta{
    string storeID;
    string mac;
    string uniqueID; //reserved

    int type;   //1-request new customer id,
                //2-id request
                //3-data upload
                //etc
    double feature[128];
    int gender; //0-femail, 1-mail
    int age;    //0-chiled, 1-youny, 2-middle, 3-older
    int roi[4];
    long timeStamp; //java: System.currentTimeMillis()
                    //c++11: milliseconds ms = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    int customerID; //0 for invalid ID
}Meta_t;

static const char Str_storeID[]   ="storeID";
static const char Str_mac[]       ="mac";
static const char Str_uniqueID[]  ="uniqueID";
static const char Str_type[]      ="type";
static const char Str_feature[]   ="feature";
static const char Str_gender[]    ="gender";
static const char Str_age[]       ="age";
static const char Str_roi[]       ="roi";
static const char Str_timeStamp[] ="timeStamp";
static const char Str_customerID[]="customerID";

#include <cstdlib>
#include <ctime>
#include <random>
#include <chrono>
static int randNum(int min, int max)
{
    srand(time(0));
    return rand() % max + min;
}
static double randomDouble()
{
    const double lower_bound = 0;
    const double upper_bound = 1;
    std::uniform_real_distribution<double> unif(lower_bound, upper_bound);

    std::random_device rand_dev;          // Use random_device to get a random seed.

    std::mt19937 rand_engine(rand_dev()); // mt19937 is a good pseudo-random number generator.
    double x = unif(rand_engine);
    return x;
}
static int generate_fake_meta_for_debug(Meta_t& meta)
{
    int d = randNum(0,100);
    meta.storeID    = string("storeID") + std::to_string(d);
    meta.mac        = "1234:1234:1234:ffff";
    meta.uniqueID   = std::to_string(d);
    meta.type       = 0;
    meta.gender     = 0;
    meta.age        = 2;
    for (int i=0;i<4;i++)
        meta.roi[i] = i;
    //meta.timeStamp =  std::chrono::duration_cast<  std::chrono::milliseconds >( std::chrono::system_clock::now().time_since_epoch());
    std::chrono::milliseconds ms = std::chrono::duration_cast<  std::chrono::milliseconds >( std::chrono::system_clock::now().time_since_epoch());
    meta.timeStamp = ms.count();

    meta.customerID = 0;

    for (int i=0;i<128;i++)
        meta.feature[i]=randomDouble();

    return 0;
}

#if 0
{
    "storeID":"idsample",
    "mac":"b4:a3:82:5e:5f:15",
    "uniqueID":"internalid",
    "type": 0,
    "feature":[
        1.1,
        2.2,
        3.3,
        ...
        128.8
    ],
    "gender":0,
    "age":0,
    "roi":[1,2,3,4],
    "timeStamp":12345678,
    "customerID":0
}
#endif

#endif
