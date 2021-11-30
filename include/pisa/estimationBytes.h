//
//  estimationBytes.h
//  estimationBytes.cpp
//
//  Created by Joshua Lee on 11/29/21.
//

#ifndef estimationBytes_h
#define estimationBytes_h
#include <iostream>
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
namespace pisa{
namespace bp{

uint32_t GammaEncoding(const std::vector<uint32_t>& postingList);
int VarByteEncoding(const std::vector<uint32_t>& postingList);
uint32_t max(uint32_t* start, size_t length);
int Simple9(const std::vector<uint32_t>& postingList);
int blockSizePFD(const std::vector<uint32_t>& postingList, int bstr, int index);
int OptPFD(const std::vector<uint32_t>& postingList);
void writeBytes(const std::vector<uint32_t>& postingList, int range);
}
}
#endif /* estimationBytes_h */
