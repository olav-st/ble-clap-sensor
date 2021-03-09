#include <zephyr.h>
#include <logging/log.h>
#include <net/socket.h>
#include <stdio.h>
#include <string.h>
#include <drivers/adc.h>
#include <hal/nrf_saadc.h>
#include <settings/settings.h>
#include <sys/ring_buffer.h>

LOG_MODULE_REGISTER(clap_detection);

const struct device *adc_dev;

//ADC settings
#define ADC_DEVICE_NAME DT_ADC_0_NAME
#define ADC_RESOLUTION 10
#define ADC_GAIN ADC_GAIN_1_6
#define ADC_REFERENCE ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)
#define ADC_1ST_CHANNEL_ID 0
#define ADC_1ST_CHANNEL_INPUT NRF_SAADC_INPUT_AIN7

static const struct adc_channel_cfg m_1st_channel_cfg = {
    .gain = ADC_GAIN,
    .reference = ADC_REFERENCE,
    .acquisition_time = ADC_ACQUISITION_TIME,
    .channel_id = ADC_1ST_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
    .input_positive = ADC_1ST_CHANNEL_INPUT,
#endif
};

//Long and short term sample averages
#define NUM_SAMPLES_LONG_TERM_AVG 40
#define NUM_SAMPLES_SHORT_TERM_AVG 4
int long_term_average = 0;
int short_term_average = 0;

//Ring buffers to store ADC samples in
RING_BUF_DECLARE(long_term_sample_buffer, NUM_SAMPLES_LONG_TERM_AVG*sizeof(int16_t));
RING_BUF_DECLARE(short_term_sample_buffer, NUM_SAMPLES_SHORT_TERM_AVG*sizeof(int16_t));

//Clap treshold value, stored in settings
int treshold = 10;

static int clap_detection_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
    const char *next;
    int rc;

    if (settings_name_steq(name, "treshold", &next) && !next)
    {
        if (len != sizeof(treshold))
        {
            return -EINVAL;
        }

        rc = read_cb(cb_arg, &treshold, sizeof(treshold));
        if (rc >= 0)
        {
            return 0;
        }

        printk("Loded new treshold value: %d", treshold);

        return rc;
    }


    return -ENOENT;
}

struct settings_handler settings_conf = 
{
    .name = "clap_detection",
    .h_set = clap_detection_settings_set
};

int adc_init()
{
    int err;

    adc_dev = device_get_binding(DT_LABEL(DT_NODELABEL(adc)));
    if (!adc_dev)
    {
        LOG_ERR("device_get_binding %s failed\n", DT_LABEL(DT_NODELABEL(adc)));
    }
    err = adc_channel_setup(adc_dev, &m_1st_channel_cfg);
    if (err)
    {
        LOG_ERR("Error in adc setup: %d\n", err);
    }

    return err;
}

int adc_sample(int16_t* buffer, size_t num_samples)
{
    int ret;

    const struct adc_sequence_options options =
    {
        .extra_samplings = num_samples - 1,
        .interval_us     = 0,
    };

    const struct adc_sequence sequence =
    {
        .channels = BIT(ADC_1ST_CHANNEL_ID),
        .buffer = buffer,
        .buffer_size = sizeof(buffer) * num_samples,
        .resolution = ADC_RESOLUTION,
        .options = &options,
    };

    if (!adc_dev)
    {
        return -1;
    }

    ret = adc_read(adc_dev, &sequence);
    if(ret < 0)
    {
        LOG_ERR("ADC read err: %d\n", ret);
    }

    return ret;
}

int clap_detection_init()
{
    int err = adc_init();
    if(err)
    {
        return err;
    }
    settings_subsys_init();
    settings_register(&settings_conf);
    settings_load();
    LOG_INF("Loaded treshold value: %d", treshold);

    return 0;
}

bool clap_detection_check()
{
    int16_t sample;
    int err = adc_sample(&sample, 1);
    if(err)
    {
        LOG_ERR("Failed to sample using ADC: %d\n", err);
        return false;
    }
    //Add the sample to the ring buffer
    ring_buf_put(&long_term_sample_buffer, &sample, sizeof(sample));
    ring_buf_put(&short_term_sample_buffer, &sample, sizeof(sample));

    if(ring_buf_space_get(&long_term_sample_buffer) == 0 &&
        ring_buf_space_get(&short_term_sample_buffer) == 0
        )
    {
        //Calculate long term average
        long_term_average = 0;
        for (int i = 0; i < NUM_SAMPLES_LONG_TERM_AVG; i++) 
        {
            ring_buf_get(&long_term_sample_buffer, &sample, sizeof(sample));
            long_term_average += abs(sample);
        }
        long_term_average = long_term_average / NUM_SAMPLES_LONG_TERM_AVG;

        //Calculate short term average
        short_term_average = 0;
        for (int i = 0; i < NUM_SAMPLES_SHORT_TERM_AVG; i++) 
        {
            ring_buf_get(&short_term_sample_buffer, &sample, sizeof(sample));
            short_term_average += abs(sample);
        }
        short_term_average = short_term_average / NUM_SAMPLES_SHORT_TERM_AVG;

        //Check for a clap
        int clap_likeness = abs(short_term_average-long_term_average);
        //printk("C: %d, T: %d\n", clap_likeness, treshold);
        //printk("ST: %d, LT: %d\n", short_term_average, long_term_average);
        if(clap_likeness > treshold)
        {
            ring_buf_reset(&short_term_sample_buffer);
            k_msleep(50);
            return true;
        }
    }
    return false;
}

void clap_detection_set_treshold(int value)
{
    LOG_INF("Saving new treshold value: %d", value);
    settings_save_one("clap_detection/treshold", &value, sizeof(value));
    settings_load();
}