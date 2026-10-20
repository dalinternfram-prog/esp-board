
#pragma once
#include "file_manager.hpp"
#include <string>
#include <vector>
#include <memory>
#include <ArduinoJson.h>


namespace file {

constexpr std::string FileVectorTag = "FileVectorTag";


    class fileVector{
// {fileName}
//{fileName}json;
//json describe how many elements;each vector adding time,offset,size;

struct fileVectorJson{
size_t byteSize;
int offset;
uint32_t AddingTime; 
};

protected:
std::shared_ptr<FileManager>FileManager_;
std::string fileName{};

fileVector(std::string fileName,std::shared_ptr<FileManager>FileManager_);



public:


void push_back(const std::vector<uint8_t>&data);


void GetCurrentSize();

std::vector<uint8_t> Fetch(int i);


std::vector<uint8_t> Pop();

 std::vector<uint8_t> PopFront();



    };
}