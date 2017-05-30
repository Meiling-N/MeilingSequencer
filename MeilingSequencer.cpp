/*
ver.1.0
20170528
author:@Nonohoo
genmmya@outlook.jp
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include "LoadingFile.h"

//円周率
#define pi 3.1415926535

//格納用
typedef struct SoundData{
    double hz;//==-1でノイズ
    double sinpuku;
    long StartTime;
    long EndTime;
    /*
    10000分率
    Ata:一番最初の振幅の大きさ %
    Atp:ピークの位置 %
    Dee:decay終了位置 %
    Sua:伸ばすときの振幅の大きさ %
    Sue:伸ばし終了位置 %
    Ree:減衰終了位置 %
    */
    int atk_SinpukuRatio;
    int atk_PositionRatio;
    int dec_PositionRatio;
    int sus_SinpukuRatio;
    int sus_PositionRatio;
    int rel_PositionRatio;

}soundData;


//プロトタイプ宣言
void createSoundData(LoadingFile* melody);
void createMusic(int);
short int createWaves(int pos,vector<soundData>);
double getAmplitude(int pos, soundData);
bool isOverlapString(string moziretu,string chk_char);
void createHeader(int datasize,bool isStereo);
char* int2Chars(int value);

//定数
const double samplingfreq = 44100.0;    //サンプリング周波数
const int kenban = 119;
const int DECRES_TIME = 200;            //窓関数の窓幅
const float step = 2 * M_PI / samplingfreq;//波形合成のあれ
const float base_Hz = 27.50;            //基音、最低音
const int RATIO_UPLIMIT = 10000; //振幅とかの%、10000分率
//ファイル作成用
FILE *fpw;
short int *pbuff;
char* headBuff;

//ファイル名保存用
vector<string> filePaths;
//制御用文字列との対応付け
double onkai[kenban+1];//A0 to C7 and nothing
map<string,int> name2Scale;//etc. 紅 to 50
map<string,double> name2Length;//etc. 華 to /8
map<string,string> name2AdLength;//etc. 雨 to 付点
map<string,string> name2Additional;//etc. 符 to Z
map<string,double> name2Pitch;//etc. A to 440.0
map<string,double> name2Amplitude;//etc. m to 5000.0
map<string,string> name2ADSR;//etc. to Ata

//全イベントデータ格納先
vector<soundData> soundEvents;

//その他
double bpm = 120;
bool isHelp = false;
bool isAdvanceMode = false;
bool isHertzMode = false;
bool isMergeFile = false;


using namespace std;
int main(int argc, char *argv[]){
    int mergeFilesCmnd_pos = 0;
    int fileSize = samplingfreq;

    //雑なコマンドライン確認
    for(int i = 0; i < argc; i++){
        if(argv[i][0] == '-'){
            switch(argv[i][1]){
                case 'h':
                    isHelp = true;
                break;

                case 'a':
                    isAdvanceMode = true;
                break;

                case 'b':
                    bpm = std::stod(argv[i + 1]);
                    cout << "change bpm:" << bpm << endl;
                break;

                case 'z':
                    isHertzMode = true;
                break;

                case 'm':
                    isMergeFile = true;
                    mergeFilesCmnd_pos = i;
                break;

                default:;
            }
        }
    }

    if(argc == 1 || isHelp){
        cout << "使い方(How to)" << endl;
        cout << "exe filename:ファイル読み込み、wav生成(read file and create .wav)" << endl;
        cout << "ファイル名は一番最後に記すこと(you must write filename at last)" << endl;
        cout << "-h:help(これ)(this)" << endl;
        cout << "-a:拡張機能モード(Advanced mode)" << endl;
        cout << "  -絶対時間軸処理になる(be to Absolutely_Axis of Time)" << endl;
        cout << "  -ADSRの編集機能追加(add Attack,Decay,Sustain and Release edit)" << endl;
        cout << "-b:bpmの変更(change bpm)" << endl;
        cout << "  -このコマンドの次は必ず数字にすること(you must write number(bpm) after this)" << endl;
        cout << "-z:周波数指定可能モード(accept to Specify Hertz, mode)" <<endl;
        cout << "  -既存の文字での音高指定と併用可(you can be used together defaultAllocate)" << endl;
        cout << "-m:複数ファイル結合(merge some Files)" << endl;
        cout << "  -このコマンド以降は全てファイル名とすること(you must write all filenames and don't write any command after this command)" << endl;
        cout << "example: ./a.out -z -m test.txt test2.txt" << endl;
        return 0;
    }

    //ファイル内の文字重複確認クソザコプログラム用
    string chk_overlap = "";


    //ファイルを複数使う場合はファイル名を全部保存する
    if(isMergeFile){
        for(int i = mergeFilesCmnd_pos + 1; i < argc; i++){
            filePaths.push_back(argv[i]);
        }
    }
    
    //音階表記などのデフォルトファイル読み込み
    LoadingFile* configFile = new LoadingFile("defaultAllocate_meiling.ini",READ);
    if (!configFile->isThereFile()){
        cerr << "ファイルの読み込みに失敗しました" << endl;
        cerr << "No such a file: " << "defaultAllocate_meiling.ini" << endl;
        return -1;
    }
    
    //まずC4とかと文字の関連付け
    map<string,string> name2Cord;
    string tmpLine;
    vector<string> tmpStr;
    while(configFile->getStringLine(&tmpLine)){
        tmpStr = configFile->linesplit(tmpLine,'=');
        if(tmpStr[0] == ",")break;
        if(isOverlapString(chk_overlap,configFile->trim(tmpStr[1]))){
            return -1;
        }else{
            chk_overlap += configFile->trim(tmpStr[1]);
        }
        name2Cord[configFile->trim(tmpStr[1])] = configFile->trim(tmpStr[0]);
    }

    //Cord名から数字へ
    //音階割り当て読み込み
    /*
    name2Scale["紅"] = 39;//C4
    */
    map<string, string>::iterator ite = name2Cord.begin();
    while(ite != name2Cord.end()){
        string::size_type length = (*ite).second.size();
        int cord2Num = 0;
        for(int i = 0;i < length; i++){
            char tmp = (*ite).second[i];
            //国際式
            switch (tmp){
                case '#': cord2Num++;break;
                case 'A': break;
                case 'B': cord2Num+=2;break;
                case 'C': cord2Num+=3-12;break;
                case 'D': cord2Num+=5-12;break;
                case 'E': cord2Num+=7-12;break;
                case 'F': cord2Num+=8-12;break;
                case 'G': cord2Num+=10-12;break;
                case 'M': cord2Num = kenban;break;
                default:
                    if(tmp - 48 >= 0 && tmp - 48 < 10){
                        if(i + 1 < length && (*ite).second[i+1] == '0'){
                            cord2Num += 12 * 10;
                        }else{
                            cord2Num += 12 * (tmp-48);
                        }
                    }else{
                        if(tmp != 'M'){
                            cout << "ERROR: invalid left Character in 'defaultAllocate_Meiling.ini' file" << endl;
                            return -1;
                        }else{
                            cord2Num = 0;
                        }
                    }
            }
        }
        if(cord2Num < 0 || cord2Num > kenban){
            cout << cord2Num <<endl;
            cout << "ERROR: Sorry,don't accept that 'Cord' in '.ini'" << endl;
            cout << "Error'Cord':" << (*ite).second << endl;
            return -1;
        }
        name2Scale[(*ite).first] = cord2Num;
		ite++;
	}

    //音符長さ文字割り当て
    while(configFile->getStringLine(&tmpLine)){
        tmpStr = configFile->linesplit(tmpLine,'=');
        if(tmpStr[0] == ",")break;
        if(isOverlapString(chk_overlap,configFile->trim(tmpStr[1]))){
            return -1;
        }else{
            chk_overlap += configFile->trim(tmpStr[1]);
        }
        name2Length[configFile->trim(tmpStr[1])] = 	std::stod(configFile->trim(tmpStr[0]));
    }


    //3連符など別枠割り当て
    while(configFile->getStringLine(&tmpLine)){
        tmpStr = configFile->linesplit(tmpLine,'=');
        if(tmpStr[0] == ",")break;
        if(isOverlapString(chk_overlap,configFile->trim(tmpStr[1]))){
            return -1;
        }else{
            chk_overlap += configFile->trim(tmpStr[1]);
        }
        name2AdLength[configFile->trim(tmpStr[1])] = configFile->trim(tmpStr[0]);
    }

    //振幅指定読み込み
    LoadingFile* ampFile = new LoadingFile("defaultAmplitude_meiling.ini",READ);
    if (!ampFile->isThereFile()){
        cerr << "ファイルの読み込みに失敗しました" << endl;
        cerr << "No such a file: " << "defaultAmplitude_meiling.ini" << endl;
        return -1;
    }
    while(ampFile->getStringLine(&tmpLine)){
        tmpStr = ampFile->linesplit(tmpLine,'=');
        if(tmpStr[0] == ",")break;
        if(isOverlapString(chk_overlap,ampFile->trim(tmpStr[1]))){
            return -1;
        }else{
            chk_overlap += ampFile->trim(tmpStr[1]);
        }
        name2Amplitude[ampFile->trim(tmpStr[1])] = std::stod(ampFile->trim(tmpStr[0]));
    }

    //絶対時間軸ファイル読み込み
    if(isAdvanceMode){
        LoadingFile* adiFile = new LoadingFile("defaultAdvantage_meiling.ini",READ);
        if (!adiFile->isThereFile()){
            cerr << "ファイルの読み込みに失敗しました" << endl;
            cerr << "No such a file: " << "defaultAdvantage_meiling.ini" << endl;
            return -1;
        }
        //軸名
        while(adiFile->getStringLine(&tmpLine)){
            tmpStr = adiFile->linesplit(tmpLine,'=');
            if(tmpStr[0] == ",")break;
            if(isOverlapString(chk_overlap,adiFile->trim(tmpStr[1]))){
                return -1;
            }else{
                chk_overlap += adiFile->trim(tmpStr[1]);
            }
            name2Additional[adiFile->trim(tmpStr[1])] = adiFile->trim(tmpStr[0]);
        }
        //ADSR名
        while(adiFile->getStringLine(&tmpLine)){
            tmpStr = adiFile->linesplit(tmpLine,'=');
            if(tmpStr[0] == ",")break;
            if(isOverlapString(chk_overlap,adiFile->trim(tmpStr[1]))){
                return -1;
            }else{
                chk_overlap += adiFile->trim(tmpStr[1]);
            }
            name2ADSR[adiFile->trim(tmpStr[1])] = adiFile->trim(tmpStr[0]);
        }
    }else{
        name2Additional["dummy"] = -1;
        name2ADSR["dummy"] = -1;
    }

    //Hz指定読み込み
    if(isHertzMode){
        LoadingFile* hzFile = new LoadingFile("defaultHertz_meiling.ini",READ);
        if (!hzFile->isThereFile()){
            cerr << "ファイルの読み込みに失敗しました" << endl;
            cerr << "No such a file: " << "defaultHertz_meiling.ini" << endl;
            return -1;
        }
        while(hzFile->getStringLine(&tmpLine)){
            tmpStr = hzFile->linesplit(tmpLine,'=');
            if(tmpStr[0] == ",")break;
            if(isOverlapString(chk_overlap,hzFile->trim(tmpStr[1]))){
                return -1;
            }else{
                chk_overlap += hzFile->trim(tmpStr[1]);
            }
            name2Pitch[hzFile->trim(tmpStr[1])] = std::stod(hzFile->trim(tmpStr[0]));
        }
    }else{
        name2Pitch["dummy"] = -1;
    }   

    //ファイルからの読み込みここまで    

    //A0-C8ぐらいまで割り当て
    onkai[0] = base_Hz;
    for(int i = 1; i < kenban; i++){
        onkai[i] = onkai[i-1] * 1.0594631;
    }
    //配列最後は0Hz
    onkai[kenban] = 0;


    //音データの作成
    cout << "Create SoundData..." << endl;

    if(isMergeFile){
        string path;
        //保存したファイル名全部なくなるまでイベント作成
        while(!filePaths.empty()){
            path = filePaths.front();
            cout << "load this...:" << path << endl;
            LoadingFile* melodyFile = new LoadingFile(path,READ);
            if (!melodyFile->isThereFile()){
                cerr << "ファイルの読み込みに失敗しました" << endl;
                cerr << "No such a file:" << path << endl;
                return -1;
            }
            createSoundData(melodyFile);
            filePaths.erase(filePaths.begin());
        }
    }else{
        //ぶれいんふぁっくファイル読み込み
        LoadingFile* melodyFile = new LoadingFile(argv[argc - 1],READ);
        if (!melodyFile->isThereFile()){
            cerr << "ファイルの読み込みに失敗しました" << endl;
            cerr << "No such a file:" << argv[argc - 1] << endl;
            return -1;
        }
        createSoundData(melodyFile);
    }
    
    cout << "Events:" << soundEvents.size() << endl;

    //wavファイルの大きさ決定
    for(int i = 0; i < soundEvents.size();i++){
        if(soundEvents[i].EndTime > fileSize){
            fileSize = soundEvents[i].EndTime;
        }
    }
    
    //新規ファイル作成
    fpw = fopen("melody_meilingSeque.wav", "wb");
    if (fpw == NULL) exit(EXIT_FAILURE);
    //メモリ確保
    pbuff = (short int*) malloc(sizeof(short int) * fileSize);
    headBuff = (char*) malloc(sizeof(char) * 44);
    if (pbuff == NULL || headBuff == NULL) exit(EXIT_FAILURE);
    
    printf("filesize:%d\n",fileSize);

    ////////////////////////////////////////////////////
    cout << "create wavHeader..." << endl;
    //ステレオとか対応してるわけないでしょ(笑)
    //なんでそこまでやらなきゃいけないんですかね...
    createHeader(sizeof(short int) * fileSize + sizeof(char) * 44,false);
    cout << "create Music..." << endl;
    createMusic(fileSize);
    ///////////////////////////////////////////////////
    cout << "wrote:" << fwrite(pbuff, sizeof(short int), fileSize, fpw) << endl;
    fclose(fpw);
    free(pbuff);
    free(headBuff);

    cout << "End MeilingSequencer" << endl;

    return 0;
}



void createSoundData(LoadingFile* melodyFile){
    cout << "Crateing..." << endl;
    //読み込みファイルの内容確認
    vector<string> splitedLine;
    string line;
    bool isCommentOut = false;
    int mozi_count = 0;
    //絶対時間軸、この位置に音符を置いていく
    long absolute_TimePosition = 0;
    //置く音符の長さデフォ4分音符
    int note_Length = samplingfreq  / (bpm / 60);
    //付点用の現在の音符の長さ
    double noteLength_Number = 4;
    //振幅
    double amplitude = 5000;
    //ADSRデフォ値
    int atk_AmpRat = 0;
    int atk_PosRat = 100;
    int dec_PosRat = 101;
    int sus_AmpRat = RATIO_UPLIMIT;
    int sus_PosRat = 9900;
    int rel_PosRat = RATIO_UPLIMIT;

    //文字読み取りtmp
    unsigned char lead = 0;//tmp
    int char_size = 0; //

    char octave_count = 0;
    bool ti_Flag = false;
    int ti_Length = 0;


    //1行ずつ解析
    while (melodyFile->getStringLine(&line)){
        //コメントアウト*********
        if(line[0] == '/' && line[1] == '/' && !isCommentOut)continue;
        if(line[0] == '/' && line[1] == '*' && !isCommentOut){
            isCommentOut = true;
            continue;
        }
        if(line[0] == '*' && line[1] == '/' && isCommentOut){
            isCommentOut = false;
            continue;
        }
        if(isCommentOut)continue;
        //コメントアウトここまで&&&

        //コメント以外解析ここから***********
        /*
        line : 一行分
        splitedLine : 1行中の,で区切られたやつ全部
        l1:100,80,90の場合
        splitedLine[0] = 100 
                   [1] = 80 
                   [2] = 90
        */        
        splitedLine = melodyFile->linesplit(line,',');
        //1区切り(,)ずつ解析
        for(int i = 0;i < splitedLine.size(); i++){
            //1区切りの中の1文字ずつ解析
            for(int j = 0; j < splitedLine[i].size(); j += char_size){
                //OSにより文字の大きさが違うため、ここで飛ばしサイズを決定する
                lead = splitedLine[i][j];
                if (lead < 128) {
                    char_size = 1;
                } else if (lead < 224) {
                    char_size = 2;
                } else if (lead < 240) {
                    char_size = 3;
                } else {
                    char_size = 4;
                }
                
                //1つの文字取得
                string note = splitedLine[i].substr(j,char_size).c_str();

                //ADSR用に数値読み取り
                if(name2ADSR.find(note) != name2ADSR.end()){
                    string tmp = name2ADSR.find(note)->second;
                    //0-9の数値が続く限り読み取り
                    int count = 0;
                    while(splitedLine[i][j + char_size + count] >= '0' && splitedLine[i][j + char_size + count] <= '9'){
                        count++;
                    };
                    //変更する数値を入れる
                    string nums = "";//substrでできない悲哀のあれ
                    for(int num_str = 0;num_str < count; num_str++){
                        nums += splitedLine[i][j + char_size + num_str];
                    }
                    int num = stoi(nums);
                    //あとは列挙して一致したやつに入れる
                    if(tmp == "Ata")atk_AmpRat = num;
                    if(tmp == "Atp")atk_PosRat = num;
                    if(tmp == "Dee")dec_PosRat = num;
                    if(tmp == "Sua")sus_AmpRat = num;
                    if(tmp == "Sue")sus_PosRat = num;
                    if(tmp == "Ree")rel_PosRat = num;

                    char_size += count;
                    continue;
                }
    

                //音符の長さ
                if( name2Length.find(note) != name2Length.end() ){
                    note_Length = (samplingfreq  / (bpm / 60)) * 4 / name2Length.find(note)->second;
                    if(ti_Flag){
                        note_Length += ti_Length;
                        ti_Flag = false;
                        ti_Length = 0;
                    }
                    noteLength_Number = name2Length.find(note)->second;
                    continue;
                }

                //音符長さ追加、オクターブ
                if( name2AdLength.find(note) != name2AdLength.end() ){
                    //付点
                    if(name2AdLength.find(note)->second == "Hf"){
                        noteLength_Number = noteLength_Number * 2;
                        note_Length += (samplingfreq  / (bpm / 60)) * 4 / noteLength_Number;
                    }

                    //n連符:nr
                    if(name2AdLength.find(note)->second.find("r") != -1){
                        string len = melodyFile->trim(name2AdLength.find(note)->second,"r");
                        noteLength_Number = noteLength_Number * std::stod(len);
                        note_Length += (samplingfreq  / (bpm / 60)) * 4 / noteLength_Number;
                    }

                    //オクターブあげ
                    if(name2AdLength.find(note)->second.find("va") != -1){
                        octave_count++;
                    }
                    //オクターブさげ
                    if(name2AdLength.find(note)->second.find("vb") != -1){
                        octave_count--;
                    }

                    //直前の長さを保存し次の長さが来た時に加える
                    if(name2AdLength.find(note)->second == "ti" && !ti_Flag){
                        ti_Length = note_Length;
                        ti_Flag = true;
                    }
                    
                    if(name2AdLength.find(note)->second != "Nz"){
                        continue;
                    }
                }

                //振幅
                if(name2Amplitude.find(note) != name2Amplitude.end()){
                    amplitude = name2Amplitude.find(note)->second;
                    continue;
                }

                /*振幅や音符の長さなど諸々の更新ここまで*/


                //イベントリストに突っ込む用
                SoundData tmp_SD;
                //ADSRの更新
                tmp_SD.atk_SinpukuRatio  = atk_AmpRat;
                tmp_SD.atk_PositionRatio = atk_PosRat;
                tmp_SD.dec_PositionRatio = dec_PosRat;
                tmp_SD.sus_SinpukuRatio  = sus_AmpRat;
                tmp_SD.sus_PositionRatio = sus_PosRat;
                tmp_SD.rel_PositionRatio = rel_PosRat;

                //事前に指定した文字と一致する場合はその指定Hzなどをいれる
                if( name2Scale.find(note) != name2Scale.end() ) {
                    int tmp = name2Scale.find(note)->second;
                    if(tmp != kenban){
                        tmp += octave_count*12;
                    }
                    if(tmp < 0) tmp = 0;
                    else if(tmp > kenban) tmp = kenban;
                    tmp_SD.hz = onkai[tmp];
                    tmp_SD.sinpuku = amplitude;
                    //44100 / 120/m 
                    tmp_SD.StartTime = absolute_TimePosition;
                    tmp_SD.EndTime   = absolute_TimePosition + note_Length;
                    soundEvents.push_back(tmp_SD);
                }

                //ノイズ
                if( name2AdLength.find(note) != name2AdLength.end() &&
                    name2AdLength.find(note)->second == "Nz") {
                    tmp_SD.hz = -1;
                    tmp_SD.sinpuku = amplitude;
                    tmp_SD.StartTime = absolute_TimePosition;
                    tmp_SD.EndTime   = absolute_TimePosition + note_Length;
                    soundEvents.push_back(tmp_SD);
                }

                //指定Hz
                if(isHertzMode){
                    if( name2Pitch.find(note) != name2Pitch.end() ) {
                        tmp_SD.hz = name2Scale.find(note)->second;
                        tmp_SD.sinpuku = amplitude;
                        //44100 / 120/m 
                        tmp_SD.StartTime = absolute_TimePosition;
                        tmp_SD.EndTime   = absolute_TimePosition + note_Length;
                        soundEvents.push_back(tmp_SD);
                    }
                }

                //絶対時間軸かどうか
                if(!isAdvanceMode){
                    absolute_TimePosition += note_Length;
                }else{
                    if(name2Additional.find(note) != name2Additional.end()){
                        if(name2Additional.find(note)->second == "Z"){
                            absolute_TimePosition += note_Length;
                        }
                    }
                }
            }//1文字解析ここまで
        }//1区切り解析ここまで&&&&&&&&&&&
        cout << line << endl;
    }//whileここまで
}



void createMusic(int file_size){

    std::vector<soundData> musicArray;
    std::vector<int> eraseNumber;
    //作成データに基づきファイルに書き込み
    for(int i = 0; i < file_size; i++){
        for(int j = 0; j < soundEvents.size(); j++){
            //頭悪いから計算回数多い手順
            if(soundEvents[j].StartTime < i && i < soundEvents[j].EndTime){
                musicArray.push_back(soundEvents[j]);
            }else if(i > soundEvents[j].EndTime){
                eraseNumber.push_back(j - eraseNumber.size());
            }
        }
        *(pbuff+i) += createWaves(i,musicArray);
        musicArray.clear();
        //鳴り終わったら消していくで
        if(!eraseNumber.empty()){
            for(int k = 0;k < eraseNumber.size();k++){
                soundEvents.erase(soundEvents.begin() + eraseNumber[k]);
            }
        }
        eraseNumber.clear();
    }
    printf("EndCreate\n");
}


short int createWaves(int pos,vector<soundData> hz_data){
    //hz_data:1番目[hz,sinpuku,soundTime],2...
    
    double res = 0;
    //波形合成
    for(int i = 0; i < hz_data.size(); i++){
        res += getAmplitude(pos, hz_data[i]);
    }
    
    if(res > 32765){
        res = 32765;
    }else if(res < -32765){
        res = -32765;
    }
    return (short int)res;
}

/*
10000分率
Ata:一番最初の振幅の大きさ %
Atp:ピークの位置 %
Dee:decay終了位置 %
Sua:伸ばすときの振幅の大きさ %
Sue:伸ばし終了位置 %
Ree:減衰終了位置 %
(減衰は0を目指す)
int atk_SinpukuRatio;
int atk_PositionRation;
int dec_PositionRation;
int sus_SinpukuRatio;
int sus_PositionRatio;
int rel_PositionRatio;
*/
//流石にこれらを何回もメモリ確保すんのあれなきがする
long startTime = 0;
long endTime = 0;
long toneTime = 0;
long atack_Pos = 0;
long decay_Pos = 0;
long susta_Pos = 0;
long relea_Pos = 0;
double atkAmpRat = 0;
double susAmpRat = 0;
double calic_Amp = 0;
double movingPoint_p = 0;
long x = 0;
//まどかんすうつかってしんぷくをけいさん
//なぜこんな面倒なことをしてしまったのか
double getAmplitude(int pos, soundData data){
    //ぶっちゃけ合ってる自信ねぇわ
    //アタックのタイミングでmovingPoint_pが1/2になるように調整
    //relea_Posで1を目指す
    startTime = data.StartTime;
    endTime = data.EndTime;
    toneTime = endTime - startTime;
    x = pos - startTime;

    atack_Pos = toneTime * data.atk_PositionRatio / RATIO_UPLIMIT;
    decay_Pos = toneTime * data.dec_PositionRatio / RATIO_UPLIMIT;
    susta_Pos = toneTime * data.sus_PositionRatio / RATIO_UPLIMIT;
    relea_Pos = toneTime * data.rel_PositionRatio / RATIO_UPLIMIT;
    atkAmpRat = data.atk_SinpukuRatio / (double)(RATIO_UPLIMIT * 2.0);
    susAmpRat = 1.0 - data.sus_SinpukuRatio / (double)(RATIO_UPLIMIT * 2.0);

    //release
    if(susta_Pos <= x && x < relea_Pos){
        if(relea_Pos - susta_Pos == 0){
            movingPoint_p = 0;
        }else{
            movingPoint_p = ((1.0 - susAmpRat) / (double)(relea_Pos - susta_Pos)) * (double)(x - relea_Pos) + 1.0;
        }
    }
    //sustain
    if(decay_Pos <= x && x < susta_Pos){
        if(atack_Pos - decay_Pos == 0){
            movingPoint_p = susAmpRat;
        }else{
            movingPoint_p = ((0.5 - susAmpRat) / (double)(atack_Pos - decay_Pos)) * (double)(decay_Pos - atack_Pos) + 0.5;
        }
    }
    //decay
    if(atack_Pos <= x && x < decay_Pos){
        if(atack_Pos - decay_Pos == 0){
            movingPoint_p = susAmpRat;
        }else{
            movingPoint_p = ((0.5 - susAmpRat) / (double)(atack_Pos - decay_Pos)) * (double)(x - atack_Pos) + 0.5;
        }
    }
    //attack
    if(x < atack_Pos){
        if(atack_Pos == 0){
            movingPoint_p = atkAmpRat;
        }else{
            movingPoint_p = ((0.5 - atkAmpRat) / (double)atack_Pos) * (double)x + atkAmpRat;
        }
    }
    //atkのタイミングでの値を入れて
    if(data.hz == -1){
        calic_Amp = data.sinpuku * ((double)rand() / RAND_MAX - 0.5);
    }else{
        calic_Amp = data.sinpuku * sinf(x * step * data.hz);
    }
    //窓で振幅を決定
    calic_Amp *= (0.5 - 0.5 * cos(2.0 * pi * movingPoint_p));

    return calic_Amp;
}

bool isOverlapString(string moziretu,string chk_char){
    if(moziretu.find(chk_char) != -1){
        cout << "Overlap Character:" << chk_char << endl;
        return true;
    }
    return false;
}


void createHeader(int data_size,bool isStereo) {
    /*すげー冗長に見える*/
    int sampleRate = (int)samplingfreq;
    char header[44];
    headBuff[0] = 'R';
    headBuff[1] = 'I';
    headBuff[2] = 'F';
    headBuff[3] = 'F';
    const char* size = int2Chars((data_size - 8));  // ファイルサイズ-8バイト数
    for(int i = 4; i < 8; i++)headBuff[i] = size[i-4];
    headBuff[8] = 'W'; 
    headBuff[9] = 'A'; 
    headBuff[10] = 'V'; 
    headBuff[11] = 'E'; 
    headBuff[12] = 'f'; 
    headBuff[13] = 'm'; 
    headBuff[14] = 't'; 
    headBuff[15] = ' '; 
    headBuff[16] = 16; 
    for(int i = 17; i < 20; i++)headBuff[i] = 0;
    headBuff[20] = 1; // フォーマットID 1 =リニアPCM  ID3=IEEE_FLOAT,
    headBuff[21] = 0; 
    headBuff[22] = isStereo ? 2 : 1; //チャンネル 1 = モノラル チャンネル 2 = ステレオ
    headBuff[23] = 0; 
    const char* sample = int2Chars(sampleRate);// サンプルレート
    for(int i = 24; i < 28; i++)headBuff[i] = sample[i-24];
    // バイト/秒 = サンプルレート x 2チャンネル x 2バイト(16bit)
    // バイト/秒 = サンプルレート x 1チャンネル x 2バイト(short)
    const char* byteParsec = isStereo ? int2Chars(sampleRate * 4) : int2Chars(sampleRate * 2);;
    for(int i = 28; i < 32; i++)headBuff[i] = byteParsec[i-28];
    headBuff[32] = isStereo ? 4 : 2;// ブロックサイズ4バイト(2チャンネル x 2バイト)
    headBuff[33] = 0; 
    headBuff[34] = 16; 
    headBuff[35] = 0; 
    headBuff[36] = 'd'; 
    headBuff[37] = 'a'; 
    headBuff[38] = 't';
    headBuff[39] = 'a';  
    const char* datasz = int2Chars(data_size-44);         // データサイズ
    for(int i = 40; i < 44; i++)headBuff[i] = datasz[i-40];

    fwrite(headBuff, sizeof(char), 44, fpw);
}

//int型32ビットデータをリトルエンディアンのバイト配列にする
char* int2Chars(int value) {
    char* bt = new char[4];
    bt[0] = (char)(value & 0x000000ff);
    bt[1] = (char)((value & 0x0000ff00) >> 8);
    bt[2] = (char)((value & 0x00ff0000) >> 16);
    bt[3] = (char)((value & 0xff000000) >> 24);
    return bt;
}
