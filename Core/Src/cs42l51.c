#include "cs42l51.h"

#include "main.h"

#define I2C_TIMEOUT_MS 100U

#define CS42L51_I2C_ADDR (0x4A << 1)

/* From https://github.com/STMicroelectronics/stm32-cs42l51/blob/main/cs42l51.h */
#define CS42L51_PDWN_HW  0x00U
#define CS42L51_PDWN_SW  0x01U

#define CS42L51_ID       0xD8U
#define CS42L51_ID_MASK  0xF8U

/* From https://github.com/STMicroelectronics/stm32-cs42l51/blob/main/cs42l51_reg.h */
#define CS42L51_CHIP_ID                     0x01U
#define CS42L51_PWR_CTRL_1                  0x02U
#define CS42L51_MIC_PWR_SPEED_CTRL          0x03U
#define CS42L51_INTERFACE_CTRL              0x04U
#define CS42L51_MIC_CTRL                    0x05U
#define CS42L51_ADC_CTRL                    0x06U
#define CS42L51_ADCX_INPUT_SELECT           0x07U
#define CS42L51_DAC_OUTPUT_CTRL             0x08U
#define CS42L51_DAC_CTRL                    0x09U
#define CS42L51_ALCA_PGAA_CTRL              0x0AU
#define CS42L51_ALCB_PGAB_CTRL              0x0BU
#define CS42L51_ADCA_ATTENUATOR             0x0CU
#define CS42L51_ADCB_ATTENUATOR             0x0DU
#define CS42L51_ADCA_MIXER_VOL_CTRL         0x0EU
#define CS42L51_ADCB_MIXER_VOL_CTRL         0x0FU
#define CS42L51_PCMA_MIXER_VOL_CTRL         0x10U
#define CS42L51_PCMB_MIXER_VOL_CTRL         0x11U
#define CS42L51_BEEP_FREQ_AND_TIMING_CFG    0x12U
#define CS42L51_BEEP_OFF_TIME_AND_VOL       0x13U
#define CS42L51_BEEP_CFG_AND_TONE_CFG       0x14U
#define CS42L51_TONE_CTRL                   0x15U
#define CS42L51_AOUTA_VOL_CTRL              0x16U
#define CS42L51_AOUTB_VOL_CTRL              0x17U
#define CS42L51_ADC_PCM_CHANNEL_MIXER       0x18U
#define CS42L51_LIMITER_THR_SZC_DISABLE     0x19U
#define CS42L51_LIMITER_RELEASE_RATE_REG    0x1AU
#define CS42L51_LIMITER_ATTACK_RATE_REG     0x1BU
#define CS42L51_ALC_ENABLE_AND_ATTACK_RATE  0x1CU
#define CS42L51_ALC_RELEASE_RATE            0x1DU
#define CS42L51_ALC_THR                     0x1EU
#define CS42L51_NOISE_GATE_CFG_AND_MISC     0x1FU
#define CS42L51_STATUS                      0x20U
#define CS42L51_CHARGE_PUMP_FREQ            0x21U

/* From CS42L51 datasheet */
/* Reg 0x03: MIC Power Control & Speed Control */
#define CS42L51_SPEED_SSM   (1U << 5) /* Single-Speed Mode (SSM) - 4 to 50 kHz sample rates */
#define CS42L51_PDN_MICB    (1U << 3)
#define CS42L51_PDN_MICA    (1U << 2)
#define CS42L51_PDN_MICBIAS (1U << 1)

/* Reg 0x04: Interface Control */
#define INTF_DAC_I2S        (1U << 3)
#define INTF_ADC_I2S        (1U << 2)

/* Reg 0x07: ADCx Input Select, Invert & Mute */
/* ADC input mux select: 0 = AIN1x, 1 = AIN2x, 2 = AIN3x */
#define CS42L51_INPUT_MUX   0U

/* Reg 0x08: DAC Output Control */
#define DAC_HP_GAIN_0P6047  (3U << 5)
#define DAC_MUTE_B          (1U << 1)
#define DAC_MUTE_A          (1U << 0)

/* Reg 0x0E and 0x0F: ADCx Mixer Volume Control */
#define MUTE_ADCMIXx        (1U << 7)

extern I2C_HandleTypeDef hi2c4;

static int cs42l51_read_reg(uint8_t reg, uint8_t *val)
{
    HAL_StatusTypeDef s;

    s = HAL_I2C_Mem_Read(&hi2c4, CS42L51_I2C_ADDR,
                        reg, I2C_MEMADD_SIZE_8BIT,
                        val, 1, I2C_TIMEOUT_MS);

    return s == HAL_OK ? CS42L51_OK : CS42L51_ERROR;
}

static int cs42l51_write_reg(uint8_t reg, uint8_t val)
{
    HAL_StatusTypeDef s;

    s = HAL_I2C_Mem_Write(&hi2c4, CS42L51_I2C_ADDR,
                          reg, I2C_MEMADD_SIZE_8BIT,
                          &val, 1, I2C_TIMEOUT_MS);

    return s == HAL_OK ? CS42L51_OK : CS42L51_ERROR;
}

int cs42l51_init(void)
{
    int r;
    uint8_t id;

    /* Hardware reset */
    HAL_GPIO_WritePin(AUDIO_NRST_GPIO_Port, AUDIO_NRST_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(AUDIO_NRST_GPIO_Port, AUDIO_NRST_Pin, GPIO_PIN_SET);
    HAL_Delay(5);

    id = 0;

    /* Read chip ID */
    r = cs42l51_read_reg(CS42L51_CHIP_ID, &id);
    if(r != CS42L51_OK) {
        return CS42L51_ERROR;
    }

    /* Verify chip ID */
    if((id & CS42L51_ID_MASK) != CS42L51_ID) {
        return CS42L51_ERROR;
    }

    r = 0;

    /* Software power down */
    r |= cs42l51_write_reg(CS42L51_PWR_CTRL_1, CS42L51_PDWN_SW);

    /* Set single-speed mode; disable mic pre-amplifiers and bias circuits */
    r |= cs42l51_write_reg(CS42L51_MIC_PWR_SPEED_CTRL,
                           CS42L51_SPEED_SSM | CS42L51_PDN_MICB |
                           CS42L51_PDN_MICA | CS42L51_PDN_MICBIAS);

    /* Codec in slave mode, I2S mode for DAC and ADC */
    r |= cs42l51_write_reg(CS42L51_INTERFACE_CTRL, INTF_DAC_I2S | INTF_ADC_I2S);

    /* Mic crontrol - disable all */
    r |= cs42l51_write_reg(CS42L51_MIC_CTRL, 0x0U);

    /* Select channels input in ADC mux */
    r |= cs42l51_write_reg(CS42L51_ADCX_INPUT_SELECT,
                           (uint8_t) ((CS42L51_INPUT_MUX << 6) | (CS42L51_INPUT_MUX << 4)));

    /* Disable ADC -> DAC internal codec mixer - data flows only through microcontroller */
    r |= cs42l51_write_reg(CS42L51_ADCA_MIXER_VOL_CTRL, MUTE_ADCMIXx);
    r |= cs42l51_write_reg(CS42L51_ADCB_MIXER_VOL_CTRL, MUTE_ADCMIXx);
    r |= cs42l51_write_reg(CS42L51_PCMA_MIXER_VOL_CTRL, 0x0U); /* O dB */
    r |= cs42l51_write_reg(CS42L51_PCMB_MIXER_VOL_CTRL, 0x0U);

    /* Set DAC output gain */
    r |= cs42l51_write_reg(CS42L51_DAC_OUTPUT_CTRL, DAC_HP_GAIN_0P6047);

    /* Set output channel gain */
    r |= cs42l51_write_reg(CS42L51_AOUTA_VOL_CTRL, 0x0U); /* 0 dB */
    r |= cs42l51_write_reg(CS42L51_AOUTB_VOL_CTRL, 0x0U);

    return r == 0 ? CS42L51_OK : CS42L51_ERROR;
}

int cs42l51_start(void)
{
    int r;
    
    /* Power up */
    r = cs42l51_write_reg(CS42L51_PWR_CTRL_1, 0x0U);

    HAL_Delay(10);

    return r;
}

int cs42l51_stop(void)
{
    int r;
    
    /* Mute */
    r = cs42l51_set_mute(1);

    /* Software power down */
    r |= cs42l51_write_reg(CS42L51_PWR_CTRL_1, CS42L51_PDWN_SW);

    return r;
}

int cs42l51_set_volume(int16_t half_db)
{
    int r;

    /* Clip volume */
    if(half_db > 24) {
        /* +12 dB */
        half_db = 24;
    }

    if(half_db < -204) {
        /* -102 dB */
        half_db = -204;
    }

    /* Set volume */
    r  = cs42l51_write_reg(CS42L51_AOUTA_VOL_CTRL, (uint8_t) half_db);
    r |= cs42l51_write_reg(CS42L51_AOUTB_VOL_CTRL, (uint8_t) half_db);

    return r;
}

int cs42l51_set_mute(uint8_t mute)
{
    uint8_t v;
    
    /* Default gain */
    v = DAC_HP_GAIN_0P6047;

    /* Set mute flags */
    if(mute) {
        v |= DAC_MUTE_A | DAC_MUTE_B;
    }

    return cs42l51_write_reg(CS42L51_DAC_OUTPUT_CTRL, v);
}
