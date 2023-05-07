#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "../CJsonObject.hpp"
using json = neb::CJsonObject;
int main()
{
    std::string jsonbuf = "{\"refresh_interval\":60,\"timeout\":{\"time\":6666}}";
    std::string buf;
    int value;

    json oJson(jsonbuf);
    buf = oJson.ToString();
    std::cout << buf << std::endl;
    std::cout << oJson.ToFormattedString() << std::endl;
    oJson.Get("refresh_interval",value);
    std::cout << value << std::endl;

    oJson["timeout"].Get("time",value);
    std::cout << value << std::endl;
}