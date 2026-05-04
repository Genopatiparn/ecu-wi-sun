/***************************************************************************//**
 * @file app.c
 * @brief Application code
 *******************************************************************************/
// -----------------------------------------------------------------------------
//                                 Includes
// -----------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "sl_sleeptimer.h" 
#include "sl_iostream.h"
#include "app.h"
#include "cmsis_os2.h" 

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------
#define LOCAL_PORT 5555

typedef struct {
    char name[20];
    char ip[40];
    int port;
} NodeDirectory;

// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------
static void init_udp_socket(void);
static void trim_whitespace(char *str);

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------
NodeDirectory my_network[] = {
    {"Main", "fd12:3456::b635:22ff:fe98:2478", 1234},
    {"Node 1", "fd12:3456::da7a:3bff:fe41:991f", 5555},
    {"Node 2", "fd12:3456::b635:22ff:fe98:2462", 5555}
};
#define NUM_NODES (sizeof(my_network) / sizeof(NodeDirectory))

int global_sock = -1;

char chat_msg[128];
int chat_idx = 0;

// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------
static void trim_whitespace(char *str) {
    if (str == NULL) return;
    char *end = str + strlen(str) - 1;
    while(end >= str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    char *start = str;
    while(*start && isspace((unsigned char)*start)) start++;
    if (start != str) memmove(str, start, strlen(start) + 1);
}

static void init_udp_socket(void) {
    struct sockaddr_in6 local_addr;

    global_sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (global_sock < 0) {
        printf("[Error] Failed to create socket\n");
        return;
    }

    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin6_family = AF_INET6;
    local_addr.sin6_port = htons(LOCAL_PORT);
    local_addr.sin6_addr = in6addr_any;

    if (bind(global_sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        printf("[Error] Port bind failed\n");
        close(global_sock);
        global_sock = -1;
        return;
    }
    printf("[UDP] Socket initialized on port %d\n", LOCAL_PORT);
}

void udp_receive_task(void *arg) {
    (void)arg;
    while (1) {
        if (global_sock < 0) {
            osDelay(100);
            continue;
        }

        char rx_buffer[128];
        struct sockaddr_in6 from_addr;
        socklen_t from_len = sizeof(from_addr);

        int recv_bytes = recvfrom(global_sock, rx_buffer, sizeof(rx_buffer) - 1, 0,
                                  (struct sockaddr *)&from_addr, &from_len);

        if (recv_bytes > 0) {
            rx_buffer[recv_bytes] = '\0';
            char sender_ip[40];
            inet_ntop(AF_INET6, &from_addr.sin6_addr, sender_ip, sizeof(sender_ip));

            char sender_name[20] = "Unknown Node";
            for(int i = 0; i < (int)NUM_NODES; i++) {
                if (strcmp(sender_ip, my_network[i].ip) == 0) {
                    strncpy(sender_name, my_network[i].name, sizeof(sender_name) - 1);
                    sender_name[sizeof(sender_name) - 1] = '\0';
                    break;
                }
            }

            printf("\n[Received from %s] %s\n", sender_name, rx_buffer);
            printf(">>> Target:Message -> ");
            for(int i = 0; i < chat_idx; i++) putchar(chat_msg[i]); 
            fflush(stdout);
        }
    }
}

void app_task(void *args)
{
  (void) args;

  printf("\n[Wi-SUN] Waiting for network connection...\n");
  sl_wisun_app_core_util_connect_and_wait();
  printf("[Wi-SUN] Network connected successfully! (Operational)\n");

  init_udp_socket();

  const osThreadAttr_t rx_task_attr = {
      .name = "udp_rx_task",
      .stack_size = 2048,
      .priority = (osPriority_t) osPriorityNormal,
  };
  osThreadNew(udp_receive_task, NULL, &rx_task_attr);

  // Clear screen to hide any debug messages from SDK
  printf("\033[2J\033[H");  // Clear screen and move cursor to top
  
  printf("\n--- Wi-SUN Node Command Center --- \n");
  printf("Format -> TargetName:Your Message\n");
  printf("Single   -> Node 1:Hello\n");
  printf("Multiple -> Node 1,Node 2:coffee\n");
  printf("Broadcast-> all:Hi everyone\n");
  printf("---------------------------------- \n");
  printf(">>> Target:Message -> ");
  fflush(stdout);

  while (1) {
    char c = 0;
    size_t bytes_read;
    if (sl_iostream_read(SL_IOSTREAM_STDIN, &c, 1, &bytes_read) == SL_STATUS_OK && bytes_read > 0) {
        putchar(c);
        if (c == '\r' || c == '\n') {
            chat_msg[chat_idx] = '\0';
            if (chat_idx > 0) {
                char *colon_ptr = strchr(chat_msg, ':');
                if (colon_ptr != NULL) {
                    *colon_ptr = '\0';
                    char *targets_str = chat_msg;
                    char *message_to_send = colon_ptr + 1;

                    trim_whitespace(targets_str);

                    // Check for broadcast
                    if (strcasecmp(targets_str, "all") == 0 || strcasecmp(targets_str, "broadcast") == 0) {
                        printf("\n[Broadcasting to all nodes...]");
                        int sent_count = 0;
                        for(int i = 0; i < (int)NUM_NODES; i++) {
                            struct sockaddr_in6 dest_addr;
                            memset(&dest_addr, 0, sizeof(dest_addr));
                            dest_addr.sin6_family = AF_INET6;
                            dest_addr.sin6_port = htons(my_network[i].port);
                            inet_pton(AF_INET6, my_network[i].ip, &dest_addr.sin6_addr);

                            sendto(global_sock, message_to_send, strlen(message_to_send), 0,
                                   (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                            printf("\n  -> Sent to %s", my_network[i].name);
                            sent_count++;
                        }
                        printf("\n[Broadcast complete: %d nodes]\n", sent_count);
                    } else {
                        // Parse multiple targets separated by comma
                        char targets_copy[128];
                        strncpy(targets_copy, targets_str, sizeof(targets_copy) - 1);
                        targets_copy[sizeof(targets_copy) - 1] = '\0';

                        int sent_count = 0;
                        int not_found_count = 0;
                        char *token = strtok(targets_copy, ",");
                        
                        while (token != NULL) {
                            trim_whitespace(token);
                            
                            char target_ip[40] = "";
                            int target_port = 0;
                            for(int i = 0; i < (int)NUM_NODES; i++) {
                                if (strcasecmp(token, my_network[i].name) == 0) {
                                    strncpy(target_ip, my_network[i].ip, sizeof(target_ip) - 1);
                                    target_ip[sizeof(target_ip) - 1] = '\0';
                                    target_port = my_network[i].port;
                                    break;
                                }
                            }

                            if (strlen(target_ip) > 0) {
                                struct sockaddr_in6 dest_addr;
                                memset(&dest_addr, 0, sizeof(dest_addr));
                                dest_addr.sin6_family = AF_INET6;
                                dest_addr.sin6_port = htons(target_port);
                                inet_pton(AF_INET6, target_ip, &dest_addr.sin6_addr);

                                sendto(global_sock, message_to_send, strlen(message_to_send), 0,
                                       (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                                printf("\n[Sent to %s] Success", token);
                                sent_count++;
                            } else {
                                printf("\n[!] Error: Node '%s' not found!", token);
                                not_found_count++;
                            }
                            
                            token = strtok(NULL, ",");
                        }
                        
                        if (sent_count > 0) {
                            printf("\n[Summary: %d sent", sent_count);
                            if (not_found_count > 0) printf(", %d failed", not_found_count);
                            printf("]\n");
                        }
                    }
                } else {
                    printf("\n[!] Invalid format. Use 'Name:Message'\n");
                }

                chat_idx = 0;
                printf("\n>>> Target:Message -> ");
            }
        } else if (c == '\b' || c == 127) {
            if (chat_idx > 0) { chat_idx--; printf(" \b"); }
        } else {
            if (chat_idx < (int)(sizeof(chat_msg) - 1)) chat_msg[chat_idx++] = c;
        }
    }

    sl_wisun_app_core_util_dispatch_thread();
    sl_sleeptimer_delay_millisecond(10);
  }
}

// -----------------------------------------------------------------------------
//                          Static Function Definitions
// -----------------------------------------------------------------------------
