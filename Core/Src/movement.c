/*
 * movement.c - Motor control and movement functions - FIXED VERSION
 *
 * Implements precise movement using DRV8833 motor driver and encoders
 * FIXED: Proper encoder overflow handling and safe movement functions
 * FIXED: Removed unused variables
 */

#include "micromouse.h"
<<<<<<< Updated upstream
#include "velocity_profile.h"
=======
#include "movement.h"
#include "logging_tests.h"
>>>>>>> Stashed changes
#include <stdlib.h> // for abs() function

// FIXED: Add static variables for proper encoder overflow tracking
static uint16_t last_left_count = 32768;
static uint16_t last_right_count = 32768;
static int32_t left_total = 0;
static int32_t right_total = 0;

<<<<<<< Updated upstream
//extern uint16_t current_left_raw;
volatile uint16_t current_left_raw;
=======

static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}


static inline int clampi(int v, int lo, int hi){ return v < lo ? lo : (v > hi ? hi : v); }


>>>>>>> Stashed changes
/**
 * @brief Update encoder totals with proper overflow handling - NEW FUNCTION
 */
void update_encoder_totals(void)
{
    uint16_t current_left_raw = __HAL_TIM_GET_COUNTER(&htim2);
    uint16_t current_right_raw = __HAL_TIM_GET_COUNTER(&htim4);

    // Calculate differences accounting for 16-bit overflow
    int16_t left_diff = current_left_raw - last_left_count;
    int16_t right_diff = current_right_raw - last_right_count;

    // FIXED: Invert left encoder to match right encoder direction
    left_diff = -left_diff;  // Make left encoder positive for forward movement

    // Update totals
    left_total += left_diff;
    right_total += right_diff;

    // Update last counts
    last_left_count = current_left_raw;
    last_right_count = current_right_raw;
}

/**
 * @brief Get safe left encoder total - NEW FUNCTION
 */
int32_t get_left_encoder_total(void) {
    update_encoder_totals();
    return left_total;
}

/**
 * @brief Get safe right encoder total - NEW FUNCTION
 */
int32_t get_right_encoder_total(void) {
    update_encoder_totals();
    return right_total;
}

/**
 * @brief Reset encoder totals - NEW FUNCTION
 */
void reset_encoder_totals(void) {
    left_total = 0;
    right_total = 0;
    last_left_count = __HAL_TIM_GET_COUNTER(&htim2);
    last_right_count = __HAL_TIM_GET_COUNTER(&htim4);
}

/**
 * @brief Start encoder timers - FIXED VERSION
 */
void start_encoders(void) {
    HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL); // Right encoder
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL); // Left encoder

    // Reset encoder counts
    __HAL_TIM_SET_COUNTER(&htim4, 32768);
    __HAL_TIM_SET_COUNTER(&htim2, 32768);

    HAL_Delay(1);
    // FIXED: Initialize our safe tracking variables
    last_left_count = 32768;
    last_right_count = 32768;
    left_total = 0;
    right_total = 0;
    encoders.left_total = 0;
    encoders.right_total = 0;
}

/**
 * @brief Move forward one cell - FIXED VERSION
 */
void move_forward(void)
{
    // Use safe encoder reading
    int32_t start_left = get_left_encoder_total();
    int32_t start_right = get_right_encoder_total();

    // Check bounds before moving
    int new_x = robot.x + dx[robot.direction];
    int new_y = robot.y + dy[robot.direction];
    if (new_x < 0 || new_x >= MAZE_SIZE || new_y < 0 || new_y >= MAZE_SIZE) {
        send_bluetooth_message("Cannot move - would go out of bounds!\r\n");
        return;
    }

    motor_set_fixed(0, true, 800);  // Left motor forward
    motor_set_fixed(1, true, 800);  // Right motor forward

    // Move until target distance reached
    int32_t target_counts = ENCODER_COUNTS_PER_CELL;
    while (1) {
        int32_t current_left = get_left_encoder_total();
        int32_t current_right = get_right_encoder_total();
        int32_t left_traveled = current_left - start_left;
        int32_t right_traveled = current_right - start_right;
        int32_t avg_traveled = (left_traveled + right_traveled) / 2;

        if (avg_traveled >= target_counts) {
            break;
        }
        HAL_Delay(1);
    }

    // Stop motors
    stop_motors();

    // Update position only after successful movement
    robot.x = new_x;
    robot.y = new_y;
    HAL_Delay(100); // Settling time
}



/**
 * @brief Turn left 90 degrees - FIXED VERSION (removed unused variables)
 */
void turn_left(void) {
    // REMOVED: unused variable 'start_left'
    int32_t start_right = get_right_encoder_total();


    // Left motor reverse, right motor forward
	motor_set_fixed(0, false, 800); // Left reverse
	motor_set_fixed(1, true, 800);  // Right forward

    int32_t target_counts = ENCODER_COUNTS_PER_TURN;
    while (1) {
        int32_t current_right = get_right_encoder_total();
        int32_t right_traveled = abs(current_right - start_right);

        if (right_traveled >= target_counts) {
            break;
        }
        HAL_Delay(1);
    }

    stop_motors();
    robot.direction = (robot.direction + 3) % 4; // Turn left
    HAL_Delay(200);
}

/**
 * @brief Turn right 90 degrees - FIXED VERSION (removed unused variables)
 */
void turn_right(void) {
    int32_t start_left = get_left_encoder_total();
    // REMOVED: unused variable 'start_right'

    // Left motor forward, right motor backward
    motor_set_fixed(0, true, 800);  // Left forward
    motor_set_fixed(1, false, 800); // Right reverse

    int32_t target_counts = ENCODER_COUNTS_PER_TURN;
    while (1) {
        int32_t current_left = get_left_encoder_total();
        int32_t left_traveled = abs(current_left - start_left);

        if (left_traveled >= target_counts) {
            break;
        }
        HAL_Delay(1);
    }

    stop_motors();
    robot.direction = (robot.direction + 1) % 4; // Turn right
    HAL_Delay(200);
}

/**
 * @brief Turn around 180 degrees
 */
void turn_around(void) {
    turn_right();
    turn_right();
}

/**
 * @brief Stop both motors
 */
void stop_motors(void)
{
    // Stop all PWM channels
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);  // Left motor PWM = 0
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);  // Left motor direction = 0
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);  // Right motor PWM = 0
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);  // Right motor direction = 0
}


/**
 * @brief Move forward a specific distance - FIXED VERSION
 */
void move_forward_distance(int distance_mm) {
    int32_t target_counts = (distance_mm * ENCODER_COUNTS_PER_CELL) / CELL_SIZE_MM;

    // FIXED: Use safe encoder reading
    int32_t start_left = get_left_encoder_total();
    int32_t start_right = get_right_encoder_total();

    // Set motors to move forward
    HAL_GPIO_WritePin(MOTOR_IN1_GPIO_Port, MOTOR_IN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_IN2_GPIO_Port, MOTOR_IN2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_IN3_GPIO_Port, MOTOR_IN3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_IN4_GPIO_Port, MOTOR_IN4_Pin, GPIO_PIN_RESET);

    while (1) {
        int32_t current_left = get_left_encoder_total();
        int32_t current_right = get_right_encoder_total();
        int32_t left_traveled = current_left - start_left;
        int32_t right_traveled = current_right - start_right;
        int32_t avg_traveled = (left_traveled + right_traveled) / 2;

        if (avg_traveled >= target_counts) {
            break;
        }
        HAL_Delay(1);
    }

    stop_motors();
}

void move_forward_adaptive_speed(float speed_multiplier) {
    // Simple implementation - modify movement timing
    int original_delay = 1;
    int new_delay = (int)(original_delay / speed_multiplier);
    if (new_delay < 1) new_delay = 1;

    // Use existing move_forward but modify timing
    move_forward();
}

bool is_speed_run_ready(void) {
    return (robot.center_reached && robot.returned_to_start);
}

// helper to set speed (0–1000 = 0–100%)
void motor_set(uint16_t ch_pwm, GPIO_TypeDef *dirPort, uint16_t dirPin, bool forward, uint16_t duty) {
    // Validate inputs
    if (duty > 1000) duty = 1000;

    // Set PWM duty cycle
    __HAL_TIM_SET_COMPARE(&htim3, ch_pwm, duty);

    // FIXED: Proper DRV8833 control
    // For DRV8833: PWM on INx, Direction control on INy
    // Forward: INx=PWM, INy=LOW
    // Backward: INx=PWM, INy=HIGH
    if (forward) {
        HAL_GPIO_WritePin(dirPort, dirPin, GPIO_PIN_RESET);  // Direction LOW for forward
    } else {
        HAL_GPIO_WritePin(dirPort, dirPin, GPIO_PIN_SET);    // Direction HIGH for backward
    }
}
// Fixed motor_set function for DRV8833
void motor_set_fixed(uint8_t motor, bool forward, uint16_t duty) {
    if (motor == 0) { // Left motor
        if (forward) {
            // Left forward: IN1=PWM, IN2=LOW
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty); // PA6 = PWM
            HAL_GPIO_WritePin(MOTOR_IN2_GPIO_Port, MOTOR_IN2_Pin, GPIO_PIN_RESET); // PA7 = LOW
        } else {
            // Left reverse: IN1=LOW, IN2=PWM
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0); // PA6 = LOW
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, duty); // PA7 = PWM
        }
    } else { // Right motor
    	bool actual_forward = !forward;  // invert direction
        if (actual_forward) {
            // Right forward: IN3=PWM, IN4=LOW
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, duty); // PB0 = PWM
            HAL_GPIO_WritePin(MOTOR_IN4_GPIO_Port, MOTOR_IN4_Pin, GPIO_PIN_RESET); // PB1 = LOW
        } else {
            // Right reverse: IN3=LOW, IN4=PWM
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0); // PB0 = LOW
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, duty); // PB1 = PWM
        }
    }
}

// Add this function to test motors individually
void test_motors_individual(void) {
    send_bluetooth_message("Testing motors individually...\r\n");

    // Test left motor forward
    send_bluetooth_message("Left motor forward...\r\n");
    motor_set_fixed(0, true, 600);
    HAL_Delay(2000);
    stop_motors();
    HAL_Delay(500);

    // Test left motor reverse
    send_bluetooth_message("Left motor reverse...\r\n");
    motor_set_fixed(0, false, 600);
    HAL_Delay(2000);
    stop_motors();
    HAL_Delay(500);

    // Test right motor forward
    send_bluetooth_message("Right motor forward...\r\n");
    motor_set_fixed(1, true, 600);
    HAL_Delay(2000);
    stop_motors();
    HAL_Delay(500);

    // Test right motor reverse
    send_bluetooth_message("Right motor reverse...\r\n");
    motor_set_fixed(1, false, 600);
    HAL_Delay(2000);
    stop_motors();
    HAL_Delay(1000);

    send_bluetooth_message("Motor test complete!\r\n");
}



/**
 * @brief Get encoder status for debugging - NEW FUNCTION
 */
void send_encoder_status(void) {
    update_encoder_totals();
    send_bluetooth_printf("Encoders - Left:%ld Right:%ld Raw_L:%d Raw_R:%d\r\n",
                         left_total, right_total,
                         __HAL_TIM_GET_COUNTER(&htim2), __HAL_TIM_GET_COUNTER(&htim4));
}

/**
 * @brief Move forward with S-curve velocity profile
 */
void move_forward_with_profile(float distance_mm, float max_speed) {
    VelocityProfile profile;
    velocity_profile_init(&profile, distance_mm, max_speed);

    int32_t start_left = get_left_encoder_total();
    int32_t start_right = get_right_encoder_total();

    // Calculate target encoder counts
    int32_t target_counts = (distance_mm * ENCODER_COUNTS_PER_CELL) / CELL_SIZE_MM;

    while (!velocity_profile_is_complete(&profile)) {
        velocity_profile_update(&profile);
        float target_vel = velocity_profile_get_target_velocity(&profile);

        // Convert mm/s to PWM duty (simple linear conversion)
        uint16_t duty = (uint16_t)(target_vel * 0.8f); // Scale factor to be tuned
        if (duty > 1000) duty = 1000; // Cap at max PWM
        if (duty < 50) duty = 50; // Minimum PWM for movement

        // Check distance traveled
        int32_t current_left = get_left_encoder_total();
        int32_t current_right = get_right_encoder_total();
        int32_t avg_traveled = ((current_left - start_left) + (current_right - start_right)) / 2;

        if (avg_traveled >= target_counts) {
            break; // Reached target distance
        }

        // Apply to motors
        motor_set(TIM_CHANNEL_1, MOTOR_IN2_GPIO_Port, MOTOR_IN2_Pin, true, duty);
        motor_set(TIM_CHANNEL_3, MOTOR_IN4_GPIO_Port, MOTOR_IN4_Pin, true, duty);

        HAL_Delay(5); // 200Hz update rate
    }

    stop_motors();
}

<<<<<<< Updated upstream
/**
 * @brief Simple smooth movement for one cell
 */
void move_forward_smooth(float distance_mm) {
    move_forward_with_profile(distance_mm, 600.0f); // 600 mm/s max speed
}

=======
//// ================== WALL FOLLOW PID (ADD) Rivindu===================
//
//// ---------- Tunables ----------
//static int   WF_BASE_PWM        = 500;     // cruise PWM
//static int   WF_PWM_MIN_MOVE    = 50;      // overcome stiction
//static int   WF_PWM_MAX         = 1000;    // clamp
//
////Kp=0.003115  Ki=0.001479  Kd=0.270515
//static float WF_KP = 1.0f;
//static float WF_KI = 0.0f;   // integral uses e_int += e * dt  (dt in seconds)
//static float WF_KD = 0.0f;   // derivative uses d = Δe / dt
//static float WF_DERIV_ALPHA     = 0.85f;   // derivative low-pass (0..1)
//static float WF_INT_LIMIT       = 250.0f;  // anti-windup clamp
//static float WF_SINGLE_ALPHA    = 0.03f;   // EMA for single-wall target tracking
//static float WF_BOTH_SCALE      = 1.0f;   // overall aggressiveness when both walls seen
//static float WF_U_SCALE      = 100.0f;
//
//// Front-wall behaviour
//static bool  WF_BRAKE_ON_FRONT  = true;
//static int   WF_SLOW_PWM        = 380;     // slow when front wall seen
//static uint8_t WF_FRONT_HOLD_MS = 120;     // brief brake pulse before stop (if you enable braking)
//
//
//// ---------- Internal state ----------
//typedef enum { WF_AUTO=0, WF_LEFT, WF_RIGHT } wf_mode_t;
//static wf_mode_t wf_mode = WF_AUTO;
//
//static float e_int = 0.0f, e_prev = 0.0f, d_filt = 0.0f;
//static uint32_t wf_last_ms = 0;
//
//static float target_left  = 23.0f;  // learned sensor targets for single-wall
//static float target_right = 25.0f;
//
//static inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
//static inline float clampf(float v, float lo, float hi){ return v < lo ? lo : (v > hi ? hi : v); }
//
//// Expose simple C API (so main.c can call without needing the enum)
//void wall_follow_reset_int(int mode, int base_pwm);   // forward decl
//void wall_follow_step(void);                          // forward decl
//
//// Call once before starting wall-follow
//void wall_follow_reset_int(int mode, int base_pwm)
//{
//    wf_mode = (mode == 1) ? WF_LEFT : (mode == 2) ? WF_RIGHT : WF_AUTO;
//    WF_BASE_PWM = base_pwm;
//
//    e_int = 0.0f; e_prev = 0.0f; d_filt = 0.0f;
//    wf_last_ms = HAL_GetTick();
//    update_sensors();
//
//    // bootstrap targets from current readings (prevents initial jump)
//    //target_left  = (float)sensors.side_left;
//    //target_right = (float)sensors.side_right;
//}
//
//// One control step; call at ~200–500 Hz inside your loop
//void wall_follow_step(void)
//{
//    // Get fresh sensors (uses your emitter-sync diff scheme)
//    update_sensors();  // reads FL/FR/SL/SR and sets wall flags
//
//    // dt
//    uint32_t now = HAL_GetTick();
//    float dt = (now - wf_last_ms) / 1000.0f;
//    if (dt <= 0.0f) dt = 0.001f;
//    wf_last_ms = now;
//
//    // Determine mode automatically if requested
//    bool Lw = sensors.wall_left;
//    bool Rw = sensors.wall_right;
//    bool Fw = sensors.wall_front;
//
//    if (wf_mode == WF_AUTO) {
//        if (Lw && Rw)       wf_mode = WF_AUTO;   // center using both
//        else if (Lw)        wf_mode = WF_LEFT;
//        else if (Rw)        wf_mode = WF_RIGHT;
//        else                wf_mode = WF_AUTO;   // nothing: just go straight
//    }
//
//    // Log-ratio error; positive => closer to LEFT (so slow left / speed right)
//    // Add +1.0f to avoid log(0). Use both-wall centering if available, else single-wall track.
//    float e = 0.0f;
//
//    if (Lw && Rw) {
//        float L = (float)sensors.side_left;
//        float R = (float)sensors.side_right;
//        e = WF_BOTH_SCALE * (logf(L + 1.0f) - logf(R + 1.0f));
//        // keep single-wall targets gently aligned to present gap
//        //target_left  = (1.0f - WF_SINGLE_ALPHA)*target_left  + WF_SINGLE_ALPHA*L;
//        //target_right = (1.0f - WF_SINGLE_ALPHA)*target_right + WF_SINGLE_ALPHA*R;
//
//    } else if (Lw) {
//        float L = (float)sensors.side_left;
//        //arget_left  = (1.0f - WF_SINGLE_ALPHA)*target_left  + WF_SINGLE_ALPHA*L;
//        e = logf(L + 1.0f) - logf(target_left + 1.0f);
//
//    } else if (Rw) {
//        float R = (float)sensors.side_right;
//        //target_right = (1.0f - WF_SINGLE_ALPHA)*target_right + WF_SINGLE_ALPHA*R;
//        e = logf(target_right + 1.0f) - logf(R + 1.0f);
//
//    } else {
//        // No side walls -> no correction (let heading/gyro PID handle straightness if you run it)
//        e = 0.0f;
//    }
//
//    // PID on error
//    e_int += e * dt;
//    e_int  = clampf(e_int, -WF_INT_LIMIT, WF_INT_LIMIT);
//
//    float d_raw = (e - e_prev) / dt;
//    d_filt = WF_DERIV_ALPHA * d_filt + (1.0f - WF_DERIV_ALPHA) * d_raw;
//
//    float u_norm = WF_KP*e + WF_KI*e_int + WF_KD*d_filt;  // u > 0 => speed up right / slow left
//    float u = u_norm * WF_U_SCALE;
//    e_prev = e;
//
//    // Front wall policy
//    int base = WF_BASE_PWM;
//    if (Fw && WF_BRAKE_ON_FRONT) {
//        base = WF_SLOW_PWM;
//        // If you want a hard stop, uncomment:
//        // motor_set(0, true, 0); motor_set(1, true, 0); HAL_Delay(WF_FRONT_HOLD_MS); return;
//    }
//
//    // Map correction to wheel PWMs (right = base+u, left = base-u)
//    int pwm_right = clampi((int)lroundf((float)base - u), 0, WF_PWM_MAX);
//    int pwm_left  = clampi((int)lroundf((float)base + u), 0, WF_PWM_MAX);
//
//    if (pwm_right > 0 && pwm_right < WF_PWM_MIN_MOVE) pwm_right = WF_PWM_MIN_MOVE;
//    if (pwm_left  > 0 && pwm_left  < WF_PWM_MIN_MOVE) pwm_left  = WF_PWM_MIN_MOVE;
//
//    // Apply (both forward)
//    motor_set(0, true, (uint16_t)pwm_left);   // Left
//    motor_set(1, true, (uint16_t)pwm_right);  // Right
//
////    send_bluetooth_printf("# L:%d R:%d e=%.3f u_norm=%.3f u=%.1f\n",
////        sensors.side_left, sensors.side_right, e, u_norm, u);
//
//}
//




// ================== WALL FOLLOW PID with LUT ===================

// ---------- Tunables ----------
static int   WF_BASE_PWM        = 500;     // cruise PWM
static int   WF_PWM_MIN_MOVE    = 50;      // minimum PWM to overcome stiction
static int   WF_PWM_MAX         = 1000;    // clamp maximum PWM

// PID gains (your tuned values)
static float WF_KP = 0.05f;
static float WF_KI = 0.0f;
static float WF_KD = 0.0f;

static float WF_DERIV_ALPHA     = 0.35f;   // derivative low-pass filter (0..1)
static float WF_INT_LIMIT       = 250.0f;  // anti-windup clamp
static float WF_SINGLE_ALPHA    = 0.03f;   // smoothing for single-wall tracking
static float WF_BOTH_SCALE      = 1.0f;    // aggressiveness when both walls seen
static float WF_U_SCALE         = 100.0f;  // scale PID output to PWM units

// Front-wall behaviour
static bool  WF_BRAKE_ON_FRONT  = true;
static int   WF_SLOW_PWM        = 380;     // slow down when front wall detected
static uint8_t WF_FRONT_HOLD_MS = 120;     // brake pulse before stop

// ---------- Lookup Tables ----------
// Right sensor LUT
#define R_LUT_SIZE 33
static const int   right_adc[R_LUT_SIZE]   = {43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11};
static const float right_dist[R_LUT_SIZE]  = {1.17,1.23,1.29,1.36,1.43,1.51,1.59,1.67,1.76,1.85,1.95,2.05,2.16,2.28,2.40,2.53,2.67,2.82,2.98,3.16,3.34,3.55,3.76,4.00,4.26,4.54,4.85,5.19,5.57,5.98,6.45,6.96};

// Left sensor LUT
#define L_LUT_SIZE 32
static const int   left_adc[L_LUT_SIZE]    = {42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11};
static const float left_dist[L_LUT_SIZE]   = {1.17f,1.23f,1.29f,1.36f,1.43f,1.51f,1.59f,1.67f,1.76f,1.85f,1.95f,2.05f,2.16f,2.28f,2.40f,2.53f,2.67f,2.82f,2.98f,3.16f,3.34f,3.55f,3.76f,4.00f,4.26f,4.54f,4.85f,5.19f,5.57f,5.98f,6.45f,6.96f};

// ---------- Helper: Linear interpolation lookup ----------
static float lut_lookup(int raw, const int *adc_table, const float *dist_table, int size)
{
    // Clamp below/above table
    if (raw >= adc_table[0]) return dist_table[0];
    if (raw <= adc_table[size-1]) return dist_table[size-1];

    // Find interval [i, i+1] where raw fits
    for (int i=0; i<size-1; i++) {
        if (raw <= adc_table[i] && raw >= adc_table[i+1]) {
            float t = (float)(raw - adc_table[i+1]) / (float)(adc_table[i] - adc_table[i+1]);
            return dist_table[i+1] + t * (dist_table[i] - dist_table[i+1]);
        }
    }
    return (float)raw; // fallback (should not happen)
}

// ---------- Internal state ----------
typedef enum { WF_AUTO=0, WF_LEFT, WF_RIGHT } wf_mode_t;
static wf_mode_t wf_mode = WF_AUTO;

static float e_int = 0.0f, e_prev = 0.0f, d_filt = 0.0f;
static uint32_t wf_last_ms = 0;

static float target_left  = 5.0f;   // desired left wall distance (cm)
static float target_right = 5.0f;   // desired right wall distance (cm)

// ---------- Main wall-follow step ----------
void wall_follow_step(void)
{
    // Update sensors (fills sensors.side_left, sensors.side_right, wall flags)
    update_sensors();

    // Compute dt
    uint32_t now = HAL_GetTick();
    float dt = (now - wf_last_ms) / 1000.0f;
    if (dt <= 0.0f) dt = 0.001f;
    wf_last_ms = now;

    // Wall presence flags
    bool Lw = sensors.wall_left;
    bool Rw = sensors.wall_right;
    bool Fw = sensors.wall_front;

    // Auto mode selection
    if (wf_mode == WF_AUTO) {
        if (Lw && Rw)       wf_mode = WF_AUTO;   // both walls → center
        else if (Lw)        wf_mode = WF_LEFT;   // left wall only
        else if (Rw)        wf_mode = WF_RIGHT;  // right wall only
        else                wf_mode = WF_AUTO;   // no walls → straight
    }

    // --- Error calculation ---
    float e = 0.0f;

    if (Lw && Rw) {
        // Both walls: balance distances
        float L = lut_lookup(sensors.side_left,  left_adc,  left_dist,  L_LUT_SIZE);
        float R = lut_lookup(sensors.side_right, right_adc, right_dist, R_LUT_SIZE);
        e = WF_BOTH_SCALE * (L - R);

    } else if (Lw) {
        // Left wall only: hold target distance
        float L = lut_lookup(sensors.side_left, left_adc, left_dist, L_LUT_SIZE);
        e = target_left - L;

    } else if (Rw) {
        // Right wall only: hold target distance
        float R = lut_lookup(sensors.side_right, right_adc, right_dist, R_LUT_SIZE);
        e = R - target_right;

    } else {
        // No walls: no correction
        e = 0.0f;
    }

    // --- PID controller ---
    e_int += e * dt;
    e_int  = clampf(e_int, -WF_INT_LIMIT, WF_INT_LIMIT);

    float d_raw = (e - e_prev) / dt;
    d_filt = WF_DERIV_ALPHA * d_filt + (1.0f - WF_DERIV_ALPHA) * d_raw;

    float u_norm = WF_KP*e + WF_KI*e_int + WF_KD*d_filt;
    float u = u_norm * WF_U_SCALE;
    e_prev = e;

    // --- Base speed adjustment (front wall handling) ---
    int base = WF_BASE_PWM;
    if (Fw && WF_BRAKE_ON_FRONT) {
        base = WF_SLOW_PWM;
    }

    // --- Apply correction to motors ---
    int pwm_right = clampi((int)lroundf((float)base - u), 0, WF_PWM_MAX);
    int pwm_left  = clampi((int)lroundf((float)base + u), 0, WF_PWM_MAX);

    // Ensure motors overcome stiction
    if (pwm_right > 0 && pwm_right < WF_PWM_MIN_MOVE) pwm_right = WF_PWM_MIN_MOVE;
    if (pwm_left  > 0 && pwm_left  < WF_PWM_MIN_MOVE) pwm_left  = WF_PWM_MIN_MOVE;

    // Send to motors
    motor_set(0, true, (uint16_t)pwm_left);   // Left motor
    motor_set(1, true, (uint16_t)pwm_right);  // Right motor
}




/*
 * // after init
 *
fusion_reset();
fusion_set_heading_ref_to_current();  // optional on straight
while (1) {
    fusion_step();  // ~200–400 Hz
}
*
 * */


// =================== SINGLE STRAIGHT CONTROLLER (FUSION) ===================
// Uses your existing wall PID state/gains (WF_*) and your gyro PID gains (Kp_g/Ki_g/Kd_g)

static float fus_theta = 0.0f;          // integrated heading (deg)
static float fus_theta_ref = 0.0f;      // heading lock for current straight
static float fus_conf_s = 0.0f;         // smoothed wall confidence
static uint32_t fus_last_ms = 0;
static float fus_u_prev = 0.0f;         // rate limit state

// small heading-PID locals (assist only)
static float h_int = 0.0f, h_prev = 0.0f, h_df = 0.0f;

// knobs (not “tuning” — just safety rails)
static const float FUS_CONF_EMA        = 0.90f;  // confidence smoothing
static const float FUS_HEAD_CAP_FRAC   = 0.25f;  // max heading authority (fraction of base)
static const float FUS_U_RATE_LIM      = 120.0f; // max |Δu| per step (PWM units)

extern float Kp_g, Ki_g, Kd_g;                 // your gyro PID gains

void fusion_reset(void)
{
    // reset wall PID memory (reuse your existing state)
    e_int = 0.0f; e_prev = 0.0f; d_filt = 0.0f;
    wf_last_ms = HAL_GetTick();

    // reset fusion/heading memory
    fus_theta = 0.0f;
    fus_theta_ref = 0.0f;
    fus_conf_s = 0.0f;
    fus_u_prev = 0.0f;
    h_int = 0.0f; h_prev = 0.0f; h_df = 0.0f;
    fus_last_ms = HAL_GetTick();

    // capture initial targets to avoid a jump at start
    // for the Target, values should be updated -------------------->
    update_sensors();
    //target_left  = (float)sensors.side_left;
    //target_right = (float)sensors.side_right;
}

void fusion_set_heading_ref_to_current(void)
{
    fus_theta_ref = fus_theta;
}

// Call at ~200–500 Hz. Pass 0 to use WF_BASE_PWM.
void fusion_step(int base_pwm)
{
    // --- timing ---
    uint32_t now = HAL_GetTick();
    float dt = (now - fus_last_ms) * 0.001f;
    if (dt <= 0.0f) dt = 0.001f;
    fus_last_ms = now;

    // --- sensors + gyro ---
    update_sensors();
    bool Lw = sensors.wall_left;
    bool Rw = sensors.wall_right;
    bool Fw = sensors.wall_front;

    int  L = sensors.side_left;
    int  R = sensors.side_right;

    mpu9250_read_gyro();
    float gz = mpu9250_get_gyro_z_compensated();   // deg/s
    fus_theta += gz * dt;

    // -------- WALL PID (log-ratio + your WF_* state) --------
    float e_wall = 0.0f;
    if (Lw && Rw) {
        e_wall = WF_BOTH_SCALE * (logf((float)L + 1.0f) - logf((float)R + 1.0f));
        // keep single-wall targets gently aligned (same as wall_follow_step)
        //target_left  = (1.0f - WF_SINGLE_ALPHA)*target_left  + WF_SINGLE_ALPHA*(float)L;
        //target_right = (1.0f - WF_SINGLE_ALPHA)*target_right + WF_SINGLE_ALPHA*(float)R;
    } else if (Lw) {
        //target_left  = (1.0f - WF_SINGLE_ALPHA)*target_left  + WF_SINGLE_ALPHA*(float)L;
        e_wall = logf((float)L + 1.0f) - logf(target_left + 1.0f);
    } else if (Rw) {
        //target_right = (1.0f - WF_SINGLE_ALPHA)*target_right + WF_SINGLE_ALPHA*(float)R;
        e_wall = logf(target_right + 1.0f) - logf((float)R + 1.0f);
    } else {
        e_wall = 0.0f;
    }

    // step the SAME wall PID states/gains
    e_int += e_wall * dt;
    e_int  = clampf(e_int, -WF_INT_LIMIT, WF_INT_LIMIT);
    float d_raw = (e_wall - e_prev) / dt;
    d_filt = WF_DERIV_ALPHA * d_filt + (1.0f - WF_DERIV_ALPHA) * d_raw;
    float u_wall = WF_KP*e_wall + WF_KI*e_int + WF_KD*d_filt;
    e_prev = e_wall;

    // -------- HEADING PID (assist; reuses your gyro PID gains) --------
    float e_head = fus_theta_ref - fus_theta;

    // allow heading integrator only when walls are NOT present
    if (!(Lw || Rw)) {
        h_int += e_head * dt;
        if (h_int > 200.0f) h_int = 200.0f;
        if (h_int < -200.0f) h_int = -200.0f;
    }

    const float H_ALPHA = 0.90f;                  // small derivative filter
    float h_draw = (e_head - h_prev) / dt;
    h_df = H_ALPHA*h_df + (1.0f - H_ALPHA)*h_draw;
    h_prev = e_head;

    float u_head = Kp_g*e_head + Ki_g*h_int + Kd_g*h_df;

    // limit heading authority so it never fights good wall info
    int base_unclamped = (base_pwm > 0) ? base_pwm : WF_BASE_PWM;
    float head_cap = FUS_HEAD_CAP_FRAC * (float)base_unclamped;   // e.g., 25% of base
    if (u_head >  head_cap) u_head =  head_cap;
    if (u_head < -head_cap) u_head = -head_cap;

    // -------- BLEND (confidence from side walls) --------
    float conf = 0.0f; if (Lw) conf += 0.5f; if (Rw) conf += 0.5f;
    fus_conf_s = FUS_CONF_EMA*fus_conf_s + (1.0f - FUS_CONF_EMA)*conf;  // smooth handoffs
    float u = fus_conf_s*u_wall + (1.0f - fus_conf_s)*u_head;

    // -------- OUTPUT SHAPING --------
    // optional rate limit on correction to avoid jerk
    float du = u - fus_u_prev;
    if (du >  FUS_U_RATE_LIM) du =  FUS_U_RATE_LIM;
    if (du < -FUS_U_RATE_LIM) du = -FUS_U_RATE_LIM;
    u = fus_u_prev + du;
    fus_u_prev = u;

    int base = (base_pwm > 0) ? base_pwm : WF_BASE_PWM;
    if (Fw && WF_BRAKE_ON_FRONT) base = WF_SLOW_PWM;

    // right = base + u, left = base - u
    int pwm_right = clampi((int)lroundf((float)base + u), 0, WF_PWM_MAX);
    int pwm_left  = clampi((int)lroundf((float)base - u), 0, WF_PWM_MAX);

    if (pwm_right > 0 && pwm_right < WF_PWM_MIN_MOVE) pwm_right = WF_PWM_MIN_MOVE;
    if (pwm_left  > 0 && pwm_left  < WF_PWM_MIN_MOVE) pwm_left  = WF_PWM_MIN_MOVE;

    motor_set(0, true, (uint16_t)pwm_left);
    motor_set(1, true, (uint16_t)pwm_right);
}






// Enhanced wall following with sensor fusion


// Wall following PID parameters
//static float Kp_wall = 0.25f;   // Proportional gain for wall following
//static float Ki_wall = 0.03f;   // Integral gain for wall following
//static float Kd_wall = 0.01f;   // Derivative gain for wall following
//
//// Wall following state variables
//static float wall_error_prev = 0.0f;
//static float wall_integral = 0.0f;
//static float wall_deriv_filt = 0.0f;
//static uint32_t wall_last_ms = 0;
//
//// Wall following configuration constants
//static const float WALL_TARGET_DISTANCE = 1800.0f;  // Target sensor reading for single wall
//static const float WALL_INTEGRAL_LIMIT = 800.0f;    // Anti-windup limit
//static const float WALL_DERIV_FILTER_ALPHA = 0.75f; // Derivative filter
//static const float GYRO_WALL_BLEND_RATIO = 0.65f;   // 65% gyro, 35% wall correction
//
///**
// * @brief Reset wall following PID state
// * Call this before starting a new wall following movement
// */
//void wallFollowPID_Reset(void) {
//    wall_error_prev = 0.0f;
//    wall_integral = 0.0f;
//    wall_deriv_filt = 0.0f;
//    wall_last_ms = HAL_GetTick();
//
//    // Also reset gyro PID
//    moveStraightGyroPID_Reset();
//}
//
///**
// * @brief Enhanced movement with sensor fusion: gyro + wall following
// *
// * @param base_pwm Base PWM for forward movement (recommended: 600-700)
// * @param wall_mode Wall following mode (NONE, LEFT, RIGHT, BOTH)
// *
// * Usage examples:
// * - moveStraightSensorFusion(650, WALL_FOLLOW_BOTH);   // Center between walls
// * - moveStraightSensorFusion(600, WALL_FOLLOW_LEFT);   // Follow left wall
// * - moveStraightSensorFusion(650, WALL_FOLLOW_NONE);   // Gyro only
// */
//void moveStraightSensorFusion(int base_pwm, WallFollowMode_t wall_mode) {
//    // Always update sensors and gyro
//    update_sensors();
//    mpu9250_read_gyro();
//
//    // Calculate timing for wall PID
//    uint32_t now = HAL_GetTick();
//    float dt = (now - wall_last_ms) / 1000.0f;
//    if (dt <= 0.0f) dt = 0.001f;
//    wall_last_ms = now;
//
//    // --- GYRO CORRECTION (Heading Stability) ---
//    float gyro_error = mpu9250_get_gyro_z_compensated();
//
//    // Use existing gyro PID gains and state
//    extern float Kp_g, Ki_g, Kd_g;
//    static float gyro_integral_local = 0.0f;
//    static float gyro_prev_error = 0.0f;
//
//    gyro_integral_local += gyro_error * dt;
//    gyro_integral_local = fmaxf(-2000.0f, fminf(2000.0f, gyro_integral_local)); // Anti-windup
//
//    float gyro_derivative = (gyro_error - gyro_prev_error) / dt;
//    float gyro_correction = (Kp_g * gyro_error) + (Ki_g * gyro_integral_local) + (Kd_g * gyro_derivative);
//    gyro_prev_error = gyro_error;
//
//    // --- WALL FOLLOWING CORRECTION (Lateral Position) ---
//    float wall_correction = 0.0f;
//    bool wall_active = false;
//
//    if (wall_mode != WALL_FOLLOW_NONE) {
//        float wall_error = 0.0f;
//
//        switch (wall_mode) {
//            case WALL_FOLLOW_LEFT:
//                if (sensors.wall_left) {
//                    // Error: positive = too close to wall, negative = too far
//                    wall_error = (float)sensors.side_left - WALL_TARGET_DISTANCE;
//                    wall_error = -wall_error * 0.5f; // Invert and reduce gain for single wall
//                    wall_active = true;
//                }
//                break;
//
//            case WALL_FOLLOW_RIGHT:
//                if (sensors.wall_right) {
//                    // Error: positive = too close to wall, negative = too far
//                    wall_error = (float)sensors.side_right - WALL_TARGET_DISTANCE;
//                    wall_error = wall_error * 0.5f; // Reduce gain for single wall
//                    wall_active = true;
//                }
//                break;
//
//            case WALL_FOLLOW_BOTH:
//                if (sensors.wall_left && sensors.wall_right) {
//                    // Centering error: positive = closer to right, negative = closer to left
//                    wall_error = (float)sensors.side_right - (float)sensors.side_left;
//                    wall_active = true;
//                } else if (sensors.wall_left && !sensors.wall_right) {
//                    // Only left wall - maintain distance
//                    wall_error = (float)sensors.side_left - WALL_TARGET_DISTANCE;
//                    wall_error = -wall_error * 0.3f; // Reduced gain for single wall mode
//                    wall_active = true;
//                } else if (sensors.wall_right && !sensors.wall_left) {
//                    // Only right wall - maintain distance
//                    wall_error = (float)sensors.side_right - WALL_TARGET_DISTANCE;
//                    wall_error = wall_error * 0.3f; // Reduced gain for single wall mode
//                    wall_active = true;
//                }
//                break;
//
//            default:
//                break;
//        }
//
//        // Calculate wall PID correction if wall following is active
//        if (wall_active) {
//            wall_integral += wall_error * dt;
//            wall_integral = fmaxf(-WALL_INTEGRAL_LIMIT, fminf(WALL_INTEGRAL_LIMIT, wall_integral));
//
//            float wall_deriv_raw = (wall_error - wall_error_prev) / dt;
//            wall_deriv_filt = WALL_DERIV_FILTER_ALPHA * wall_deriv_filt +
//                             (1.0f - WALL_DERIV_FILTER_ALPHA) * wall_deriv_raw;
//
//            wall_correction = (Kp_wall * wall_error) +
//                             (Ki_wall * wall_integral) +
//                             (Kd_wall * wall_deriv_filt);
//
//            wall_error_prev = wall_error;
//        }
//    }
//
//    // --- SENSOR FUSION: COMBINE CORRECTIONS ---
//    float total_correction;
//
//    if (wall_active) {
//        // Blend gyro and wall corrections
//        total_correction = (GYRO_WALL_BLEND_RATIO * gyro_correction) +
//                          ((1.0f - GYRO_WALL_BLEND_RATIO) * wall_correction);
//    } else {
//        // Use gyro only
//        total_correction = gyro_correction;
//    }
//
//    // --- APPLY TO MOTORS ---
//    int motor_left_speed = (int)roundf((float)base_pwm + total_correction);
//    int motor_right_speed = (int)roundf((float)base_pwm - total_correction);
//
//    // Clamp motor speeds
//    motor_left_speed = fmaxf(0, fminf(800, motor_left_speed));
//    motor_right_speed = fmaxf(0, fminf(800, motor_right_speed));
//
//    // Apply to motors
//    motor_set(0, true, (uint16_t)motor_left_speed);   // Left motor
//    motor_set(1, true, (uint16_t)motor_right_speed);  // Right motor
//
//    // Optional debug output (comment out in final version)
////    #ifdef WALL_FOLLOW_DEBUG
////    static uint32_t debug_last = 0;
////    if (now - debug_last > 50) {  // Debug every 50ms
////        send_bluetooth_printf("WF: SL=%d SR=%d GC=%.1f WC=%.1f TC=%.1f M_L=%d M_R=%d\r\n",
////                             sensors.side_left, sensors.side_right,
////                             gyro_correction, wall_correction, total_correction,
////                             motor_left_speed, motor_right_speed);
////        debug_last = now;
////    }
////    #endif
//}
//
///**
// * @brief Move forward distance with sensor fusion
// * Enhanced version that replaces move_forward_distance()
// *
// * @param target_counts Encoder counts to travel
// * @param wall_mode Wall following mode
// */
//void move_forward_distance_fusion(int target_counts, WallFollowMode_t wall_mode) {
//    // Reset encoders and PID controllers
//    reset_encoder_totals();
//    wallFollowPID_Reset();
//
//    int32_t start_left = get_left_encoder_total();
//    int32_t start_right = get_right_encoder_total();
//
//    const int base_pwm = 650; // Adjust as needed
//
//    while (1) {
//        // Use sensor fusion controller
//        moveStraightSensorFusion(base_pwm, wall_mode);
//
//        // Check distance traveled
//        int32_t current_left = get_left_encoder_total();
//        int32_t current_right = get_right_encoder_total();
//        int32_t left_traveled = current_left - start_left;
//        int32_t right_traveled = current_right - start_right;
//        int32_t avg_traveled = (left_traveled + right_traveled) / 2;
//
//        if (avg_traveled >= target_counts) {
//            break;
//        }
//
//        HAL_Delay(2); // 500Hz control loop
//    }
//
//    break_motors();
//    HAL_Delay(50); // Brief settling time
//}



>>>>>>> Stashed changes
// Add to movement.c
void debug_encoder_setup(void) {
    send_bluetooth_message("=== ENCODER DEBUG ===\r\n");

    // Check if timer clocks are enabled
    if (RCC->APB1ENR & RCC_APB1ENR_TIM2EN) {
        send_bluetooth_message("TIM2 clock: ENABLED\r\n");
    } else {
        send_bluetooth_message("TIM2 clock: DISABLED\r\n");
    }

    if (RCC->APB1ENR & RCC_APB1ENR_TIM4EN) {
        send_bluetooth_message("TIM4 clock: ENABLED\r\n");
    } else {
        send_bluetooth_message("TIM4 clock: DISABLED\r\n");
    }

    // Check timer register values
    send_bluetooth_printf("TIM2 registers - CR1:0x%08X SMCR:0x%08X\r\n",
                         TIM2->CR1, TIM2->SMCR);
    send_bluetooth_printf("TIM4 registers - CR1:0x%08X SMCR:0x%08X\r\n",
                         TIM4->CR1, TIM4->SMCR);
}

void test_encoder_manual(void) {
    send_bluetooth_message("Testing manual encoder increment...\r\n");

    // Manual test - set counter values
    TIM2->CNT = 100;
    TIM4->CNT = 200;
    HAL_Delay(10);

    send_bluetooth_printf("TIM2 CNT: %d, TIM4 CNT: %d\r\n",
                         TIM2->CNT, TIM4->CNT);
}

// Add to main.c after your current debug code
void test_encoder_rotation(void) {
    send_bluetooth_message("\r\n🔄 ENCODER ROTATION TEST\r\n");
    send_bluetooth_message("Manually rotate each wheel and watch the counts:\r\n");

    // Reset counters to known values
    TIM2->CNT = 1000; // Left encoder
    TIM4->CNT = 2000; // Right encoder

    send_bluetooth_message("Initial values set - TIM2: 1000, TIM4: 2000\r\n");
    send_bluetooth_message("Now manually rotate the wheels...\r\n");

    // Monitor for 10 seconds
    for(int i = 0; i < 20; i++) {
        uint16_t left_raw = TIM2->CNT;
        uint16_t right_raw = TIM4->CNT;
        int32_t left_total = get_left_encoder_total();
        int32_t right_total = get_right_encoder_total();

        send_bluetooth_printf("T+%ds: Raw L:%d R:%d | Total L:%ld R:%ld\r\n",
                             i/2, left_raw, right_raw, left_total, right_total);
        HAL_Delay(500);
    }

    send_bluetooth_message("Rotation test complete!\r\n");
}
