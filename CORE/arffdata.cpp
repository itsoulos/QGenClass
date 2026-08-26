#include <CORE/arffdata.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <unordered_map>

using namespace std;

ArffData::ArffData()
{
    patterns = 0;
    features = 0;
}


bool ArffData::load(const string& filename)
{
    ifstream file(filename);

    if (!file.is_open())
    {
        cerr << "ERROR: Cannot open file: "
             << filename << endl;

        return false;
    }

    xpoint.clear();
    ypoint.clear();

    patterns = 0;
    features = 0;

    string line;
    bool dataSection = false;

    // Χρησιμοποιείται όταν οι κατηγορίες είναι strings
    unordered_map<string, double> classMap;

    double nextClass = 0.0;

    while (getline(file, line))
    {
        trim(line);

        // Αγνόηση κενών γραμμών
        if (line.empty())
            continue;

        // Αγνόηση σχολίων ARFF
        if (line[0] == '%')
            continue;

        string lowerLine = toLower(line);

        // -------------------------------
        // HEADER
        // -------------------------------

        if (!dataSection)
        {
            if (lowerLine.find("@data") == 0)
            {
                dataSection = true;
            }

            continue;
        }

        // -------------------------------
        // DATA SECTION
        // -------------------------------

        vector<string> tokens = splitCSV(line);

        if (tokens.size() < 2)
            continue;

        // Η τελευταία στήλη είναι η κατηγορία
        int currentFeatures =
            static_cast<int>(tokens.size()) - 1;

        if (features == 0)
        {
            features = currentFeatures;
        }
        else if (currentFeatures != features)
        {
            cerr << "ERROR: Wrong number of fields in line:"
                 << endl;

            cerr << line << endl;

            cerr << "Expected "
                 << features + 1
                 << " fields but found "
                 << tokens.size()
                 << endl;

            return false;
        }

        vector<double> pattern(features);

        // -------------------------------
        // FEATURES
        // -------------------------------

        for (int j = 0; j < features; j++)
        {
            trim(tokens[j]);

            if (tokens[j] == "?")
            {
                // Missing value
                // Προς το παρόν το θεωρούμε 0
                pattern[j] = 0.0;
            }
            else
            {
                try
                {
                    pattern[j] = stod(tokens[j]);
                }
                catch (...)
                {
                    cerr << "ERROR: Feature is not numeric: "
                         << tokens[j]
                         << endl;

                    cerr << "Line:"
                         << endl
                         << line
                         << endl;

                    return false;
                }
            }
        }

        // -------------------------------
        // CLASS
        // -------------------------------

        string classToken = tokens.back();

        trim(classToken);
        removeQuotes(classToken);

        double y;

        if (isNumber(classToken))
        {
            y = stod(classToken);
        }
        else
        {
            // Αν η κατηγορία είναι string,
            // δημιουργούμε αριθμητική αντιστοίχιση.

            auto it = classMap.find(classToken);

            if (it == classMap.end())
            {
                classMap[classToken] = nextClass;

                y = nextClass;

                cout << "Class \""
                     << classToken
                     << "\" -> "
                     << nextClass
                     << endl;

                nextClass += 1.0;
            }
            else
            {
                y = it->second;
            }
        }

        // Αποθήκευση pattern
        xpoint.push_back(pattern);

        // Αποθήκευση class
        ypoint.push_back(y);
    }

    file.close();

    patterns = static_cast<int>(xpoint.size());

    if (!dataSection)
    {
        cerr << "ERROR: @data section not found in "
             << filename
             << endl;

        return false;
    }

    if (patterns == 0)
    {
        cerr << "ERROR: No patterns found in "
             << filename
             << endl;

        return false;
    }

    cout << endl;
    cout << "Loaded file: "
         << filename
         << endl;

    cout << "Patterns : "
         << patterns
         << endl;

    cout << "Features : "
         << features
         << endl;

    return true;
}


void ArffData::trim(string& s)
{
    while (!s.empty() &&
           isspace(static_cast<unsigned char>(s.front())))
    {
        s.erase(s.begin());
    }

    while (!s.empty() &&
           isspace(static_cast<unsigned char>(s.back())))
    {
        s.pop_back();
    }
}


string ArffData::toLower(string s)
{
    transform(
        s.begin(),
        s.end(),
        s.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(tolower(c));
        }
        );

    return s;
}


vector<string> ArffData::splitCSV(const string& line)
{
    vector<string> result;

    string token;

    bool insideSingleQuote = false;
    bool insideDoubleQuote = false;

    for (char c : line)
    {
        if (c == '\'' && !insideDoubleQuote)
        {
            insideSingleQuote = !insideSingleQuote;

            token += c;
        }
        else if (c == '"' && !insideSingleQuote)
        {
            insideDoubleQuote = !insideDoubleQuote;

            token += c;
        }
        else if (c == ',' &&
                 !insideSingleQuote &&
                 !insideDoubleQuote)
        {
            trim(token);

            result.push_back(token);

            token.clear();
        }
        else
        {
            token += c;
        }
    }

    trim(token);

    result.push_back(token);

    return result;
}


bool ArffData::isNumber(const string& s)
{
    if (s.empty())
        return false;

    try
    {
        size_t pos = 0;

        stod(s, &pos);

        return pos == s.size();
    }
    catch (...)
    {
        return false;
    }
}


void ArffData::removeQuotes(string& s)
{
    if (s.size() >= 2)
    {
        if ((s.front() == '\'' && s.back() == '\'') ||
            (s.front() == '"' && s.back() == '"'))
        {
            s = s.substr(1, s.size() - 2);
        }
    }
}
