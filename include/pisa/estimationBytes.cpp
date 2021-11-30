//
//  main.cpp
//  estimationBytes.cpp
//
//  Created by Joshua Lee on 11/29/21.
//

#include "estimationBytes.h"

namespace pisa{

namespace bp{
void writeBytes(const std::vector<uint32_t>& postingList, int range){
    int estimation = OptPFD(postingList);
    std::ofstream myfile("invData.txt", std::ios::app);
    myfile << estimation << " " << range << " " << postingList.size() << std::endl;
    myfile.close();
}

uint32_t GammaEncoding(const std::vector<uint32_t>& postingList){
    int last = 0;
    int countBits = 2*(floor(log2(postingList[0])))+1;
    size_t i = 0;
    while (i < postingList.size()){
        std::cout << countBits << std::endl;
        int current = postingList[i];
        int delta = current - last;
        last = current;
        countBits += 2*(floor(log2(delta)))+1;
        i +=1;
    }
    return (countBits/8+1);
}

int VarByteEncoding(const std::vector<uint32_t>& postingList){
    int last = 0;
    int countBytes = 0;
    size_t i = 0;
    while (i < postingList.size()){
        std::cout << countBytes << std::endl;
        int current = postingList[i];
        int delta = current - last;
        last = current;
        while (delta >= 128){
            delta = delta/128;
            countBytes += 1;
        }
        countBytes += 1;
        i +=1;
    }
    return countBytes;
}
uint32_t max(uint32_t* start, size_t length){
    uint32_t curr_max = 0;
    size_t i = 0;
    while (i < length){
        if (start[i]>curr_max){
            curr_max = start[i];
        }
    }
    return curr_max;
}
int Simple9(const std::vector<uint32_t>& postingList){
    uint32_t newList[postingList.size()];
    newList[0] = postingList[0];
    for (size_t y = 1; y < postingList.size(); ++y){
        newList[y] = postingList[y]-postingList[y-1]-1;
    }
    size_t i = 0;
    int countBytes = 0;
    while (i < postingList.size()){
        std::cout << countBytes << std::endl;
        if ((postingList.size()-1-i>= 28) && (max(newList+i,28))<=1){
            i+=28;
        }
        else if ((postingList.size()-1-i >= 14) && (max(newList+i,14)) <=3){
            i+=14;
        }
        else if ((postingList.size()-1-i >= 9) && (max(newList+i,9)) <=7){
            i+=9;
        }
        else if ((postingList.size()-1-i >= 7) && (max(newList+i,7)) <=15){
            i+=7;
        }
        else if ((postingList.size()-1-i >= 5) && (max(newList+i,5)) <=31){
            i+=5;
        }
        else if ((postingList.size()-1-i >= 4) && (max(newList+i,4)) <=127){
            i+=4;
        }
        else if ((postingList.size()-1-i >= 3) && (max(newList+i,3)) <=511){
            i+=3;
        }
        else if ((postingList.size()-1-i >= 2) && (max(newList+i,2)) <=16383){
            i+=2;
        }
        else{ //assuming all numbers are less than 268435456 (2^28)
            i+=1;
        }
        countBytes +=4;
    }
    return countBytes;
    
}

int blockSizePFD(const std::vector<uint32_t>& postingList, int bstr, int index){
    int offsetCount = -1; //in case the first number in postingList overflows
    std::vector<uint32_t> offset;
    std::vector<uint32_t> higherBits;
    for (size_t y = index; y < fmin(index+128,postingList.size()); ++y){
        if (postingList[y] > pow(2,bstr)-1){
            int shiftNum = postingList[y] >> bstr;
            higherBits.push_back(shiftNum);
            offset.push_back(offsetCount+1);
            offsetCount = 0;
        }
        else{
            offsetCount += 1;
        }
    }
    return (Simple9(higherBits)+Simple9(offset)+bstr*16); //divide 128 by 8 for bytes
    
}
int OptPFD(const std::vector<uint32_t>& postingList){
    std::vector<int> bstrVals = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,16, 20, 32};
    int countBytes = 0;
    int i = 0;
    uint32_t newList[postingList.size()];
    newList[0] = postingList[0];
    for (size_t y = 1; y < postingList.size(); ++y){
        newList[y] = postingList[y]-postingList[y-1]-1;
    }
    while (i < postingList.size()){
        std::vector<int> byteSizes;
        for (int bstr: bstrVals){
            byteSizes.push_back(blockSizePFD(postingList,bstr,i));
        }
        countBytes += *std::min_element(bstrVals.begin(),bstrVals.end());
        i+=128;
    }
    return countBytes;
}
}
}
