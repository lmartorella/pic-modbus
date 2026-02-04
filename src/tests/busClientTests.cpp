#include <catch2/catch.hpp>
#include <stdbool.h>
#include <string.h>
#include <queue>

#include "pic-modbus/bus_client.h"
#include "pic-modbus/crc.h"
#include "pic-modbus/rs485.h"

using namespace std::string_literals;

extern "C" {
    RS485_LINE_STATE rs485_state;
    bool rs485_isMarkCondition;
    uint8_t rs485_buffer[RS485_BUF_SIZE];
}

union BigEndian {
    uint16_t be;
    struct {
        uint8_t b0;
        uint8_t b1;
    };

    static BigEndian fromH(uint16_t value) {
        BigEndian ret;
        ret.be = htobe16(value);
        return ret;
    }
};

class Rs485Mock {
private:
    int receivePointer;
    std::vector<uint8_t> lastDataWritten;
public:
    // For CRC calculation
    std::vector<uint8_t> packetData;

    Rs485Mock() {
        reset();
    }

    bool writeInProgress() {
        return false;
    }

    void write(int size) {
        if (rs485_state != RS485_LINE_TX) {
            crc_reset();
        }
        rs485_isMarkCondition = false;
        rs485_state = RS485_LINE_TX;
        for (auto i = 0; i < size; ++i) {
            lastDataWritten.push_back(rs485_buffer[i]);
            crc_update(rs485_buffer[i]);
        }
    }

    int readAvail() {
        return receivePointer;
    }

    void discard(int size) {
        if (receivePointer != size) {
            throw std::runtime_error("Discard called with the wrong current buffer size");
        }
        for (int i = 0; i < size; i++) {
            crc_update(rs485_buffer[i]);
        }
        receivePointer = 0;
    }

    void reset() {
        receivePointer = 0;
        lastDataWritten.clear();
        packetData.clear();
    }

    void simulateData(const std::vector<uint8_t>& data) {
        for (auto it = data.begin(); it != data.end(); ++it, receivePointer++) {
            rs485_buffer[receivePointer] = *it;
            packetData.push_back(*it);
        }
        rs485_isMarkCondition = false;
    }

    void simulateData(const std::vector<BigEndian>& data) {
        for (auto it = data.begin(); it != data.end(); ++it) {
            simulateData({ it->b0, it->b1 });
        }
    }

    void simulateMark() {
        rs485_isMarkCondition = true;
        crc_reset();
        packetData.clear();
    }

    std::vector<uint8_t> getDataWritten() {
        return lastDataWritten;
    }
};

static Rs485Mock rs485mock;

extern "C" {
    _Bool rs485_writeInProgress() {
        return rs485mock.writeInProgress();
    }

    void rs485_write(uint8_t size) {
        rs485mock.write(size);
    }

    uint8_t rs485_readAvail() {
        return (uint8_t)rs485mock.readAvail();
    }

    void rs485_discard(uint8_t size) {
        rs485mock.discard(size);
    }
}

static std::vector<uint8_t> operator+ (const std::vector<uint8_t>& vec1, const std::vector<uint8_t>& vec2) {
    std::vector<uint8_t> ret(vec1);
    ret.insert(ret.end(), vec2.begin(), vec2.end());
    return ret;
}

// In crc16.cpp
extern uint16_t calcCrc16(uint16_t prevCrc, const uint8_t* buffer, int wLength);

class RegisterRange { 
    bool readyForRead;
    bool isWritten;

    std::vector<uint8_t> bufferToSend;
    std::vector<uint8_t> bufferReceived;
public:
    int address;
    const int writeSize; // in register count
    const int readSize; // in register count

    RegisterRange(int address, int readSize, int writeSize)
        :address(address), readSize(readSize), writeSize(writeSize)
    { 
        reset();
    }

    void reset() {
        bufferToSend.clear();
        bufferReceived.clear();
        readyForRead = false;
        isWritten = false;
    }

    /**
     * The function handler that produces the function response data to send to the server
     * during a read call. The buffer size is `readSize`.
     */
    void onSend() {
        REQUIRE(readyForRead);
        uint8_t* di = rs485_buffer;
        int toRead = std::min(RS485_BUF_SIZE, (int)(bufferToSend.size()));
        for (int i = 0; i < toRead; i++, di++) {
            *di = bufferToSend[0];
            bufferToSend.erase(bufferToSend.begin());
        }

        // Done
        if (bufferToSend.size() == 0) {
            reset();
        }
    }

    /**
     * The function handler that consumes the function data sent by the server
     * during a write call. The buffer size is `writeSize`.
     */
    bool onReceive() {
        REQUIRE(!isWritten);
        int toWrite = std::min(RS485_BUF_SIZE, (int)((writeSize * 2) - bufferReceived.size()));
        const uint8_t* si = rs485_buffer;
        for (int i = 0; i < toWrite; i++, si++) {
            bufferReceived.push_back(*si);
        }

        if (bufferReceived.size() == writeSize * 2) {
            isWritten = true;
        }
        return true;
    }

    void prepareDataToSend(const std::vector<uint8_t>& data) {
        REQUIRE(!readyForRead);
        readyForRead = true;
        bufferToSend = data;
    }
    
    void checkDataReceived(const std::vector<uint8_t>& data) {
        REQUIRE(isWritten);
        REQUIRE(bufferReceived == data);
        // Done
        reset();
    }

    bool addressMatch(int address) {
        return address >= this->address && address < (this->address + std::max(readSize, writeSize));
    }

    bool validateReg(int address, int size, int function) {
        if (size <= 0) {
            bus_cl_exceptionCode = ERR_INVALID_SIZE;
            return false;
        }
        if (function == READ_HOLDING_REGISTERS) {
            if ((address + size) > this->address + readSize) {
                bus_cl_exceptionCode = ERR_INVALID_SIZE;
                return false;
            }
        } else {
            if ((address + size) > this->address + writeSize) {
                bus_cl_exceptionCode = ERR_INVALID_SIZE;
                return false;
            }
        }
        return true;
    }
};

class RegistersMock {
public:
    std::vector<RegisterRange> ranges;
    RegistersMock(const std::vector<RegisterRange>& ranges)
        :ranges(ranges) { }

    void reset() {
        for (auto it = ranges.begin(); it != ranges.end(); ++it) {
            it->reset();
        }
    }

    bool validateReg() {
        int address = be16toh(bus_cl_header.address.registerAddressBe);
        for (auto it = ranges.begin(); it != ranges.end(); ++it) {
            if (it->addressMatch(address)) {
                return it->validateReg(address, be16toh(bus_cl_header.address.countBe), bus_cl_header.header.function);
            }
        }
        bus_cl_exceptionCode = ERR_INVALID_ADDRESS;
        return false;
    }

    bool onReceive() {
        int address = be16toh(bus_cl_header.address.registerAddressBe);
        for (auto it = ranges.begin(); it != ranges.end(); ++it) {
            if (it->addressMatch(address)) {
                return it->onReceive();
            }
        }
        throw std::runtime_error("onReceive called with invalid header");
    }

    bool onSend() {
        int address = be16toh(bus_cl_header.address.registerAddressBe);
        for (auto it = ranges.begin(); it != ranges.end(); ++it) {
            if (it->addressMatch(address)) {
                return it->onSend();
            }
        }
        throw std::runtime_error("onReceive called with invalid header");
    }
};

static RegistersMock registersMock({
    // Starts at 1024, 2 for read, 2 for write
    RegisterRange(1024, 2, 2),
    // Starts at 2048, 2 for read, 0 for write
    RegisterRange(2048, 2, 0),
    // Starts at 2048, 0 for read, 2 for write
    RegisterRange(4096, 0, 2)
});

extern "C" {
    _Bool regs_validateReg() {
        return registersMock.validateReg();
    }

    _Bool regs_onReceive() {
        return registersMock.onReceive();
    }

    _Bool regs_onSend() {
        return registersMock.onSend();
    }
}

static void initRs485() {
    rs485_isMarkCondition = true;
    rs485_state = RS485_LINE_RX;
    rs485_discard(rs485_readAvail());
    crc_reset();

    registersMock.reset();
    rs485mock.reset();
}

static std::vector<uint8_t> crcOf(const std::vector<uint8_t>& data) {
    uint16_t crc = calcCrc16(0xffff, &data[0], data.size());
    // CRC is LSB first
    return std::vector<uint8_t>({ (uint8_t)(crc & 0xff), (uint8_t)(crc >> 8) });
}
static std::vector<uint8_t> crcOf(const std::vector<uint8_t>& data1, const std::vector<uint8_t>& data2) {
    uint16_t crc = calcCrc16(0xffff, &data1[0], data1.size());
    crc = calcCrc16(crc, &data2[0], data2.size());
    // CRC is LSB first
    return std::vector<uint8_t>({ (uint8_t)(crc & 0xff), (uint8_t)(crc >> 8) });
}

static std::vector<uint8_t> padWithCrc(const std::vector<uint8_t>& data) {
    return data + crcOf(data);
}

TEST_CASE("No addressing, skip packet") {
    initRs485();
    bus_cl_init();

    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    // Not addressed
    rs485mock.simulateData({ 0x1 });      // Address station 1, I'm 2
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    rs485mock.simulateData({ 0x2 });      // Function 2, wrong function
    REQUIRE(bus_cl_poll() == false);
    // After 2 bytes read, the client still search for a whole holding register read/write header
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    rs485mock.simulateData({ 0x3, 0x5 }); 
    REQUIRE(bus_cl_poll() == false);
    // After 2 bytes read, the client still search for a whole holding register read/write header
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    rs485mock.simulateData({ 0x3, 0x5 });
    REQUIRE(bus_cl_poll() == false);
    // Now that the header is decoded, it is discarded due wrong addressing
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_IDLE);

    rs485mock.simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}

TEST_CASE("Addressed but truncated packet") {
    initRs485();
    bus_cl_init();

    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    // Addressed and correct function
    rs485mock.simulateData({ 0x2, 0x10 });
    REQUIRE(bus_cl_poll() == false);
    // After 2 bytes read, the client will wants data
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    rs485mock.simulateData({ 0x3 });
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    // Mark condition, reset state
    rs485mock.simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    REQUIRE(rs485mock.getDataWritten() == std::vector<uint8_t>({ }));
}

TEST_CASE("Wrong function") {
    initRs485();
    bus_cl_init();

    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    // Addressed and incorrect function
    rs485mock.simulateData({ 0x2, 0x11 });
    REQUIRE(bus_cl_poll() == false);
    // After 2 bytes read, the client will start skipping data
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    // Complete a holding register header. Now it start skipping data
    rs485mock.simulateData({ 0x3, 0x4, 0x5, 0x6 });
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_RESPONSE);

    // Mark condition, now the station will respond
    rs485mock.simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    REQUIRE(rs485mock.getDataWritten() == padWithCrc({ 0x2, 0x91, 0x1 }));

    rs485_state = RS485_LINE_RX;
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE); 
}

static void testWrongAddress(int address) {
    initRs485();
    bus_cl_init();

    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    // Addressed and correct function
    rs485mock.simulateData({ 0x2, 0x3 });
    REQUIRE(bus_cl_poll() == false);

    rs485mock.simulateData({ BigEndian::fromH(address), BigEndian::fromH(1) });
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_RESPONSE);

    // Mark condition, now the station will respond
    rs485mock.simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    // Function error
    REQUIRE(rs485mock.getDataWritten() == padWithCrc({ 0x2, 0x83, ERR_INVALID_ADDRESS }));

    rs485_state = RS485_LINE_RX;
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE); 
}

TEST_CASE("Wrong address, 0") {
    testWrongAddress(0);
}
TEST_CASE("Wrong address, 1026") {
    testWrongAddress(1026);
}

static void testSizeSetup(int address, int size, uint8_t function) {
    initRs485();
    bus_cl_init();

    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);

    // Addressed and correct function
    rs485mock.simulateData({ 0x2, function });
    rs485mock.simulateData({ BigEndian::fromH(address), BigEndian::fromH(size) });
    REQUIRE(bus_cl_poll() == false);
}

static void testWrongSizeRead(int address, int size) {
    testSizeSetup(address, size, 0x03);

    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_RESPONSE);

    // Mark condition, now the station will respond
    rs485mock.simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    // Function size error
    REQUIRE(rs485mock.getDataWritten() == padWithCrc({ 0x2, 0x83, ERR_INVALID_SIZE }));

    rs485_state = RS485_LINE_RX;
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}

static void testWrongSizeWrite(int address, int size) {
    testSizeSetup(address, size, 0x10);
    rs485mock.simulateData({ (uint8_t)(size * 2) });
    REQUIRE(bus_cl_poll() == false);

    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_RESPONSE);

    // Mark condition, now the station will respond
    rs485mock.simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    // Function size error
    REQUIRE(rs485mock.getDataWritten() == padWithCrc({ 0x2, 0x90, 0x3 }));

    rs485_state = RS485_LINE_RX;
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}

TEST_CASE("Wrong function size read: 0 is considered an error") {
    testWrongSizeRead(1024, 0);
}
TEST_CASE("Wrong function size read: size > max read") {
    testWrongSizeRead(1024, 3);
}
TEST_CASE("Wrong function size read: size") {
    testWrongSizeRead(1024, 255);
}
TEST_CASE("Wrong function size read: size H byte") {
    testWrongSizeRead(1024, 256);
}

TEST_CASE("Wrong function size write: 0 is considered an error") {
    testWrongSizeWrite(1024, 0);
}
TEST_CASE("Wrong function size write: size > max read") {
    testWrongSizeWrite(1024, 3);
}
TEST_CASE("Wrong function size write: size") {
    testWrongSizeWrite(1024, 255);
}
TEST_CASE("Wrong function size write: size H byte") {
    testWrongSizeWrite(1024, 256);
}

TEST_CASE("Wrong function size read: reg 4096 can't read") {
    testWrongSizeRead(4096, 1);
}
TEST_CASE("Wrong function size write: reg 2048 can't write") {
    testWrongSizeWrite(2048, 1);
}

TEST_CASE("Wrong CRC in read") {
    testSizeSetup(1024, 2, 0x3);

    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_CHECK_REQUEST_CRC);
    rs485mock.simulateData({ 0xde, 0xad });
    REQUIRE(bus_cl_poll() == false);

    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_IDLE);
    // Mark condition, now the station will NOT respond due to CRC error
    rs485mock.simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}

TEST_CASE("Wrong byte size in write") {
    testSizeSetup(1024, 2, 0x10);
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_RECEIVE_DATA_SIZE);

    rs485mock.simulateData({ 5 });
    REQUIRE(bus_cl_poll() == false);

    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_RESPONSE);

    // Mark condition, now the station will respond
    rs485mock.simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    // Function size error
    REQUIRE(rs485mock.getDataWritten() == padWithCrc({ 0x2, 0x90, ERR_INVALID_SIZE }));

    rs485_state = RS485_LINE_RX;
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}

TEST_CASE("Wrong CRC in write") {
    testSizeSetup(1024, 2, 0x10);
    rs485mock.simulateData({ 4 });
    REQUIRE(bus_cl_poll() == false);

    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_RECEIVE_DATA);
    rs485mock.simulateData({ 0xf1, 0xf2, 0xf3, 0xf4 });
    REQUIRE(bus_cl_poll() == false);
    registersMock.ranges[0].checkDataReceived({ 0xf1, 0xf2, 0xf3, 0xf4 });

    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_CHECK_REQUEST_CRC);
    rs485mock.simulateData({ 0xde, 0xad });
    REQUIRE(bus_cl_poll() == false);

    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_IDLE);
    // Mark condition, now the station will NOT respond due to CRC error
    rs485mock.simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}

static void testCorrectRead(RegisterRange& range) {
    testSizeSetup(range.address, range.readSize, 0x3);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_CHECK_REQUEST_CRC);

    rs485mock.simulateData(crcOf(rs485mock.packetData));
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_RESPONSE);

    rs485mock.simulateMark();

    std::vector<uint8_t> dataToSend;
    for (int i = 0; i < range.readSize * 2; i++) {
        dataToSend.push_back((uint8_t)(i + 0x40));
    }
    range.prepareDataToSend(dataToSend);

    // Since TX buffer is infinite, the whole message will be pushed out
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    // Read response
    std::vector<uint8_t> expectedMessageHeader({ 0x2, 0x3, (uint8_t)(range.readSize * 2) });
    REQUIRE(rs485mock.getDataWritten() == expectedMessageHeader + dataToSend + crcOf(expectedMessageHeader, dataToSend));
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    rs485_state = RS485_LINE_RX;
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}

static void testCorrectWrite(RegisterRange& range) {
    testSizeSetup(range.address, range.writeSize, 0x10);
    // Still missing the content size
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_RECEIVE_DATA_SIZE);
    // Write sizeL * 2 bytes
    rs485mock.simulateData({ (uint8_t)(range.writeSize * 2) });
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_RECEIVE_DATA);
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_RECEIVE_DATA);

    // Push stream data to keep CRC correct
    std::vector<uint8_t> dataToSend;
    for (int i = 0; i < (range.writeSize * 2); i++) {
        dataToSend.push_back((uint8_t)i);
    }
    rs485mock.simulateData(dataToSend);
    REQUIRE(bus_cl_poll() == false);

    range.checkDataReceived(dataToSend);
    
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_CHECK_REQUEST_CRC);

    rs485mock.simulateData(crcOf(rs485mock.packetData));
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_RESPONSE);

    rs485mock.simulateMark();
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    // Read empty response
    auto address = BigEndian::fromH(range.address);
    auto size = BigEndian::fromH(range.writeSize);
    REQUIRE(rs485mock.getDataWritten() == padWithCrc({ 0x2, 0x10, address.b0, address.b1, size.b0, size.b1 }));
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH);

    rs485_state = RS485_LINE_RX;
    REQUIRE(bus_cl_poll() == false);
    REQUIRE(bus_cl_rtu_state == BUS_CL_RTU_IDLE);
}

TEST_CASE("Correct size, register 1024, read") {
    testCorrectRead(registersMock.ranges[0]);
}
TEST_CASE("Correct size, register 1024, write") {
    testCorrectWrite(registersMock.ranges[0]);
}
