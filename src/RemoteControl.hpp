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
    static constexpr int FIRST_COMMAND_ID = 200;
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
        // start two receive coroutines
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

    /// @brief Save modified configuration to flash
    ///
    [[nodiscard]] AwaitableCoroutine save();

    /// @brief Check if the configuration is modified and needs to be saved
    /// @return true if modified
    bool modified() {return this->commandsModified != 0;}


    /// @brief Set to learning mode
    /// @param index Command index to learn
    void setLearn(int index) {
        this->learnIndex = index;
    }

    /// @brief Clear command to learn
    ///
    void clear() {
        this->commands[this->learnIndex].type = Command::Type::NONE;
        this->commandsModified = 1 << this->learnIndex;
    }

    /// @brief Set to normal mode
    ///
    void setNormal() {this->learnIndex = -1;}

    /// @brief Get command to learn
    /// @return Command to learn
    const Command &getLearnCommand() {
        return this->commands[this->learnIndex];
    }

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
    int commandsModified = 0;
    int command;
    Command lastCommand;
    int learnIndex = -1;

    int sequenceNumber = 0;
    CoroutineTaskList<> tasks;

    // must be last
    Coroutine receiveCoroutines[2];
};
