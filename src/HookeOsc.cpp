#include "plugin.hpp"


static const int maxPolyphony = engine::PORT_MAX_CHANNELS;


struct HookeOsc : Module {
	enum ParamIds {
		FREQ_PARAM,
		CHAOS_PARAM,
		SLOW_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		PITCH_INPUT,
		KMOD_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};
    
    int currentPolyphony = 1;
	int loopCounter = 0;
	
	float value[maxPolyphony] = {};
	float vel[maxPolyphony] = {};
	//float spring_kc[maxPolyphony] = {};
    float spring_k[maxPolyphony] = {};
	
	HookeOsc() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(SLOW_PARAM, 0.f, 1.f, 0.f, "Slow mode");
		configParam(FREQ_PARAM, -54.f, 54.f, 0.f, "Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4, 0.f);
		configParam(CHAOS_PARAM, 0.f, 0.4f, 0.f, "Chaos");
		
		// Initialize springs extension
		for (int i=0; i<maxPolyphony; i++) {
		    value[i] = 1.f;
		}
	}
	
	
	void process(const ProcessArgs& args) override {
	    
	    if (loopCounter-- == 0) {
            loopCounter = 3;
            processEvery4Samples(args);
        }
	    
	    generateOutput();
	}
	
	void processEvery4Samples(const ProcessArgs& args) {
	    currentPolyphony = std::max(1, inputs[PITCH_INPUT].getChannels());
        outputs[OUT_OUTPUT].setChannels(currentPolyphony);
        
	    float chaos = params[CHAOS_PARAM].getValue();
	    float pitchParam = params[FREQ_PARAM].getValue() / 12.f;
	    
	    float slow = params[SLOW_PARAM].getValue();
	    
	    float k_mod = 0.f;
	    if (inputs[KMOD_INPUT].isConnected()) {
	        k_mod = inputs[KMOD_INPUT].getVoltage() / 10.f;
	    }
	    
	    for (int i=0; i<currentPolyphony; i++) {
	        
	        float pitch = pitchParam + inputs[PITCH_INPUT].getVoltage(i);
	        float freq = dsp::FREQ_C4 * std::pow(2.f, pitch);
	        
	        // 0.000142465 is the magic number to tune the oscillator
	        if (slow == 1.f) {
	            spring_k[i] = freq * 0.000001 * ((random::normal()-0.45f) * chaos + 1) + k_mod;
	        } else {
	            spring_k[i] = freq * 0.000142465 * ((random::normal()-0.45f) * chaos + 1) + k_mod;
	        }
	        
	        /*
	        float ks = pitch;
	        if (ks != prev_ks) {
	            printf("%f %f\n", ks, freq);
	        }
	        prev_ks = ks;
	        */
        }
	}
	
	void processEvery8Samples(const ProcessArgs& args) {
	    
	}
	
	void generateOutput() {
	    for (int i=0; i<currentPolyphony; i++) {
            vel[i] += -spring_k[i]*spring_k[i] * value[i];
            
            value[i] += vel[i];
            
            if (value[i] < -1.f) {
              value[i] = -0.99f;
              vel[i] = spring_k[i]*spring_k[i];
            } else if (value[i] > 1.f) {
              value[i] = 0.99f;
              vel[i] = -spring_k[i]*spring_k[i];
            }
	        
	        outputs[OUT_OUTPUT].setVoltage(value[i] * 5.f, i);
	    }
	}
};


struct HookeOscWidget : ModuleWidget {
	HookeOscWidget(HookeOsc* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/HookeOsc.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        
        addParam(createParam<CKSS>(mm2px(Vec(5.2, 20.6)), module, HookeOsc::SLOW_PARAM));
        
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 44.23)), module, HookeOsc::FREQ_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 98.068)), module, HookeOsc::CHAOS_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 60.545)), module, HookeOsc::PITCH_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 78.835)), module, HookeOsc::KMOD_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 113.475)), module, HookeOsc::OUT_OUTPUT));
	}
};


Model* modelHookeOsc = createModel<HookeOsc, HookeOscWidget>("HookeOsc");
