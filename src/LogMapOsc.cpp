#include "plugin.hpp"


struct LogMapOsc : Module {
	enum ParamIds {
		FREQ_PARAM,
		RCVMOD_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		PITCH_INPUT,
		RMOD_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	float t = 0.f;
	//float cycle;
	float prev, val;

	LogMapOsc() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(FREQ_PARAM, -54.f, 54.f, 0.f, "Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(RCVMOD_PARAM, 3.2f, 3.994f, 3.f, "");
		prev = 0.5f;
    	val = prev;
	}

	void process(const ProcessArgs& args) override {
		float pitch = params[FREQ_PARAM].getValue() / 12.f;
		pitch += inputs[PITCH_INPUT].getVoltage();
		//float freq = dsp::FREQ_C4 * dsp::approxExp2_taylor5(pitch + 30) / 1073741824;	// works only if pitch <= 1.f
		float freq = dsp::FREQ_C4 * std::pow(2.f, pitch);
		float cycle = 0.5f / freq;
		float r = params[RCVMOD_PARAM].getValue();

		t += args.sampleTime;
	    if (t >= cycle) {
          t -= cycle;
          prev = val;
          val = r * prev *(1.0f - prev);
        }
        
        float l = t / cycle;
		float value = 2.0f * lerp(prev, val, l) - 1.0f;


		outputs[OUT_OUTPUT].setVoltage((value-0.2f) * 5.f);
	}

	// Precise method, which guarantees v = v1 when t = 1.
	float lerp(float v0, float v1, float t) {
		return (1 - t) * v0 + t * v1;
	}
};


struct LogMapOscWidget : ModuleWidget {
	LogMapOscWidget(LogMapOsc* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/LogMapOsc.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(12.7, 42.05)), module, LogMapOsc::FREQ_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(18.608, 77.487)), module, LogMapOsc::RCVMOD_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 57.089)), module, LogMapOsc::PITCH_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.149, 77.487)), module, LogMapOsc::RMOD_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(12.7, 112.417)), module, LogMapOsc::OUT_OUTPUT));
	}
};


Model* modelLogMapOsc = createModel<LogMapOsc, LogMapOscWidget>("LogMapOsc");
