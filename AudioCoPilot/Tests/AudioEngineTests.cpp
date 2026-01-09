#include <catch2/catch_all.hpp>
#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "Core/DeviceManager.h"
#include "Core/AudioEngine.h"

TEST_CASE("AudioEngine: Initialization", "[audioengine]")
{
    DeviceManager deviceManager;
    AudioEngine engine(deviceManager);
    
    SECTION("Starts not running")
    {
        REQUIRE_FALSE(engine.isRunning());
    }
    
    SECTION("Returns empty levels when not running")
    {
        auto levels = engine.getCurrentLevels();
        REQUIRE(levels.empty());
    }
}

TEST_CASE("DeviceManager: Device enumeration", "[devicemanager]")
{
    DeviceManager deviceManager;
    
    SECTION("Can get device list without crashing")
    {
        auto devices = deviceManager.getAvailableDevices();
        // Não podemos garantir que há devices, mas não deve crashar
        REQUIRE_NOTHROW(deviceManager.getAvailableDevices());
    }
    
    SECTION("Returns 'No Device' when not initialized")
    {
        auto current = deviceManager.getCurrentDevice();
        REQUIRE(current == "No Device");
    }
}

