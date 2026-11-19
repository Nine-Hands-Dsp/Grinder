# Outil hors plugin : compile le processeur + la fenetre de Grinder sous Linux, exporte des captures PNG et mesure le DSP
cmake_minimum_required(VERSION 3.22)
project(GrinderRender VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
add_subdirectory(${JUCE_DIR} JUCE)
set(W ${CMAKE_CURRENT_SOURCE_DIR}/../..)
file(STRINGS ${W}/CMakeLists.txt CM_PRJ REGEX "^project\\(Grinder VERSION")
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" CM_VER "${CM_PRJ}")
juce_add_binary_data(GrinderAssets SOURCES ${W}/logo/NineHandsLogoWhite.svg
  ${W}/fonts/Poppins-Regular.ttf ${W}/fonts/Poppins-Medium.ttf ${W}/fonts/Poppins-Bold.ttf)
juce_add_console_app(GrinderRender PRODUCT_NAME "GrinderRender")
target_sources(GrinderRender PRIVATE main.cpp ${W}/PluginProcessor.cpp ${W}/PluginEditor.cpp ${W}/WindowMinSize.cpp)
target_include_directories(GrinderRender PRIVATE ${W})
target_compile_definitions(GrinderRender PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_ALSA=0 JUCE_JACK=0
  JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_USE_XSHM=0 JucePlugin_Name="Grinder" JucePlugin_VersionString="${CM_VER}"
  JucePlugin_Build_VST3=0 JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(GrinderRender PRIVATE GrinderAssets juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
