#include "LoadingFile.h"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

LoadingFile::LoadingFile(string name,fileSpecifier specifier){
    this->fileName = name;
    this->actMode = specifier;

    switch(this->actMode){
        case READ:
            readFile.open(name,ios::in);
        break;

        case WRITE:
            writeFile.open(name, ios::out);
        break;

        case ADD:
            writeFile.open(name, ios::app);
        break;

        case READ_BYNARY:
            readFile.open(name,ios::in | ios::binary);
        break;

        case WRITE_BYNARY:
            writeFile.open(name, ios::out | ios::binary);
        break;

        case ADD_BYNARY:
            writeFile.open(name, ios::app | ios::binary);
        break;

        default:;
        //Error
        //readFile   = 0;
        //writeFile  = 0;
    }
}

LoadingFile::~LoadingFile(){
    //readFile   = 0;
    //writeFile  = 0;
}

bool LoadingFile::isThereFile(){
    if(readFile == 0 && writeFile == 0){
        return false;
    }

    if(readFile != 0){
        //cout << "Found READ Order..." << endl;
        return readFile.is_open();
    }

    if(writeFile != 0){
        //cout << "Found WRITE Order..." << endl;
        return writeFile.is_open();
    }
    
    return false;
}

bool LoadingFile::getStringLine(string* ret){
    return getline(readFile, *ret);
}


vector<string> LoadingFile::linesplit(const string& str,const char sep){
    vector<string> splited;
    istringstream ss(str);
    string buffer;
    while( std::getline(ss, buffer, sep) ) {
        splited.push_back(buffer);
    }
    return splited;
}

string LoadingFile::trim(const string& str, const char* trimList){
    string res;
    //左からトリム対象以外の文字を見つける
    string::size_type left = str.find_first_not_of(trimList);
    if(left != string::npos){
        //右からも同様
        string::size_type right = str.find_last_not_of(trimList);
        res = str.substr(left, right - left + 1);
    }
    return res;
}