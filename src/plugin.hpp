#pragma once
#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model* modelGaussian;
extern Model* modelWobble;
extern Model* modelHookeOsc;
extern Model* modelPSwitch;
extern Model* modelLogMapOsc;
extern Model* modelTriliumCV;
