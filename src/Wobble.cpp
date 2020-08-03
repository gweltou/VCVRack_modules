#include "plugin.hpp"
#include <samplerate.h>


#define HISTORY_SIZE (1<<12)

struct Wobble : Module {
	enum ParamIds {
		RATE_PARAM,
		DEPTH_PARAM,
		COLOR_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		IN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		DBG_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};
	
	const float max_depth = (float) (1<<12);
	
	dsp::DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer;
	dsp::DoubleRingBuffer<float, 16> outBuffer;
	SRC_STATE* src;
	float rate;
	float depth;
	float color;
	
	
	float delay = 0.f;   // index in delay buffer
	float vel = 0.f;

	Wobble() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(RATE_PARAM, 0.f, 1.f, 0.1f, "Rate");
		configParam(DEPTH_PARAM, 0.f, max_depth, max_depth/2, "Depth");
		configParam(COLOR_PARAM, 0.f, 1.f, 0.f, "Color");
		
		src = src_new(SRC_SINC_FASTEST, 1, NULL);
		assert(src);
	}
	
	~Wobble() {
		src_delete(src);
	}

	void process(const ProcessArgs& args) override {
	    rate = params[RATE_PARAM].getValue() * 1e-8;
	    depth = params[DEPTH_PARAM].getValue();
	    color = params[COLOR_PARAM].getValue();
	    
	    vel += rate * (0.5 - delay + (random::uniform()*1 - 0.5f));
	    delay += vel;	    
	    delay = clamp(delay, 0.f, 1.f);
        
	    outputs[DBG_OUTPUT].setVoltage(delay*10.f);
	    
	    float dry = inputs[IN_INPUT].getVoltage();
	    
	    if (!historyBuffer.full()) {
		    historyBuffer.push(dry);
		}
		
		float index = std::round(500 + delay * depth);
		// How many samples do we need consume to catch up?
		float consume = index - historyBuffer.size();

		if (outBuffer.empty()) {
			double ratio = 1.f;
			if (std::fabs(consume) >= 16.f) {
				// Here's where the delay magic is. Smooth the ratio depending on how divergent we are from the correct delay time.
				ratio = std::pow(10.f, clamp(consume / 10000.f, -1.f, 1.f));
			}

			SRC_DATA srcData;
			srcData.data_in = (const float*) historyBuffer.startData();
			srcData.data_out = (float*) outBuffer.endData();
			srcData.input_frames = std::min((int) historyBuffer.size(), 16);
			srcData.output_frames = outBuffer.capacity();
			srcData.end_of_input = false;
			srcData.src_ratio = ratio;
			src_process(src, &srcData);
			historyBuffer.startIncr(srcData.input_frames_used);
			outBuffer.endIncr(srcData.output_frames_gen);
		}

		float wet = 0.f;
		if (!outBuffer.empty()) {
			wet = outBuffer.shift();
		}

		outputs[OUT_OUTPUT].setVoltage(wet);
	}
};


struct WobbleWidget : ModuleWidget {
	WobbleWidget(Wobble* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Wobble.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 33.647)), module, Wobble::RATE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 52.878)), module, Wobble::DEPTH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.56, 72.11)), module, Wobble::COLOR_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 98.645)), module, Wobble::IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 113.475)), module, Wobble::OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 85)), module, Wobble::DBG_OUTPUT));
	}
};


Model* modelWobble = createModel<Wobble, WobbleWidget>("Wobble");
