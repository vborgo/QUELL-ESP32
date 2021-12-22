#include "esp_log.h"
#include "protocolTask.h"
#include "terminalTask.h"

static const char *TAG = "main";



void createTasks(void)
{   
    /* Create Terminal Task */
    terminalTaskInit();

    /* Create Protocol task */
    protocolTaskInit();
}

void app_main(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    /* Create Tasks*/
    createTasks();
}
