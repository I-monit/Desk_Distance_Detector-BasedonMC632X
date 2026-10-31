
#ifndef __STEPMOTOR_H__
#define __STEPMOTOR_H__



#define FH_SM_DEVICE_NAME                 "fh_stepmotor"
#define FH_SM_PLAT_DEVICE_NAME	"fh_stepmotor"

#define FH_SM_MISC_DEVICE_NAME                 "fh_stepmotor%d"



#define MAX_FHSM_NR (2)

#define FH_SM_IOCTL_MAGIC        's'
#define RESERVERD                 0
#define FH_SM_SET_PARAM           1
#define FH_SM_START_SYNC          2
#define FH_SM_STOP                3
#define FH_SM_SET_LUT             4
#define FH_SM_GET_LUT             5
#define FH_SM_GET_PARAM           6
#define FH_SM_START_ASYNC         7
#define FH_SM_GET_CUR_CYCLE       8
#define FH_SM_CHECKISTOP          9
#define FH_SM_UPDATE_CYCLE        10


#define MOTOR_CTRL     (0x0)
#define MOTOR_RESET    (0x4)
#define MOTOR_MODE     (0x8)
#define MOTOR_TIMING0     (0xc)
#define MOTOR_TIMING1     (0x10)
#ifdef FH_STEPMOTOR_V1_1
#define MOTOR_INIT     (0x14)
#define MOTOR_DEBOUNCE     (0x18)
#define MOTOR_MANUAL_CONFIG0     (0x1c)
#define MOTOR_MANUAL_CONFIG1     (0x20)
#define MOTOR_INT_EN     (0x24)
#define MOTOR_INT_STATUS     (0x28)
#define MOTOR_STATUS0     (0x2c)
#else
#define MOTOR_MANUAL_CONFIG0     (0x14)
#define MOTOR_MANUAL_CONFIG1     (0x18)
#define MOTOR_INT_EN     (0x1c)
#define MOTOR_INT_STATUS     (0x20)
#define MOTOR_STATUS0     (0x24)
#endif
#define MOTOR_KEEP      (0x48)
#define MOTOR_MEM     (0x100)



enum fh_sm_mode {
    fh_sm_constant = 0,
    fh_sm_high = 1,
    fh_sm_manual_4 = 2,
    fh_sm_manual_8 = 3,
};
struct fh_sm_timingparam {
        int period;
        int counter;
        int copy;
        int microstep;
};


#ifdef FH_STEPMOTOR_V1_1
struct fh_sm_param_init {
	int NeedInit;
	int InitRunNum;
	int NeedUpdatePos;
	int InitStep;
	int InitMicroStep;
};

struct fh_sm_param_autostop {
	int AutoStopEn;
	int AutoStopSigPol;
	int AutoStopSigDeb;
	int AutoStopSigDebSet;
	int AutoStopChechGPIO;
};

#endif

struct fh_sm_param_keep {
    int KeepNumEn; //0/1:enable the keep infinite/finite time
    int KeepDampEn;//0/1:Full-width/Attenuated pwm
    int KeepDamp;//Set attenuated ratio
    int KeepNum;//the period of pwm when KeepNumEn is 1
};

struct fh_sm_param
{
	 enum fh_sm_mode mode;
	int direction;
	int output_invert_A;
	int output_invert_B;
	unsigned int manual_pwm_choosenA;// please lookup doc to set
	unsigned int manual_pwm_choosenB;// please lookup doc to set
	struct fh_sm_timingparam timingparam;
#ifdef FH_STEPMOTOR_V1_1
	struct fh_sm_param_init initparam;
	int keep;
	struct fh_sm_param_autostop autostopparam;
#endif
#ifdef FH_STEPMOTOR_V1_2
    int dampen;//0/1:Full-width/Attenuated pwm
    int damp;//Set attenuated ratio
    struct fh_sm_param_keep keepparam;
    int stoplevel;
#endif
};

struct fh_sm_lut
{
    int lutsize;
    int* lut;
};

#endif /* __STEPMOTOR_H__ */
