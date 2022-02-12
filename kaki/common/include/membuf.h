//
// Created by felix on 12/02/2022.
//

#pragma once
#include <streambuf>
#include <span>

struct membuf : std::streambuf
{
    membuf(char* begin, char* end) {
        this->setg(begin, begin, end);
    }

    explicit membuf(std::span<uint8_t> span) {
        this->setg((char *) span.begin().base(), (char *) span.begin().base(), (char *) span.end().base());
    }
};