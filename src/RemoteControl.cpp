#include "RemoteControl.hpp"
#include <coco/Storage.hpp>
//#include <coco/debug.hpp>


// RemoteContol

AwaitableCoroutine RemoteControl::load() {
    int result;

    for (int i = 0; i < COMMAND_COUNT; ++i) {
        auto &command = this->commands[i];
        command.type = Command::Type::NONE;
        co_await this->storage.read(FIRST_COMMAND_ID + i, &command, sizeof(Command), result);
    }
    this->commandsModified = 0;
}

AwaitableCoroutine RemoteControl::save() {
    int result;

    // save all modified commands
    for (int i = 0; i < COMMAND_COUNT; ++i) {
        if (this->commandsModified & (1 << i)) {
            auto &command = this->commands[i];
            co_await this->storage.write(FIRST_COMMAND_ID + i, &command, sizeof(Command), result);
        }
    }
    this->commandsModified = 0;
}

bool RemoteControl::compare(Command &c1, Command &c2) {
    if (c1.type == c2.type) {
        if (c1.type == Command::Type::NEC
            && c1.necPacket.address1 == c2.necPacket.address1
            && c1.necPacket.address2 == c2.necPacket.address2
            && c1.necPacket.command == c2.necPacket.command)
            return true;

        if (c1.type == Command::Type::NUBERT
            && c1.nubertPacket == c2.nubertPacket)
            return true;

        if (c1.type == Command::Type::RC6
            && (this->lastCommand.type != Command::Type::RC6 || c1.rc6Packet != this->lastCommand.rc6Packet)
            //&& c1.rc6Packet.trailer != this->sequence
            && c1.rc6Packet.control == c2.rc6Packet.control
            && c1.rc6Packet.data == c2.rc6Packet.data)
            return true;
    }
    return false;
}

Coroutine RemoteControl::receive(Buffer &buffer) {
    while (true) {
        co_await buffer.read();

        Command c1;
        if (nec::decode(buffer.array<const uint8_t>(), c1.necPacket)) {
            //debug::out << "NEC " << dec(packet.address1) << ' ' << dec(packet.address2) << ' ' << dec(packet.command) << '\n';
            c1.type = Command::Type::NEC;
        } else if (nubert::decode(buffer.array<const uint8_t>(), c1.nubertPacket)) {
            //debug::out << "Nubert " << dec(packet) << '\n';
            c1.type = Command::Type::NUBERT;
        } else if (rc6::decode(buffer.array<const uint8_t>(), c1.rc6Packet)) {
            //debug::out << "RC6 " << dec(packet.mode) << ' ' << dec(packet.trailer) << ' ' << dec(packet.control) << ' ' << dec(packet.data) << '\n';
            c1.type = Command::Type::RC6;
        } else {
            // nothing decoded
            continue;
        }

        // check if one of the stored commands match
        if (this->learnIndex == -1) {
            for (int i = 0; i < COMMAND_COUNT; ++i) {
                auto &c2 = this->commands[i];
                if (compare(c1, c2)) {
                    this->command = i;

                    // notify
                    ++this->sequenceNumber;
                    this->tasks.doAll();
                }
            }
        } else {
            // learn command
            this->commands[this->learnIndex] = c1;
            this->commandsModified |= 1 << this->learnIndex;

            // notify
            ++this->sequenceNumber;
            this->tasks.doAll();
        }

        // store last command
        this->lastCommand = c1;
    }
}
