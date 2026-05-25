#pragma once
#include <SingleHeader/MetalCpp.h>
#include <iostream>
namespace nitro::rhi::metal
{
    inline void checkNSError(NS::Error *error, std::string message = "Something went wrong on metal")
    {
        if (!error)
            return;
        throw std::runtime_error(message + " " + error->description()->utf8String());
    }
};
