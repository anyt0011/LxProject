/**
 ******************************************************************************
 * File Name          : bsp_fdb_init.c
 * Description        :
 ******************************************************************************
 * @attention
 *
 *
 ******************************************************************************
 */

#define LOG_TAG "bsp_fdb_init.c"

/* Private includes ----------------------------------------------------------*/
#include "string.h"
/* 底层HAL头文件包含 */

#include "main.h"

/* OS头文件包含 */
#include "FreeRTOS.h"
#include "semphr.h"

/* 三方库头文件包含 */
#include "elog.h"
#include "flashDB.h"
#include "bsp_fdb_init.h"
#include "bsp_rtc.h"
#include "sfud.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define INITED 1
#define NOT_INITED 0

/* 时间序列单条日志最大长度 */
#define FDB_MAX_TSL_LEN 512

/* 启用单元测试，注意：单元测试的报文打印数据量很大，尤其是反复测试后，elog异步
 * 打印可能会出现丢数据的情况，可以暂时关闭异步打印，开启同步打印的方式进行测试*/
#define FDB_TEST_DEMO 0

/* Private macro -------------------------------------------------------------*/

/* 内存队列配置 */
#define LOG_QUEUE_LENGTH 32 /* 队列深度 */
#define LOG_QUEUE_ITEM_SIZE sizeof(elog_entry_t)

/* 日志条目结构体 */
typedef struct
{
    char log[128];        /* 日志内容缓冲区 */
    size_t log_len;       /* 实际日志长度 */
    fdb_time_t timestamp; /* 时间戳 */
} elog_entry_t;

static QueueHandle_t log_queue;

/* Private variables ---------------------------------------------------------*/

/*定义一个KV数据库结构体*/
static struct fdb_kvdb kvdb = {0};

/*定义一个TS数据库结构体*/
struct fdb_tsdb tsdb = {0};

/*定义一个flashDB数据库互斥锁*/
static SemaphoreHandle_t fdb_kv_mutex = NULL;
static SemaphoreHandle_t fdb_ts_mutex = NULL;
/*定义一个日志查询缓存，这个是单条日志的缓存*/
static char read_buf[2048];

static uint32_t last_ts_time = 0;

uint32_t fdb_is_inited = NOT_INITED;

/*单元测试*/
#define FDB_KVDB_TEST_DEMO 0
extern void kvdb_basic_sample(fdb_kvdb_t kvdb);
#if FDB_KVDB_TEST_DEMO
extern void kvdb_basic_sample(fdb_kvdb_t kvdb);
extern void kvdb_type_string_sample(fdb_kvdb_t kvdb);
extern void kvdb_type_blob_sample(fdb_kvdb_t kvdb);
#endif /*FDB_TEST_DEMO*/

#if FDB_TSDB_TEST_DEMO
extern void tsdb_sample(fdb_tsdb_t tsdb);
#endif /*FDB_TEST_DEMO*/

/* Private function prototypes -----------------------------------------------*/
static void flashdb_writer_task(void *arg);


static void _fdb_kv_lock(void)
{
    xSemaphoreTake(fdb_kv_mutex, portMAX_DELAY);
}

static void _fdb_kv_unlock(void)
{
    xSemaphoreGive(fdb_kv_mutex);
}

static void _fdb_ts_lock(void)
{
    xSemaphoreTake(fdb_ts_mutex, portMAX_DELAY);
}

static void _fdb_ts_unlock(void)
{
    xSemaphoreGive(fdb_ts_mutex);
}

/**
 * FDB获取时间戳。
 *
 * @param tim  输入参数，表示从纪元开始经过的秒数。
 * @param tm   输出参数，指向 TIME_FMT 结构体的指针，用于存储转换后的日期和时间信息。
 */
static fdb_time_t _fdb_get_time(void)
{
    uint32_t  test_timestamp = bsp_time(NULL);

    return test_timestamp;
}

/* 单次遍历执行的操作 */
static bool query_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    fdb_tsdb_t db = arg;

    fdb_blob_read((fdb_db_t)db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, read_buf, sizeof(read_buf))));

    /* 如果是Aseert类型的日志或者Error类型的日志，则查询输出 */
    if ((read_buf[8] == 'A' || read_buf[8] == 'E'))
    {
        elog_raw(read_buf);
        vTaskDelay(5);
    }

    return false;
}

/* 按照查询时间 */
static bool query_by_time_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;

    fdb_tsdb_t db = arg;

    fdb_blob_read((fdb_db_t)db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, read_buf, sizeof(read_buf))));

    /* 如果是Aseert类型的日志或者Error类型的日志，则查询输出 */
    if ((read_buf[8] == 'A' || read_buf[8] == 'E'))
    {
        elog_raw(read_buf);
        vTaskDelay(5);
    }

    return false;
}

size_t count = 0;
fdb_err_t kv_status = 0;
/**
 * FDB初始化函数，初始化KVDB和TSDB
 *
 * @param tim  输入参数，表示从纪元开始经过的秒数。
 * @param tm   输出参数，指向 TIME_FMT 结构体的指针，用于存储转换后的日期和时间信息。
 */
void bsp_fdb_init(void)
{
    if (INITED == fdb_is_inited)
    {
        return;
    }
    else
    {
        fdb_err_t result;
        struct fdb_default_kv default_kv;

        /*创建flashDB互斥锁*/
        fdb_kv_mutex = xSemaphoreCreateMutex();
        fdb_ts_mutex = xSemaphoreCreateMutex();

        if ((fdb_kv_mutex == NULL) || (fdb_ts_mutex == NULL) ) 
        {
            return;
        }
       
        /*互斥锁注册到flasdhDB KV数据库*/
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, (void *)_fdb_kv_lock);
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, (void *)_fdb_kv_unlock);
        bool state = true;
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_NOT_FORMAT, &state);
        
        // KV数据库初始化
        result = fdb_kvdb_init(&kvdb, "env", "KVDB", &default_kv, NULL);
        
        // 初始化不过卡死在这里，这里后面有待优化
        if (result != FDB_NO_ERR)
        {
            log_i("kvdb init error");

            while (1)
            {
                vTaskDelay(500);
            }
        }
       
        log_i("kvdb init SUCCES");

//
#if FDB_KVDB_TEST_DEMO
        /* run basic KV samples */
        kvdb_basic_sample(&kvdb);
        /* run string KV samples */
        kvdb_type_string_sample(&kvdb);
        /* run blob KV samples */
        kvdb_type_blob_sample(&kvdb);
#endif

        /*互斥锁注册到flasdhDB tsdb数据库*/
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_LOCK, (void *)_fdb_ts_lock);
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_UNLOCK, (void *)_fdb_ts_unlock);

        /*获取最后一次插入TSL的时间戳*/
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_GET_LAST_TIME, &last_ts_time);

        // TS数据库初始化
        //&tsdb: TSDB 的实例。
        //"log": TSDB 的分区名称。
        //"TSDB": 挂载分区的名称(FAL模式：分区名称,如果是文件模式这里就是文件路径)。
        //_fdb_get_time: 获取硬件 RTC 时间的回调函数（用于提供时间戳）。
        // FDB_MAX_TSL_LEN: 每条时间序列日志（TSL）的最大长度。
        // NULL: 额外参数。
        result = fdb_tsdb_init(&tsdb, "log", "TSDB", _fdb_get_time, FDB_MAX_TSL_LEN, NULL);

        bool temp = true;
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_ROLLOVER, &temp);
        // 初始化不过卡死在这里，这里后面有待优化
        if (result != FDB_NO_ERR)
        {
            log_i("tsdb init ERROR");

            while (1)
            {
                vTaskDelay(500);
            }
        }

        log_i("tsdb init SUCCES");

        /* 创建内存队列 */
        log_queue = xQueueCreate(LOG_QUEUE_LENGTH, LOG_QUEUE_ITEM_SIZE);

        /* 创建后台任务 */
        xTaskCreate(flashdb_writer_task, "log_writer", 512, NULL,
                    configMAX_PRIORITIES - 2, NULL);

        fdb_is_inited = INITED;

#if 1

        /* 准备查询时间范围（从 1970-01-01 00:00:00 到 2020-05-05 00:00:00） */
        struct tm tm_from = {.tm_year = 2024 - 1900, .tm_mon = 1, .tm_mday = 6, .tm_hour = 0, .tm_min = 0, .tm_sec = 0};
        struct tm tm_to = {.tm_year = 2025 - 1900, .tm_mon = 1, .tm_mday = 7, .tm_hour = 0, .tm_min = 0, .tm_sec = 0};

        log_i("=========== Starting to query logs from [2025-01-06 00:00:00] to [2025-01-07 00:00:00], please wait =============");
        fdb_tsl_iter(&tsdb, query_cb, &tsdb);

        time_t from_time = mktime(&tm_from), to_time = mktime(&tm_to);

        /* 在指定时间范围内查询 TSDB 中的所有 TSL */
        fdb_tsl_iter_by_time(&tsdb, from_time, to_time, query_by_time_cb, &tsdb);
        /* 查询指定时间范围内具有 FDB_TSL_WRITE 状态的 TSL 数量 */
        count = fdb_tsl_query_count(&tsdb, from_time, to_time, FDB_TSL_WRITE);

        log_i("=========== Query completed, a total of %d records were found=============\r\n", count);
//            fdb_tsl_clean(&tsdb);
//            fdb_is_inited =  0;
        
#endif
#if FDB_TSDB_TEST_DEMO
        tsdb_sample(&tsdb);
#endif /*FDB_TEST_DEMO*/
    }
}

/**
 * another copy string function
 *
 * @param cur_len current copied log length, max size is ELOG_LINE_BUF_SIZE
 * @param dst destination
 * @param src source
 *
 * @return copied length
 */
/*---------------------------------------------------------------
 * 后台写入任务
 *---------------------------------------------------------------*/
static void flashdb_writer_task(void *arg)
{
    elog_entry_t entry;
    /* 构造 TSDB 数据 */
    struct fdb_blob blob;

    while (1)
    {
        if (xQueueReceive(log_queue, &entry, portMAX_DELAY) == pdPASS)
        {

            blob.buf = entry.log;
            blob.size = entry.log_len + 1; /* 包含结束符 */

            fdb_tsl_append(&tsdb, fdb_blob_make(&blob, entry.log, entry.log_len));
        }
    }
}

/*---------------------------------------------------------------
 * EasyLogger 输出钩子函数
 *---------------------------------------------------------------*/
void elog_flashdb_hook(uint8_t level, const char *log, size_t size)
{
    elog_entry_t entry;

    if (NOT_INITED == fdb_is_inited)
    {
        return;
    }

    // error以下的日志不会保存
    if (ELOG_LVL_ERROR < level)
    {
        return;
    }

    if (size > sizeof(entry.log) - 1)
    {
        size = sizeof(entry.log) - 1; /* 截断超长日志 */
    }

    memcpy(entry.log, log, size);
    entry.log[size] = '\0';
    entry.log_len = size;
    // entry.timestamp = _fdb_get_time();

    /* 非阻塞式入队 */
    xQueueSend(log_queue, &entry, 0);
}


void testt(void)
{
    struct fdb_blob blob;
    int temp = 0;

    log_i("==================== kvdb_basic_sample ====================\r\n");

    { /* GET the KV value */
        /* get the "boot_count" KV value */
        fdb_kv_get_blob(&kvdb, "temp", fdb_blob_make(&blob, &temp, sizeof(temp)));
        /* the blob.saved.len is more than 0 when get the value successful */
        if (blob.saved.len > 0) {
            log_i("get the 'temp' value is %d\r\n", temp);
        } else {
            log_i("get the 'temp' failed\r\n");
        }
    }

    { /* CHANGE the KV value */
        /* increase the boot count */
        temp ++;
        /* change the "boot_count" KV's value */
        fdb_kv_set_blob(&kvdb, "temp", fdb_blob_make(&blob, &temp, sizeof(temp)));
        log_i("set the 'temp' value to %d\r\n", temp);
    }

    log_i("===========================================================\r\n");
    
}