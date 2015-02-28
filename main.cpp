#include <iostream>
#include <fstream>
#include "lightscript.h"

using namespace std;

int main()
{
    ifstream f("script.ls");
    vector<char> data((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    if (f.is_open())
        f.close();

    Lightscript script{data};
    script.compile();
    return 0;
}

