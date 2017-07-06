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

package com.juce.jucedemoplugin;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Looper;
import android.os.Handler;
import android.os.ParcelUuid;
import android.os.Environment;
import android.view.*;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.graphics.*;
import android.text.ClipboardManager;
import android.text.InputType;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.Pair;
import java.lang.Runnable;
import java.lang.ref.WeakReference;
import java.lang.reflect.*;
import java.util.*;
import java.io.*;
import java.net.URL;
import java.net.HttpURLConnection;
import android.media.AudioManager;
import android.media.MediaScannerConnection;
import android.media.MediaScannerConnection.MediaScannerConnectionClient;
import android.Manifest;

import android.media.midi.*;
import android.bluetooth.*;
import android.bluetooth.le.*;


//==============================================================================
public class JuceDemoPlugin   extends Activity
{
    //==============================================================================
    static
    {
        System.loadLibrary ("juce_jni");
    }

    //==============================================================================
    public boolean isPermissionDeclaredInManifest (int permissionID)
    {
        String permissionToCheck = getAndroidPermissionName(permissionID);

        try
        {
            PackageInfo info = getPackageManager().getPackageInfo(getApplicationContext().getPackageName(), PackageManager.GET_PERMISSIONS);

            if (info.requestedPermissions != null)
                for (String permission : info.requestedPermissions)
                    if (permission.equals (permissionToCheck))
                        return true;
        }
        catch (PackageManager.NameNotFoundException e)
        {
            Log.d ("JUCE", "isPermissionDeclaredInManifest: PackageManager.NameNotFoundException = " + e.toString());
        }

        Log.d ("JUCE", "isPermissionDeclaredInManifest: could not find requested permission " + permissionToCheck);
        return false;
    }

    //==============================================================================
    // these have to match the values of enum PermissionID in C++ class RuntimePermissions:
    private static final int JUCE_PERMISSIONS_RECORD_AUDIO = 1;
    private static final int JUCE_PERMISSIONS_BLUETOOTH_MIDI = 2;

    private static String getAndroidPermissionName (int permissionID)
    {
        switch (permissionID)
        {
            case JUCE_PERMISSIONS_RECORD_AUDIO:     return Manifest.permission.RECORD_AUDIO;
            case JUCE_PERMISSIONS_BLUETOOTH_MIDI:   return Manifest.permission.ACCESS_COARSE_LOCATION;
        }

        // unknown permission ID!
        assert false;
        return new String();
    }

    public boolean isPermissionGranted (int permissionID)
    {
        return getApplicationContext().checkCallingOrSelfPermission (getAndroidPermissionName (permissionID)) == PackageManager.PERMISSION_GRANTED;
    }

    private Map<Integer, Long> permissionCallbackPtrMap;

    public void requestRuntimePermission (int permissionID, long ptrToCallback)
    {
        String permissionName = getAndroidPermissionName (permissionID);

        if (getApplicationContext().checkCallingOrSelfPermission (permissionName) != PackageManager.PERMISSION_GRANTED)
        {
            // remember callbackPtr, request permissions, and let onRequestPermissionResult call callback asynchronously
            permissionCallbackPtrMap.put (permissionID, ptrToCallback);
            requestPermissionsCompat (new String[]{permissionName}, permissionID);
        }
        else
        {
            // permissions were already granted before, we can call callback directly
            androidRuntimePermissionsCallback (true, ptrToCallback);
        }
    }

    private native void androidRuntimePermissionsCallback (boolean permissionWasGranted, long ptrToCallback);

    @Override
    public void onRequestPermissionsResult (int permissionID, String permissions[], int[] grantResults)
    {
        boolean permissionsGranted = (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED);

        if (! permissionsGranted)
            Log.d ("JUCE", "onRequestPermissionsResult: runtime permission was DENIED: " + getAndroidPermissionName (permissionID));

        Long ptrToCallback = permissionCallbackPtrMap.get (permissionID);
        permissionCallbackPtrMap.remove (permissionID);
        androidRuntimePermissionsCallback (permissionsGranted, ptrToCallback);
    }

    //==============================================================================
    public interface JuceMidiPort
    {
        boolean isInputPort();

        // start, stop does nothing on an output port
        void start();
        void stop();

        void close();

        // send will do nothing on an input port
        void sendMidi (byte[] msg, int offset, int count);
    }

    //==============================================================================
    //==============================================================================
    public class BluetoothManager extends ScanCallback
    {
        BluetoothManager()
        {
        }

        public String[] getMidiBluetoothAddresses()
        {
            return bluetoothMidiDevices.toArray (new String[bluetoothMidiDevices.size()]);
        }

        public String getHumanReadableStringForBluetoothAddress (String address)
        {
            BluetoothDevice btDevice = BluetoothAdapter.getDefaultAdapter().getRemoteDevice (address);
            return btDevice.getName();
        }

        public int getBluetoothDeviceStatus (String address)
        {
            return getAndroidMidiDeviceManager().getBluetoothDeviceStatus (address);
        }

        public void startStopScan (boolean shouldStart)
        {
            BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

            if (bluetoothAdapter == null)
            {
                Log.d ("JUCE", "BluetoothManager error: could not get default Bluetooth adapter");
                return;
            }

            BluetoothLeScanner bluetoothLeScanner = bluetoothAdapter.getBluetoothLeScanner();

            if (bluetoothLeScanner == null)
            {
                Log.d ("JUCE", "BluetoothManager error: could not get Bluetooth LE scanner");
                return;
            }

            if (shouldStart)
            {
                ScanFilter.Builder scanFilterBuilder = new ScanFilter.Builder();
                scanFilterBuilder.setServiceUuid (ParcelUuid.fromString (bluetoothLEMidiServiceUUID));

                ScanSettings.Builder scanSettingsBuilder = new ScanSettings.Builder();
                scanSettingsBuilder.setCallbackType (ScanSettings.CALLBACK_TYPE_ALL_MATCHES)
                                   .setScanMode (ScanSettings.SCAN_MODE_LOW_POWER)
                                   .setScanMode (ScanSettings.MATCH_MODE_STICKY);

                bluetoothLeScanner.startScan (Arrays.asList (scanFilterBuilder.build()),
                                              scanSettingsBuilder.build(),
                                              this);
            }
            else
            {
                bluetoothLeScanner.stopScan (this);
            }
        }

        public boolean pairBluetoothMidiDevice(String address)
        {
            BluetoothDevice btDevice = BluetoothAdapter.getDefaultAdapter().getRemoteDevice (address);

            if (btDevice == null)
            {
                Log.d ("JUCE", "failed to create buletooth device from address");
                return false;
            }

            return getAndroidMidiDeviceManager().pairBluetoothDevice (btDevice);
        }

        public void unpairBluetoothMidiDevice (String address)
        {
            getAndroidMidiDeviceManager().unpairBluetoothDevice (address);
        }

        public void onScanFailed (int errorCode)
        {
        }

        public void onScanResult (int callbackType, ScanResult result)
        {
            if (callbackType == ScanSettings.CALLBACK_TYPE_ALL_MATCHES
                 || callbackType == ScanSettings.CALLBACK_TYPE_FIRST_MATCH)
            {
                BluetoothDevice device = result.getDevice();

                if (device != null)
                    bluetoothMidiDevices.add (device.getAddress());
            }

            if (callbackType == ScanSettings.CALLBACK_TYPE_MATCH_LOST)
            {
                Log.d ("JUCE", "ScanSettings.CALLBACK_TYPE_MATCH_LOST");
                BluetoothDevice device = result.getDevice();

                if (device != null)
                {
                    bluetoothMidiDevices.remove (device.getAddress());
                    unpairBluetoothMidiDevice (device.getAddress());
                }
            }
        }

        public void onBatchScanResults (List<ScanResult> results)
        {
            for (ScanResult result : results)
                onScanResult (ScanSettings.CALLBACK_TYPE_ALL_MATCHES, result);
        }

        private BluetoothLeScanner scanner;
        private static final String bluetoothLEMidiServiceUUID = "03B80E5A-EDE8-4B33-A751-6CE34EC4C700";

        private HashSet<String> bluetoothMidiDevices = new HashSet<String>();
    }

    public static class JuceMidiInputPort extends MidiReceiver implements JuceMidiPort
    {
        private native void handleReceive (long host, byte[] msg, int offset, int count, long timestamp);

        public JuceMidiInputPort (MidiDeviceManager mm, MidiOutputPort actualPort, MidiPortPath portPathToUse, long hostToUse)
        {
            owner = mm;
            androidPort = actualPort;
            portPath = portPathToUse;
            juceHost = hostToUse;
            isConnected = false;
        }

        @Override
        protected void finalize() throws Throwable
        {
            close();
            super.finalize();
        }

        @Override
        public boolean isInputPort()
        {
            return true;
        }

        @Override
        public void start()
        {
            if (owner != null && androidPort != null && ! isConnected) {
                androidPort.connect(this);
                isConnected = true;
            }
        }

        @Override
        public void stop()
        {
            if (owner != null && androidPort != null && isConnected) {
                androidPort.disconnect(this);
                isConnected = false;
            }
        }

        @Override
        public void close()
        {
            if (androidPort != null) {
                try {
                    androidPort.close();
                } catch (IOException exception) {
                    Log.d("JUCE", "IO Exception while closing port");
                }
            }

            if (owner != null)
                owner.removePort (portPath);

            owner = null;
            androidPort = null;
        }

        @Override
        public void onSend (byte[] msg, int offset, int count, long timestamp)
        {
            if (count > 0)
                handleReceive (juceHost, msg, offset, count, timestamp);
        }

        @Override
        public void onFlush()
        {}

        @Override
        public void sendMidi (byte[] msg, int offset, int count)
        {
        }

        MidiDeviceManager owner;
        MidiOutputPort androidPort;
        MidiPortPath portPath;
        long juceHost;
        boolean isConnected;
    }

    public static class JuceMidiOutputPort implements JuceMidiPort
    {
        public JuceMidiOutputPort (MidiDeviceManager mm, MidiInputPort actualPort, MidiPortPath portPathToUse)
        {
            owner = mm;
            androidPort = actualPort;
            portPath = portPathToUse;
        }

        @Override
        protected void finalize() throws Throwable
        {
            close();
            super.finalize();
        }

        @Override
        public boolean isInputPort()
        {
            return false;
        }

        @Override
        public void start()
        {
        }

        @Override
        public void stop()
        {
        }

        @Override
        public void sendMidi (byte[] msg, int offset, int count)
        {
            if (androidPort != null)
            {
                try {
                    androidPort.send(msg, offset, count);
                } catch (IOException exception)
                {
                    Log.d ("JUCE", "send midi had IO exception");
                }
            }
        }

        @Override
        public void close()
        {
            if (androidPort != null) {
                try {
                    androidPort.close();
                } catch (IOException exception) {
                    Log.d("JUCE", "IO Exception while closing port");
                }
            }

            if (owner != null)
                owner.removePort (portPath);

            owner = null;
            androidPort = null;
        }

        MidiDeviceManager owner;
        MidiInputPort androidPort;
        MidiPortPath portPath;
    }

    private static class MidiPortPath extends Object
    {
        public MidiPortPath (int deviceIdToUse, boolean direction, int androidIndex)
        {
            deviceId = deviceIdToUse;
            isInput = direction;
            portIndex = androidIndex;

        }

        public int deviceId;
        public int portIndex;
        public boolean isInput;

        @Override
        public int hashCode()
        {
            Integer i = new Integer ((deviceId * 128) + (portIndex < 128 ? portIndex : 127));
            return i.hashCode() * (isInput ? -1 : 1);
        }

        @Override
        public boolean equals (Object obj)
        {
            if (obj == null)
                return false;

            if (getClass() != obj.getClass())
                return false;

            MidiPortPath other = (MidiPortPath) obj;
            return (portIndex == other.portIndex && isInput == other.isInput && deviceId == other.deviceId);
        }
    }

    //==============================================================================
    public class MidiDeviceManager extends MidiManager.DeviceCallback implements MidiManager.OnDeviceOpenedListener
    {
        //==============================================================================
        private class DummyBluetoothGattCallback extends BluetoothGattCallback
        {
            public DummyBluetoothGattCallback (MidiDeviceManager mm)
            {
                super();
                owner = mm;
            }

            public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState)
            {
                if (newState == BluetoothProfile.STATE_CONNECTED)
                {
                    gatt.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_HIGH);
                    owner.pairBluetoothDeviceStepTwo (gatt.getDevice());
                }
            }
            public void onServicesDiscovered(BluetoothGatt gatt, int status) {}
            public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {}
            public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {}
            public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {}
            public void onDescriptorRead(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {}
            public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {}
            public void onReliableWriteCompleted(BluetoothGatt gatt, int status) {}
            public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status) {}
            public void onMtuChanged(BluetoothGatt gatt, int mtu, int status) {}

            private MidiDeviceManager owner;
        }

        //==============================================================================
        private class MidiDeviceOpenTask extends java.util.TimerTask
        {
            public MidiDeviceOpenTask (MidiDeviceManager deviceManager, MidiDevice device, BluetoothGatt gattToUse)
            {
                owner = deviceManager;
                midiDevice = device;
                btGatt = gattToUse;
            }

            @Override
            public boolean cancel()
            {
                synchronized (MidiDeviceOpenTask.class)
                {
                    owner = null;
                    boolean retval = super.cancel();

                    if (btGatt != null)
                    {
                        btGatt.disconnect();
                        btGatt.close();

                        btGatt = null;
                    }

                    if (midiDevice != null)
                    {
                        try
                        {
                            midiDevice.close();
                        }
                        catch (IOException e)
                        {}

                        midiDevice = null;
                    }

                    return retval;
                }
            }

            public String getBluetoothAddress()
            {
                synchronized (MidiDeviceOpenTask.class)
                {
                    if (midiDevice != null)
                    {
                        MidiDeviceInfo info = midiDevice.getInfo();
                        if (info.getType() == MidiDeviceInfo.TYPE_BLUETOOTH)
                        {
                            BluetoothDevice btDevice = (BluetoothDevice) info.getProperties().get (info.PROPERTY_BLUETOOTH_DEVICE);
                            if (btDevice != null)
                                return btDevice.getAddress();
                        }
                    }
                }

                return "";
            }

            public BluetoothGatt getGatt() { return btGatt; }

            public int getID()
            {
                return midiDevice.getInfo().getId();
            }

            @Override
            public void run()
            {
                synchronized (MidiDeviceOpenTask.class)
                {
                    if (owner != null && midiDevice != null)
                        owner.onDeviceOpenedDelayed (midiDevice);
                }
            }

            private MidiDeviceManager owner;
            private MidiDevice midiDevice;
            private BluetoothGatt btGatt;
        }

        //==============================================================================
        public MidiDeviceManager()
        {
            manager = (MidiManager) getSystemService (MIDI_SERVICE);

            if (manager == null)
            {
                Log.d ("JUCE", "MidiDeviceManager error: could not get MidiManager system service");
                return;
            }

            openPorts = new HashMap<MidiPortPath, WeakReference<JuceMidiPort>> ();
            midiDevices = new ArrayList<Pair<MidiDevice,BluetoothGatt>>();
            openTasks = new HashMap<Integer, MidiDeviceOpenTask>();
            btDevicesPairing = new HashMap<String, BluetoothGatt>();

            MidiDeviceInfo[] foundDevices = manager.getDevices();
            for (MidiDeviceInfo info : foundDevices)
                onDeviceAdded (info);

            manager.registerDeviceCallback (this, null);
        }

        protected void finalize() throws Throwable
        {
            manager.unregisterDeviceCallback (this);

            synchronized (MidiDeviceManager.class)
            {
                btDevicesPairing.clear();

                for (Integer deviceID : openTasks.keySet())
                    openTasks.get (deviceID).cancel();

                openTasks = null;
            }

            for (MidiPortPath key : openPorts.keySet())
                openPorts.get (key).get().close();

            openPorts = null;

            for (Pair<MidiDevice, BluetoothGatt> device : midiDevices)
            {
                if (device.second != null)
                {
                    device.second.disconnect();
                    device.second.close();
                }

                device.first.close();
            }

            midiDevices.clear();

            super.finalize();
        }

        public String[] getJuceAndroidMidiInputDevices()
        {
            return getJuceAndroidMidiDevices (MidiDeviceInfo.PortInfo.TYPE_OUTPUT);
        }

        public String[] getJuceAndroidMidiOutputDevices()
        {
            return getJuceAndroidMidiDevices (MidiDeviceInfo.PortInfo.TYPE_INPUT);
        }

        private String[] getJuceAndroidMidiDevices (int portType)
        {
            // only update the list when JUCE asks for a new list
            synchronized (MidiDeviceManager.class)
            {
                deviceInfos = getDeviceInfos();
            }

            ArrayList<String> portNames = new ArrayList<String>();

            int index = 0;
            for (MidiPortPath portInfo = getPortPathForJuceIndex (portType, index); portInfo != null; portInfo = getPortPathForJuceIndex (portType, ++index))
                portNames.add (getPortName (portInfo));

            String[] names = new String[portNames.size()];
            return portNames.toArray (names);
        }

        private JuceMidiPort openMidiPortWithJuceIndex (int index, long host, boolean isInput)
        {
            synchronized (MidiDeviceManager.class)
            {
                int portTypeToFind = (isInput ? MidiDeviceInfo.PortInfo.TYPE_OUTPUT : MidiDeviceInfo.PortInfo.TYPE_INPUT);
                MidiPortPath portInfo = getPortPathForJuceIndex (portTypeToFind, index);

                if (portInfo != null)
                {
                    // ports must be opened exclusively!
                    if (openPorts.containsKey (portInfo))
                        return null;

                    Pair<MidiDevice,BluetoothGatt> devicePair = getMidiDevicePairForId (portInfo.deviceId);

                    if (devicePair != null)
                    {
                        MidiDevice device = devicePair.first;
                        if (device != null)
                        {
                            JuceMidiPort juceMidiPort = null;

                            if (isInput)
                            {
                                MidiOutputPort outputPort = device.openOutputPort(portInfo.portIndex);

                                if (outputPort != null)
                                    juceMidiPort = new JuceMidiInputPort(this, outputPort, portInfo, host);
                            }
                            else
                            {
                                MidiInputPort inputPort = device.openInputPort(portInfo.portIndex);

                                if (inputPort != null)
                                    juceMidiPort = new JuceMidiOutputPort(this, inputPort, portInfo);
                            }

                            if (juceMidiPort != null)
                            {
                                openPorts.put(portInfo, new WeakReference<JuceMidiPort>(juceMidiPort));

                                return juceMidiPort;
                            }
                        }
                    }
                }
            }

            return null;
        }

        public JuceMidiPort openMidiInputPortWithJuceIndex (int index, long host)
        {
            return openMidiPortWithJuceIndex (index, host, true);
        }

        public JuceMidiPort openMidiOutputPortWithJuceIndex (int index)
        {
            return openMidiPortWithJuceIndex (index, 0, false);
        }

        /* 0: unpaired, 1: paired, 2: pairing */
        public int getBluetoothDeviceStatus (String address)
        {
            synchronized (MidiDeviceManager.class)
            {
                if (! address.isEmpty())
                {
                    if (findMidiDeviceForBluetoothAddress (address) != null)
                        return 1;

                    if (btDevicesPairing.containsKey (address))
                        return 2;

                    if (findOpenTaskForBluetoothAddress (address) != null)
                        return 2;
                }
            }

            return 0;
        }

        public boolean pairBluetoothDevice (BluetoothDevice btDevice)
        {
            String btAddress = btDevice.getAddress();
            if (btAddress.isEmpty())
                return false;

            synchronized (MidiDeviceManager.class)
            {
                if (getBluetoothDeviceStatus (btAddress) != 0)
                    return false;


                btDevicesPairing.put (btDevice.getAddress(), null);
                BluetoothGatt gatt = btDevice.connectGatt (getApplicationContext(), true, new DummyBluetoothGattCallback (this));

                if (gatt != null)
                {
                    btDevicesPairing.put (btDevice.getAddress(), gatt);
                }
                else
                {
                    pairBluetoothDeviceStepTwo (btDevice);
                }
            }

            return true;
        }

        public void pairBluetoothDeviceStepTwo (BluetoothDevice btDevice)
        {
            manager.openBluetoothDevice(btDevice, this, null);
        }

        public void unpairBluetoothDevice (String address)
        {
            if (address.isEmpty())
                return;

            synchronized (MidiDeviceManager.class)
            {
                if (btDevicesPairing.containsKey (address))
                {
                    BluetoothGatt gatt = btDevicesPairing.get (address);
                    if (gatt != null)
                    {
                        gatt.disconnect();
                        gatt.close();
                    }

                    btDevicesPairing.remove (address);
                }

                MidiDeviceOpenTask openTask = findOpenTaskForBluetoothAddress (address);
                if (openTask != null)
                {
                    int deviceID = openTask.getID();
                    openTask.cancel();
                    openTasks.remove (deviceID);
                }

                Pair<MidiDevice, BluetoothGatt> midiDevicePair = findMidiDeviceForBluetoothAddress (address);
                if (midiDevicePair != null)
                {
                    MidiDevice midiDevice = midiDevicePair.first;
                    onDeviceRemoved (midiDevice.getInfo());

                    try {
                        midiDevice.close();
                    }
                    catch (IOException exception)
                    {
                        Log.d ("JUCE", "IOException while closing midi device");
                    }
                }
            }
        }

        private Pair<MidiDevice, BluetoothGatt> findMidiDeviceForBluetoothAddress (String address)
        {
            for (Pair<MidiDevice,BluetoothGatt> midiDevice : midiDevices)
            {
                MidiDeviceInfo info = midiDevice.first.getInfo();
                if (info.getType() == MidiDeviceInfo.TYPE_BLUETOOTH)
                {
                    BluetoothDevice btDevice = (BluetoothDevice) info.getProperties().get (info.PROPERTY_BLUETOOTH_DEVICE);
                    if (btDevice != null && btDevice.getAddress().equals (address))
                        return midiDevice;
                }
            }

            return null;
        }

        private MidiDeviceOpenTask findOpenTaskForBluetoothAddress (String address)
        {
            for (Integer deviceID : openTasks.keySet())
            {
                MidiDeviceOpenTask openTask = openTasks.get (deviceID);
                if (openTask.getBluetoothAddress().equals (address))
                    return openTask;
            }

            return null;
        }

        public void removePort (MidiPortPath path)
        {
            openPorts.remove (path);
        }

        public String getInputPortNameForJuceIndex (int index)
        {
            MidiPortPath portInfo = getPortPathForJuceIndex (MidiDeviceInfo.PortInfo.TYPE_OUTPUT, index);
            if (portInfo != null)
                return getPortName (portInfo);

            return "";
        }

        public String getOutputPortNameForJuceIndex (int index)
        {
            MidiPortPath portInfo = getPortPathForJuceIndex (MidiDeviceInfo.PortInfo.TYPE_INPUT, index);
            if (portInfo != null)
                return getPortName (portInfo);

            return "";
        }

        public void onDeviceAdded (MidiDeviceInfo info)
        {
            // only add standard midi devices
            if (info.getType() == info.TYPE_BLUETOOTH)
                return;

            manager.openDevice (info, this, null);
        }

        public void onDeviceRemoved (MidiDeviceInfo info)
        {
            synchronized (MidiDeviceManager.class)
            {
                Pair<MidiDevice, BluetoothGatt> devicePair = getMidiDevicePairForId (info.getId());

                if (devicePair != null)
                {
                    MidiDevice midiDevice = devicePair.first;
                    BluetoothGatt gatt = devicePair.second;

                    // close all ports that use this device
                    boolean removedPort = true;

                    while (removedPort == true)
                    {
                        removedPort = false;
                        for (MidiPortPath key : openPorts.keySet())
                        {
                            if (key.deviceId == info.getId())
                            {
                                openPorts.get(key).get().close();
                                removedPort = true;
                                break;
                            }
                        }
                    }

                    if (gatt != null)
                    {
                        gatt.disconnect();
                        gatt.close();
                    }

                    midiDevices.remove (devicePair);
                }
            }
        }

        public void onDeviceStatusChanged (MidiDeviceStatus status)
        {
        }

        @Override
        public void onDeviceOpened (MidiDevice theDevice)
        {
            synchronized (MidiDeviceManager.class)
            {
                MidiDeviceInfo info = theDevice.getInfo();
                int deviceID = info.getId();
                BluetoothGatt gatt = null;
                boolean isBluetooth = false;

                if (! openTasks.containsKey (deviceID))
                {
                    if (info.getType() == MidiDeviceInfo.TYPE_BLUETOOTH)
                    {
                        isBluetooth = true;
                        BluetoothDevice btDevice = (BluetoothDevice) info.getProperties().get (info.PROPERTY_BLUETOOTH_DEVICE);
                        if (btDevice != null)
                        {
                            String btAddress = btDevice.getAddress();
                            if (btDevicesPairing.containsKey (btAddress))
                            {
                                gatt = btDevicesPairing.get (btAddress);
                                btDevicesPairing.remove (btAddress);
                            }
                            else
                            {
                                // unpair was called in the mean time
                                try
                                {
                                    Pair<MidiDevice, BluetoothGatt> midiDevicePair = findMidiDeviceForBluetoothAddress (btDevice.getAddress());
                                    if (midiDevicePair != null)
                                    {
                                        gatt = midiDevicePair.second;

                                        if (gatt != null)
                                        {
                                            gatt.disconnect();
                                            gatt.close();
                                        }
                                    }

                                    theDevice.close();
                                }
                                catch (IOException e)
                                {}

                                return;
                            }
                        }
                    }

                    MidiDeviceOpenTask openTask = new MidiDeviceOpenTask (this, theDevice, gatt);
                    openTasks.put (deviceID, openTask);

                    new java.util.Timer().schedule (openTask, (isBluetooth ? 2000 : 100));
                }
            }
        }

        public void onDeviceOpenedDelayed (MidiDevice theDevice)
        {
            synchronized (MidiDeviceManager.class)
            {
                int deviceID = theDevice.getInfo().getId();

                if (openTasks.containsKey (deviceID))
                {
                    if (! midiDevices.contains(theDevice))
                    {
                        BluetoothGatt gatt = openTasks.get (deviceID).getGatt();
                        openTasks.remove (deviceID);
                        midiDevices.add (new Pair<MidiDevice,BluetoothGatt> (theDevice, gatt));
                    }
                }
                else
                {
                    // unpair was called in the mean time
                    MidiDeviceInfo info = theDevice.getInfo();
                    BluetoothDevice btDevice = (BluetoothDevice) info.getProperties().get (info.PROPERTY_BLUETOOTH_DEVICE);
                    if (btDevice != null)
                    {
                        String btAddress = btDevice.getAddress();
                        Pair<MidiDevice, BluetoothGatt> midiDevicePair = findMidiDeviceForBluetoothAddress (btDevice.getAddress());
                        if (midiDevicePair != null)
                        {
                            BluetoothGatt gatt = midiDevicePair.second;

                            if (gatt != null)
                            {
                                gatt.disconnect();
                                gatt.close();
                            }
                        }
                    }

                    try
                    {
                        theDevice.close();
                    }
                    catch (IOException e)
                    {}
                }
            }
        }

        public String getPortName(MidiPortPath path)
        {
            int portTypeToFind = (path.isInput ? MidiDeviceInfo.PortInfo.TYPE_INPUT : MidiDeviceInfo.PortInfo.TYPE_OUTPUT);

            synchronized (MidiDeviceManager.class)
            {
                for (MidiDeviceInfo info : deviceInfos)
                {
                    int localIndex = 0;
                    if (info.getId() == path.deviceId)
                    {
                        for (MidiDeviceInfo.PortInfo portInfo : info.getPorts())
                        {
                            int portType = portInfo.getType();
                            if (portType == portTypeToFind)
                            {
                                int portIndex = portInfo.getPortNumber();
                                if (portIndex == path.portIndex)
                                {
                                    String portName = portInfo.getName();
                                    if (portName.isEmpty())
                                        portName = (String) info.getProperties().get(info.PROPERTY_NAME);

                                    return portName;
                                }
                            }
                        }
                    }
                }
            }

            return "";
        }

        public MidiPortPath getPortPathForJuceIndex (int portType, int juceIndex)
        {
            int portIdx = 0;
            for (MidiDeviceInfo info : deviceInfos)
            {
                for (MidiDeviceInfo.PortInfo portInfo : info.getPorts())
                {
                    if (portInfo.getType() == portType)
                    {
                        if (portIdx == juceIndex)
                            return new MidiPortPath (info.getId(),
                                    (portType == MidiDeviceInfo.PortInfo.TYPE_INPUT),
                                    portInfo.getPortNumber());

                        portIdx++;
                    }
                }
            }

            return null;
        }

        private MidiDeviceInfo[] getDeviceInfos()
        {
            synchronized (MidiDeviceManager.class)
            {
                MidiDeviceInfo[] infos = new MidiDeviceInfo[midiDevices.size()];

                int idx = 0;
                for (Pair<MidiDevice,BluetoothGatt> midiDevice : midiDevices)
                    infos[idx++] = midiDevice.first.getInfo();

                return infos;
            }
        }

        private Pair<MidiDevice, BluetoothGatt> getMidiDevicePairForId (int deviceId)
        {
            synchronized (MidiDeviceManager.class)
            {
                for (Pair<MidiDevice,BluetoothGatt> midiDevice : midiDevices)
                    if (midiDevice.first.getInfo().getId() == deviceId)
                        return midiDevice;
            }

            return null;
        }

        private MidiManager manager;
        private HashMap<String, BluetoothGatt> btDevicesPairing;
        private HashMap<Integer, MidiDeviceOpenTask> openTasks;
        private ArrayList<Pair<MidiDevice, BluetoothGatt>> midiDevices;
        private MidiDeviceInfo[] deviceInfos;
        private HashMap<MidiPortPath, WeakReference<JuceMidiPort>> openPorts;
    }

    public MidiDeviceManager getAndroidMidiDeviceManager()
    {
        if (getSystemService (MIDI_SERVICE) == null)
            return null;

        synchronized (JuceDemoPlugin.class)
        {
            if (midiDeviceManager == null)
                midiDeviceManager = new MidiDeviceManager();
        }

        return midiDeviceManager;
    }

    public BluetoothManager getAndroidBluetoothManager()
    {
        BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();

        if (adapter == null)
            return null;

        if (adapter.getBluetoothLeScanner() == null)
            return null;

        synchronized (JuceDemoPlugin.class)
        {
            if (bluetoothManager == null)
                bluetoothManager = new BluetoothManager();
        }

        return bluetoothManager;
    }

    //==============================================================================
    @Override
    public void onCreate (Bundle savedInstanceState)
    {
        super.onCreate (savedInstanceState);

        isScreenSaverEnabled = true;
        hideActionBar();
        viewHolder = new ViewHolder (this);
        setContentView (viewHolder);

        setVolumeControlStream (AudioManager.STREAM_MUSIC);

        permissionCallbackPtrMap = new HashMap<Integer, Long>();
    }

    @Override
    protected void onDestroy()
    {
        quitApp();
        super.onDestroy();

        clearDataCache();
    }

    @Override
    protected void onPause()
    {
        suspendApp();

        try
        {
            Thread.sleep (1000); // This is a bit of a hack to avoid some hard-to-track-down
                                 // openGL glitches when pausing/resuming apps..
        } catch (InterruptedException e) {}

        super.onPause();
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        resumeApp();
    }

    @Override
    public void onConfigurationChanged (Configuration cfg)
    {
        super.onConfigurationChanged (cfg);
        setContentView (viewHolder);
    }

    private void callAppLauncher()
    {
        launchApp (getApplicationInfo().publicSourceDir,
                   getApplicationInfo().dataDir);
    }

    private void hideActionBar()
    {
        // get "getActionBar" method
        java.lang.reflect.Method getActionBarMethod = null;
        try
        {
            getActionBarMethod = this.getClass().getMethod ("getActionBar");
        }
        catch (SecurityException e)     { return; }
        catch (NoSuchMethodException e) { return; }
        if (getActionBarMethod == null) return;

        // invoke "getActionBar" method
        Object actionBar = null;
        try
        {
            actionBar = getActionBarMethod.invoke (this);
        }
        catch (java.lang.IllegalArgumentException e) { return; }
        catch (java.lang.IllegalAccessException e) { return; }
        catch (java.lang.reflect.InvocationTargetException e) { return; }
        if (actionBar == null) return;

        // get "hide" method
        java.lang.reflect.Method actionBarHideMethod = null;
        try
        {
            actionBarHideMethod = actionBar.getClass().getMethod ("hide");
        }
        catch (SecurityException e)     { return; }
        catch (NoSuchMethodException e) { return; }
        if (actionBarHideMethod == null) return;

        // invoke "hide" method
        try
        {
            actionBarHideMethod.invoke (actionBar);
        }
        catch (java.lang.IllegalArgumentException e) {}
        catch (java.lang.IllegalAccessException e) {}
        catch (java.lang.reflect.InvocationTargetException e) {}
    }

    void requestPermissionsCompat (String[] permissions, int requestCode)
    {
        Method requestPermissionsMethod = null;
        try
        {
            requestPermissionsMethod = this.getClass().getMethod ("requestPermissions",
                                                                  String[].class, int.class);
        }
        catch (SecurityException e)     { return; }
        catch (NoSuchMethodException e) { return; }
        if (requestPermissionsMethod == null) return;

        try
        {
            requestPermissionsMethod.invoke (this, permissions, requestCode);
        }
        catch (java.lang.IllegalArgumentException e) {}
        catch (java.lang.IllegalAccessException e) {}
        catch (java.lang.reflect.InvocationTargetException e) {}
    }

    //==============================================================================
    private native void launchApp (String appFile, String appDataDir);
    private native void quitApp();
    private native void suspendApp();
    private native void resumeApp();
    private native void setScreenSize (int screenWidth, int screenHeight, int dpi);

    //==============================================================================
    public native void deliverMessage (long value);
    private android.os.Handler messageHandler = new android.os.Handler();

    public final void postMessage (long value)
    {
        messageHandler.post (new MessageCallback (value));
    }

    private final class MessageCallback  implements Runnable
    {
        public MessageCallback (long value_)        { value = value_; }
        public final void run()                     { deliverMessage (value); }

        private long value;
    }

    //==============================================================================
    private ViewHolder viewHolder;
    private MidiDeviceManager midiDeviceManager = null;
    private BluetoothManager bluetoothManager = null;
    private boolean isScreenSaverEnabled;
    private java.util.Timer keepAliveTimer;

    public final ComponentPeerView createNewView (boolean opaque, long host)
    {
        ComponentPeerView v = new ComponentPeerView (this, opaque, host);
        viewHolder.addView (v);
        return v;
    }

    public final void deleteView (ComponentPeerView view)
    {
        ViewGroup group = (ViewGroup) (view.getParent());

        if (group != null)
            group.removeView (view);
    }

    public final void deleteNativeSurfaceView (NativeSurfaceView view)
    {
        ViewGroup group = (ViewGroup) (view.getParent());

        if (group != null)
            group.removeView (view);
    }

    final class ViewHolder  extends ViewGroup
    {
        public ViewHolder (Context context)
        {
            super (context);
            setDescendantFocusability (ViewGroup.FOCUS_AFTER_DESCENDANTS);
            setFocusable (false);
        }

        protected final void onLayout (boolean changed, int left, int top, int right, int bottom)
        {
            setScreenSize (getWidth(), getHeight(), getDPI());

            if (isFirstResize)
            {
                isFirstResize = false;
                callAppLauncher();
            }
        }

        private final int getDPI()
        {
            DisplayMetrics metrics = new DisplayMetrics();
            getWindowManager().getDefaultDisplay().getMetrics (metrics);
            return metrics.densityDpi;
        }

        private boolean isFirstResize = true;
    }

    public final void excludeClipRegion (android.graphics.Canvas canvas, float left, float top, float right, float bottom)
    {
        canvas.clipRect (left, top, right, bottom, android.graphics.Region.Op.DIFFERENCE);
    }

    //==============================================================================
    public final void setScreenSaver (boolean enabled)
    {
        if (isScreenSaverEnabled != enabled)
        {
            isScreenSaverEnabled = enabled;

            if (keepAliveTimer != null)
            {
                keepAliveTimer.cancel();
                keepAliveTimer = null;
            }

            if (enabled)
            {
                getWindow().clearFlags (WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            }
            else
            {
                getWindow().addFlags (WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

                // If no user input is received after about 3 seconds, the OS will lower the
                // task's priority, so this timer forces it to be kept active.
                keepAliveTimer = new java.util.Timer();

                keepAliveTimer.scheduleAtFixedRate (new TimerTask()
                {
                    @Override
                    public void run()
                    {
                        android.app.Instrumentation instrumentation = new android.app.Instrumentation();

                        try
                        {
                            instrumentation.sendKeyDownUpSync (KeyEvent.KEYCODE_UNKNOWN);
                        }
                        catch (Exception e)
                        {
                        }
                    }
                }, 2000, 2000);
            }
        }
    }

    public final boolean getScreenSaver()
    {
        return isScreenSaverEnabled;
    }

    //==============================================================================
    public final String getClipboardContent()
    {
        ClipboardManager clipboard = (ClipboardManager) getSystemService (CLIPBOARD_SERVICE);
        return clipboard.getText().toString();
    }

    public final void setClipboardContent (String newText)
    {
        ClipboardManager clipboard = (ClipboardManager) getSystemService (CLIPBOARD_SERVICE);
        clipboard.setText (newText);
    }

    //==============================================================================
    public final void showMessageBox (String title, String message, final long callback)
    {
        AlertDialog.Builder builder = new AlertDialog.Builder (this);
        builder.setTitle (title)
               .setMessage (message)
               .setCancelable (true)
               .setPositiveButton ("OK", new DialogInterface.OnClickListener()
                    {
                        public void onClick (DialogInterface dialog, int id)
                        {
                            dialog.cancel();
                            JuceDemoPlugin.this.alertDismissed (callback, 0);
                        }
                    });

        builder.create().show();
    }

    public final void showOkCancelBox (String title, String message, final long callback,
                                       String okButtonText, String cancelButtonText)
    {
        AlertDialog.Builder builder = new AlertDialog.Builder (this);
        builder.setTitle (title)
               .setMessage (message)
               .setCancelable (true)
               .setPositiveButton (okButtonText.isEmpty() ? "OK" : okButtonText, new DialogInterface.OnClickListener()
                    {
                        public void onClick (DialogInterface dialog, int id)
                        {
                            dialog.cancel();
                            JuceDemoPlugin.this.alertDismissed (callback, 1);
                        }
                    })
               .setNegativeButton (cancelButtonText.isEmpty() ? "Cancel" : cancelButtonText, new DialogInterface.OnClickListener()
                    {
                        public void onClick (DialogInterface dialog, int id)
                        {
                            dialog.cancel();
                            JuceDemoPlugin.this.alertDismissed (callback, 0);
                        }
                    });

        builder.create().show();
    }

    public final void showYesNoCancelBox (String title, String message, final long callback)
    {
        AlertDialog.Builder builder = new AlertDialog.Builder (this);
        builder.setTitle (title)
               .setMessage (message)
               .setCancelable (true)
               .setPositiveButton ("Yes", new DialogInterface.OnClickListener()
                    {
                        public void onClick (DialogInterface dialog, int id)
                        {
                            dialog.cancel();
                            JuceDemoPlugin.this.alertDismissed (callback, 1);
                        }
                    })
               .setNegativeButton ("No", new DialogInterface.OnClickListener()
                    {
                        public void onClick (DialogInterface dialog, int id)
                        {
                            dialog.cancel();
                            JuceDemoPlugin.this.alertDismissed (callback, 2);
                        }
                    })
               .setNeutralButton ("Cancel", new DialogInterface.OnClickListener()
                    {
                        public void onClick (DialogInterface dialog, int id)
                        {
                            dialog.cancel();
                            JuceDemoPlugin.this.alertDismissed (callback, 0);
                        }
                    });

        builder.create().show();
    }

    public native void alertDismissed (long callback, int id);

    //==============================================================================
    public final class ComponentPeerView extends ViewGroup
                                         implements View.OnFocusChangeListener
    {
        public ComponentPeerView (Context context, boolean opaque_, long host)
        {
            super (context);
            this.host = host;
            setWillNotDraw (false);
            opaque = opaque_;

            setFocusable (true);
            setFocusableInTouchMode (true);
            setOnFocusChangeListener (this);
            requestFocus();

            // swap red and blue colours to match internal opengl texture format
            ColorMatrix colorMatrix = new ColorMatrix();

            float[] colorTransform = { 0,    0,    1.0f, 0,    0,
                                       0,    1.0f, 0,    0,    0,
                                       1.0f, 0,    0,    0,    0,
                                       0,    0,    0,    1.0f, 0 };

            colorMatrix.set (colorTransform);
            paint.setColorFilter (new ColorMatrixColorFilter (colorMatrix));
        }

        //==============================================================================
        private native void handlePaint (long host, Canvas canvas, Paint paint);

        @Override
        public void onDraw (Canvas canvas)
        {
            handlePaint (host, canvas, paint);
        }

        @Override
        public boolean isOpaque()
        {
            return opaque;
        }

        private boolean opaque;
        private long host;
        private Paint paint = new Paint();

        //==============================================================================
        private native void handleMouseDown (long host, int index, float x, float y, long time);
        private native void handleMouseDrag (long host, int index, float x, float y, long time);
        private native void handleMouseUp   (long host, int index, float x, float y, long time);

        @Override
        public boolean onTouchEvent (MotionEvent event)
        {
            int action = event.getAction();
            long time = event.getEventTime();

            switch (action & MotionEvent.ACTION_MASK)
            {
                case MotionEvent.ACTION_DOWN:
                    handleMouseDown (host, event.getPointerId(0), event.getX(), event.getY(), time);
                    return true;

                case MotionEvent.ACTION_CANCEL:
                case MotionEvent.ACTION_UP:
                    handleMouseUp (host, event.getPointerId(0), event.getX(), event.getY(), time);
                    return true;

                case MotionEvent.ACTION_MOVE:
                {
                    int n = event.getPointerCount();
                    for (int i = 0; i < n; ++i)
                        handleMouseDrag (host, event.getPointerId(i), event.getX(i), event.getY(i), time);

                    return true;
                }

                case MotionEvent.ACTION_POINTER_UP:
                {
                    int i = (action & MotionEvent.ACTION_POINTER_INDEX_MASK) >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
                    handleMouseUp (host, event.getPointerId(i), event.getX(i), event.getY(i), time);
                    return true;
                }

                case MotionEvent.ACTION_POINTER_DOWN:
                {
                    int i = (action & MotionEvent.ACTION_POINTER_INDEX_MASK) >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
                    handleMouseDown (host, event.getPointerId(i), event.getX(i), event.getY(i), time);
                    return true;
                }

                default:
                    break;
            }

            return false;
        }

        //==============================================================================
        private native void handleKeyDown (long host, int keycode, int textchar);
        private native void handleKeyUp (long host, int keycode, int textchar);

        public void showKeyboard (String type)
        {
            InputMethodManager imm = (InputMethodManager) getSystemService (Context.INPUT_METHOD_SERVICE);

            if (imm != null)
            {
                if (type.length() > 0)
                {
                    imm.showSoftInput (this, android.view.inputmethod.InputMethodManager.SHOW_IMPLICIT);
                    imm.setInputMethod (getWindowToken(), type);
                }
                else
                {
                    imm.hideSoftInputFromWindow (getWindowToken(), 0);
                }
            }
        }

        @Override
        public boolean onKeyDown (int keyCode, KeyEvent event)
        {
            switch (keyCode)
            {
                case KeyEvent.KEYCODE_VOLUME_UP:
                case KeyEvent.KEYCODE_VOLUME_DOWN:
                    return super.onKeyDown (keyCode, event);

                default: break;
            }

            handleKeyDown (host, keyCode, event.getUnicodeChar());
            return true;
        }

        @Override
        public boolean onKeyUp (int keyCode, KeyEvent event)
        {
            handleKeyUp (host, keyCode, event.getUnicodeChar());
            return true;
        }

        @Override
        public boolean onKeyMultiple (int keyCode, int count, KeyEvent event)
        {
            if (keyCode != KeyEvent.KEYCODE_UNKNOWN || event.getAction() != KeyEvent.ACTION_MULTIPLE)
                return super.onKeyMultiple (keyCode, count, event);

            if (event.getCharacters() != null)
            {
                int utf8Char = event.getCharacters().codePointAt (0);
                handleKeyDown (host, utf8Char, utf8Char);
                return true;
            }

            return false;
        }

        // this is here to make keyboard entry work on a Galaxy Tab2 10.1
        @Override
        public InputConnection onCreateInputConnection (EditorInfo outAttrs)
        {
            outAttrs.actionLabel = "";
            outAttrs.hintText = "";
            outAttrs.initialCapsMode = 0;
            outAttrs.initialSelEnd = outAttrs.initialSelStart = -1;
            outAttrs.label = "";
            outAttrs.imeOptions = EditorInfo.IME_ACTION_DONE | EditorInfo.IME_FLAG_NO_EXTRACT_UI;
            outAttrs.inputType = InputType.TYPE_NULL;

            return new BaseInputConnection (this, false);
        }

        //==============================================================================
        @Override
        protected void onSizeChanged (int w, int h, int oldw, int oldh)
        {
            super.onSizeChanged (w, h, oldw, oldh);
            viewSizeChanged (host);
        }

        @Override
        protected void onLayout (boolean changed, int left, int top, int right, int bottom)
        {
            for (int i = getChildCount(); --i >= 0;)
                requestTransparentRegion (getChildAt (i));
        }

        private native void viewSizeChanged (long host);

        @Override
        public void onFocusChange (View v, boolean hasFocus)
        {
            if (v == this)
                focusChanged (host, hasFocus);
        }

        private native void focusChanged (long host, boolean hasFocus);

        public void setViewName (String newName)    {}

        public void setSystemUiVisibilityCompat (int visibility)
        {
            Method systemUIVisibilityMethod = null;
            try
            {
                systemUIVisibilityMethod = this.getClass().getMethod ("setSystemUiVisibility", int.class);
            }
            catch (SecurityException e)     { return; }
            catch (NoSuchMethodException e) { return; }
            if (systemUIVisibilityMethod == null) return;

            try
            {
                systemUIVisibilityMethod.invoke (this, visibility);
            }
            catch (java.lang.IllegalArgumentException e) {}
            catch (java.lang.IllegalAccessException e) {}
            catch (java.lang.reflect.InvocationTargetException e) {}
        }

        public boolean isVisible()                  { return getVisibility() == VISIBLE; }
        public void setVisible (boolean b)          { setVisibility (b ? VISIBLE : INVISIBLE); }

        public boolean containsPoint (int x, int y)
        {
            return true; //xxx needs to check overlapping views
        }
    }

    //==============================================================================
    public static class NativeSurfaceView    extends SurfaceView
                                          implements SurfaceHolder.Callback
    {
        private long nativeContext = 0;

        NativeSurfaceView (Context context, long nativeContextPtr)
        {
            super (context);
            nativeContext = nativeContextPtr;
        }

        public Surface getNativeSurface()
        {
            Surface retval = null;

            SurfaceHolder holder = getHolder();
            if (holder != null)
                retval = holder.getSurface();

            return retval;
        }

        //==============================================================================
        @Override
        public void surfaceChanged (SurfaceHolder holder, int format, int width, int height)
        {
            surfaceChangedNative (nativeContext, holder, format, width, height);
        }

        @Override
        public void surfaceCreated (SurfaceHolder holder)
        {
            surfaceCreatedNative (nativeContext, holder);
        }

        @Override
        public void surfaceDestroyed (SurfaceHolder holder)
        {
            surfaceDestroyedNative (nativeContext, holder);
        }

        @Override
        protected void dispatchDraw (Canvas canvas)
        {
            super.dispatchDraw (canvas);
            dispatchDrawNative (nativeContext, canvas);
        }

        //==============================================================================
        @Override
        protected void onAttachedToWindow ()
        {
            super.onAttachedToWindow();
            getHolder().addCallback (this);
        }

        @Override
        protected void onDetachedFromWindow ()
        {
            super.onDetachedFromWindow();
            getHolder().removeCallback (this);
        }

        //==============================================================================
        private native void dispatchDrawNative (long nativeContextPtr, Canvas canvas);
        private native void surfaceCreatedNative (long nativeContextptr, SurfaceHolder holder);
        private native void surfaceDestroyedNative (long nativeContextptr, SurfaceHolder holder);
        private native void surfaceChangedNative (long nativeContextptr, SurfaceHolder holder,
                                                  int format, int width, int height);
    }

    public NativeSurfaceView createNativeSurfaceView (long nativeSurfacePtr)
    {
        return new NativeSurfaceView (this, nativeSurfacePtr);
    }

    //==============================================================================
    public final int[] renderGlyph (char glyph1, char glyph2, Paint paint, android.graphics.Matrix matrix, Rect bounds)
    {
        Path p = new Path();

        char[] str = { glyph1, glyph2 };
        paint.getTextPath (str, 0, (glyph2 != 0 ? 2 : 1), 0.0f, 0.0f, p);

        RectF boundsF = new RectF();
        p.computeBounds (boundsF, true);
        matrix.mapRect (boundsF);

        boundsF.roundOut (bounds);
        bounds.left--;
        bounds.right++;

        final int w = bounds.width();
        final int h = Math.max (1, bounds.height());

        Bitmap bm = Bitmap.createBitmap (w, h, Bitmap.Config.ARGB_8888);

        Canvas c = new Canvas (bm);
        matrix.postTranslate (-bounds.left, -bounds.top);
        c.setMatrix (matrix);
        c.drawPath (p, paint);

        final int sizeNeeded = w * h;
        if (cachedRenderArray.length < sizeNeeded)
            cachedRenderArray = new int [sizeNeeded];

        bm.getPixels (cachedRenderArray, 0, w, 0, 0, w, h);
        bm.recycle();
        return cachedRenderArray;
    }

    private int[] cachedRenderArray = new int [256];

    //==============================================================================
    public static class HTTPStream
    {
        public HTTPStream (HttpURLConnection connection_,
                           int[] statusCode, StringBuffer responseHeaders) throws IOException
        {
            connection = connection_;

            try
            {
                inputStream = new BufferedInputStream (connection.getInputStream());
            }
            catch (IOException e)
            {
                if (connection.getResponseCode() < 400)
                    throw e;
            }
            finally
            {
                statusCode[0] = connection.getResponseCode();
            }

            if (statusCode[0] >= 400)
                inputStream = connection.getErrorStream();
            else
                inputStream = connection.getInputStream();

            for (java.util.Map.Entry<String, java.util.List<String>> entry : connection.getHeaderFields().entrySet())
                if (entry.getKey() != null && entry.getValue() != null)
                    responseHeaders.append (entry.getKey() + ": "
                                             + android.text.TextUtils.join (",", entry.getValue()) + "\n");
        }

        public final void release()
        {
            try
            {
                inputStream.close();
            }
            catch (IOException e)
            {}

            connection.disconnect();
        }

        public final int read (byte[] buffer, int numBytes)
        {
            int num = 0;

            try
            {
                num = inputStream.read (buffer, 0, numBytes);
            }
            catch (IOException e)
            {}

            if (num > 0)
                position += num;

            return num;
        }

        public final long getPosition()                 { return position; }
        public final long getTotalLength()              { return -1; }
        public final boolean isExhausted()              { return false; }
        public final boolean setPosition (long newPos)  { return false; }

        private HttpURLConnection connection;
        private InputStream inputStream;
        private long position;
    }

    public static final HTTPStream createHTTPStream (String address, boolean isPost, byte[] postData,
                                                     String headers, int timeOutMs, int[] statusCode,
                                                     StringBuffer responseHeaders, int numRedirectsToFollow,
                                                     String httpRequestCmd)
    {
        // timeout parameter of zero for HttpUrlConnection is a blocking connect (negative value for juce::URL)
        if (timeOutMs < 0)
            timeOutMs = 0;
        else if (timeOutMs == 0)
            timeOutMs = 30000;

        // headers - if not empty, this string is appended onto the headers that are used for the request. It must therefore be a valid set of HTML header directives, separated by newlines.
        // So convert headers string to an array, with an element for each line
        String headerLines[] = headers.split("\\n");

        for (;;)
        {
            try
            {
                HttpURLConnection connection = (HttpURLConnection) (new URL(address).openConnection());

                if (connection != null)
                {
                    try
                    {
                        connection.setInstanceFollowRedirects (false);
                        connection.setConnectTimeout (timeOutMs);
                        connection.setReadTimeout (timeOutMs);

                        // Set request headers
                        for (int i = 0; i < headerLines.length; ++i)
                        {
                            int pos = headerLines[i].indexOf (":");

                            if (pos > 0 && pos < headerLines[i].length())
                            {
                                String field = headerLines[i].substring (0, pos);
                                String value = headerLines[i].substring (pos + 1);

                                if (value.length() > 0)
                                    connection.setRequestProperty (field, value);
                            }
                        }

                        connection.setRequestMethod (httpRequestCmd);
                        if (isPost)
                        {
                            connection.setDoOutput (true);

                            if (postData != null)
                            {
                                OutputStream out = connection.getOutputStream();
                                out.write(postData);
                                out.flush();
                            }
                        }

                        HTTPStream httpStream = new HTTPStream (connection, statusCode, responseHeaders);

                        // Process redirect & continue as necessary
                        int status = statusCode[0];

                        if (--numRedirectsToFollow >= 0
                             && (status == 301 || status == 302 || status == 303 || status == 307))
                        {
                            // Assumes only one occurrence of "Location"
                            int pos1 = responseHeaders.indexOf ("Location:") + 10;
                            int pos2 = responseHeaders.indexOf ("\n", pos1);

                            if (pos2 > pos1)
                            {
                                String newLocation = responseHeaders.substring(pos1, pos2);
                                // Handle newLocation whether it's absolute or relative
                                URL baseUrl = new URL (address);
                                URL newUrl = new URL (baseUrl, newLocation);
                                String transformedNewLocation = newUrl.toString();

                                if (transformedNewLocation != address)
                                {
                                    address = transformedNewLocation;
                                    // Clear responseHeaders before next iteration
                                    responseHeaders.delete (0, responseHeaders.length());
                                    continue;
                                }
                            }
                        }

                        return httpStream;
                    }
                    catch (Throwable e)
                    {
                        connection.disconnect();
                    }
                }
            }
            catch (Throwable e) {}

            return null;
        }
    }

    public final void launchURL (String url)
    {
        startActivity (new Intent (Intent.ACTION_VIEW, Uri.parse (url)));
    }

    public static final String getLocaleValue (boolean isRegion)
    {
        java.util.Locale locale = java.util.Locale.getDefault();

        return isRegion ? locale.getCountry()
                        : locale.getLanguage();
    }

    private static final String getFileLocation (String type)
    {
        return Environment.getExternalStoragePublicDirectory (type).getAbsolutePath();
    }

    public static final String getDocumentsFolder()  { return Environment.getDataDirectory().getAbsolutePath(); }
    public static final String getPicturesFolder()   { return getFileLocation (Environment.DIRECTORY_PICTURES); }
    public static final String getMusicFolder()      { return getFileLocation (Environment.DIRECTORY_MUSIC); }
    public static final String getMoviesFolder()     { return getFileLocation (Environment.DIRECTORY_MOVIES); }
    public static final String getDownloadsFolder()  { return getFileLocation (Environment.DIRECTORY_DOWNLOADS); }

    //==============================================================================
    private final class SingleMediaScanner  implements MediaScannerConnectionClient
    {
        public SingleMediaScanner (Context context, String filename)
        {
            file = filename;
            msc = new MediaScannerConnection (context, this);
            msc.connect();
        }

        @Override
        public void onMediaScannerConnected()
        {
            msc.scanFile (file, null);
        }

        @Override
        public void onScanCompleted (String path, Uri uri)
        {
            msc.disconnect();
        }

        private MediaScannerConnection msc;
        private String file;
    }

    public final void scanFile (String filename)
    {
        new SingleMediaScanner (this, filename);
    }

    public final Typeface getTypeFaceFromAsset (String assetName)
    {
        try
        {
            return Typeface.createFromAsset (this.getResources().getAssets(), assetName);
        }
        catch (Throwable e) {}

        return null;
    }

    final protected static char[] hexArray = "0123456789ABCDEF".toCharArray();

    public static String bytesToHex (byte[] bytes)
    {
        char[] hexChars = new char[bytes.length * 2];

        for (int j = 0; j < bytes.length; ++j)
        {
            int v = bytes[j] & 0xff;
            hexChars[j * 2]     = hexArray[v >>> 4];
            hexChars[j * 2 + 1] = hexArray[v & 0x0f];
        }

        return new String (hexChars);
    }

    final private java.util.Map dataCache = new java.util.HashMap();

    synchronized private final File getDataCacheFile (byte[] data)
    {
        try
        {
            java.security.MessageDigest digest = java.security.MessageDigest.getInstance ("MD5");
            digest.update (data);

            String key = bytesToHex (digest.digest());

            if (dataCache.containsKey (key))
                return (File) dataCache.get (key);

            File f = new File (this.getCacheDir(), "bindata_" + key);
            f.delete();
            FileOutputStream os = new FileOutputStream (f);
            os.write (data, 0, data.length);
            dataCache.put (key, f);
            return f;
        }
        catch (Throwable e) {}

        return null;
    }

    private final void clearDataCache()
    {
        java.util.Iterator it = dataCache.values().iterator();

        while (it.hasNext())
        {
            File f = (File) it.next();
            f.delete();
        }
    }

    public final Typeface getTypeFaceFromByteArray (byte[] data)
    {
        try
        {
            File f = getDataCacheFile (data);

            if (f != null)
                return Typeface.createFromFile (f);
        }
        catch (Exception e)
        {
            Log.e ("JUCE", e.toString());
        }

        return null;
    }

    public final int getAndroidSDKVersion()
    {
        return android.os.Build.VERSION.SDK_INT;
    }

    public final String audioManagerGetProperty (String property)
    {
        Object obj = getSystemService (AUDIO_SERVICE);
        if (obj == null)
            return null;

        java.lang.reflect.Method method;

        try
        {
            method = obj.getClass().getMethod ("getProperty", String.class);
        }
        catch (SecurityException e)     { return null; }
        catch (NoSuchMethodException e) { return null; }

        if (method == null)
            return null;

        try
        {
            return (String) method.invoke (obj, property);
        }
        catch (java.lang.IllegalArgumentException e) {}
        catch (java.lang.IllegalAccessException e) {}
        catch (java.lang.reflect.InvocationTargetException e) {}

        return null;
    }

    public final boolean hasSystemFeature (String property)
    {
        return getPackageManager().hasSystemFeature (property);
    }
}
