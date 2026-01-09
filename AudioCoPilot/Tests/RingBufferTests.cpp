#include <catch2/catch_all.hpp>
#include "Core/RingBuffer.h"

TEST_CASE("RingBuffer: Basic write and read", "[ringbuffer]")
{
    RingBuffer<float> buffer(16);
    
    float writeData[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float readData[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    
    SECTION("Write and read back correctly")
    {
        size_t written = buffer.write(writeData, 4);
        REQUIRE(written == 4);
        
        size_t read = buffer.read(readData, 4);
        REQUIRE(read == 4);
        
        REQUIRE(readData[0] == 1.0f);
        REQUIRE(readData[1] == 2.0f);
        REQUIRE(readData[2] == 3.0f);
        REQUIRE(readData[3] == 4.0f);
    }
    
    SECTION("Reports available correctly")
    {
        REQUIRE(buffer.availableToRead() == 0);
        
        buffer.write(writeData, 4);
        REQUIRE(buffer.availableToRead() == 4);
        
        buffer.read(readData, 2);
        REQUIRE(buffer.availableToRead() == 2);
    }
    
    SECTION("Clear works")
    {
        buffer.write(writeData, 4);
        buffer.clear();
        REQUIRE(buffer.availableToRead() == 0);
    }
}

TEST_CASE("RingBuffer: Overflow handling", "[ringbuffer]")
{
    RingBuffer<float> buffer(8); // Capacidade 8, mas só 7 úteis (SPSC)
    
    float data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    SECTION("Stops writing when full")
    {
        size_t written = buffer.write(data, 10);
        REQUIRE(written == 7); // Máximo é capacity - 1
    }
}

