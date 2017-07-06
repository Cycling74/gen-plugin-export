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


/**
    Represents an individual BLOCKS device.
*/
class Block   : public juce::ReferenceCountedObject
{
public:
    //==============================================================================
    /** Destructor. */
    virtual ~Block();

    /** The different block types.

        @see Block::getType()
    */
    enum Type
    {
        unknown = 0,
        lightPadBlock,
        liveBlock,
        loopBlock,
        developerControlBlock,
        seaboardBlock // on-screen seaboard view
    };

    /** The Block class is reference-counted, so always use a Block::Ptr when
        you are keeping references to them.
    */
    using Ptr = juce::ReferenceCountedObjectPtr<Block>;

    /** The Block class is reference-counted, so Block::Array is useful when
        you are storing lists of them.
    */
    using Array = juce::ReferenceCountedArray<Block>;

    /** The Block's serial number. */
    const juce::String serialNumber;

    using UID = uint64;

    /** This Block's UID.

        This will be globally unique, and remains constant for a particular device.
    */
    const UID uid;

    //==============================================================================
    /** Returns the type of this device.

        @see Block::Type
    */
    virtual Type getType() const = 0;

    /** Returns a human-readable description of this device type. */
    virtual juce::String getDeviceDescription() const = 0;

    /** Returns the battery level in the range 0.0 to 1.0. */
    virtual float getBatteryLevel() const = 0;

    /** Returns true if the battery is charging. */
    virtual bool isBatteryCharging() const = 0;

    //==============================================================================
    /** Returns true if this block is connected and active. */
    virtual bool isConnected() const = 0;

    /** Returns true if this block is directly connected to the application,
        as opposed to only being connected to a different block via a connection port.

        @see ConnectionPort
    */
    virtual bool isMasterBlock() const = 0;

    //==============================================================================
    /** Returns the width of the device in logical device units. */
    virtual int getWidth() const = 0;

    /** Returns the height of the device in logical device units. */
    virtual int getHeight() const = 0;

    /** Returns true if the device is a physical hardware block (i.e. not a virtual block). */
    virtual bool isHardwareBlock() const = 0;

    /** Returns the length of one logical device unit as physical millimeters. */
    virtual float getMillimetersPerUnit() const = 0;

    //==============================================================================
    /** If this block has a grid of LEDs, this will return an object to control it.
        Note that the pointer that is returned belongs to this object, and the caller must
        neither delete it or use it after the lifetime of this Block object has finished.
        If there are no LEDs, then this method will return nullptr.
    */
    virtual LEDGrid* getLEDGrid() const = 0;

    /** If this block has a row of LEDs, this will return an object to control it.
        Note that the pointer that is returned belongs to this object, and the caller must
        neither delete it or use it after the lifetime of this Block object has finished.
        If there are no LEDs, then this method will return nullptr.
    */
    virtual LEDRow* getLEDRow() const = 0;

    /** If this block has any status LEDs, this will return an array of objects to control them.
        Note that the objects in the array belong to this Block object, and the caller must
        neither delete them or use them after the lifetime of this Block object has finished.
    */
    virtual juce::Array<StatusLight*> getStatusLights() const = 0;

    /** If this block has a pressure-sensitive surface, this will return an object to
        access its data.
        Note that the pointer returned does is owned by this object, and the caller must
        neither delete it or use it after the lifetime of this Block object has finished.
        If the device is not touch-sensitive, then this method will return nullptr.
    */
    virtual TouchSurface* getTouchSurface() const = 0;

    /** If this block has any control buttons, this will return an array of objects to control them.
        Note that the objects in the array belong to this Block object, and the caller must
        neither delete them or use them after the lifetime of this Block object has finished.
    */
    virtual juce::Array<ControlButton*> getButtons() const = 0;

    //==============================================================================
    /** This returns true if the block supports generating graphics by drawing into a JUCE
        Graphics context. This should only be true for virtual on-screen blocks; hardware
        blocks will instead use the LED Grid for visuals.
    */
    virtual bool supportsGraphics() const = 0;

    //==============================================================================
    /** These are the edge-connectors that a device may have. */
    struct ConnectionPort
    {
        enum class DeviceEdge
        {
            north,
            south,
            east,
            west
        };

        /** The side of the device on which this port is located. */
        DeviceEdge edge;

        /** The index of this port along the device edge.
            For north and south edges, index 0 is the left-most port.
            For east and west edges, index 0 is the top-most port.
        */
        int index;

        bool operator== (const ConnectionPort&) const noexcept;
        bool operator!= (const ConnectionPort&) const noexcept;
    };

    /** Returns a list of the connectors that this device has. */
    virtual juce::Array<ConnectionPort> getPorts() const = 0;

    //==============================================================================
    /** A program that can be loaded onto a block. */
    struct Program
    {
        /** Creates a Program for the corresponding LEDGrid. */
        Program (Block&);

        /** Destructor. */
        virtual ~Program();

        /** Returns the LittleFoot program to execute on the BLOCKS device. */
        virtual juce::String getLittleFootProgram() = 0;

        Block& block;
    };

    /** Sets the Program to run on this block.

        The supplied Program's lifetime will be managed by this class, so do not
        use the Program in other places in your code.
    */
    virtual juce::Result setProgram (Program*) = 0;

    /** Returns a pointer to the currently loaded program. */
    virtual Program* getProgram() const = 0;

    //==============================================================================
    /** A message that can be sent to the currently loaded program. */
    struct ProgramEventMessage
    {
        int32 values[3];
    };

    /** Sends a message to the currently loaded program.

        To receive the message the program must provide a littlefoot function called
        handleMessage with the following form:
        @code
        void handleMessage (int param1, int param2, int param3)
        {
            // Do something with the two integer parameters that the app has sent...
        }
        @endcode
    */
    virtual void sendProgramEvent (const ProgramEventMessage&) = 0;

    /** Interface for objects listening to custom program events. */
    struct ProgramEventListener
    {
        virtual ~ProgramEventListener() {}

        /** Called whenever a message from a block is received. */
        virtual void handleProgramEvent (Block& source, const ProgramEventMessage&) = 0;
    };

    /** Adds a new listener for custom program events from the block. */
    virtual void addProgramEventListener (ProgramEventListener*);

    /** Removes a listener for custom program events from the block. */
    virtual void removeProgramEventListener (ProgramEventListener*);

    //==============================================================================
    /** Returns the size of the data block that setDataByte and other functions can write to. */
    virtual uint32 getMemorySize() = 0;

    /** Sets a single byte on the littlefoot heap. */
    virtual void setDataByte (size_t offset, uint8 value) = 0;

    /** Sets multiple bytes on the littlefoot heap. */
    virtual void setDataBytes (size_t offset, const void* data, size_t num) = 0;

    /** Sets multiple bits on the littlefoot heap. */
    virtual void setDataBits (uint32 startBit, uint32 numBits, uint32 value) = 0;

    /** Gets a byte from the littlefoot heap. */
    virtual uint8 getDataByte (size_t offset) = 0;

    /** Sets the current program as the block's default state. */
    virtual void saveProgramAsDefault() = 0;

    //==============================================================================
    /** Allows the user to provide a function that will receive log messages from the block. */
    virtual void setLogger (std::function<void(const String&)> loggingCallback) = 0;

    /** Sends a firmware update packet to a block, and waits for a reply. Returns an error code. */
    virtual bool sendFirmwareUpdatePacket (const uint8* data, uint8 size,
                                           std::function<void (uint8)> packetAckCallback) = 0;

    //==============================================================================
    /** Interface for objects listening to input data port. */
    struct DataInputPortListener
    {
        virtual ~DataInputPortListener() {}

        /** Called whenever a message from a block is received. */
        virtual void handleIncomingDataPortMessage (Block& source, const void* messageData, size_t messageSize) = 0;
    };

    /** Adds a new listener for the data input port. */
    virtual void addDataInputPortListener (DataInputPortListener*);

    /** Removes a listener for the data input port. */
    virtual void removeDataInputPortListener (DataInputPortListener*);

    /** Sends a message to the block. */
    virtual void sendMessage (const void* messageData, size_t messageSize) = 0;

    //==============================================================================
    /** This type is used for timestamping events. It represents a number of milliseconds since the block
        device was booted.
    */
    using Timestamp = uint32;

protected:
    //==============================================================================
    Block (const juce::String& serialNumberToUse);

    juce::ListenerList<DataInputPortListener> dataInputPortListeners;
    juce::ListenerList<ProgramEventListener> programEventListeners;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Block)
};
