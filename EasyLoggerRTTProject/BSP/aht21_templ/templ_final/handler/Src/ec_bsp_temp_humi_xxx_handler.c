/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file ec_bsp_temp_humi_xxx_handler.c
 *
 * @par dependencies
 * - ec_bsp_temp_humi_xxx_handler.h
 * - ec_bsp_aht21_driver.h
 * - elog.h
 *
 * @author Jack | R&D Dept. | EternalChip
 *
 * @brief Provide the handler APIs for temperature and humidity sensors and
 *        corresponding operations.
 *
 * Processing flow:
 *
 * 1. The `bsp_temp_humi_xxx_handler_inst` function instantiates the
 *    `bsp_temp_humi_xxx_handler_t` object with the required interfaces.
 *
 * 2. The `temp_humi_handler_thread` function handles temperature and humidity
 *    events by reading data and invoking callbacks.
 *
 * @version V1.0 2023-12-03
 *
 * @note 1 tab == 4 spaces!
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "ec_bsp_temp_humi_xxx_handler.h"
#include "elog.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define TEMP_HUMI_NOT_INITIATED       false
#define TEMP_HUMI_INITIATED           true

#define OS_QUEUE_CREATE handler_instance->os_interface->os_queue_create

#define MY_MAX_DELAY 0xffffffffUL
//******************************** Defines **********************************//

//******************************** Variables ********************************//

// Flag to indicate if the handler has been initialized
static bsp_temp_humi_xxx_handler_t *gp_temp_humi_instance = NULL;

// Private data structure to manage handler state
typedef struct temp_humi_handler_private_data {
    bool is_initated;   /* Initialization flag */
} temp_humi_handler_private_data_t;

//******************************** Variables ********************************//

//******************************** Functions ********************************//

/**
 * @brief Mount the handler instance globally for use in other functions.
 *
 * @param[in] instance Pointer to the handler instance to mount.
 */
void __mount_handler(bsp_temp_humi_xxx_handler_t *instance) {
    gp_temp_humi_instance = instance;
}

/**
 * @brief Initialize the temperature and humidity handler.
 *
 * @param[in] handler_instance Pointer to the handler instance.
 *
 * @retval TEMP_HUMI_OK            Initialization successful.
 * @retval TEMP_HUMI_ERRORRESOURCE Resource allocation or queue creation failed.
 */
static temp_humi_status_t bsp_temp_xxx_handler_init(
    bsp_temp_humi_xxx_handler_t *handler_instance) {
    log_d("bsp_temp_xxx_handler_init start");
    int8_t ret = 0;

    // Create event queue
    if (NULL == OS_QUEUE_CREATE) {
        log_e("os_interface is NULL");
        return TEMP_HUMI_ERRORRESOURCE;
    }

    ret = OS_QUEUE_CREATE(10, sizeof(temp_humi_xxx_event_t),
                          &(handler_instance->event_queue_handle));
    log_d("OS_QUEUE_CREATE ret=%d\r\n", ret);
    if (ret) {
#ifdef AHT21_HANDLER_DEBUG
        log_e("error: create queue failed!");
#endif
        return TEMP_HUMI_ERRORRESOURCE;
    }

    // Initialize AHT21 instance
    ret = aht21_inst(handler_instance->paht21_instance,
                     handler_instance->iic_driver_interface,
#ifdef OS_SUPPORTING
                     handler_instance->yield_interface,
#endif // End of OS_SUPPORTING
                     handler_instance->timebase_interface
    );
#ifdef AHT21_HANDLER_DEBUG
    log_d("aht21_inst ret = %d", ret);
#endif
    if (ret) {
        log_e("aht21_inst failed");
        return TEMP_HUMI_ERRORRESOURCE;
    }
    log_d("bsp_temp_xxx_handler_init end");

    return TEMP_HUMI_OK;
}

/**
 * @brief De-initialize the temperature and humidity handler.
 *
 * @return TEMP_HUMI_OK De-initialization successful.
 */
static temp_humi_status_t bsp_temp_xxx_handler_deinit(void) {
    
    gp_temp_humi_instance->p_private_data->is_initated =\
                                        TEMP_HUMI_NOT_INITIATED;
    
    return TEMP_HUMI_OK;
}

/**
 * @brief Instantiate the temperature and humidity handler.
 *
 * @param[in] handler_instance Pointer to the handler instance.
 * @param[in] input_arg Pointer to the input arguments containing required
 *                      interfaces.
 *
 * @retval TEMP_HUMI_OK            Initialization successful.
 * @retval TEMP_HUMI_ERRORRESOURCE Null pointer or resource error.
 */
temp_humi_status_t bsp_temp_humi_xxx_handler_inst(
    bsp_temp_humi_xxx_handler_t *handler_instance,
    temp_humi_handler_all_input_arg_t *input_arg) 
{
    temp_humi_status_t ret = TEMP_HUMI_OK;
    
    log_d("bsp_temp_humi_xxx_handler_inst start");

    if (NULL == handler_instance) {
        log_e("handler_instance is NULL");
        return TEMP_HUMI_ERRORRESOURCE;
    }

    if (NULL == input_arg) {
        log_e("input_arg is NULL");
        return TEMP_HUMI_ERRORRESOURCE;
    }

    if (NULL == input_arg->timebase_interface) {
        log_e("timebase_interface is NULL");
        return TEMP_HUMI_ERRORRESOURCE;
    }

    if (NULL == input_arg->iic_driver_interface) {
        log_e("iic_driver_interface is NULL");
        return TEMP_HUMI_ERRORRESOURCE;
    }

    if (NULL == input_arg->os_interface) {
        log_e("os_interface is NULL");
        return TEMP_HUMI_ERRORRESOURCE;
    }

    // Assign input interfaces to handler instance
    // Better : Void * Varibale passed to low level driver
    handler_instance->timebase_interface = input_arg->timebase_interface;
    handler_instance->iic_driver_interface = input_arg->iic_driver_interface;
    handler_instance->yield_interface = input_arg->yield_interface;
    handler_instance->os_interface = input_arg->os_interface;

    // Initialize the handler
    ret = bsp_temp_xxx_handler_init(handler_instance);
    if ( TEMP_HUMI_OK != ret)
    {
        return TEMP_HUMI_ERRORRESOURCE;
    }
    
    handler_instance->p_private_data->is_initated = TEMP_HUMI_INITIATED;
    
    log_d("bsp_temp_humi_xxx_handler_inst end");
    return TEMP_HUMI_OK;
}

/**
 * @brief Fetch temperature and humidity values.
 *
 * @param[in]  handler_instance Pointer to the handler instance.
 * @param[in]  event            Event data specifying the type of data to fetch
 *                              and its lifetime.
 * @param[out] temp             Pointer to store the fetched temperature value.
 * @param[out] humi             Pointer to store the fetched humidity value.
 *
 * @retval TEMP_HUMI_OK   Data fetched successfully.
 * @retval TEMP_HUMI_ERROR Error in fetching data.
 */
temp_humi_status_t get_temp_humi(
    bsp_temp_humi_xxx_handler_t *handler_instance,
    temp_humi_xxx_event_t event,
    float *temp,
    float *humi) {
    temp_humi_status_t ret = TEMP_HUMI_OK;
    uint32_t tim = handler_instance->timebase_interface->pf_get_tick_count();

    switch (event.type) {
        case TEMP_HUMI_EVENT_TEMP:
            // Re-fetch data if the current time exceeds the data's lifetime
            if ((tim - handler_instance->last_temp_tick) > event.lifetime) {
                handler_instance->paht21_instance->pf_read_temp_humi(
                    handler_instance->paht21_instance, temp, humi);
                // Update last temperature tick
                handler_instance->last_temp_tick = tim;
            }
            break;

        case TEMP_HUMI_EVENT_HUMI:
            if ((tim - handler_instance->last_humi_tick) > event.lifetime) {
                handler_instance->paht21_instance->pf_read_humi(
                    handler_instance->paht21_instance, humi);
                // Update last humidity tick
                handler_instance->last_humi_tick = tim;
            }
            break;

        case TEMP_HUMI_EVENT_BOTH:
            if ((tim - handler_instance->last_temp_tick) > event.lifetime) {
                handler_instance->paht21_instance->pf_read_temp_humi(
                    handler_instance->paht21_instance, temp, humi);
                handler_instance->last_temp_tick = tim;
                log_i("New Data");
            } else {
                log_i("Old Data");
            }

            if ((tim - handler_instance->last_humi_tick) > event.lifetime) {
                handler_instance->paht21_instance->pf_read_humi(
                    handler_instance->paht21_instance, humi);
                // Update last humidity tick
                handler_instance->last_humi_tick = tim;
            }
            break;

        default:
            // Error in configuration
            *temp = 0;
            *humi = 0;
            ret = TEMP_HUMI_ERROR;
            break;
    }
    return ret;
}

/**
 * @brief Handler thread for processing temperature and humidity events.
 *
 * @param[in] argument Pointer to input arguments (input_arg).
 */
void temp_humi_handler_thread(void *argument) {
    log_d("temp_humi_handler_thread start");
    float temp = 0;
    float humi = 0;
    temp_humi_handler_all_input_arg_t *input_arg = NULL;
    temp_humi_xxx_event_t event;
    temp_humi_status_t ret = TEMP_HUMI_OK;

    // AHT21 instance
    bsp_aht21_driver_t bsp_aht21_driver;
    // Handler instance structure
    bsp_temp_humi_xxx_handler_t temp_humi_xxx_handler_instance = {0};

    if (NULL == argument) {
        log_e("temp_humi_handler_thread argument is NULL");
        return;
    }
    
    input_arg = (temp_humi_handler_all_input_arg_t *)argument;

    temp_humi_xxx_handler_instance.paht21_instance = &bsp_aht21_driver;
    
    ret = bsp_temp_humi_xxx_handler_inst(
        &temp_humi_xxx_handler_instance,
        input_arg
    );
    
    if( TEMP_HUMI_OK == ret )
    {    
        __mount_handler(&temp_humi_xxx_handler_instance);
    }
    
    for (;;) {
        // 1. Read event from the queue
        ret = temp_humi_xxx_handler_instance.os_interface->os_queue_get(
            temp_humi_xxx_handler_instance.event_queue_handle,
            &event,
            MY_MAX_DELAY
        );
        log_d("get ret=%d\r\n", ret);

        // 2. Fetch temperature and humidity
        if (TEMP_HUMI_OK ==
           get_temp_humi(&temp_humi_xxx_handler_instance, event, &temp, &humi))
        {
            // Data fetched successfully
        } else {
            // Data fetch error
        }

        // 3. Execute the callback function

        event.pfcallback(&temp, &humi);
    }
}

/**
 * @brief API to read temperature and humidity.
 *
 * @param[in] event Event containing temperature and humidity request details.
 *
 * @retval TEMP_HUMI_OK            Event successfully queued.
 * @retval TEMP_HUMI_ERRORRESOURCE Resource allocation or initialization error.
 */
temp_humi_status_t bsp_temp_humi_xxx_read(temp_humi_xxx_event_t *event) {
    log_d("bsp_temp_humi_xxx_read");
    temp_humi_status_t ret = TEMP_HUMI_OK;

    // 1. Check if the handler is initialized
    if (TEMP_HUMI_NOT_INITIATED ==
        gp_temp_humi_instance->p_private_data->is_initated) {
        log_e("temp_humi_xxx_handler_instance is not initialized");
        return TEMP_HUMI_ERRORRESOURCE;
    }

    // 2. Queue the event for processing
    ret = gp_temp_humi_instance->os_interface->os_queue_put(
        gp_temp_humi_instance->event_queue_handle, event, MY_MAX_DELAY);
    log_d("ret = %d", ret);
    if (ret) {
        log_e("error: os_queue_put event failed!");
        return TEMP_HUMI_ERRORRESOURCE;
    }
    return TEMP_HUMI_OK;
}

//******************************** Functions ********************************//
