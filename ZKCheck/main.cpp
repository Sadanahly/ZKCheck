/*
 * ZKcheck ��һ��ר�����ZKϵͳ������C/C++���벹���������
 * Copyright (C) 2016-2017 ���пƼ���ѧ�ִ����ݹ�����ʵʱ����ʵ���� & �й������ع����Ź�˾��709�о�����
 */

#include "zkcheckexecutor.h"

#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
static char exename[1024] = {0};
#endif

/**
 * ZKcheck ���main����
 * @return ZKCheckExecutor::check()�ķ���ֵ
 */
int main(int argc, char* argv[])
{
// MS Visual C++ �ڴ�й©���Ը���
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

