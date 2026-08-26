#ifndef ARFFDATA_H
#define ARFFDATA_H

#include <string>
#include <vector>

class ArffData
{
public:
    std::vector<std::vector<double>> xpoint;
    std::vector<double> ypoint;

    int patterns;
    int features;

    ArffData();

    bool load(const std::string& filename);

private:
    static void trim(std::string& s);

    static std::string toLower(std::string s);

    static std::vector<std::string>
    splitCSV(const std::string& line);

    static bool isNumber(const std::string& s);

    static void removeQuotes(std::string& s);
};

#endif
