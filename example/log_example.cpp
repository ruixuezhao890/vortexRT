#include "bridge.h"
#include <cstdio>
#include <cstring>
#include "stm32f4xx.h"
#include <vortexRT.h>
#include "usart.h"


void console_logger(const char *msg) {
    char buffer[1024] = {0};

    const int len = snprintf(buffer, sizeof(buffer), "%s\r\n", msg);

    const size_t send_len = static_cast<size_t>(len) < sizeof(buffer) ? len : sizeof(buffer) - 1;
    HAL_UART_Transmit(&huart1, reinterpret_cast<uint8_t *>(buffer), send_len,HAL_MAX_DELAY);
}
void file_stream_logger(const char *msg) {
    //todo
}

void OS::context_switch_user_hook() {
}

void OS::system_timer_user_hook() {
}


typedef OS::process<OS::pr0, 2048> TProc0;
typedef OS::process<OS::pr1, 2048> TProc1;
typedef OS::process<OS::pr2, 2048> TProc2;
// typedef OS::process<OS::pr3, 300> TProc3;

TProc0 Proc0;
TProc1 Proc1;
TProc2 Proc2;
// TProc3 Proc3;
OS::TMutex myMutex;

void app_main() {
    VX_LOG_INIT();
    VX_LOG_SUBSCRIBE(console_logger, vx_log::Level::Debug);
    VX_LOG_SUBSCRIBE(file_stream_logger, vx_log::Level::Trace);
    myMutex.lock();
    VX_LOG_TRACE("VX_LOG_TRACE");
    VX_LOG_DEBUG("VX_LOG_DEBUG");
    VX_LOG_INFO("VX_LOG_INFO");
    VX_LOG_WARNING("VX_LOG_WARNING");
    VX_LOG_ERROR("VX_LOG_ERROR");
    VX_LOG_CRITICAL("VX_LOG_CRITICAL");
    VX_LOG_ALWAYS("VX_LOG_ALWAYS");

    VX_LOG_INFO("test args %f", 3.14);
    myMutex.unlock();
    OS::run();
}

namespace OS {
    template<>
    OS_PROCESS void TProc0::exec() {
        VX_LOG_INFO("TProc0!");
        for (;;) {
            HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_9);
            OS::sleep(1000);
        }
    }

    template<>
    OS_PROCESS void TProc1::exec() {
        VX_LOG_INFO("TProc1!");
        VX_LOG_DEBUG("TProc1! %d", 23);
        for (;;) {
            HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_10);
            sleep(1000);
        }
    }

    template<>
    OS_PROCESS void TProc2::exec() {
        VX_LOG_INFO("TProc2!");
        for (;;) {
            VX_LOG_INFO("running");
            sleep(1000);
        }
    }

}
