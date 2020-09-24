#include "plugin.hpp"


struct PSwitch : Module {
	enum ParamIds {
	    ENUMS(PROB_PARAM, 8),
		NUM_PARAMS
	};
	enum InputIds {
		TRIGGER_INPUT,
		ENUMS(IN_INPUT, 8),
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
	    ENUMS(SWITCH_LIGHT, 8),
		NUM_LIGHTS
	};
	
	dsp::SchmittTrigger trigTrigger;
	int open = 0;

	PSwitch() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int i=0; i<8; i++) {
		    configParam(PROB_PARAM + i, 0.f, 1.f, 0.5f, "");
		}
	}

	void process(const ProcessArgs& args) override {
	    if (trigTrigger.process(rescale(inputs[TRIGGER_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
	        // Turn led off
	        lights[SWITCH_LIGHT + open].setBrightness(0.f);
	    
	        float p[8] = {
			    params[PROB_PARAM].getValue(),
			    params[PROB_PARAM+1].getValue(),
			    params[PROB_PARAM+2].getValue(),
			    params[PROB_PARAM+3].getValue(),
			    params[PROB_PARAM+4].getValue(),
			    params[PROB_PARAM+5].getValue(),
			    params[PROB_PARAM+6].getValue()};
			float sum_p = p[0] + p[1] + p[2] + p[3] + p[4] + p[5] + p[6] + p[7];
            
			float r = random::uniform() * sum_p;
			float cumul = 0.f;
			for (int i=0; i<8; i++) {
			    if (r < p[i] + cumul) {
			        open = i;
			        break;
			    }
			    cumul += p[i];
			}
			
			// Turn led on
			lights[SWITCH_LIGHT + open].setBrightness(0.9f);
		}
		
		float out = inputs[IN_INPUT+open].getVoltage();
		
		outputs[OUT_OUTPUT].setVoltage(out);
	}
};


struct PSwitchWidget : ModuleWidget {
	PSwitchWidget(PSwitch* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/pSwitch.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(15.724, 38.305)), module, PSwitch::PROB_PARAM+0));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.487, 47.054)), module, PSwitch::PROB_PARAM+1));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(15.724, 55.803)), module, PSwitch::PROB_PARAM+2));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.487, 64.552)), module, PSwitch::PROB_PARAM+3));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(15.724, 73.301)), module, PSwitch::PROB_PARAM+4));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.487, 82.049)), module, PSwitch::PROB_PARAM+5));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(15.724, 90.798)), module, PSwitch::PROB_PARAM+6));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(20.487, 99.547)), module, PSwitch::PROB_PARAM+7));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 15.83)), module, PSwitch::TRIGGER_INPUT));
		
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.849, 38.305)), module, PSwitch::IN_INPUT+0));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.612, 47.054)), module, PSwitch::IN_INPUT+1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.849, 55.803)), module, PSwitch::IN_INPUT+2));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.612, 64.552)), module, PSwitch::IN_INPUT+3));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.849, 73.301)), module, PSwitch::IN_INPUT+4));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.612, 82.049)), module, PSwitch::IN_INPUT+5));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.849, 90.798)), module, PSwitch::IN_INPUT+6));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.612, 99.547)), module, PSwitch::IN_INPUT+7));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(12.7, 113.475)), module, PSwitch::OUT_OUTPUT));
		
		addChild(createLight<SmallLight<GreenLight>>(mm2px(Vec(20.4, 37.505)), module, PSwitch::SWITCH_LIGHT + 0));
		addChild(createLight<SmallLight<GreenLight>>(mm2px(Vec(3.8, 46.254)), module, PSwitch::SWITCH_LIGHT + 1));
		addChild(createLight<SmallLight<GreenLight>>(mm2px(Vec(20.4, 55.003)), module, PSwitch::SWITCH_LIGHT + 2));
		addChild(createLight<SmallLight<GreenLight>>(mm2px(Vec(3.8, 63.752)), module, PSwitch::SWITCH_LIGHT + 3));
		addChild(createLight<SmallLight<GreenLight>>(mm2px(Vec(20.4, 72.501)), module, PSwitch::SWITCH_LIGHT + 4));
		addChild(createLight<SmallLight<GreenLight>>(mm2px(Vec(3.8, 81.249)), module, PSwitch::SWITCH_LIGHT + 5));
		addChild(createLight<SmallLight<GreenLight>>(mm2px(Vec(20.4, 89.998)), module, PSwitch::SWITCH_LIGHT + 6));
		addChild(createLight<SmallLight<GreenLight>>(mm2px(Vec(3.8, 98.747)), module, PSwitch::SWITCH_LIGHT + 7));
	}
};


Model* modelPSwitch = createModel<PSwitch, PSwitchWidget>("PSwitch");
