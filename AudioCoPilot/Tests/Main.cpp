#define CATCH_CONFIG_RUNNER
#include <catch2/catch_all.hpp>

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_gui_basics/juce_gui_basics.h>

int main(int argc, char* argv[])
{
    // Inicializa JUCE
    juce::initialiseJuce_GUI();
    
    // Roda testes
    int result = Catch::Session().run(argc, argv);
    
    // Limpa JUCE
    juce::shutdownJuce_GUI();
    
    return result;
}

