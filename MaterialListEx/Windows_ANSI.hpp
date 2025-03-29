#pragma once

#include <Windows.h>
#include <stdio.h>
#include <string>

template<typename U16T = char16_t>
std::basic_string<char> ConvertUtf16ToAnsi(std::basic_string<U16T> u16String)
{
	static_assert(sizeof(U16T) == sizeof(wchar_t), "U16T size must be same as wchar_t size");

	if (u16String.size() == 0)
	{
		return std::basic_string<char>{};//���ؿ��ַ���
	}

	// ��ȡת��������Ļ�������С��������ֹ����
	int sizeNeeded = WideCharToMultiByte(//ע��˺���u16�����ַ�������u8�����ֽ���
		CP_ACP,                // ʹ�õ�ǰANSI����ҳ
		WC_NO_BEST_FIT_CHARS,  // �滻�޷�ֱ��ӳ����ַ�
		(wchar_t*)u16String.data(),//static_assert��֤�ײ��С��ͬ
		u16String.size(),//���������С����ת�����������\0
		NULL,
		0,
		NULL,
		NULL
	);

	if (sizeNeeded == 0)
	{
		//ERROR_INSUFFICIENT_BUFFER//�� �ṩ�Ļ�������С�����󣬻��ߴ��������Ϊ NULL��
		//ERROR_INVALID_FLAGS//�� Ϊ��־�ṩ��ֵ��Ч��
		//ERROR_INVALID_PARAMETER//�� �κβ���ֵ����Ч��
		//ERROR_NO_UNICODE_TRANSLATION//�� ���ַ����з�����Ч�� Unicode��
		
		printf("\nWideCharToMultiByte failed. Error code: %lu\n", GetLastError());
		return std::basic_string<char>{};
	}

	//����string��Ԥ�����С
	std::basic_string<char> ansiString;
	ansiString.resize(sizeNeeded);

	// ִ��ʵ��ת��
	int convertedSize = WideCharToMultiByte(
		CP_ACP,
		WC_NO_BEST_FIT_CHARS,
		(wchar_t *)u16String.data(),
		u16String.size(),
		ansiString.data(),
		ansiString.size(),
		NULL,
		NULL
	);

	if (convertedSize == 0)
	{
		printf("\nWideCharToMultiByte failed. Error code: %lu\n", GetLastError());
		return std::basic_string<char>{};
	}

	return ansiString;
}

#define ANSISTR(u16str) ConvertUtf16ToAnsi(u16str)