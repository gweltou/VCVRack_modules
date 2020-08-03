#include "plugin.hpp"


struct Gaussian : Module {
	enum ParamIds {
		MU_PARAM,
		SIGMA_PARAM,
		OFFSET_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		TRIGGER_INPUT,
		SIGMACV_INPUT,
		MUCV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

    dsp::SchmittTrigger trigTrigger;
	//float lastValue = 0.f;
	float value = 0.f;
	float mu;
	float sigma;
	
	Gaussian() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(MU_PARAM, -1.f, 1.f, 0.f, "Mu");
		configParam(SIGMA_PARAM, 0.f, 1.f, 0.1f, "Sigma");
		configParam(OFFSET_PARAM, 0.f, 1.f, 0.f, "Offset");
	}

	void process(const ProcessArgs& args) override {
	    if (inputs[TRIGGER_INPUT].isConnected()) {
	        // Trigger input is connected
			if (trigTrigger.process(rescale(inputs[TRIGGER_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
				sample();
			}
		} else {
		    // Trigger input is not connected, generate noise
		    if (outputs[CV_OUTPUT].isConnected()) {
		        sample();
		    }
		}
		
		outputs[CV_OUTPUT].setVoltage(value * 10.f);
	}
	
	void sample() {
	    sigma = params[SIGMA_PARAM].getValue();
	    if (inputs[SIGMACV_INPUT].isConnected()) {
            sigma += inputs[SIGMACV_INPUT].getVoltage(0) / 10.f;
            sigma = clamp(sigma, 0.f, 1.f);
        }
        
        mu = params[MU_PARAM].getValue();
        if (inputs[MUCV_INPUT].isConnected()) {
            mu += inputs[MUCV_INPUT].getVoltage(0) / 10.f;
            mu = clamp(mu, -1.f, 1.f);
        }
	    
	    if (params[OFFSET_PARAM].getValue() == 0.f) {
            // Bipolar
            value = random::normal() * sigma;
            value = mu + clamp(value, -0.5f, 0.5f);
        } else {
            // Unipolar
            value = std::fabs(random::normal()) * sigma;
            value = mu + clamp(value, 0.f, 1.f);
        }
	}
};


struct GaussianWidget : ModuleWidget {
	GaussianWidget(Gaussian* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Gaussian.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        
        addParam(createParam<CKSS>(mm2px(Vec(5.2, 13.4)), module, Gaussian::OFFSET_PARAM));
        
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 36.293)), module, Gaussian::SIGMA_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 69.803)), module, Gaussian::MU_PARAM));
		
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 52.64)), module, Gaussian::SIGMACV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 86.28)), module, Gaussian::MUCV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 99.174)), module, Gaussian::TRIGGER_INPUT));
		
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 113.475)), module, Gaussian::CV_OUTPUT));
	}
};


Model* modelGaussian = createModel<Gaussian, GaussianWidget>("Gaussian");
