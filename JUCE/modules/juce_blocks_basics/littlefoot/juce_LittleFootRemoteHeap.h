/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace littlefoot
{

using namespace juce;

//==============================================================================
/**
    This class manages the synchronisation of a remote block of heap memory used
    by a littlefoot program running on a block.

    Data in the block can be changed by calling setByte, setBytes, setBits etc, and
    these changes will be flushed to the device when sendChanges is called.
*/
template <typename ImplementationClass>
struct LittleFootRemoteHeap
{
    LittleFootRemoteHeap (uint32 blockSizeToUse) noexcept  : blockSize (blockSizeToUse)
    {
        resetDeviceStateToUnknown();
    }

    void clear() noexcept
    {
        zeromem (targetData, sizeof (targetData));
        invalidateData();
    }

    void resetDeviceStateToUnknown()
    {
        invalidateData();
        messagesSent.clear();
        resetDataRangeToUnknown (0, ImplementationClass::maxBlockSize);
    }

    void resetDataRangeToUnknown (size_t offset, size_t size) noexcept
    {
        auto* latestState = getLatestExpectedDataState();

        for (size_t i = 0; i < size; ++i)
            latestState[offset + i] = unknownByte;
    }

    void setByte (size_t offset, uint8 value) noexcept
    {
        if (offset < blockSize)
        {
            if (targetData[offset] != value)
            {
                targetData[offset] = value;
                needsSyncing = true;

                if (programStateKnown && offset < programSize)
                    programStateKnown = false;
            }
        }
        else
        {
            jassertfalse;
        }
    }

    void setBytes (size_t offset, const uint8* data, size_t num) noexcept
    {
        for (size_t i = 0; i < num; ++i)
            setByte (offset + i, data[i]);
    }

    void setBits (uint32 startBit, uint32 numBits, uint32 value) noexcept
    {
        if (startBit + numBits > 8 * blockSize)
        {
            jassertfalse;
            return;
        }

        if (readLittleEndianBitsInBuffer (targetData, startBit, numBits) != value)
        {
            writeLittleEndianBitsInBuffer (targetData, startBit, numBits, value);
            invalidateData();
        }
    }

    uint8 getByte (size_t offset) noexcept
    {
        if (offset < blockSize)
            return targetData [offset];

        jassertfalse;
        return 0;
    }

    void invalidateData() noexcept
    {
        needsSyncing = true;
        programStateKnown = false;
    }

    bool isFullySynced() const noexcept
    {
        return ! needsSyncing;
    }

    void sendChanges (ImplementationClass& bi, bool forceSend)
    {
        if ((needsSyncing && messagesSent.isEmpty()) || forceSend)
        {
            for (int maxChanges = 30; --maxChanges >= 0;)
            {
                uint16 data[ImplementationClass::maxBlockSize];
                auto* latestState = getLatestExpectedDataState();

                for (uint32 i = 0; i < blockSize; ++i)
                    data[i] = latestState[i];

                uint32 packetIndex = messagesSent.isEmpty() ? lastPacketIndexReceived
                                                            : messagesSent.getLast()->packetIndex;

                packetIndex = (packetIndex + 1) & ImplementationClass::maxPacketCounter;

                if (! Diff (data, targetData, blockSize).createChangeMessage (bi, data, messagesSent, packetIndex))
                    break;

                dumpStatus();
            }
        }

        for (auto* m : messagesSent)
        {
            if (m->dispatchTime >= Time::getCurrentTime() - RelativeTime::milliseconds (250))
                break;

            m->dispatchTime = Time::getCurrentTime();
            bi.sendMessageToDevice (m->packet);
            //DBG ("Sending packet " << (int) m->packetIndex << " - " << m->packet.size() << " bytes, device " << bi.getDeviceIndex());

            if (getTotalSizeOfMessagesSent() > 200)
                break;
        }
    }

    void handleACKFromDevice (ImplementationClass& bi, uint32 packetIndex) noexcept
    {
        //DBG ("ACK " << (int) packetIndex << "   device " << (int) bi.getDeviceIndex());

        if (lastPacketIndexReceived != packetIndex)
        {
            lastPacketIndexReceived = packetIndex;

            for (int i = messagesSent.size(); --i >= 0;)
            {
                auto& m = *messagesSent.getUnchecked(i);

                if (m.packetIndex == packetIndex)
                {
                    for (uint32 j = 0; j < blockSize; ++j)
                        deviceState[j] = m.resultDataState[j];

                    messagesSent.removeRange (0, i + 1);
                    dumpStatus();
                    sendChanges (bi, false);

                    if (messagesSent.isEmpty())
                        needsSyncing = false;

                    return;
                }
            }

            resetDeviceStateToUnknown();
        }
    }

    bool isProgramLoaded() noexcept
    {
        if (! programStateKnown)
        {
            programStateKnown = true;

            uint8 deviceMemory[ImplementationClass::maxBlockSize];

            for (size_t i = 0; i < blockSize; ++i)
                deviceMemory[i] = (uint8) deviceState[i];

            littlefoot::Program prog (deviceMemory, (uint32) blockSize);
            programLoaded = prog.checksumMatches();
            programSize = prog.getProgramSize();
        }

        return programLoaded;
    }

    const size_t blockSize;

    static constexpr uint16 unknownByte = 0x100;

private:
    uint16 deviceState[ImplementationClass::maxBlockSize];
    uint8 targetData[ImplementationClass::maxBlockSize] = { 0 };
    uint32 programSize = 0;
    bool needsSyncing = true, programStateKnown = true, programLoaded = false;

    uint16* getLatestExpectedDataState() noexcept
    {
        return messagesSent.isEmpty() ? deviceState
                                      : messagesSent.getLast()->resultDataState;
    }

    struct ChangeMessage
    {
        typename ImplementationClass::PacketBuilder packet;
        Time dispatchTime;
        uint32 packetIndex;
        uint16 resultDataState[ImplementationClass::maxBlockSize];
    };

    OwnedArray<ChangeMessage> messagesSent;
    uint32 lastPacketIndexReceived = 0;

    int getTotalSizeOfMessagesSent() const noexcept
    {
        int total = 0;

        for (auto* m : messagesSent)
            if (m->dispatchTime != Time())
                total += m->packet.size();

        return total;
    }

    void dumpStatus()
    {
       #if DUMP_LITTLEFOOT_HEAP_STATUS
        int differences = 0;
        constexpr int diffLen = 50;
        char areas[diffLen + 1] = { 0 };

        for (int i = 0; i < diffLen; ++i)
            areas[i] = '.';

        for (int i = 0; i < (int) blockSize; ++i)
        {
            if (targetData[i] != deviceState[i])
            {
                ++differences;
                areas[i * diffLen / (int) blockSize] = 'X';
            }
        }

        double proportionOK = ((int) blockSize - differences) / (double) blockSize;

        juce::ignoreUnused (proportionOK);

        DBG ("Heap: " << areas << "  " << String (roundToInt (100 * proportionOK)) << "%  "
               << (isProgramLoaded() ? "Ready" : "Loading"));
       #endif
    }

    struct Diff
    {
        Diff (uint16* current, const uint8* target, size_t blockSizeToUse)
            : newData (target), blockSize (blockSizeToUse)
        {
            ranges.ensureStorageAllocated ((int) blockSize);

            for (int i = 0; i < (int) blockSize; ++i)
                ranges.add ({ i, 1, newData[i] == current[i], false });

            coalesceUniformRegions();
            coalesceSequences();
            trim();
        }

        bool createChangeMessage (const ImplementationClass& bi,
                                  const uint16* currentState,
                                  OwnedArray<ChangeMessage>& messagesCreated,
                                  uint32 nextPacketIndex)
        {
            if (ranges.isEmpty())
                return false;

            auto deviceIndex = bi.getDeviceIndex();

            if (deviceIndex < 0)
                return false;

            auto& message = *messagesCreated.add (new ChangeMessage());

            message.packetIndex = nextPacketIndex;

            for (uint32 i = 0; i < blockSize; ++i)
                message.resultDataState[i] = currentState[i];

            auto& p = message.packet;
            p.writePacketSysexHeaderBytes ((uint8) deviceIndex);
            p.beginDataChanges (nextPacketIndex);

            uint8 lastValue = 0;
            bool packetOverflow = false;

            for (auto& r : ranges)
            {
                if (r.isSkipped)
                {
                    packetOverflow = ! p.skipBytes (r.length);
                }
                else if (r.isMixed)
                {
                    jassert (r.length > 1);
                    packetOverflow = ! p.setMultipleBytes (newData + r.index, r.length);

                    if (! packetOverflow)
                        lastValue = newData[r.index + r.length - 1];
                }
                else
                {
                    auto value = newData[r.index];
                    packetOverflow = ! p.setMultipleBytes (value, lastValue, r.length);

                    if (! packetOverflow)
                        lastValue = value;
                }

                if (packetOverflow)
                    break;

                if (! r.isSkipped)
                    for (int i = r.index; i < r.index + r.length; ++i)
                        message.resultDataState[i] = newData[i];
            }

            p.endDataChanges (! packetOverflow);
            p.writePacketSysexFooter();

            return packetOverflow;
        }

    private:
        struct ByteSequence
        {
            int index, length;
            bool isSkipped, isMixed;
        };

        const uint8* const newData;
        const size_t blockSize;
        Array<ByteSequence> ranges;

        void coalesceUniformRegions()
        {
            for (int i = ranges.size(); --i > 0;)
            {
                auto& r1 = ranges.getReference (i - 1);
                auto r2 = ranges.getReference (i);

                if (r1.isSkipped == r2.isSkipped
                     && (r1.isSkipped || newData[r1.index] == newData[r2.index]))
                {
                    r1.length += r2.length;
                    ranges.remove (i);
                    i = jmin (ranges.size() - 1, i + 1);
                }
            }
        }

        void coalesceSequences()
        {
            for (int i = ranges.size(); --i > 0;)
            {
                auto& r1 = ranges.getReference (i - 1);
                auto r2 = ranges.getReference (i);

                if (! (r1.isSkipped || r2.isSkipped)
                     && (r1.isMixed || r1.length == 1)
                     && (r2.isMixed || r2.length == 1))
                {
                    if (r1.length + r2.length < 32)
                    {
                        r1.length += r2.length;
                        r1.isMixed = true;
                        ranges.remove (i);
                        i = jmin (ranges.size() - 1, i + 1);
                    }
                }
            }
        }

        void trim()
        {
            while (ranges.size() > 0 && ranges.getLast().isSkipped)
                ranges.removeLast();
        }
    };
};

}
