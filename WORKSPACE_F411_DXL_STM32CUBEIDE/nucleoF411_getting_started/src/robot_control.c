#include "robot_control.h"
#include "stm32f4xx.h"
#include "stm32f4xx_nucleo.h"
#include "drv_uart.h"
#include "dynamixel.h"

RobotState robot_state = STOPPED;

void move_up(int velocity) {
    dxl_setGoalVelocity(1, -velocity);
    dxl_setGoalVelocity(2, velocity);
    robot_state = MOVING_FORWARD;
}

void move_down(int velocity) {
    dxl_setGoalVelocity(1, velocity);
    dxl_setGoalVelocity(2, -velocity);
    robot_state = MOVING_BACKWARD;
}

void move_left(int velocity) {
    dxl_setGoalVelocity(1, velocity);
    dxl_setGoalVelocity(2, velocity);
    robot_state = TURNING_LEFT;
}

void move_right(int velocity) {
    dxl_setGoalVelocity(1, -velocity);
    dxl_setGoalVelocity(2, -velocity);
    robot_state = TURNING_RIGHT;
}

void stop_robot(int velocity) {
    dxl_setGoalVelocity(1, velocity);
    dxl_setGoalVelocity(2, velocity);
    robot_state = STOPPED;
}

void detect_line() {
    // Read line sensor
    GPIO_PinState lineStateRight = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
    GPIO_PinState lineStateLeft = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);

    // Get the current velocities
    int speed1 = dxl_getPresentVelocity(1);
    int speed2 = dxl_getPresentVelocity(2);

    // Send velocity information to the client
    term_printf_zigbee("\r\nSPEEDS:%d,%d\n", speed1, speed2);

    // Check line sensor states and set robot movement accordingly
    if (lineStateLeft == GPIO_PIN_SET && lineStateRight == GPIO_PIN_SET) {
        move_up(50);//default value
    } else if (lineStateLeft == GPIO_PIN_SET && lineStateRight == GPIO_PIN_RESET) {
    	dxl_setGoalVelocity(1, 10);
        dxl_setGoalVelocity(2, 10);
        robot_state = TURNING_LEFT;
    } else if (lineStateRight == GPIO_PIN_SET && lineStateLeft == GPIO_PIN_RESET) {
        dxl_setGoalVelocity(1, -10);
        dxl_setGoalVelocity(2, -10);
        robot_state = TURNING_LEFT;
    } else if (lineStateRight == GPIO_PIN_RESET && lineStateLeft == GPIO_PIN_RESET) {
        stop_robot(0);
    }
}

