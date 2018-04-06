
#ifndef CONFIG_H
#define CONFIG_H

#include <string.h>
#include <stdio.h>
using namespace std;
class Config
{
public:
	Config();
	~Config();
	int updateConfig(string ip, int port, string user, string passwd);
};

#endif
