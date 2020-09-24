#include "plugin.hpp"

#define RAISING 1
#define FALLING 0

static const int maxPolyphony = engine::PORT_MAX_CHANNELS;


struct HookeOsc : Module {
	enum ParamIds {
		FREQ_PARAM,
		KCVMOD_PARAM,
		CHAOS_PARAM,
		CHAOSFREQ_PARAM,
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
	float timeCounter = 0.f;
	
	bool applyChaos = false;
	
	int state[maxPolyphony] = {};
	float spring_k[maxPolyphony] = {};
	float spring_kp[maxPolyphony] = {0.f};
	float vel[maxPolyphony] = {};
	float value[maxPolyphony] = {};
	float prev_value[maxPolyphony] = {};
	
	HookeOsc() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(SLOW_PARAM, 0.f, 1.f, 0.f, "Slow mode");
		configParam(FREQ_PARAM, -54.f, 54.f, 0.f, "Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4, 0.f);
		configParam(KCVMOD_PARAM, -1.f, 1.f, 0.f, "K/m modulation");
		configParam(CHAOS_PARAM, 0.f, 1.f, 0.f, "Chaos amount");
		configParam(CHAOSFREQ_PARAM, 0.1f, 1.f, 0.1f, "Chaos frequency");
		
		// Initialize springs extension
		for (int i=0; i<maxPolyphony; i++) {
		    value[i] = 1.f;
		    prev_value[i] = 1.f;
		    state[i] = FALLING;
		}
	}
	
	void process(const ProcessArgs& args) override {
	    timeCounter += args.sampleTime;
	    
	    if (loopCounter-- == 0) {
            loopCounter = 3;
            processEvery4Samples(args);
        }
        
	    generateOutput();
	}
	
	void processEvery4Samples(const ProcessArgs& args) {
	    currentPolyphony = std::max(1, inputs[PITCH_INPUT].getChannels());
        outputs[OUT_OUTPUT].setChannels(currentPolyphony);
        
	    float pitchParam = params[FREQ_PARAM].getValue() / 12.f;
	    
	    float slow = params[SLOW_PARAM].getValue();
	    
	    // CHAOS
	    float chaos = params[CHAOS_PARAM].getValue() * 10.f;
	    float chaosFreq = params[CHAOSFREQ_PARAM].getValue() * 0.1f;
	    
	    
	    for (int i=0; i<currentPolyphony; i++) {
	        
	        float pitch = pitchParam + inputs[PITCH_INPUT].getVoltage(i);
	        float freq = dsp::FREQ_C4 * std::pow(2.f, pitch);
	        
	        
	        // 6.194130435 is the magic number to tune the oscillator
	        if (slow == 1.f) {
	            spring_k[i] = freq * args.sampleTime * 0.02173913f;
	        } else {
	            spring_k[i] = freq * args.sampleTime * 6.194130435f;
	        }
	        
	        spring_kp[i] = 0.0f;
	        if ((state[i] == RAISING) && (value[i] < prev_value[i])) {
	            // Switch state and apply chaos
	            state[i] = FALLING;
	            if (random::uniform() < chaosFreq)
	                spring_kp[i] = spring_k[i] * (random::normal()-0.44f) * chaos * 0.7f;
	            
	        } else if ((state[i] == FALLING) && (value[i] > prev_value[i])) {
	            // Switch state and apply chaos
	            state[i] = RAISING;
	            if (random::uniform() < chaosFreq)
	                spring_kp[i] = spring_k[i] * (random::normal()-0.44f) * chaos * 0.7f;
	        }
	        //spring_k[i] += spring_k[i] * chaos;
	        
	        //spring_k[i] += k_mod;
	        prev_value[i] = value[i];
        }
	}
	
	void generateOutput() {
	    float k;
	    float k_mod = 0.f;
	    int kmodPolyphony = inputs[KMOD_INPUT].getChannels();
	    
	    for (int i=0; i<currentPolyphony; i++) {
	        if (kmodPolyphony > 0) {
	            k_mod = params[KCVMOD_PARAM].getValue() * inputs[KMOD_INPUT].getVoltage(i % kmodPolyphony) * 0.1f;
	        }
	        
	        k = spring_k[i] + k_mod + spring_kp[i];
            vel[i] += -k*k * value[i];
            
            value[i] += vel[i];
            
            if (value[i] < -1.f) {
              value[i] = -0.99f;
              vel[i] = k*k;
            } else if (value[i] > 1.f) {
              value[i] = 0.99f;
              vel[i] = -k*k;
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
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        
        addParam(createParam<CKSS>(mm2px(Vec(9.8, 14.7)), module, HookeOsc::SLOW_PARAM));
        
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(12.7, 42.05)), module, HookeOsc::FREQ_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(18.608, 77.487)), module, HookeOsc::KCVMOD_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(12.7, 96.548)), module, HookeOsc::CHAOS_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(19, 105)), module, HookeOsc::CHAOSFREQ_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 57.089)), module, HookeOsc::PITCH_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.149, 77.487)), module, HookeOsc::KMOD_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(12.7, 112.417)), module, HookeOsc::OUT_OUTPUT));
	}
};


Model* modelHookeOsc = createModel<HookeOsc, HookeOscWidget>("HookeOsc");
