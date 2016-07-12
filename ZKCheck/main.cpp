/*
 * ZKcheck 是一款专门针对ZK系统开发的C/C++代码补充检测软件。
 * Copyright (C) 2016-2017 华中科技大学现代数据工程与实时计算实验室 & 中国船舶重工集团公司第709研究所。
 */

#include "zkcheckexecutor.h"

#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
static char exename[1024] = {0};
#endif

/**
 * ZKcheck 入口main函数
 * @return ZKCheckExecutor::check()的返回值
 */
int main(int argc, char* argv[])
{
// MS Visual C++ 内存泄漏调试跟踪
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif

    ZKCheckExecutor exec;
#ifdef _WIN32
    GetModuleFileNameA(nullptr, exename, sizeof(exename) / sizeof(exename[0])-1);
    argv[0] = exename;
#endif

#ifdef NDEBUG
    try {
#endif
       std::cout << exec.check(argc, argv) << std::endl;
	   system("pause");
	   return 0;
#ifdef NDEBUG
    } catch (const InternalError& e) {
        std::cout << e.errorMessage << std::endl;
    } catch (const std::exception& error) {
        std::cout << error.what() << std::endl;
    } catch (...) {
        std::cout << "Unknown exception" << std::endl;
    }
    return EXIT_FAILURE;
#endif
}

