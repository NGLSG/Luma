/**
 * @brief 包含各种机器人（Bot）实现类的聚合头文件。
 *
 * 这个头文件汇集了不同AI模型（如ChatGPT, Claude, Gemini, LLama）的实现，
 * 以及自定义规则（CustomRule）的实现，方便统一引用。
 */
#ifndef BOTS_H
#define BOTS_H
#include "ChatGPT_Impl.h"
#include "Claude_Impl.h"
#include "Gemini_Impl.h"
#include "LLama_Impl.h"
#include "CustomRule_Impl.h"
#endif