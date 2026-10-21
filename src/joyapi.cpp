#include "SDL.h"
#include "i_video.h"
#include "joyapi.h"
#include "ptrapi.h"
#include "kbdapi.h"
#include "input.h"
#include <string.h>

int joy_num_pads;

int joy_ack;

int joyinput[MAX_CONTROLLERS][16];
int joyinputmap[MAX_CONTROLLERS][4];

SDL_GameController* ControllerHandles[MAX_CONTROLLERS];

int MaxJoysticks;
int ControllerIndex;
int JoystickIndex;

static unsigned int lastTime = 0;

/***************************************************************************
IPT_CalJoy() - Open Gamecontroller
 ***************************************************************************/
void 
IPT_CalJoy(
	void
)
{
	if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0)
		return;

    IPT_CloJoy(0);
    MaxJoysticks = SDL_NumJoysticks();
    for (int device = 0; device < MaxJoysticks; device++)
    {
        if (!SDL_IsGameController(device))
            continue;
        SDL_JoystickID instance = SDL_JoystickGetDeviceInstanceID(device);
        int slot = -1, known = 0;
        for (int i = 0; i < MAX_CONTROLLERS; i++)
        {
            if (!ControllerHandles[i])
            {
                if (slot == -1)
                    slot = i;
            }
            else if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(ControllerHandles[i])) == instance)
                known = 1;
        }
        // Keep surviving players' slots and avoid reopening existing handles.
        if (!known && slot != -1)
        {
            ControllerHandles[slot] = SDL_GameControllerOpen(device);
            if (ControllerHandles[slot])
                joy_num_pads++;
        }
    }
}

int IPT_JoyPadAttached(int slot)
{
    return slot >= 0 && slot < MAX_CONTROLLERS && ControllerHandles[slot]
        && SDL_GameControllerGetAttached(ControllerHandles[slot]);
}

/***************************************************************************
IPT_CloJoy() - Close detached controllers without moving surviving slots
 ***************************************************************************/
void IPT_CloJoy(int closeall)
{
    joy_num_pads = 0;
    for (int i = 0; i < MAX_CONTROLLERS; i++)
    {
        if (ControllerHandles[i] && (closeall || !IPT_JoyPadAttached(i)))
        {
            SDL_GameControllerClose(ControllerHandles[i]);
            ControllerHandles[i] = NULL;
        }
        if (IPT_JoyPadAttached(i))
            joy_num_pads++;
        else
        {
            memset(joyinput[i], 0, sizeof(joyinput[i]));
            memset(joyinputmap[i], 0, sizeof(joyinputmap[i]));
        }
    }
    if (!joy_num_pads)
        joy_ack = 0;
}

/***************************************************************************
IPT_ConvertAxisValue() - Convert raw values from axis to usable input values
 ***************************************************************************/
static int
IPT_ConvertAxisValue(
	int AxisValue,        //Raw value from axis
	int RangeDivison      //Defines axis value range division
)
{
	int deadzone = 8000;
	int result;

	if (AxisValue == 0 ||
		(AxisValue > 0 && AxisValue < deadzone) ||
		(AxisValue < 0 && AxisValue > -deadzone))
		return 0;

	if (AxisValue > 0)
		result = (AxisValue - deadzone) * RangeDivison / (32767 - deadzone);
	else
		result = (AxisValue + deadzone) * RangeDivison / (32768 - deadzone);

	return result;
}

/***************************************************************************
I_HandleJoystickEvent() - Get current button or axis status
 ***************************************************************************/
void 
I_HandleJoystickEvent(
	SDL_Event *sdlevent
)
{
	joy_num_pads = 0;
	for (ControllerIndex = 0;
		ControllerIndex < MAX_CONTROLLERS;
		++ControllerIndex)
	{
		if (ControllerHandles[ControllerIndex] != 0 && SDL_GameControllerGetAttached(ControllerHandles[ControllerIndex]))
		{
			joy_num_pads++;
			joyinput[ControllerIndex][JOYUP] = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_UP);
			joyinput[ControllerIndex][JOYDOWN] = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_DOWN);
			joyinput[ControllerIndex][JOYLEFT] = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_LEFT);
			joyinput[ControllerIndex][JOYRIGHT] = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
			joyinput[ControllerIndex][JOYSTART] = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_START);
			joyinput[ControllerIndex][JOYBACK] = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_BACK);
			joyinput[ControllerIndex][JOYLEFTSHOULDER] = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
			joyinput[ControllerIndex][JOYRIGHTSHOULDER] = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
			joyinput[ControllerIndex][JOYA] = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_A);
			joyinput[ControllerIndex][JOYB] = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_B);
			joyinput[ControllerIndex][JOYX] = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_X);
			joyinput[ControllerIndex][JOYY] = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_Y);

			joyinputmap[ControllerIndex][FIRE] = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], (SDL_GameControllerButton)j_lookup[0]);
			joyinputmap[ControllerIndex][CHWEAPON] = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], (SDL_GameControllerButton)j_lookup[1]);
			joyinputmap[ControllerIndex][MEGABOMB] = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], (SDL_GameControllerButton)j_lookup[2]);
			joyinputmap[ControllerIndex][MEGAFIRE] = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], (SDL_GameControllerButton)j_lookup[3]);

			if (!g_drawcursor)
			{
				joyinput[ControllerIndex][JOYSTICKX] = IPT_ConvertAxisValue(SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_LEFTX), 10);
				joyinput[ControllerIndex][JOYSTICKY] = IPT_ConvertAxisValue(SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_LEFTY), 8);
			}
			else
			{
				joyinput[ControllerIndex][JOYSTICKX] = IPT_ConvertAxisValue(SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_LEFTX), 4);
				joyinput[ControllerIndex][JOYSTICKY] = IPT_ConvertAxisValue(SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_LEFTY), 4);
			}
			
			joyinput[ControllerIndex][JOYTRIGGERLEFT] = IPT_ConvertAxisValue(SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_TRIGGERLEFT), 4);
			joyinput[ControllerIndex][JOYTRIGGERRIGHT] = IPT_ConvertAxisValue(SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_TRIGGERRIGHT), 4);
		}
		
		if (sdlevent->type == SDL_CONTROLLERBUTTONUP) 
			joy_ack = 0;
		
		if (sdlevent->type == SDL_CONTROLLERBUTTONDOWN) 
			joy_ack = 1;
	}
}

/***************************************************************************
IPT_CalJoyRumbleLow() - Gamecontroller rumbles low
 ***************************************************************************/
void 
IPT_CalJoyRumbleLow(
	void
)
{
	for (ControllerIndex = 0;
		ControllerIndex < MAX_CONTROLLERS;
		++ControllerIndex)
	{
		if (ControllerHandles[ControllerIndex])
			SDL_GameControllerRumble(ControllerHandles[ControllerIndex], 0x3fff, 0x3fff, 1000);
	}
}

/***************************************************************************
IPT_CalJoyRumbleMedium() - Gamecontroller rumbles medium
 ***************************************************************************/
void 
IPT_CalJoyRumbleMedium(
	void
)
{
	for (ControllerIndex = 0;
		ControllerIndex < MAX_CONTROLLERS;
		++ControllerIndex)
	{
		if (ControllerHandles[ControllerIndex])
		    SDL_GameControllerRumble(ControllerHandles[ControllerIndex], 0x7ffe, 0x7ffe, 1000);
	}
}

/***************************************************************************
IPT_CalJoyRumbleHigh() - Gamecontroller rumbles high
 ***************************************************************************/
void 
IPT_CalJoyRumbleHigh(
	void
)
{
	for (ControllerIndex = 0;
		ControllerIndex < MAX_CONTROLLERS;
		++ControllerIndex)
	{
		if (ControllerHandles[ControllerIndex])
			SDL_GameControllerRumble(ControllerHandles[ControllerIndex], 0xbffd, 0xbffd, 1000);
	}
}

/***************************************************************************
JOY_Wait() - Waits for button to be released
 ***************************************************************************/
void 
JOY_Wait(
	int index,
	int button
)
{
	while (joyinput[index][button])
	{
		I_GetEvent();
	}
}

/***************************************************************************
JOY_IsKey() - Tests to see if button is down if so waits for release
 ***************************************************************************/
int 
JOY_IsKey(
	int button
)
{
	for (ControllerIndex = 0;
		ControllerIndex < MAX_CONTROLLERS;
		++ControllerIndex)
	{
		if (joyinput[ControllerIndex][button])
		{
			JOY_Wait(ControllerIndex, button);
			return 1;
		}
	}
	
	return 0;
}

/***************************************************************************
JOY_MapsInput() - Maps input to key
 ***************************************************************************/
int
JOY_MapsInput(
	void
)
{
	for (ControllerIndex = 0;
		ControllerIndex < MAX_CONTROLLERS;
		++ControllerIndex)
	{
		if (joyinput[ControllerIndex][JOYSTICKX] || joyinput[ControllerIndex][JOYSTICKY] ||
			joyinput[ControllerIndex][JOYUP] || joyinput[ControllerIndex][JOYDOWN] ||
			joyinput[ControllerIndex][JOYLEFT] || joyinput[ControllerIndex][JOYRIGHT] ||
			joyinput[ControllerIndex][JOYSTART] || joyinput[ControllerIndex][JOYBACK] ||
			joyinput[ControllerIndex][JOYA] || joyinput[ControllerIndex][JOYB] ||
			joyinput[ControllerIndex][JOYX] || joyinput[ControllerIndex][JOYY] ||
			joyinput[ControllerIndex][JOYRIGHTSHOULDER] || joyinput[ControllerIndex][JOYLEFTSHOULDER])
		{
			unsigned int currentTime;
			currentTime = SDL_GetTicks();

			if (currentTime > lastTime + 200)
			{
				lastTime = currentTime;

				if (joyinput[ControllerIndex][JOYSTICKX] > 0 ||
					joyinput[ControllerIndex][JOYRIGHT])
					return SC_RIGHT;
				if (joyinput[ControllerIndex][JOYSTICKX] < 0 ||
					joyinput[ControllerIndex][JOYLEFT])
					return SC_LEFT;
				if (joyinput[ControllerIndex][JOYSTICKY] > 0 ||
					joyinput[ControllerIndex][JOYDOWN])
					return SC_DOWN;
				if (joyinput[ControllerIndex][JOYSTICKY] < 0 ||
					joyinput[ControllerIndex][JOYUP])
					return SC_UP;
				if (joyinput[ControllerIndex][JOYA] ||
					joyinput[ControllerIndex][JOYSTART])
					return SC_ENTER;
				if (joyinput[ControllerIndex][JOYB] ||
					joyinput[ControllerIndex][JOYBACK])
					return SC_ESC;
				if (joyinput[ControllerIndex][JOYX])
					return SC_DELETE;
				if (joyinput[ControllerIndex][JOYLEFTSHOULDER])
					return SC_CTRL;
				if (joyinput[ControllerIndex][JOYRIGHTSHOULDER])
					return SC_F1;
			}
			return 0;
		}
	}
	return 0;
}

/***************************************************************************
JOY_GetInput() - Get input from joystick
 ***************************************************************************/
int
JOY_GetInput(
	void
)
{
	for (ControllerIndex = 0;
		ControllerIndex < MAX_CONTROLLERS;
		++ControllerIndex)
	{
		if (joyinput[ControllerIndex][JOYUP] || joyinput[ControllerIndex][JOYDOWN] ||
			joyinput[ControllerIndex][JOYLEFT] || joyinput[ControllerIndex][JOYRIGHT] ||
			joyinput[ControllerIndex][JOYSTART] || joyinput[ControllerIndex][JOYBACK] ||
			joyinput[ControllerIndex][JOYA] || joyinput[ControllerIndex][JOYB] ||
			joyinput[ControllerIndex][JOYX] || joyinput[ControllerIndex][JOYY] ||
			joyinput[ControllerIndex][JOYRIGHTSHOULDER] || joyinput[ControllerIndex][JOYLEFTSHOULDER])
		{
			return 1;
		}
	}
	return 0;
}

/***************************************************************************
JOY_GetMappedButton() - Get mapped button status from joystick
 ***************************************************************************/
int
JOY_GetMappedButton(
	int button
)
{
	for (ControllerIndex = 0;
		ControllerIndex < MAX_CONTROLLERS;
		++ControllerIndex)
	{
		if (joyinputmap[ControllerIndex][button])
			return 1;
	}
	return 0;
}

/***************************************************************************
JOY_GetButton() - Get button status from joystick
 ***************************************************************************/
int
JOY_GetButton(
	int button
)
{
	for (ControllerIndex = 0;
		ControllerIndex < MAX_CONTROLLERS;
		++ControllerIndex)
	{
		if (joyinput[ControllerIndex][button])
			return 1;
	}
	return 0;
}

/***************************************************************************
JOY_GetAxis() - Get axis status from joystick
 ***************************************************************************/
int
JOY_GetAxis(
	int axis
)
{
	for (ControllerIndex = 0;
		ControllerIndex < MAX_CONTROLLERS;
		++ControllerIndex)
	{
		if (joyinput[ControllerIndex][axis])
			return joyinput[ControllerIndex][axis];
	}
	return 0;
}

/* Read a single controller without combining P1 and P2 inputs. */
void IPT_ReadJoyPad(int pad, JoyPadState *out)
{
    if (!out)
        return;
    memset(out, 0, sizeof(*out));
    if (pad < 0 || pad >= MAX_CONTROLLERS)
        return;
    SDL_GameController *controller = ControllerHandles[pad];
    if (!controller || !SDL_GameControllerGetAttached(controller))
        return;
    out->Up = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
    out->Down = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    out->Left = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    out->Right = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    out->Start = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START);
    out->Back = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK);
    out->LeftShoulder = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    out->RightShoulder = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    out->AButton = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A);
    out->BButton = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B);
    out->XButton = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X);
    out->YButton = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_Y);
    for (int i = 0; i < 4; ++i)
        out->buttons[i] = SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)j_lookup[i]);
    out->StickX = IPT_ConvertAxisValue(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX), 4);
    out->StickY = IPT_ConvertAxisValue(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY), 4);
    out->TriggerLeft = IPT_ConvertAxisValue(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT), 4);
    out->TriggerRight = IPT_ConvertAxisValue(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT), 4);
}
