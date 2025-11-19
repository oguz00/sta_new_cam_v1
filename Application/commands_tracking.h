#ifndef COMMANDS_TRACKING_H_
#define COMMANDS_TRACKING_H_

#include <cstdio>






/**
 * @brief Komut verisini temsil eden yapı.
 */
constexpr size_t MAX_CMD_LENGTH = 32;
struct cmdBlock {
    uint8_t data[MAX_CMD_LENGTH]; // sabit uzunlukta buffer
    size_t length;
    queryBitEnum nmbr;
};

/**
 * @class KomutRingBuffer
 * @brief KomutBlok nesnelerini tutan dairesel kuyruk yapısı.
 */
class cmdRingBuffer {
public:
    cmdRingBuffer() : head(0), tail(0), count(0) {}

    /**
     * @brief Kuyruğa yeni bir komut ekler.
     * @param veri Veri bloğunun adresi
     * @param uzunluk Veri uzunluğu
     * @param id Komut kimliği
     * @return true başarıyla eklendiyse, false kuyruk doluysa
     */
    bool push(uint8_t* p_data, size_t len,queryBitEnum nbr_buff) {
        if (isFull() || len> MAX_CMD_LENGTH ) return false;
        memcpy(buffer[head].data,p_data, len);
        buffer[head].length = len;
        buffer[head].nmbr=nbr_buff;
        head = (head + 1) % BUFFER_SIZE;
        count++;
        return true;
    }

    /**
     * @brief Kuyruktan bir komut çıkarır.
     * @param blok Çıkarılan komut buraya yazılır
     * @return true başarıyla alındıysa, false kuyruk boşsa
     */
    bool pop(cmdBlock& block) {
        if (isEmpty()) return false;
        block = buffer[tail];
        tail = (tail + 1) % BUFFER_SIZE;
        count--;
        return true;
    }

    /**
     * @brief Kuyruğun boş olup olmadığını kontrol eder.
     */
    bool isEmpty() const { return count == 0; }

    /**
     * @brief Kuyruğun dolu olup olmadığını kontrol eder.
     */
    bool isFull() const { return count == BUFFER_SIZE; }

    /**
     * @brief Kuyruktaki eleman sayısını döndürür.
     */
    size_t size() const { return count; }

private:
    static constexpr size_t BUFFER_SIZE = 16;
    cmdBlock buffer[BUFFER_SIZE];
    size_t head;
    size_t tail;
    size_t count;
};




#endif /* COMMANDS_TRACKING_H_ */
