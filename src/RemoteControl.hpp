#pragma once

#include "EffectInfo.hpp"
#include "Strip.hpp"
#include <coco/nec.hpp>
#include <coco/nubert.hpp>
#include <coco/rc6.hpp>
#include <coco/BufferStorage.hpp>
#include <coco/BufferDevice.hpp>
#include <coco/Loop.hpp>
#include <coco/Coroutine.hpp>


using namespace coco;


class RemoteControl {
public:
    static constexpr int FIRST_COMMAND_ID = 100;
    static constexpr int COMMAND_COUNT = 5;

    struct Command {
        enum class Type : uint8_t {
            NONE,
            NEC,
            NUBERT,
            RC6
        };

        Type type;
        union {
            nec::Packet necPacket;
            uint16_t nubertPacket;
            rc6::Packet rc6Packet;
        };

        bool operator ==(const Command &) const;
    };


    /// @brief Constructor
    /// @param loop
    /// @param storage
    /// @param irDevice device of IR receiver with 2 buffers
    RemoteControl(Loop &loop, Storage &storage, BufferDevice &irDevice)
        : loop(loop)
        , storage(storage)
    {
        for (int i = 0; i < 2; ++i) {
            this->receiveCoroutines[i] = receive(irDevice.getBuffer(i));
        }
    }

    ~RemoteControl() {
        for (int i = 0; i < 2; ++i) {
            this->receiveCoroutines[i].destroy();
        }
    }

    /// @brief Load configuration from flash
    ///
    [[nodiscard]] AwaitableCoroutine load();

    /// @brief Save configuration to flash
    ///
    [[nodiscard]] AwaitableCoroutine save();

    /// @brief Set to learning mode
    /// @param index command index to learn
    void setLearn(int index) {
        this->learn = index;
    }

    void clear() {
        this->commands[this->learn].type = Command::Type::NONE;
        this->dirty = 1 << this->learn;
    }

    /// @brief  Set to normal mode
    void setNormal() {this->learn = -1;}

    const Command &getLearnCommand() {return this->commands[this->learn];}

    Barrier<int &> commandBarrier;

    int getSequenceNumber() {return this->sequenceNumber;}

    Awaitable<> untilInput(int sequenceNumber) {
        // don't wait if the sequence number has changed
        if (this->sequenceNumber != sequenceNumber)
            return {};
        return {this->tasks};
    }

    int getCommand() {return this->command;}

protected:
    bool compare(Command &c1, Command &c2);

    Coroutine receive(Buffer &buffer);

    Loop &loop;
    Storage &storage;

    Command commands[COMMAND_COUNT];
    int dirty = 0;
    int command;
    Command lastCommand;
    int learn = -1;

    int sequenceNumber = 0;
    CoroutineTaskList<> tasks;

    // must be last
    Coroutine receiveCoroutines[2];
};
