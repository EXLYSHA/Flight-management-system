#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <ctype.h>
#include <iomanip>
#include <limits.h>
using namespace std;
// 数据初始化

#define max_airport 80

typedef struct
{
    int flightID;
    char departureDate[12];
    int intlDome;
    char flightNo[10];
    int departureAirport;
    int arrivalAirport;
    char departureTime[7];
    char arrivalTime[7];
    char arrivalDate[12];
    int airplaneID;
    int airplaneModel;
    int airFares;
    int peakSeasonRates;
    int offSeasonRates;
    // 新增时间变量
    time_t depTime;
    time_t arrTime;
    // 新增标记字段
    bool isCheapest;
    bool isShortestDuration;
    bool isNextDayArrival;
} Flight;

typedef struct FlightNode
{
    Flight data;
    struct FlightNode *next;
} FlightNode;

typedef struct
{
    FlightNode HeadNode;
    int size;
} FlightList;

// 将日期和时间字符串转换为 time_t
time_t convertToTimeT(const char *dateStr, const char *timeStr) // 格式：YYYY-MM-DD
{
    struct tm tm_date;
    memset(&tm_date, 0, sizeof(struct tm));

    // 解析日期
    if (sscanf(dateStr, "%4d-%2d-%2d",
               &tm_date.tm_year, &tm_date.tm_mon, &tm_date.tm_mday) != 3)
    {
        return (time_t)-1; // 返回无效时间
    }
    tm_date.tm_year -= 1900; // 年份从1900开始
    tm_date.tm_mon -= 1;     // 月份从0开始

    // 解析时间
    if (sscanf(timeStr, "%2d:%2d",
               &tm_date.tm_hour, &tm_date.tm_min) != 2)
    {
        return (time_t)-1; // 返回无效时间
    }

    tm_date.tm_sec = 0; // 设置秒数为0

    return mktime(&tm_date);
}

// 帮助函数，将 Excel 日期序列号转换为 YYYY-MM-DD 格式
void excelSerialToDate(int serial, char *result)
{
    int serial_relative = serial - 25569;
    struct tm base = {0};
    base.tm_year = 1970 - 1900;
    base.tm_mon = 0;
    base.tm_mday = 1;

    time_t baseTime = mktime(&base);
    time_t targetTime = baseTime + serial_relative * 86400;

    struct tm *targetDate = localtime(&targetTime);
    if (targetDate == NULL)
    {
        printf("日期转换失败，序列号: %d\n", serial);
        strcpy(result, "Invalid Date");
        return;
    }

    strftime(result, 11, "%Y-%m-%d", targetDate);
}

// 帮助函数，将 Excel 序列号的小数部分转换为 HH:MM 时间格式
void excelSerialToTime(double frac, char *result)
{
    if (frac < 0.0 || frac >= 1.0)
    {
        printf("时间小数部分无效: %lf\n", frac);
        strcpy(result, "Invalid");
        return;
    }

    int totalSeconds = (int)(frac * 86400 + 0.5);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;

    snprintf(result, 6, "%02d:%02d", hours, minutes);
}

// 初始化航班列表函数（链表实现）
int initializeFlightList(const char *filename, FlightList *list)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("无法打开文件");
        return -1;
    }

    char buffer[256];
    // 初始化链表
    list->HeadNode.next = NULL;
    list->size = 0;

    // 第一行为标题，跳过
    if (!fgets(buffer, sizeof(buffer), file))
    {
        printf("文件为空或读取失败。\n");
        fclose(file);
        return -1;
    }

    // 使用尾指针优化插入操作
    FlightNode *tail = &list->HeadNode;

    while (fgets(buffer, sizeof(buffer), file))
    {

        // 创建一个新的航班节点
        FlightNode *newNode = (FlightNode *)malloc(sizeof(FlightNode));
        if (!newNode)
        {
            perror("内存分配失败");
            fclose(file);
            return -1;
        }
        memset(newNode, 0, sizeof(FlightNode));

        // 使用逗号作为分隔符解析 CSV 行
        char *token = strtok(buffer, ",");
        if (token)
        {
            newNode->data.flightID = atoi(token);
        }
        else
        {
            fprintf(stderr, "航班ID缺失，跳过该行。\n");
            free(newNode);
            continue;
        }

        // 解析并转换出发日期
        token = strtok(NULL, ",");
        if (token)
        {
            int dateSerial = atoi(token);
            excelSerialToDate(dateSerial, newNode->data.departureDate);
        }
        else
        {
            fprintf(stderr, "出发日期缺失，跳过该行。\n");
            free(newNode);
            continue;
        }

        // 解析 Intl/Dome
        token = strtok(NULL, ",");
        if (token)
        {
            newNode->data.intlDome = (strcmp(token, "Intl") == 0) ? true : false;
        }
        else
        {
            fprintf(stderr, "航班类型缺失，跳过该行。\n");
            free(newNode);
            continue;
        }

        // 解析航班号
        token = strtok(NULL, ",");
        if (token)
        {
            strncpy(newNode->data.flightNo, token, sizeof(newNode->data.flightNo) - 1);
            newNode->data.flightNo[sizeof(newNode->data.flightNo) - 1] = '\0';
        }
        else
        {
            fprintf(stderr, "航班号缺失，跳过该行。\n");
            free(newNode);
            continue;
        }

        // 解析出发机场ID
        token = strtok(NULL, ",");
        if (token)
        {
            newNode->data.departureAirport = atoi(token);
        }
        else
        {
            fprintf(stderr, "出发机场ID缺失，跳过该行。\n");
            free(newNode);
            continue;
        }

        // 解析到达机场ID
        token = strtok(NULL, ",");
        if (token)
        {
            newNode->data.arrivalAirport = atoi(token);
        }
        else
        {
            fprintf(stderr, "到达机场ID缺失，跳过该行。\n");
            free(newNode);
            continue;
        }

        // 解析并转换出发时间
        token = strtok(NULL, ",");
        if (token)
        {
            double depSerial = atof(token);
            double depFrac = depSerial - (int)depSerial; // 提取小数部分
            excelSerialToTime(depFrac, newNode->data.departureTime);
            newNode->data.depTime = convertToTimeT(newNode->data.departureDate, newNode->data.departureTime);
        }
        else
        {
            fprintf(stderr, "出发时间缺失，跳过该行。\n");
            free(newNode);
            continue;
        }

        // 解析并转换到达时间
        token = strtok(NULL, ",");
        if (token)
        {
            double arrSerial = atof(token);
            newNode->data.arrTime = arrSerial;
            double arrFrac = arrSerial - (int)arrSerial; // 提取小数部分
            double arrInt = (int)arrSerial;
            excelSerialToTime(arrFrac, newNode->data.arrivalTime);
            excelSerialToDate(arrInt, newNode->data.arrivalDate);
            newNode->data.arrTime = convertToTimeT(newNode->data.arrivalDate, newNode->data.arrivalTime);
        }
        else
        {
            fprintf(stderr, "到达时间缺失，跳过该行。\n");
            free(newNode);
            continue;
        }

        // 解析飞机 ID
        token = strtok(NULL, ",");
        if (token)
        {
            newNode->data.airplaneID = atoi(token);
        }
        else
        {
            fprintf(stderr, "飞机ID缺失，跳过该行。\n");
            free(newNode);
            continue;
        }

        // 解析飞机机型
        token = strtok(NULL, ",");
        if (token)
        {
            newNode->data.airplaneModel = atoi(token);
            if (newNode->data.airplaneModel < 1 || newNode->data.airplaneModel > 5)
            {
                fprintf(stderr, "无效的飞机机型(%d)，跳过该行。\n", newNode->data.airplaneModel);
                free(newNode);
                continue;
            }
        }
        else
        {
            fprintf(stderr, "飞机机型缺失，跳过该行。\n");
            free(newNode);
            continue;
        }

        // 解析基础费用
        token = strtok(NULL, ",");
        if (token)
        {
            newNode->data.airFares = atoi(token);
        }
        else
        {
            fprintf(stderr, "基础费用缺失，跳过该行。\n");
            free(newNode);
            continue;
        }

        // 解析旺季价格
        token = strtok(NULL, ",");
        if (token)
        {
            newNode->data.peakSeasonRates = atoi(token);
        }
        else
        {
            fprintf(stderr, "旺季价格缺失，跳过该行。\n");
            free(newNode);
            continue;
        }

        // 解析淡季价格
        token = strtok(NULL, ",");
        if (token)
        {
            newNode->data.offSeasonRates = atoi(token);
        }
        else
        {
            fprintf(stderr, "淡季价格缺失，跳过该行。\n");
            free(newNode);
            continue;
        }

        // 将新节点添加到链表末尾
        tail->next = newNode;
        tail = newNode;
        newNode->next = NULL;

        // 增加链表大小
        list->size++;
    }

    fclose(file);
    printf("成功初始化航班数据，共 %d 条记录。\n", list->size);
    return 0;
}

// 查找航班节点，根据 flightID
FlightNode *findFlight(FlightList *list, int flightID)
{
    FlightNode *current = list->HeadNode.next;
    while (current)
    {
        if (current->data.flightID == flightID)
            return current;
        current = current->next;
    }
    return NULL;
}

// 添加航班
int addFlight(FlightList *list, Flight newFlight)
{
    // 检查是否已存在相同 flightID
    if (findFlight(list, newFlight.flightID) != NULL)
    {
        printf("添加失败：航班ID %d 已存在。\n", newFlight.flightID);
        return -1;
    }

    // 创建新节点
    FlightNode *newNode = (FlightNode *)malloc(sizeof(FlightNode));
    if (!newNode)
    {
        perror("内存分配失败");
        return -1;
    }
    memcpy(&newNode->data, &newFlight, sizeof(Flight));
    newNode->next = NULL;

    // 插入到链表末尾
    FlightNode *tail = &list->HeadNode;
    while (tail->next)
    {
        tail = tail->next;
    }
    tail->next = newNode;
    list->size++;

    printf("航班ID %d 添加成功。\n", newFlight.flightID);
    return 0;
}

// 删除航班
int deleteFlight(FlightList *list, int flightID)
{
    FlightNode *prev = &list->HeadNode;
    FlightNode *current = list->HeadNode.next;

    while (current)
    {
        if (current->data.flightID == flightID)
        {
            prev->next = current->next;
            free(current);
            list->size--;
            printf("航班ID %d 删除成功。\n", flightID);
            return 0;
        }
        prev = current;
        current = current->next;
    }

    printf("删除失败：未找到航班ID %d。\n", flightID);
    return -1;
}

// 修改航班
int modifyFlight(FlightList *list, int flightID, Flight updatedFlight)
{
    FlightNode *node = findFlight(list, flightID);
    if (!node)
    {
        printf("修改失败：未找到航班ID %d。\n", flightID);
        return -1;
    }

    // 保持 flightID 不变
    updatedFlight.flightID = flightID;

    memcpy(&node->data, &updatedFlight, sizeof(Flight));
    printf("航班ID %d 修改成功。\n", flightID);
    return 0;
}

// 计算飞行持续时间（分钟）
int calculateDuration(const char *departureTime, const char *arrivalTime, bool isNextDayArrival)
{
    int depHour, depMin, arrHour, arrMin;
    sscanf(departureTime, "%d:%d", &depHour, &depMin);
    sscanf(arrivalTime, "%d:%d", &arrHour, &arrMin);

    int depTotal = depHour * 60 + depMin;
    int arrTotal = arrHour * 60 + arrMin;

    if (isNextDayArrival)
        arrTotal += 1440; // 加一天的分钟数

    return arrTotal - depTotal;
}

// 标记满足条件的航班
void markFlights(FlightList *list)
{
    if (list->size == 0)
    {
        printf("航班列表为空，无法标记。\n");
        return;
    }

    // 重置所有标记
    FlightNode *current = list->HeadNode.next;
    while (current)
    {
        current->data.isCheapest = false;
        current->data.isShortestDuration = false;
        current->data.isNextDayArrival = false;
        current = current->next;
    }

    // 标记费用最少的航班
    int minFare = INT32_MAX;
    current = list->HeadNode.next;
    while (current)
    {
        if (current->data.airFares < minFare)
            minFare = current->data.airFares;
        current = current->next;
    }
    current = list->HeadNode.next;
    while (current)
    {
        if (current->data.airFares == minFare)
            current->data.isCheapest = true;
        current = current->next;
    }

    // 标记用时最少的航班
    int minDuration = INT32_MAX;
    current = list->HeadNode.next;
    while (current)
    {
        // 判断是否是第二天到达
        bool isNextDay = false;
        // 比较时间
        int depHour, depMin, arrHour, arrMin;
        sscanf(current->data.departureTime, "%d:%d", &depHour, &depMin);
        sscanf(current->data.arrivalTime, "%d:%d", &arrHour, &arrMin);
        if (strcmp(current->data.departureDate, current->data.arrivalDate) != 0)
            isNextDay = true;

        int duration = calculateDuration(current->data.departureTime, current->data.arrivalTime, isNextDay);
        if (duration < minDuration)
            minDuration = duration;
        current = current->next;
    }

    current = list->HeadNode.next;
    while (current)
    {
        bool isNextDay = false;
        int depHour, depMin, arrHour, arrMin;
        sscanf(current->data.departureTime, "%d:%d", &depHour, &depMin);
        sscanf(current->data.arrivalTime, "%d:%d", &arrHour, &arrMin);
        if (strcmp(current->data.departureDate, current->data.arrivalDate) != 0)
            isNextDay = true;

        int duration = calculateDuration(current->data.departureTime, current->data.arrivalTime, isNextDay);
        if (duration == minDuration)
            current->data.isShortestDuration = true;
        current = current->next;
    }

    // 标记降落时间是第二天的航班
    current = list->HeadNode.next;
    while (current)
    {
        int depHour, depMin, arrHour, arrMin;
        sscanf(current->data.departureTime, "%d:%d", &depHour, &depMin);
        sscanf(current->data.arrivalTime, "%d:%d", &arrHour, &arrMin);
        if (strcmp(current->data.departureDate, current->data.arrivalDate) != 0)
            current->data.isNextDayArrival = true;
        current = current->next;
    }

    printf("航班标记完成。\n");
}

// 显示单个航班信息
void displayFlight(const Flight *flight)
{
    printf("航班ID: %d\n", flight->flightID);
    // printf("出发日期: %s\n", flight->departureDate);
    // printf("到达日期: %s\n", flight->arrivalDate);
    // printf("航班类型: %s\n", flight->intlDome ? "Intl" : "Dome");
    // printf("航班号: %s\n", flight->flightNo);
    // printf("出发机场ID: %d\n", flight->departureAirport);
    // printf("到达机场ID: %d\n", flight->arrivalAirport);
    // printf("出发时间: %s\n", flight->departureTime);
    // printf("到达时间: %s\n", flight->arrivalTime);
    // printf("飞机ID: %d\n", flight->airplaneID);
    // printf("飞机机型: %d\n", flight->airplaneModel);
    // printf("基础费用: %d\n", flight->airFares);
    // printf("旺季价格: %d\n", flight->peakSeasonRates);
    // printf("淡季价格: %d\n", flight->offSeasonRates);
    // printf("%d\n",flight->depTime);
    // printf("%d\n",flight->arrTime);
    printf("标记: ");
    if (flight->isCheapest)
        printf("Cheapest ");
    if (flight->isShortestDuration)
        printf("Shortest Duration ");
    if (flight->isNextDayArrival)
        printf("+1 Day Arrival ");
    if (!flight->isCheapest && !flight->isShortestDuration && !flight->isNextDayArrival)
        printf("None");
    printf("\n-----------------------------------\n");
}

// 将航班添加到指定的 FlightList 中 尾插法
int appendFlightToList(FlightList *list, Flight flight)
{
    // 创建新节点
    FlightNode *newNode = (FlightNode *)malloc(sizeof(FlightNode));
    if (!newNode)
    {
        perror("内存分配失败");
        return -1;
    }
    memcpy(&newNode->data, &flight, sizeof(Flight));
    newNode->next = NULL;

    // 找到链表的尾部
    FlightNode *tail = &list->HeadNode;
    while (tail->next)
    {
        tail = tail->next;
    }

    // 将新节点添加到尾部
    tail->next = newNode;
    list->size++;

    return 0;
}

// 第 6 章 机场近似搜索和推荐

// 定义机场节点
typedef struct FlightPort
{
    string Country;
    string Province;
    string Airport;
    int xiangguan;
} FlightPort;

// 定义机场顺序表
typedef struct FlightPortList
{
    FlightPort data[max_airport];
    int sum;
} FlightPortList;

FlightPortList mydata;

// 初始化机场表
int initFlightPort(const char *filename, FlightPortList *list)
{
    FILE *file = fopen(filename, "r");
    char buffer[265];
    fgets(buffer, sizeof(buffer), file); // 忽略第一行
    const char diff[] = ",/";            // 定义分割字符串的分割字符
    while (fgets(buffer, sizeof(buffer), file))
    {
        char *token = strtok(buffer, diff); // 分割字符
        token = strtok(NULL, diff);
        // printf("%s\n",token);
        list->data[list->sum].Country = token;
        token = strtok(NULL, diff);
        list->data[list->sum].Province = token;
        token = strtok(NULL, diff);
        list->data[list->sum].Airport = token;
        list->data[list->sum].xiangguan = 0;
        list->sum++;
    }
    if (list->sum == 79)
    {
        return 0;
    }
    return 1;
}

// 求字符串的最长子串
int longestCommonSubstringLength(const string &str1, const string &str2)
{
    int len1 = str1.size();
    int len2 = str2.size();
    // 创建dp表格，初始化为0
    vector<vector<int>> dp(len1, vector<int>(len2, 0));
    int maxLength = 0;
    // dp算法，计算最长公共字串长度
    for (int i = 0; i < len1; ++i)
    {
        for (int j = 0; j < len2; ++j)
        {
            if (str1[i] == str2[j])
            {
                if (i == 0 || j == 0)
                {
                    dp[i][j] = 1;
                }
                else
                {
                    dp[i][j] = dp[i - 1][j - 1] + 1;
                }
                maxLength = max(maxLength, dp[i][j]);
            }
        }
    }
    return maxLength;
}

// 函数：查找省份相同的机场
bool theSameProvince(int Aira, int Airb)
{
    if (mydata.data[Aira - 1].Province == mydata.data[Airb - 1].Province)
    {
        // cout<<mydata.data[Aira-1].Province<<endl;
        return true;
    }
    return false;
}
// 第6章结束

// 显示符合指定起飞机场和到达机场的所有航班信息
void displayAllFlights(const FlightList *list)
{
    if (list->size == 0)
    {
        printf("航班列表为空。\n");
        return;
    }

    int depAirport, arrAirport;

    // 输入起飞机场ID
    printf("请输入起飞机场ID：");
    if (scanf("%d", &depAirport) != 1)
    {
        printf("无效的输入。\n");
        while (getchar() != '\n')
            ; // 清除输入缓冲区
        return;
    }
    getchar(); // 捕获换行符

    // 输入到达机场ID
    printf("请输入到达机场ID：");
    if (scanf("%d", &arrAirport) != 1)
    {
        printf("无效的输入。\n");
        while (getchar() != '\n')
            ; // 清除输入缓冲区
        return;
    }
    getchar(); // 捕获换行符

    // 创建一个新的 FlightList 来存储筛选后的航班
    FlightList filteredList;
    filteredList.HeadNode.next = NULL;
    filteredList.size = 0;

    // 遍历主链表，筛选符合条件的航班
    FlightNode *current = list->HeadNode.next;
    while (current)
    {
        if (current->data.departureAirport == depAirport &&
            current->data.arrivalAirport == arrAirport)
        {
            if (appendFlightToList(&filteredList, current->data) != 0)
            {
                printf("添加航班ID %d 到过滤列表失败。\n", current->data.flightID);
            }
        }
        current = current->next;
    }

    if (filteredList.size == 0)
    {
        printf("没有找到符合条件的航班。\n");
        current = list->HeadNode.next;
        filteredList.HeadNode.next = NULL;
        filteredList.size = 0;
        while (current)
        {
            if (current->data.departureAirport == depAirport &&
                theSameProvince(current->data.arrivalAirport, arrAirport))
            {
                if (appendFlightToList(&filteredList, current->data) != 0)
                {
                    printf("添加航班ID %d 到过滤列表失败。\n", current->data.flightID);
                }
            }
            current = current->next;
        }
        if (filteredList.size == 0)
        {
            printf("也没有找到推荐航班。\n");
            return;
        }
        else
        {
            printf("找到以下推荐航班，共%d个。\n", filteredList.size);
        }
    }

    // 对过滤后的航班进行标记
    markFlights(&filteredList);

    // 显示过滤后的航班
    printf("\n===== 符合条件的航班 =====\n");
    current = filteredList.HeadNode.next;
    while (current)
    {
        displayFlight(&current->data);
        current = current->next;
    }

    // 释放过滤列表的内存
    current = filteredList.HeadNode.next;
    while (current)
    {
        FlightNode *temp = current;
        current = current->next;
        free(temp);
    }
}

// 用户友好的输入模式：输入新的航班信息
Flight inputNewFlight()
{
    Flight newFlight;
    memset(&newFlight, 0, sizeof(Flight));

    printf("请输入航班ID (整数): ");
    scanf("%d", &newFlight.flightID);
    getchar(); // 捕获换行符

    printf("请输入出发日期 (YYYY-MM-DD): ");
    fgets(newFlight.departureDate, sizeof(newFlight.departureDate), stdin);
    newFlight.departureDate[strcspn(newFlight.departureDate, "\n")] = '\0'; // 去除换行符

    // 修改后的航班类型输入部分，添加输入验证
    while (1)
    {
        printf("请输入航班类型 (Intl/Dome): ");
        char type[10];
        scanf("%9s", type);
        getchar(); // 捕获换行符，防止影响后续输入

        if (strcmp(type, "Intl") == 0)
        {
            newFlight.intlDome = true;
            break;
        }
        else if (strcmp(type, "Dome") == 0)
        {
            newFlight.intlDome = false;
            break;
        }
        else
        {
            printf("输入错误：航班类型必须是 \"Intl\" 或 \"Dome\"。请重新输入。\n");
        }
    }

    printf("请输入航班号: ");
    fgets(newFlight.flightNo, sizeof(newFlight.flightNo), stdin);
    newFlight.flightNo[strcspn(newFlight.flightNo, "\n")] = '\0';

    printf("请输入出发机场ID (整数): ");
    scanf("%d", &newFlight.departureAirport);
    getchar(); // 捕获换行符

    printf("请输入到达机场ID (整数): ");
    scanf("%d", &newFlight.arrivalAirport);
    getchar(); // 捕获换行符

    printf("请输入出发时间 (HH:MM): ");
    fgets(newFlight.departureTime, sizeof(newFlight.departureTime), stdin);
    newFlight.departureTime[strcspn(newFlight.departureTime, "\n")] = '\0';

    // 输入到达日期和到达时间，并进行验证
    while (1)
    {
        printf("请输入到达日期 (YYYY-MM-DD): ");
        fgets(newFlight.arrivalDate, sizeof(newFlight.arrivalDate), stdin);
        newFlight.arrivalDate[strcspn(newFlight.arrivalDate, "\n")] = '\0'; // 去除换行符

        printf("请输入到达时间 (HH:MM): ");
        fgets(newFlight.arrivalTime, sizeof(newFlight.arrivalTime), stdin);
        newFlight.arrivalTime[strcspn(newFlight.arrivalTime, "\n")] = '\0';

        // 转换为 time_t
        time_t departureTime = convertToTimeT(newFlight.departureDate, newFlight.departureTime);
        newFlight.depTime = departureTime;
        time_t arrivalTime = convertToTimeT(newFlight.arrivalDate, newFlight.arrivalTime);
        newFlight.arrTime = arrivalTime;

        if (departureTime == (time_t)-1 || arrivalTime == (time_t)-1)
        {
            printf("输入错误：日期或时间格式不正确。请重新输入。\n");
            continue;
        }

        if (difftime(arrivalTime, departureTime) < 0)
        {
            printf("输入错误：到达日期和时间不能早于出发日期和时间。请重新输入。\n");
            continue;
        }

        break; // 输入合法，退出循环
    }

    printf("请输入飞机ID (整数): ");
    scanf("%d", &newFlight.airplaneID);
    getchar(); // 捕获换行符

    printf("请输入飞机机型 (1-5): ");
    scanf("%d", &newFlight.airplaneModel);
    getchar(); // 捕获换行符

    printf("请输入基础费用 (整数): ");
    scanf("%d", &newFlight.airFares);
    getchar(); // 捕获换行符

    printf("请输入旺季价格 (整数): ");
    scanf("%d", &newFlight.peakSeasonRates);
    getchar(); // 捕获换行符

    printf("请输入淡季价格 (整数): ");
    scanf("%d", &newFlight.offSeasonRates);
    getchar(); // 捕获换行符

    // 初始化标记字段
    newFlight.isCheapest = false;
    newFlight.isShortestDuration = false;
    newFlight.isNextDayArrival = false;

    return newFlight;
}

// 用户友好的输入模式：输入修改后的航班信息
Flight inputUpdatedFlight()
{
    // 类似于 inputNewFlight，但不输入 flightID
    Flight updatedFlight;
    memset(&updatedFlight, 0, sizeof(Flight));

    printf("请输入出发日期 (YYYY-MM-DD): ");
    fgets(updatedFlight.departureDate, sizeof(updatedFlight.departureDate), stdin);
    updatedFlight.departureDate[strcspn(updatedFlight.departureDate, "\n")] = '\0'; // 去除换行符

    // 修改后的航班类型输入部分，添加输入验证
    while (1)
    {
        printf("请输入航班类型 (Intl/Dome): ");
        char type[10];
        scanf("%9s", type);
        getchar(); // 捕获换行符，防止影响后续输入

        if (strcmp(type, "Intl") == 0)
        {
            updatedFlight.intlDome = true;
            break;
        }
        else if (strcmp(type, "Dome") == 0)
        {
            updatedFlight.intlDome = false;
            break;
        }
        else
        {
            printf("输入错误：航班类型必须是 \"Intl\" 或 \"Dome\"。请重新输入。\n");
        }
    }

    printf("请输入航班号: ");
    fgets(updatedFlight.flightNo, sizeof(updatedFlight.flightNo), stdin);
    updatedFlight.flightNo[strcspn(updatedFlight.flightNo, "\n")] = '\0';

    printf("请输入出发机场ID (整数): ");
    scanf("%d", &updatedFlight.departureAirport);
    getchar(); // 捕获换行符

    printf("请输入到达机场ID (整数): ");
    scanf("%d", &updatedFlight.arrivalAirport);
    getchar(); // 捕获换行符

    printf("请输入出发时间 (HH:MM): ");
    fgets(updatedFlight.departureTime, sizeof(updatedFlight.departureTime), stdin);
    updatedFlight.departureTime[strcspn(updatedFlight.departureTime, "\n")] = '\0';

    // 输入到达日期和到达时间，并进行验证
    while (1)
    {
        printf("请输入到达日期 (YYYY-MM-DD): ");
        fgets(updatedFlight.arrivalDate, sizeof(updatedFlight.arrivalDate), stdin);
        updatedFlight.arrivalDate[strcspn(updatedFlight.arrivalDate, "\n")] = '\0'; // 去除换行符

        printf("请输入到达时间 (HH:MM): ");
        fgets(updatedFlight.arrivalTime, sizeof(updatedFlight.arrivalTime), stdin);
        updatedFlight.arrivalTime[strcspn(updatedFlight.arrivalTime, "\n")] = '\0';

        // 转换为 time_t
        time_t departureTime = convertToTimeT(updatedFlight.departureDate, updatedFlight.departureTime);
        updatedFlight.depTime = departureTime;
        time_t arrivalTime = convertToTimeT(updatedFlight.arrivalDate, updatedFlight.arrivalTime);
        updatedFlight.arrTime = arrivalTime;

        if (departureTime == (time_t)-1 || arrivalTime == (time_t)-1)
        {
            printf("输入错误：日期或时间格式不正确。请重新输入。\n");
            continue;
        }

        if (difftime(arrivalTime, departureTime) < 0)
        {
            printf("输入错误：到达日期和时间不能早于出发日期和时间。请重新输入。\n");
            continue;
        }

        break; // 输入合法，退出循环
    }

    printf("请输入飞机ID (整数): ");
    scanf("%d", &updatedFlight.airplaneID);
    getchar(); // 捕获换行符

    printf("请输入飞机机型 (1-5): ");
    scanf("%d", &updatedFlight.airplaneModel);
    getchar(); // 捕获换行符

    printf("请输入基础费用 (整数): ");
    scanf("%d", &updatedFlight.airFares);
    getchar(); // 捕获换行符

    printf("请输入旺季价格 (整数): ");
    scanf("%d", &updatedFlight.peakSeasonRates);
    getchar(); // 捕获换行符

    printf("请输入淡季价格 (整数): ");
    scanf("%d", &updatedFlight.offSeasonRates);
    getchar(); // 捕获换行符

    // 初始化标记字段
    updatedFlight.isCheapest = false;
    updatedFlight.isShortestDuration = false;
    updatedFlight.isNextDayArrival = false;

    return updatedFlight;
}

// 主菜单
void displayMenu()
{
    printf("\n===== 航班管理系统 =====\n");
    printf("1. 显示航班\n");
    printf("2. 添加航班\n");
    printf("3. 删除航班\n");
    printf("4. 修改航班\n");
    printf("5. 退出\n");
    printf("6. 实现第 6 章 机场近似搜索和推荐\n");
    printf("7. 实现第 7 章 最繁忙的机场\n");
    printf("8. 实现第 8 章 检查连通性\n");
    printf("9. 实现第 8 章 检查有时间限制的连通性\n");
    printf("10. 实现第 8 章 最优乘机方案\n");
    printf("11. 实现第 8 章 小明的乘机方案\n");
    printf("请选择操作（1-11）：");
}

// 第 7 章 最繁忙的机场 + 第 8 章 乘机方案查询

// 这个题是有向图，且不同机场之间的航班较多，邻接矩阵相比邻接表更适合
// 考虑到同一对机场之间可能有多次航班，故使用带权重的邻接矩阵

// 定义邻接矩阵
typedef struct Airport
{
    int ariport;
    int in;
    int out;
    int sum;
} Airport;

typedef struct AirportGraph
{
    int graph[max_airport][max_airport]; // 包含航班个数的邻接矩阵
    int graph_in[max_airport];           // 统计机场进入数量
    int graph_out[max_airport];          // 统计机场出去数量
} AirportGraph;

AirportGraph graph = {{0}, {0}, {0}};                // 没有时间限制的邻接矩阵
AirportGraph graph_limit = {{0}, {0}, {0}};          // 时间限制的邻接矩阵
FlightList graphlist[max_airport][max_airport];      // 包含航班信息的邻接链表矩阵，两个机场之间的点用链表的形式储存
FlightList graphlistlimit[max_airport][max_airport]; // 时间限制的链表矩阵
Airport airport[max_airport] = {0};
// 初始化链表矩阵
void initGraphList(FlightList graphlist[max_airport][max_airport])
{
    for (int i = 0; i < max_airport; i++)
    {
        for (int j = 0; j < max_airport; j++)
        {
            graphlist[i][j].size = 0;
            graphlist[i][j].HeadNode.next = NULL;
        }
    }
}

// 初始化邻接矩阵
void initGraph(const FlightList *list)
{
    initGraphList(graphlist);
    FlightNode *current = list->HeadNode.next;
    while (current)
    {
        graph.graph[current->data.departureAirport][current->data.arrivalAirport]++;
        graph.graph_in[current->data.arrivalAirport]++;
        graph.graph_out[current->data.departureAirport]++;
        FlightNode *newnode = (FlightNode *)malloc(sizeof(FlightNode));
        newnode->data = current->data;
        newnode->next = graphlist[current->data.departureAirport][current->data.arrivalAirport].HeadNode.next;
        graphlist[current->data.departureAirport][current->data.arrivalAirport].HeadNode.next = newnode;
        graphlist[current->data.departureAirport][current->data.arrivalAirport].size++;
        current = current->next;
    }
    memset(&airport, 0, sizeof(airport));
    for (int i = 1; i <= mydata.sum; i++)
    {
        airport[i].ariport = i;
        airport[i].in = graph.graph_in[i];
        airport[i].out = graph.graph_out[i];
        airport[i].sum = airport[i].in + airport[i].out;
    }
}

// 按照繁忙程度排序输出
void sortGraph()
{
    sort(airport + 1, airport + mydata.sum, [](const Airport &a, const Airport &b)
         { return a.sum > b.sum; });
    for (int i = 1; i < mydata.sum; i++)
    {
        printf("机场ID：%d\n", airport[i].ariport);
        printf("机场进入航班：%d\n", airport[i].in);
        printf("机场出去航班：%d\n", airport[i].out);
        printf("机场一共航班：%d\n", airport[i].sum);
        printf("\n");
    }
}

// 初始化被筛选的邻接矩阵
void initLimitGraph(const FlightList *list, const time_t lowD, const time_t highD, const time_t lowA, const time_t highA)
{
    memset(&graph_limit, 0, sizeof(graph_limit));
    FlightNode *current = list->HeadNode.next;
    while (current)
    {
        time_t dep = convertToTimeT(current->data.departureDate, current->data.departureTime);
        time_t arr = convertToTimeT(current->data.arrivalDate, current->data.arrivalTime);
        if (difftime(dep, lowD) >= 0 && difftime(dep, highD) <= 0)
        {
            graph_limit.graph[current->data.departureAirport][current->data.arrivalAirport]++;
            graph_limit.graph_out[current->data.departureAirport]++;
        }
        if (difftime(arr, lowA) >= 0 && difftime(arr, highA) <= 0)
        {
            graph_limit.graph[current->data.departureAirport][current->data.arrivalAirport]++;
            graph_limit.graph_in[current->data.arrivalAirport]++;
        }

        current = current->next;
    }
    memset(&airport, 0, sizeof(airport));
    for (int i = 1; i <= mydata.sum; i++)
    {
        airport[i].ariport = i;
        airport[i].in = graph_limit.graph_in[i];
        airport[i].out = graph_limit.graph_out[i];
        airport[i].sum = airport[i].in + airport[i].out;
    }
}
// 第7章结束

// 第 8 章 乘机方案查询

// 部分的代码改动在这之上的第7章代码部分

// 路径结构体
typedef struct transfer
{
    time_t time;
    int cost;
    int trans;
    FlightNode Head;
} transfer;

vector<transfer> allPath; // 记录所有可行的路径
transfer recent;          // 记录dfs正在进行的路径

// 辅助函数：深拷贝 FlightNode 链表
FlightNode *copyList(FlightNode *head)
{
    if (!head)
        return nullptr;

    // 创建一个虚拟头节点
    FlightNode *dummy = new FlightNode();
    dummy->next = nullptr;
    FlightNode *currentNew = dummy;
    FlightNode *currentOld = head;

    while (currentOld)
    {
        FlightNode *newNode = new FlightNode();
        newNode->data = currentOld->data;
        newNode->next = nullptr;
        currentNew->next = newNode;
        currentNew = newNode;
        currentOld = currentOld->next;
    }

    FlightNode *copiedHead = dummy->next;
    delete dummy; // 释放虚拟头节点
    return copiedHead;
}

// 辅助函数：清理allPath
void clearAllPath()
{
    for (auto &path : allPath)
    {
        FlightNode *current = path.Head.next;
        while (current)
        {
            FlightNode *temp = current;
            current = current->next;
            delete temp;
        }
        path.Head.next = nullptr;
    }
    allPath.clear();
}

// 辅助函数：将秒数格式化为 "D days H hours M mins"
string formatTime(time_t seconds)
{
    int days = seconds / (24 * 3600);
    seconds %= (24 * 3600);
    int hours = seconds / 3600;
    seconds %= 3600;
    int minutes = seconds / 60;

    ostringstream oss;
    oss << days << " days " << hours << " hours " << minutes << " mins";
    return oss.str();
}

// 显示所有路径信息的函数
void showPath()
{
    // 输出总选项数量
    if (allPath.size() == 0)
    {
        printf("There are no options that meet that meet this query.\n");
    }
    else
    {
        cout << "Options: " << allPath.size() << endl;

        // 遍历 allPath 中的每一个 transfer
        for (int optionNum = 1; optionNum <= allPath.size(); ++optionNum)
        {
            transfer &path = allPath[optionNum - 1];

            // 计算转机次数（trans 表示航班数量，转机次数为 trans - 1）
            int transfers = (path.trans > 0) ? path.trans - 1 : 0;

            // 格式化总时间和总费用
            string formattedTime = formatTime(path.time);
            int totalCost = path.cost;

            // 输出选项的关键信息
            cout << "[option" << optionNum << "]: "
                      << transfers << " transfers, "
                      << formattedTime << ", "
                      << totalCost << " dollars" << endl;

            // 收集航班节点
            vector<FlightNode *> flights;
            FlightNode *current = path.Head.next;
            while (current)
            {
                flights.push_back(current);
                current = current->next;
            }

            // 由于航班节点是从后向前添加的，需要反转以获得正确顺序
            reverse(flights.begin(), flights.end());

            // 收集航班ID
            string flightIDs;
            for (size_t i = 0; i < flights.size(); ++i)
            {
                flightIDs += to_string(flights[i]->data.flightID);
                if (i != flights.size() - 1)
                {
                    flightIDs += " -> ";
                }
            }

            // 计算并收集换乘时间
            vector<string> layovers;
            for (size_t i = 0; i < flights.size() - 1; ++i)
            {
                time_t layover = flights[i + 1]->data.depTime - flights[i]->data.arrTime;
                layovers.push_back(formatTime(layover));
            }

            // 输出航班ID和换乘时间
            cout << flightIDs;
            if (!layovers.empty())
            {
                cout << " (";
                for (size_t i = 0; i < layovers.size(); ++i)
                {
                    cout << layovers[i];
                    if (i != layovers.size() - 1)
                    {
                        cout << ", ";
                    }
                }
                cout << ")";
            }
            cout << endl;
        }
    }
}


// 问题（1）

// 测试能否添加新的路径到节点
bool available(transfer path, FlightNode *flight)
{
    FlightNode *current = path.Head.next;
    if (path.trans > 0)
    {
        if (current->data.arrTime + 1800 > flight->data.depTime)
        {
            return false;
        }
    }

    while (current)
    {
        if (current->data.departureAirport == flight->data.arrivalAirport)
        {
            return false;
        }
        current = current->next;
    }
    return true;
}

// 没有时间限制的深度优先
void dfsNo(int depAir, int arrAir, int depth)
{
    if (depAir == arrAir)
    {
        if (depAir == arrAir)
        {
            transfer newP;
            newP.cost = recent.cost;
            newP.Head.next = copyList(recent.Head.next); // 使用深拷贝
            newP.trans = recent.trans;
            FlightNode *current = recent.Head.next;
            // 找到最初的航班
            while (current->next)
            {
                current = current->next;
            }
            newP.time = recent.Head.next->data.arrTime - current->data.depTime;
            allPath.push_back(newP);
            return;
        }
    }
    if (depth < 0)
    {
        return;
    }
    // 如果机场AB之间存在路径，就检查航班链表邻接矩阵，找到所有符合要求的航班
    for (int i = 0; i < max_airport; i++)
    {
        if (graphlist[depAir][i].size)
        {
            FlightNode *current = graphlist[depAir][i].HeadNode.next;
            while (current)
            {
                if (available(recent, current))
                {
                    recent.cost += current->data.airFares;
                    recent.trans++;
                    FlightNode *newnode = (FlightNode *)malloc(sizeof(FlightNode));
                    newnode->data = current->data;
                    newnode->next = recent.Head.next;
                    recent.Head.next = newnode;
                    dfsNo(i, arrAir, depth - 1);
                    recent.cost -= current->data.airFares;
                    recent.trans--;
                    recent.Head.next = recent.Head.next->next;
                    free(newnode);
                }
                current = current->next;
            }
        }
    }
}

// 辅助函数：清理recent head
void clearRecentHead()
{
    FlightNode *current = recent.Head.next;
    while (current)
    {
        FlightNode *temp = current;
        current = current->next;
        free(temp);
    }
    recent.Head.next = nullptr;
    recent.cost = 0;
    recent.trans = 0;
    recent.time = 0;
}

// 筛选 allPath 中符合时间要求的路径
void sortAllPath(time_t depL, time_t depH, time_t arrL, time_t arrH)
{
    allPath.erase(
        remove_if(allPath.begin(), allPath.end(), [&](const transfer &path) -> bool
                       {
            FlightNode *current = path.Head.next;
            while (current->next)
            {
                current=current->next;
            }
            time_t firstDepTime = current->data.depTime;
            time_t lastArrTime = path.Head.next->data.arrTime;

            // 检查是否在指定时间范围内
            bool depInRange = (firstDepTime >= depL) && (firstDepTime <= depH);
            bool arrInRange = (lastArrTime >= arrL) && (lastArrTime <= arrH);
            return !(depInRange && arrInRange); }),
        allPath.end());
}

// 没有时间限制的检查连通性
void allPathNo(int depAir, int arrAir, int depth)
{
    clearAllPath();
    clearRecentHead();
    dfsNo(depAir, arrAir, depth);
    showPath();
}

// 有时间限制的检查连通性
void allPathYes(int depAir, int arrAir, int depth, time_t depL, time_t depH, time_t arrL, time_t arrH)
{
    clearAllPath();
    clearRecentHead();
    dfsNo(depAir, arrAir, depth);
    sortAllPath(depL, depH, arrL, arrH);
    showPath();
}


// 问题（2）


time_t mintime;
// 按照最大时间进行剪枝的的深度优先遍历
void dfsTime(int depAir, int arrAir, int depth)
{
    if (recent.time > mintime)
    {
        return;
    }
    if (depAir == arrAir)
    {
        if (depAir == arrAir)
        {
            transfer newP;
            newP.cost = recent.cost;
            newP.Head.next = copyList(recent.Head.next); // 使用深拷贝
            newP.trans = recent.trans;
            newP.time = recent.time;
            if (recent.time < mintime)
            {
                mintime = recent.time;
                clearAllPath();
            }
            allPath.push_back(newP);
            return;
        }
    }
    if (depth < 0)
    {
        return;
    }
    // 如果机场AB之间存在路径，就检查航班链表邻接矩阵，找到所有符合要求的航班
    for (int i = 0; i < max_airport; i++)
    {
        if (graphlist[depAir][i].size)
        {
            FlightNode *current = graphlist[depAir][i].HeadNode.next;
            while (current)
            {
                if (available(recent, current))
                {
                    recent.cost += current->data.airFares;
                    int addtime = (recent.trans>0) ? current->data.arrTime - recent.Head.next->data.arrTime : current->data.arrTime - current->data.depTime;
                    recent.time+=addtime;
                    recent.trans++;
                    FlightNode *newnode = (FlightNode *)malloc(sizeof(FlightNode));
                    newnode->data = current->data;
                    newnode->next = recent.Head.next;
                    recent.Head.next = newnode;
                    dfsTime(i, arrAir, depth - 1);
                    recent.cost -= current->data.airFares;
                    recent.time-=addtime;
                    recent.trans--;
                    recent.Head.next = recent.Head.next->next;
                    free(newnode);
                }
                current = current->next;
            }
        }
    }
}

// 寻找用时最短的路径
void allPathTime(int depAir, int arrAir)
{
    clearAllPath();
    clearRecentHead();
    mintime = INT_MAX;
    dfsTime(depAir, arrAir, max_airport);
    showPath();
}

int minmoney;
// 按照最大时间进行剪枝的的深度优先遍历
void dfsMoney(int depAir, int arrAir, int depth)
{
    if (recent.cost > minmoney)
    {
        return;
    }
    if (depAir == arrAir)
    {
        if (depAir == arrAir)
        {
            transfer newP;
            newP.cost = recent.cost;
            newP.Head.next = copyList(recent.Head.next); // 使用深拷贝
            newP.trans = recent.trans;
            newP.time = recent.time;
            if (recent.cost < minmoney)
            {
                minmoney = recent.cost;
                clearAllPath();
            }
            allPath.push_back(newP);
            return;
        }
    }
    if (depth < 0)
    {
        return;
    }
    // 如果机场AB之间存在路径，就检查航班链表邻接矩阵，找到所有符合要求的航班
    for (int i = 0; i < max_airport; i++)
    {
        if (graphlist[depAir][i].size)
        {
            FlightNode *current = graphlist[depAir][i].HeadNode.next;
            while (current)
            {
                if (available(recent, current))
                {
                    recent.cost += current->data.airFares;
                    int addtime = (recent.trans>0) ? current->data.arrTime - recent.Head.next->data.arrTime : current->data.arrTime - current->data.depTime;
                    recent.time+=addtime;
                    recent.trans++;
                    FlightNode *newnode = (FlightNode *)malloc(sizeof(FlightNode));
                    newnode->data = current->data;
                    newnode->next = recent.Head.next;
                    recent.Head.next = newnode;
                    dfsMoney(i, arrAir, depth - 1);
                    recent.cost -= current->data.airFares;
                    recent.time-=addtime;
                    recent.trans--;
                    recent.Head.next = recent.Head.next->next;
                    free(newnode);
                }
                current = current->next;
            }
        }
    }
}

// 寻找花钱最短的路径
void allPathMoney(int depAir, int arrAir)
{
    clearAllPath();
    clearRecentHead();
    minmoney = INT_MAX;
    dfsMoney(depAir, arrAir, max_airport);
    showPath();
}


// 问题（3）
// 检查小明能否进行转机
bool availableXiao(transfer path, FlightNode *flight, time_t depT, time_t arrT)
{
    if (flight->data.depTime<depT || flight->data.arrTime > arrT)
    {
        return false;
    }
    
    FlightNode *current = path.Head.next;
    if (path.trans > 0)
    {
        if (current->data.arrTime + 1800 > flight->data.depTime)
        {
            return false;
        }
    }

    while (current)
    {
        if (current->data.departureAirport == flight->data.arrivalAirport)
        {
            return false;
        }
        current = current->next;
    }
    return true;
}
// 深度优先检索小明节省时间方案
void dfsxiaoTime(int depAir, int arrAir, int depth, time_t depT, time_t arrT)
{
    if (recent.time > mintime)
    {
        return;
    }
    if (depAir == arrAir)
    {
        if (depAir == arrAir)
        {
            transfer newP;
            newP.cost = recent.cost;
            newP.Head.next = copyList(recent.Head.next); // 使用深拷贝
            newP.trans = recent.trans;
            newP.time = recent.time;
            if (recent.time < mintime)
            {
                mintime = recent.time;
                clearAllPath();
            }
            allPath.push_back(newP);
            return;
        }
    }
    if (depth < 0)
    {
        return;
    }
    // 如果机场AB之间存在路径，就检查航班链表邻接矩阵，找到所有符合要求的航班
    for (int i = 0; i < max_airport; i++)
    {
        if (graphlist[depAir][i].size)
        {
            FlightNode *current = graphlist[depAir][i].HeadNode.next;
            while (current)
            {
                if (availableXiao(recent, current, depT, arrT))
                {
                    recent.cost += current->data.airFares;
                    int addtime = (recent.trans>0) ? current->data.arrTime - recent.Head.next->data.arrTime : current->data.arrTime - current->data.depTime;
                    recent.time+=addtime;
                    recent.trans++;
                    FlightNode *newnode = (FlightNode *)malloc(sizeof(FlightNode));
                    newnode->data = current->data;
                    newnode->next = recent.Head.next;
                    recent.Head.next = newnode;
                    dfsxiaoTime(i, arrAir, depth - 1, depT, arrT);
                    recent.cost -= current->data.airFares;
                    recent.time-=addtime;
                    recent.trans--;
                    recent.Head.next = recent.Head.next->next;
                    free(newnode);
                }
                current = current->next;
            }
        }
    }
}

// 输出小明节省时间方案
void xiaoTime(int depAir, int arrAir, int N, time_t depT, time_t arrT)
{
    clearAllPath();
    clearRecentHead();
    mintime = INT_MAX;
    dfsxiaoTime(depAir,arrAir,N,depT,arrT);
    showPath();
}


// 深度优先检索小明省钱方案
void dfsxiaoMoney(int depAir, int arrAir, int depth, time_t depT, time_t arrT)
{
    if (recent.cost > minmoney)
    {
        return;
    }
    if (depAir == arrAir)
    {
        if (depAir == arrAir)
        {
            transfer newP;
            newP.cost = recent.cost;
            newP.Head.next = copyList(recent.Head.next); // 使用深拷贝
            newP.trans = recent.trans;
            newP.time = recent.time;
            if (recent.cost < minmoney)
            {
                minmoney = recent.cost;
                clearAllPath();
            }
            allPath.push_back(newP);
            return;
        }
    }
    if (depth < 0)
    {
        return;
    }
    // 如果机场AB之间存在路径，就检查航班链表邻接矩阵，找到所有符合要求的航班
    for (int i = 0; i < max_airport; i++)
    {
        if (graphlist[depAir][i].size)
        {
            FlightNode *current = graphlist[depAir][i].HeadNode.next;
            while (current)
            {
                if (availableXiao(recent, current, depT, arrT))
                {
                    recent.cost += current->data.airFares;
                    int addtime = (recent.trans>0) ? current->data.arrTime - recent.Head.next->data.arrTime : current->data.arrTime - current->data.depTime;
                    recent.time+=addtime;
                    recent.trans++;
                    FlightNode *newnode = (FlightNode *)malloc(sizeof(FlightNode));
                    newnode->data = current->data;
                    newnode->next = recent.Head.next;
                    recent.Head.next = newnode;
                    dfsxiaoMoney(i, arrAir, depth - 1,depT,arrT);
                    recent.cost -= current->data.airFares;
                    recent.time-=addtime;
                    recent.trans--;
                    recent.Head.next = recent.Head.next->next;
                    free(newnode);
                }
                current = current->next;
            }
        }
    }
}

// 输出小明节省钱方案
void xiaoMoney(int depAir, int arrAir, int N, time_t depT, time_t arrT)
{
    clearAllPath();
    clearRecentHead();
    minmoney = INT_MAX;
    dfsxiaoMoney(depAir, arrAir, N,depT,arrT);
    showPath();
}

// 第8章结束
// 主程序
int main()
{
    FlightList flightList;
    memset(&flightList, 0, sizeof(FlightList));

    // 初始化航班列表
    const char *filename = "./data/flight-data.csv"; // 请确保文件存在且格式正确
    if (initializeFlightList(filename, &flightList) != 0)
    {
        fprintf(stderr, "航班数据初始化失败。\n");
        exit(0);
    }

    mydata.sum = 0;
    const char *filename2 = "./data/id2name.csv";
    if (initFlightPort(filename2, &mydata) != 0)
    {
        fprintf(stderr, "航班数据初始化失败。\n");
        return EXIT_FAILURE;
    } // 初始化mydata列表，存储机场国家等数据
    // 标记航班
    markFlights(&flightList);

    int choice;
    while (1)
    {
        displayMenu();
        if (scanf("%d", &choice) != 1)
        {
            printf("无效的输入，请输入数字。\n");
            while (getchar() != '\n')
                ; // 清除输入缓冲区
            continue;
        }
        getchar(); // 捕获换行符

        if (choice == 1)
        {
            displayAllFlights(&flightList);
        }
        else if (choice == 2)
        {
            Flight newFlight = inputNewFlight();
            if (addFlight(&flightList, newFlight) == 0)
            {
                markFlights(&flightList);
            }
        }
        else if (choice == 3)
        {
            printf("请输入要删除的航班ID：");
            int delID;
            if (scanf("%d", &delID) != 1)
            {
                printf("无效的输入。\n");
                while (getchar() != '\n')
                    ;
                continue;
            }
            getchar(); // 捕获换行符
            if (deleteFlight(&flightList, delID) == 0)
            {
                markFlights(&flightList);
            }
        }
        else if (choice == 4)
        {
            printf("请输入要修改的航班ID：");
            int modID;
            if (scanf("%d", &modID) != 1)
            {
                printf("无效的输入。\n");
                while (getchar() != '\n')
                    ;
                continue;
            }
            getchar(); // 捕获换行符

            Flight updatedFlight = inputUpdatedFlight();
            if (modifyFlight(&flightList, modID, updatedFlight) == 0)
            {
                markFlights(&flightList);
            }
        }
        else if (choice == 5)
        {
            printf("退出系统。\n");
            break;
        }
        else if (choice == 6)
        {
            printf("直接输入机场名称！\n");
            string input;
            getline(cin, input);
            for (int i = 0; i < mydata.sum; i++)
            {
                int x = longestCommonSubstringLength(mydata.data[i].Airport, input);
                int y = longestCommonSubstringLength(mydata.data[i].Province, input);
                int z = longestCommonSubstringLength(mydata.data[i].Country, input);
                mydata.data[i].xiangguan = max(max(x, y), z); // 计算相关系数
            }
            FlightPortList temp = mydata;
            sort(temp.data, temp.data + temp.sum, [](const FlightPort &a, const FlightPort &b)
                 { return a.xiangguan > b.xiangguan; }); // 按照相关系数排序
            printf("Top 5 most similar airport names are as follow:\n");
            for (int i = 0; i < 5; i++)
            {
                cout << temp.data[i].Airport;
            }
        }
        else if (choice == 7)
        {
            initGraph(&flightList);
            sortGraph();
            while (1)
            {
                int choice = 0;
                printf("是否要按照时间筛选航班？\n");
                scanf("%d", &choice);
                if (!choice)
                {
                    break;
                }
                getchar();
                // 输入截至和开始时间范围，并进行验证
                time_t TimelowD, TimehighD;
                while (1)
                {
                    char lowDateD[12], lowTimeD[7], highDateD[12], highTimeD[7];

                    printf("请输入起飞开始日期 (YYYY-MM-DD): ");
                    fgets(lowDateD, sizeof(lowDateD), stdin);
                    lowDateD[strcspn(lowDateD, "\n")] = '\0';

                    printf("请输入起飞开始时间 (HH:MM): ");
                    fgets(lowTimeD, sizeof(lowTimeD), stdin);
                    lowTimeD[strcspn(lowTimeD, "\n")] = '\0';

                    printf("请输入起飞截至日期 (YYYY-MM-DD): ");
                    fgets(highDateD, sizeof(highDateD), stdin);
                    highDateD[strcspn(highDateD, "\n")] = '\0';

                    printf("请输入起飞截至时间 (HH:MM): ");
                    fgets(highTimeD, sizeof(highTimeD), stdin);
                    highTimeD[strcspn(highTimeD, "\n")] = '\0';

                    // 转换为 time_t
                    TimelowD = convertToTimeT(lowDateD, lowTimeD);
                    TimehighD = convertToTimeT(highDateD, highTimeD);

                    if (TimelowD == (time_t)-1 || TimehighD == (time_t)-1)
                    {
                        printf("输入错误：日期或时间格式不正确。请重新输入。\n");
                        continue;
                    }

                    if (difftime(TimehighD, TimelowD) < 0)
                    {
                        printf("输入错误：截至日期和时间不能早于开始日期和时间。请重新输入。\n");
                        continue;
                    }

                    break; // 输入合法，退出循环
                }
                time_t TimelowA, TimehighA;
                while (1)
                {
                    char lowDateA[12], lowTimeA[7], highDateA[12], highTimeA[7];

                    printf("请输入到达开始日期 (YYYY-MM-DD): ");
                    fgets(lowDateA, sizeof(lowDateA), stdin);
                    lowDateA[strcspn(lowDateA, "\n")] = '\0';

                    printf("请输入到达开始时间 (HH:MM): ");
                    fgets(lowTimeA, sizeof(lowTimeA), stdin);
                    lowTimeA[strcspn(lowTimeA, "\n")] = '\0';

                    printf("请输入到达截至日期 (YYYY-MM-DD): ");
                    fgets(highDateA, sizeof(highDateA), stdin);
                    highDateA[strcspn(highDateA, "\n")] = '\0';

                    printf("请输入到达截至时间 (HH:MM): ");
                    fgets(highTimeA, sizeof(highTimeA), stdin);
                    highTimeA[strcspn(highTimeA, "\n")] = '\0';

                    // 转换为 time_t
                    TimelowA = convertToTimeT(lowDateA, lowTimeA);
                    TimehighA = convertToTimeT(highDateA, highTimeA);

                    if (TimelowA == (time_t)-1 || TimehighA == (time_t)-1)
                    {
                        printf("输入错误：日期或时间格式不正确。请重新输入。\n");
                        continue;
                    }

                    if (difftime(TimehighA, TimelowA) < 0)
                    {
                        printf("输入错误：截至日期和时间不能早于开始日期和时间。请重新输入。\n");
                        continue;
                    }

                    break; // 输入合法，退出循环
                }
                initLimitGraph(&flightList, TimelowD, TimehighD, TimelowA, TimehighA);
                sortGraph();
            }
        }
        else if (choice == 8)
        {
            initGraph(&flightList);
            int depAirport, arrAirport;

            // 输入起飞机场ID
            printf("请输入起飞机场ID：");
            if (scanf("%d", &depAirport) != 1)
            {
                printf("无效的输入。\n");
                while (getchar() != '\n')
                    ; // 清除输入缓冲区
            }
            getchar(); // 捕获换行符

            // 输入到达机场ID
            printf("请输入到达机场ID：");
            if (scanf("%d", &arrAirport) != 1)
            {
                printf("无效的输入。\n");
                while (getchar() != '\n')
                    ; // 清除输入缓冲区
            }
            getchar(); // 捕获换行符

            allPathNo(depAirport, arrAirport, 1);
        }
        else if (choice == 9)
        {
            initGraph(&flightList);
            int depAirport, arrAirport;

            // 输入起飞机场ID
            printf("请输入起飞机场ID：");
            if (scanf("%d", &depAirport) != 1)
            {
                printf("无效的输入。\n");
                while (getchar() != '\n')
                    ; // 清除输入缓冲区
            }
            getchar(); // 捕获换行符

            // 输入到达机场ID
            printf("请输入到达机场ID：");
            if (scanf("%d", &arrAirport) != 1)
            {
                printf("无效的输入。\n");
                while (getchar() != '\n')
                    ; // 清除输入缓冲区
            }
            getchar(); // 捕获换行符

            time_t TimelowD = convertToTimeT("2017-01-01", "12:00"), TimehighD = convertToTimeT("2018-01-01", "12:00");
            while (1)
            {
                int choice = 0;
                printf("是否要限制起飞时间？\n");
                scanf("%d", &choice);
                if (!choice)
                {
                    break;
                }
                getchar();
                char lowDateD[12], lowTimeD[7], highDateD[12], highTimeD[7];

                printf("请输入起飞开始日期 (YYYY-MM-DD): ");
                fgets(lowDateD, sizeof(lowDateD), stdin);
                lowDateD[strcspn(lowDateD, "\n")] = '\0';

                printf("请输入起飞开始时间 (HH:MM): ");
                fgets(lowTimeD, sizeof(lowTimeD), stdin);
                lowTimeD[strcspn(lowTimeD, "\n")] = '\0';

                printf("请输入起飞截至日期 (YYYY-MM-DD): ");
                fgets(highDateD, sizeof(highDateD), stdin);
                highDateD[strcspn(highDateD, "\n")] = '\0';

                printf("请输入起飞截至时间 (HH:MM): ");
                fgets(highTimeD, sizeof(highTimeD), stdin);
                highTimeD[strcspn(highTimeD, "\n")] = '\0';

                // 转换为 time_t
                TimelowD = convertToTimeT(lowDateD, lowTimeD);
                TimehighD = convertToTimeT(highDateD, highTimeD);

                if (TimelowD == (time_t)-1 || TimehighD == (time_t)-1)
                {
                    printf("输入错误：日期或时间格式不正确。请重新输入。\n");
                    TimelowD = convertToTimeT("2017-01-01", "12:00"), TimehighD = convertToTimeT("2018-01-01", "12:00");
                    continue;
                }

                if (difftime(TimehighD, TimelowD) < 0)
                {
                    printf("输入错误：截至日期和时间不能早于开始日期和时间。请重新输入。\n");
                    TimelowD = convertToTimeT("2017-01-01", "12:00"), TimehighD = convertToTimeT("2018-01-01", "12:00");
                    continue;
                }

                break; // 输入合法，退出循环
            }
            time_t TimelowA = convertToTimeT("2017-01-01", "12:00"), TimehighA = convertToTimeT("2018-01-01", "12:00");
            while (1)
            {
                int choice = 0;
                printf("是否限制降落时间？\n");
                scanf("%d", &choice);
                if (!choice)
                {
                    break;
                }
                getchar();
                char lowDateA[12], lowTimeA[7], highDateA[12], highTimeA[7];

                printf("请输入到达开始日期 (YYYY-MM-DD): ");
                fgets(lowDateA, sizeof(lowDateA), stdin);
                lowDateA[strcspn(lowDateA, "\n")] = '\0';

                printf("请输入到达开始时间 (HH:MM): ");
                fgets(lowTimeA, sizeof(lowTimeA), stdin);
                lowTimeA[strcspn(lowTimeA, "\n")] = '\0';

                printf("请输入到达截至日期 (YYYY-MM-DD): ");
                fgets(highDateA, sizeof(highDateA), stdin);
                highDateA[strcspn(highDateA, "\n")] = '\0';

                printf("请输入到达截至时间 (HH:MM): ");
                fgets(highTimeA, sizeof(highTimeA), stdin);
                highTimeA[strcspn(highTimeA, "\n")] = '\0';

                // 转换为 time_t
                TimelowA = convertToTimeT(lowDateA, lowTimeA);
                TimehighA = convertToTimeT(highDateA, highTimeA);

                if (TimelowA == (time_t)-1 || TimehighA == (time_t)-1)
                {
                    printf("输入错误：日期或时间格式不正确。请重新输入。\n");
                    TimelowA = convertToTimeT("2017-01-01", "12:00"), TimehighA = convertToTimeT("2018-01-01", "12:00");
                    continue;
                }

                if (difftime(TimehighA, TimelowA) < 0)
                {
                    printf("输入错误：截至日期和时间不能早于开始日期和时间。请重新输入。\n");
                    TimelowA = convertToTimeT("2017-01-01", "12:00"), TimehighA = convertToTimeT("2018-01-01", "12:00");
                    continue;
                }

                break; // 输入合法，退出循环
            }

            allPathYes(depAirport, arrAirport, 1, TimelowD, TimehighD, TimelowA, TimehighA);
        }
        else if (choice == 10)
        {
            initGraph(&flightList);
            int depAirport, arrAirport;

            // 输入起飞机场ID
            printf("请输入起飞机场ID：");
            if (scanf("%d", &depAirport) != 1)
            {
                printf("无效的输入。\n");
                while (getchar() != '\n')
                    ; // 清除输入缓冲区
            }
            getchar(); // 捕获换行符

            // 输入到达机场ID
            printf("请输入到达机场ID：");
            if (scanf("%d", &arrAirport) != 1)
            {
                printf("无效的输入。\n");
                while (getchar() != '\n')
                    ; // 清除输入缓冲区
            }
            getchar(); // 捕获换行符

            int choice = 0;
            printf("按照最短时间还是最短价格？（最短时间输入1，最少价格输入2）\n");
            scanf("%d", &choice);
            if (choice == 1)
            {
                allPathTime(depAirport, arrAirport);
            }
            else if (choice == 2)
            {
                allPathMoney(depAirport, arrAirport);
            }
            else
            {
                printf("错误的输入，请输入1或2。");
            }
        }
        else if (choice == 11)
        {
            initGraph(&flightList);
            int depAirport, arrAirport, N;

            // 输入起飞机场ID
            printf("请输入起飞机场ID：");
            if (scanf("%d", &depAirport) != 1)
            {
                printf("无效的输入。\n");
                while (getchar() != '\n')
                    ; // 清除输入缓冲区
            }
            getchar(); // 捕获换行符

            // 输入到达机场ID
            printf("请输入到达机场ID：");
            if (scanf("%d", &arrAirport) != 1)
            {
                printf("无效的输入。\n");
                while (getchar() != '\n')
                    ; // 清除输入缓冲区
            }
            getchar(); // 捕获换行符

            // 输入最多转机次数
            printf("请输入最多转机次数：");
            if (scanf("%d", &N) != 1)
            {
                printf("无效的输入。\n");
                while (getchar() != '\n')
                    ; // 清除输入缓冲区
            }
            getchar(); // 捕获换行符

            time_t TimeDep, TimeArr;
            while (1)
            {
                char lowDateD[12], lowTimeD[7], highDateD[12], highTimeD[7];

                printf("请输入起飞开始日期 (YYYY-MM-DD): ");
                fgets(lowDateD, sizeof(lowDateD), stdin);
                lowDateD[strcspn(lowDateD, "\n")] = '\0';

                printf("请输入起飞开始时间 (HH:MM): ");
                fgets(lowTimeD, sizeof(lowTimeD), stdin);
                lowTimeD[strcspn(lowTimeD, "\n")] = '\0';

                printf("请输入降落截至日期 (YYYY-MM-DD): ");
                fgets(highDateD, sizeof(highDateD), stdin);
                highDateD[strcspn(highDateD, "\n")] = '\0';

                printf("请输入降落截至时间 (HH:MM): ");
                fgets(highTimeD, sizeof(highTimeD), stdin);
                highTimeD[strcspn(highTimeD, "\n")] = '\0';

                // 转换为 time_t
                TimeDep = convertToTimeT(lowDateD, lowTimeD);
                TimeArr = convertToTimeT(highDateD, highTimeD);

                if (TimeDep == (time_t)-1 || TimeArr == (time_t)-1)
                {
                    printf("输入错误：日期或时间格式不正确。请重新输入。\n");
                    continue;
                }

                if (difftime(TimeArr, TimeDep) < 0)
                {
                    printf("输入错误：截至日期和时间不能早于开始日期和时间。请重新输入。\n");
                    continue;
                }

                break; // 输入合法，退出循环
            }
            
            // 输出最短时间选项
            printf("输出最短时间选项\n");
            xiaoTime(depAirport,arrAirport,N,TimeDep,TimeArr);

            // 输出最少价格选项
            printf("输出最少价格选项\n");
            xiaoMoney(depAirport,arrAirport,N,TimeDep,TimeArr);
        }
        
        else
        {
            printf("无效的选择，请输入1-5。\n");
        }
    }

    // 释放链表内存
    FlightNode *current = flightList.HeadNode.next;
    while (current)
    {
        FlightNode *temp = current;
        current = current->next;
        free(temp);
    }

    return 0;
}
