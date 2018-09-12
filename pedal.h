#include <iostream>
#include <windows.h>
#include <cstdlib>
#include <Tchar.h>
#include <string>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include "allegro5/allegro.h"

bool detect = false;

ALLEGRO_JOYSTICK* joystick = nullptr;
int joystickId = -1;
int axis = -1;
DWORD vkCode = 0;
IAudioEndpointVolume *mikrofon;
bool toggleMute = false;
HHOOK kbHook = nullptr;

void changeStatus(bool status);
void registerKbHook();
void mute(bool mute);

void init(){
	al_init();

	al_install_joystick();


	CoInitialize(NULL);
    IMMDeviceEnumerator *deviceEnumerator = NULL;
    CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
    IMMDevice *defaultDevice = NULL;

    deviceEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &defaultDevice);
    deviceEnumerator->Release();
    deviceEnumerator = NULL;

    //IAudioEndpointVolume *endpointVolume = NULL;
    defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&mikrofon);
    defaultDevice->Release();


	ALLEGRO_CONFIG* conf = al_load_config_file("config.ini");
	joystickId = std::stoi(al_get_config_value(conf, NULL, "gamepad"));
	axis = std::stoi(al_get_config_value(conf, NULL, "axis"));
	vkCode = std::stoi(al_get_config_value(conf, NULL, "vkCode"));
	toggleMute = std::stoi(al_get_config_value(conf, NULL, "toggleMute"));
	al_destroy_config(conf);

	if(!toggleMute)
		mute(true);

	registerKbHook();
}

bool toggleToggleMute(){
	toggleMute = !toggleMute;
	ALLEGRO_CONFIG* conf = al_load_config_file("config.ini");
	al_set_config_value(conf, NULL, "toggleMute", std::to_string(toggleMute).c_str());
	al_save_config_file("config.ini", conf);
	al_destroy_config(conf);
	return toggleMute;
}

void mute(bool mute) {
	if(toggleMute){
		WINBOOL muteState;
		mikrofon->GetMute(&muteState);
		mute = !muteState;
		printf("toggle\n");
	}
	mikrofon->SetMute(mute, nullptr);
	changeStatus(!mute);
}


LRESULT __stdcall keyboardCallback(int code, WPARAM wParam, LPARAM lParam){
	printf("callback\n");
	static KBDLLHOOKSTRUCT kbStruct;

	kbStruct = *((KBDLLHOOKSTRUCT*)lParam);

	if(wParam == WM_KEYDOWN){
		if(detect){
			detect = false;
			axis = -1;
			vkCode = kbStruct.vkCode;
			ALLEGRO_CONFIG* conf = al_load_config_file("config.ini");
			al_set_config_value(conf, NULL, "vkCode", std::to_string((int)vkCode).c_str());
			al_set_config_value(conf, NULL, "axis", std::to_string(axis).c_str());
			al_save_config_file("config.ini", conf);
			al_destroy_config(conf);
			printf(std::to_string((int)vkCode).c_str());
		} else {
			if(vkCode == kbStruct.vkCode){
				mute(false);
			}
		}
	} else if(wParam == WM_KEYUP){
		if(vkCode == kbStruct.vkCode){
			if(!toggleMute)
				mute(true);
		}
	}
	return CallNextHookEx(kbHook, code, wParam, lParam);
}

void registerKbHook(){
	if (!(kbHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardCallback, NULL, 0)))    {
		printf("callback register fail\n");
        MessageBox(NULL, "Failed to register keyboardhook", "Error", MB_ICONERROR);
    } else {
		printf("callback registered\n");
    }
}

void pedal(){
	joystick = al_get_joystick(joystickId);

	printf("Pocet gamepadu: ");
	printf(std::to_string(al_get_num_joysticks()).c_str());
	printf("\n");
	std::cout << "Pocet gamepadu: " << al_get_num_joysticks() << std::endl;

	ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
	al_register_event_source(queue, al_get_joystick_event_source());

	ALLEGRO_EVENT event;
	bool ignoreInput = false;
	while(true){
		al_wait_for_event(queue, &event);

		switch(event.type){
			case ALLEGRO_EVENT_JOYSTICK_CONFIGURATION: {
				al_reconfigure_joysticks();
				if(joystick == nullptr){
					joystick = al_get_joystick(joystickId);
				}
			} break;
			case ALLEGRO_EVENT_JOYSTICK_AXIS: {
				if(detect){
					detect = false;

					for(int i = 0; i < al_get_num_joysticks(); i++){
						if(event.joystick.id == al_get_joystick(i)){
							joystickId = 0;
							joystick = al_get_joystick(i);
							break;
						}
					}
					axis = event.joystick.axis;
					vkCode = 0;
					ALLEGRO_CONFIG* conf = al_load_config_file("config.ini");
					al_set_config_value(conf, NULL, "gamepad", std::to_string(joystickId).c_str());
					al_set_config_value(conf, NULL, "axis", std::to_string(axis).c_str());
					al_set_config_value(conf, NULL, "vkCode", std::to_string((int)vkCode).c_str());
					al_save_config_file("config.ini", conf);
					al_destroy_config(conf);
				} else if(event.joystick.id == joystick && event.joystick.axis == axis){
					if(event.joystick.pos != 0){
						if(ignoreInput)
							break;
						mute(false);
						if(toggleMute){
							ignoreInput = true;
						}
					} else {
						if(toggleMute){
							ignoreInput = false;
						} else {
							mute(true);
						}
					}
				}
			} break;
		}
	}
}

void uninit(){
	mikrofon->Release();

    CoUninitialize();
}
