#include <fstream>
#include <iostream>

enum fileSpecifier{
    READ,
    WRITE,
    ADD,
    READ_BYNARY,
    WRITE_BYNARY,
    ADD_BYNARY,
};

using namespace std;
class LoadingFile{
    public:
        LoadingFile(string fileName,fileSpecifier rwa);
        ~LoadingFile();

        bool isThereFile();
        bool getStringLine(string* ret);
        vector<string> linesplit(const string &org,const char sep);
        string trim(const string& str, const char* trimList = " \t\v\r\n");
        
    private:
        fileSpecifier actMode;
        string fileName;
        ofstream writeFile;
        ifstream readFile;
};