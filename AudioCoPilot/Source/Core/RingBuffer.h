#pragma once

#include <atomic>
#include <vector>
#include <cstring>

/**
 * Lock-free ring buffer para comunicação entre audio thread e UI thread.
 * Single-producer, single-consumer (SPSC).
 */
template<typename T>
class RingBuffer
{
public:
    explicit RingBuffer(size_t capacity)
        : _capacity(capacity)
        , _buffer(capacity)
        , _writeIndex(0)
        , _readIndex(0)
    {
    }

    /**
     * Escreve dados no buffer (chamado do audio thread)
     * @return número de amostras escritas
     */
    size_t write(const T* data, size_t count)
    {
        size_t written = 0;
        size_t writeIdx = _writeIndex.load(std::memory_order_relaxed);
        size_t readIdx = _readIndex.load(std::memory_order_acquire);
        
        while (written < count)
        {
            size_t nextWrite = (writeIdx + 1) % _capacity;
            
            if (nextWrite == readIdx)
                break; // Buffer cheio
            
            _buffer[writeIdx] = data[written];
            writeIdx = nextWrite;
            ++written;
        }
        
        _writeIndex.store(writeIdx, std::memory_order_release);
        return written;
    }

    /**
     * Lê dados do buffer (chamado da UI thread)
     * @return número de amostras lidas
     */
    size_t read(T* data, size_t count)
    {
        size_t readCount = 0;
        size_t readIdx = _readIndex.load(std::memory_order_relaxed);
        size_t writeIdx = _writeIndex.load(std::memory_order_acquire);
        
        while (readCount < count)
        {
            if (readIdx == writeIdx)
                break; // Buffer vazio
            
            data[readCount] = _buffer[readIdx];
            readIdx = (readIdx + 1) % _capacity;
            ++readCount;
        }
        
        _readIndex.store(readIdx, std::memory_order_release);
        return readCount;
    }

    /**
     * Retorna quantas amostras estão disponíveis para leitura
     */
    size_t availableToRead() const
    {
        size_t writeIdx = _writeIndex.load(std::memory_order_acquire);
        size_t readIdx = _readIndex.load(std::memory_order_relaxed);
        
        if (writeIdx >= readIdx)
            return writeIdx - readIdx;
        else
            return _capacity - readIdx + writeIdx;
    }

    /**
     * Retorna quantas amostras podem ser escritas
     */
    size_t availableToWrite() const
    {
        return _capacity - availableToRead() - 1;
    }

    /**
     * Limpa o buffer
     */
    void clear()
    {
        _readIndex.store(0, std::memory_order_relaxed);
        _writeIndex.store(0, std::memory_order_release);
    }

private:
    const size_t _capacity;
    std::vector<T> _buffer;
    std::atomic<size_t> _writeIndex;
    std::atomic<size_t> _readIndex;
};

