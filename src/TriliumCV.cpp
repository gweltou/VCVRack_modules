#include "plugin.hpp"
#include "rs232.h"


struct TrillWidget : LedDisplay {
	LedDisplayChoice* portChoice;
	LedDisplaySeparator* portSeparator;
	LedDisplayChoice* deviceChoice;
	//LedDisplaySeparator* deviceSeparator;
	void setDevice(int portnum);
};



struct TrillPortItem : ui::MenuItem {
	//midi::Port* port;
	int driverId;
	void onAction(const event::Action& e) override {
	    printf("TrillPortItem onAction\n");
		//port->setDriverId(driverId);
	}
};

struct TrillPortChoice : LedDisplayChoice {
	//midi::Port* port;
	void onAction(const event::Action& e) override {
		//if (!port)
		//	return;
		
		//openTrillDevice(16);
		printf("TrillPortChoice onAction\n");

		ui::Menu* menu = createMenu();
		menu->addChild(createMenuLabel("Serial port"));
		/*
		for (int driverId : port->getDriverIds()) {
			TrillPortItem* item = new TrillPortItem;
			item->port = port;
			item->driverId = driverId;
			item->text = port->getDriverName(driverId);
			item->rightText = CHECKMARK(item->driverId == port->driverId);
			menu->addChild(item);
		}
		*/
	}
	void step() override {
		//text = port ? port->getDriverName(port->driverId) : "";
		if (text.empty()) {
			text = "(No driver)";
			color.a = 0.5f;
		}
		else {
			color.a = 1.f;
		}
	}
};


struct TrillDeviceItem : ui::MenuItem {
	//midi::Port* port;
	int deviceId;
	void onAction(const event::Action& e) override {
		//port->setDeviceId(deviceId);
	}
};

struct TrillDeviceChoice : LedDisplayChoice {
	//midi::Port* port;
	void onAction(const event::Action& e) override {
		//if (!port)
		//	return;
		printf("TrillDeviceChoice onAction\n");

		ui::Menu* menu = createMenu();
		menu->addChild(createMenuLabel("I2C adress"));
		{
			TrillDeviceItem* item = new TrillDeviceItem;
			//item->port = port;
			item->deviceId = -1;
			item->text = "(No device)";
			//item->rightText = CHECKMARK(item->deviceId == port->deviceId);
			menu->addChild(item);
		}
		/*
		for (int deviceId : port->getDeviceIds()) {
			TrillDeviceItem* item = new TrillDeviceItem;
			//item->port = port;
			item->deviceId = deviceId;
			item->text = port->getDeviceName(deviceId);
			item->rightText = CHECKMARK(item->deviceId == port->deviceId);
			menu->addChild(item);
		}
		*/
	}
	void step() override {
		text = "test"; // port ? port->getDeviceName(port->deviceId) : "";
		if (text.empty()) {
			text = "(No device)";
			color.a = 0.5f;
		}
		else {
			color.a = 1.f;
		}
	}
};


void TrillWidget::setDevice(int portnum) {
	clearChildren();

	math::Vec pos;

	TrillPortChoice* portChoice = createWidget<TrillPortChoice>(pos);
	portChoice->box.size.x = box.size.x;
	//portChoice->port = port;
	addChild(portChoice);
	pos = portChoice->box.getBottomLeft();
	this->portChoice = portChoice;

	this->portSeparator = createWidget<LedDisplaySeparator>(pos);
	this->portSeparator->box.size.x = box.size.x;
	addChild(this->portSeparator);

	TrillDeviceChoice* deviceChoice = createWidget<TrillDeviceChoice>(pos);
	deviceChoice->box.size.x = box.size.x;
	//deviceChoice->port = port;
	addChild(deviceChoice);
	pos = deviceChoice->box.getBottomLeft();
	this->deviceChoice = deviceChoice;

	//this->deviceSeparator = createWidget<LedDisplaySeparator>(pos);
	//this->deviceSeparator->box.size.x = box.size.x;
	//addChild(this->deviceSeparator);
}


struct TriliumCV : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		CV_OUTPUT,
		GATE_OUTPUT,
		VELOCITY_OUTPUT,
		AFTERTOUCH_OUTPUT,
		//PITCH_OUTPUT,
		MOD_OUTPUT,
		//RETRIGGER_OUTPUT,
		//CLOCK_OUTPUT,
		//CLOCK_DIV_OUTPUT,
		//START_OUTPUT,
		//STOP_OUTPUT,
		//CONTINUE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};
	
	int n,
	    writeOffset = 0,
	    portnum = 16; /* /dev/ttyUSB0  */
	bool portOpened = false;
	char buf[4096];
	char rawline[128];

	TriliumCV() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		printf("Trilium initiated\n");
		
		openTrillDevice(portnum); 
	}
	
	void openTrillDevice(int portnum) {
	    printf("trying to open Serial Device %i\n", portnum);
	    char mode[] = {'8','N','1',0};
	    int ret = RS232_OpenComport(portnum, 115200, mode, 0);
	    if (ret > 0) {
	        printf("Can not open comport\n");
	        return;
	    }
	    portOpened = true;
	}

	void process(const ProcessArgs& args) override {
	    if (portOpened) {
	        n = RS232_PollComport(portnum, (unsigned char *) buf + writeOffset, 4095 - writeOffset);
		    if (n > 0)
		    {
			    char *start = buf;
			    writeOffset += n;
			    buf[writeOffset] = '\0';

			    // Check if newline character is present in buffer
			    char *end = strchr(start, '\n');
			    while (end != NULL)
			    {
				    size_t count = end - start + 1;
				    strncpy(rawline, start, count);
				    rawline[count] = '\0'; // Replace '\n' with '\0'
				    parseRawline(rawline);
				    start = end + 1;
				    end = strchr(start, '\n');
				    writeOffset -= count;
			    }

			    if (writeOffset > 0 && start > buf)
			    {
				    // Shift the remaining data at the beginning of the buffer
				    strncpy(buf, start, writeOffset + 1); // Copy last '\0'
			    }
		    }
	    }
	}
	
	void parseRawline(char *data)
	{
		//printf("-- %s", rawline);
		// Read 2 first numbers
		char *token = strtok(data, " ");
		int v = atoi(token);
		token = strtok(NULL, " ");
		int h = atoi(token);

		if (v == 0 && h ==  0) {
			outputs[GATE_OUTPUT].setVoltage(0.f);
		} else {
		    int values[2*(v+h)];
		    for (int i = 0; i < 2 * (v + h); i++)
		    {
			    token = strtok(NULL, " ");
			    values[i] = atoi(token);
		    }
			outputs[GATE_OUTPUT].setVoltage(10.f);
			if (v+h >= 2) {
			    outputs[CV_OUTPUT].setVoltage((float) values[2] / 1792.f);
			    outputs[MOD_OUTPUT].setVoltage((float) values[0] / 179.2f);
			}
		}

		return;
	}
};


struct TriliumCVWidget : ModuleWidget {
	TriliumCVWidget(TriliumCV* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TriliumCV.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addOutput(createOutput<PJ301MPort>(mm2px(Vec(4.61505, 60.1445)), module, TriliumCV::CV_OUTPUT));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(16.214, 60.1445)), module, TriliumCV::GATE_OUTPUT));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(27.8143, 60.1445)), module, TriliumCV::VELOCITY_OUTPUT));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(4.61505, 76.1449)), module, TriliumCV::AFTERTOUCH_OUTPUT));
		//addOutput(createOutput<PJ301MPort>(mm2px(Vec(16.214, 76.1449)), module, TriliumCV::PITCH_OUTPUT));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(27.8143, 76.1449)), module, TriliumCV::MOD_OUTPUT));
		//addOutput(createOutput<PJ301MPort>(mm2px(Vec(4.61505, 92.1439)), module, TriliumCV::CLOCK_OUTPUT));
		//addOutput(createOutput<PJ301MPort>(mm2px(Vec(16.214, 92.1439)), module, TriliumCV::CLOCK_DIV_OUTPUT));
		//addOutput(createOutput<PJ301MPort>(mm2px(Vec(27.8143, 92.1439)), module, TriliumCV::RETRIGGER_OUTPUT));
		//addOutput(createOutput<PJ301MPort>(mm2px(Vec(4.61505, 108.144)), module, TriliumCV::START_OUTPUT));
		//addOutput(createOutput<PJ301MPort>(mm2px(Vec(16.214, 108.144)), module, TriliumCV::STOP_OUTPUT));
		//addOutput(createOutput<PJ301MPort>(mm2px(Vec(27.8143, 108.144)), module, TriliumCV::CONTINUE_OUTPUT));
		
		TrillWidget* trillWidget = createWidget<TrillWidget>(mm2px(Vec(3.41891, 14.8373)));
		trillWidget->box.size = mm2px(Vec(33.840, 28));
		trillWidget->setDevice(module ? module->portnum : 0);
		addChild(trillWidget);
	}
};


Model* modelTriliumCV = createModel<TriliumCV, TriliumCVWidget>("TriliumCV");
