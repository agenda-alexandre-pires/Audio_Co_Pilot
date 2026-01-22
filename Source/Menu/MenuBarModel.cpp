#include "MenuBarModel.h"

MenuBarModel::MenuBarModel(DeviceManager& deviceManager)
    : deviceManager(deviceManager)
{
    LocalizedStrings::getInstance().addChangeListener(this);
}

MenuBarModel::~MenuBarModel()
{
    LocalizedStrings::getInstance().removeChangeListener(this);
}

juce::StringArray MenuBarModel::getMenuBarNames()
{
    auto& strings = LocalizedStrings::getInstance();
    return {
        strings.getMenuApp(),
        strings.getMenuSettings(),
        strings.getMenuDevice(),
        strings.getMenuModules()
    };
}

juce::PopupMenu MenuBarModel::getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName)
{
    juce::PopupMenu menu;
    auto& strings = LocalizedStrings::getInstance();
    
    switch (topLevelMenuIndex)
    {
        case 0: // App Menu
            menu.addItem(About, strings.getMenuAbout());
            menu.addSeparator();
            menu.addItem(Quit, strings.getMenuQuit());
            break;
            
        case 1: // Settings Menu
        {
            juce::PopupMenu languageMenu;
            languageMenu.addItem(LanguageEnglish, strings.getLanguageNameEnglish(), true,
                                strings.getCurrentLanguage() == LocalizedStrings::Language::English);
            languageMenu.addItem(LanguagePortuguese, strings.getLanguageNamePortuguese(), true,
                                strings.getCurrentLanguage() == LocalizedStrings::Language::Portuguese_BR);
            
            menu.addSubMenu(strings.getMenuLanguage(), languageMenu);
            break;
        }
        
        case 2: // Device Menu
            menu.addItem(DeviceSelector, strings.getMenuDeviceSelector());
            menu.addSeparator();
            populateDeviceMenu(menu);
            break;
            
        case 3: // Modules Menu
            menu.addItem(TransferFunction, strings.getMenuModuleTransferFunction());
            menu.addItem(AntiMasking, strings.getMenuModuleAntiMasking());
            menu.addItem(RTA, strings.getMenuModuleRTA());
            menu.addItem(AIStageHand, strings.getMenuModuleAIStageHand());
            break;
    }
    
    return menu;
}

void MenuBarModel::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
    switch (menuItemID)
    {
        case About:
        {
            auto& strings = LocalizedStrings::getInstance();
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::InfoIcon,
                "Audio Co-Pilot",
                strings.getBrandLine() + "\n\n" + "Professional audio application for macOS."
            );
            break;
        }
        
        case Quit:
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
            break;
        
        case LanguageEnglish:
            LocalizedStrings::getInstance().setLanguage(LocalizedStrings::Language::English);
            break;
        
        case LanguagePortuguese:
            LocalizedStrings::getInstance().setLanguage(LocalizedStrings::Language::Portuguese_BR);
            break;
        
        case TransferFunction:
            if (moduleActivationCallback)
                moduleActivationCallback(TransferFunction);
            break;

        case AntiMasking:
            if (moduleActivationCallback)
                moduleActivationCallback(AntiMasking);
            break;
        
        case RTA:
            if (moduleActivationCallback)
                moduleActivationCallback(RTA);
            break;

        case AIStageHand:
            if (moduleActivationCallback)
                moduleActivationCallback(AIStageHand);
            break;
        
        case DeviceSelector:
            if (moduleActivationCallback)
                moduleActivationCallback(DeviceSelector);
            break;
        
        default:
            // Device selection
            if (menuItemID >= 100 && menuItemID < 200)
            {
                int deviceIndex = menuItemID - 100;
                auto devices = deviceManager.getAvailableFullDuplexDevices();
                if (deviceIndex >= 0 && deviceIndex < devices.size())
                {
                    deviceManager.switchDevice(devices[deviceIndex]);
                }
            }
            break;
    }
}

void MenuBarModel::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &LocalizedStrings::getInstance())
    {
        menuItemsChanged();
    }
}

void MenuBarModel::populateDeviceMenu(juce::PopupMenu& menu)
{
    auto& strings = LocalizedStrings::getInstance();
    
    auto inputDevices = deviceManager.getAvailableInputDevices();
    auto outputDevices = deviceManager.getAvailableOutputDevices();
    
    // Combine all devices (deduplicate)
    juce::StringArray allDevices;
    for (const auto& device : inputDevices)
    {
        if (!allDevices.contains(device))
            allDevices.add(device);
    }
    for (const auto& device : outputDevices)
    {
        if (!allDevices.contains(device))
            allDevices.add(device);
    }
    
    juce::String currentInput = deviceManager.getCurrentInputDeviceName();
    juce::String currentOutput = deviceManager.getCurrentOutputDeviceName();
    
    int itemId = 100;
    for (const auto& device : allDevices)
    {
        bool isSelected = (device == currentInput || device == currentOutput);
        menu.addItem(itemId, device, true, isSelected);
        itemId++;
    }
    
    if (allDevices.isEmpty())
    {
        menu.addItem(-1, strings.getNoDeviceSelected(), false, false);
    }
}
