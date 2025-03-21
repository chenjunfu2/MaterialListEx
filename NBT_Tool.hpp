#pragma once

#include "NBT_Node.hpp"

class NBT_Tool
{
private:
	NBT_Node nRoot{ NBT_Node::NBT_Compound{} };//Ĭ�Ͽսڵ�

	// ��ȡ�������ֵ
	template<typename T>
	static bool ReadBigEndian(const std::string &data, size_t &szCurrent, T& tVal)
	{
		if (szCurrent + sizeof(T) >= data.size())
		{
			return false;
		}

		if constexpr (sizeof(T) == 1)
		{
			tVal = (T)(uint8_t)data[szCurrent++];
		}
		else
		{
			T tTmp = 0;
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				tTmp = (tTmp << 8) | (T)(uint8_t)(data[szCurrent++]);//��Ϊֻ�����ƣ��������з��ŵ��µ�����λ��bug������ת��Ϊ�޷�������
			}
			tVal = tTmp;
		}

		return true;
	}

	// ��ȡ�������ֵ�����ٰ棩����������Ҫȷ��data��Χ��ȫ��
	template<typename T>
	static inline void FastReadBigEndian(const std::string &data, size_t &szCurrent, T &tVal)
	{
		if constexpr (sizeof(T) == 1)
		{
			tVal = (T)(uint8_t)data[szCurrent++];
		}
		else
		{
			T tTmp = 0;
			for (size_t i = 0; i < sizeof(T); ++i)
			{
				tTmp = (tTmp << 8) | (T)(uint8_t)(data[szCurrent++]);//��Ϊֻ�����ƣ��������з��ŵ��µ�����λ��bug������ת��Ϊ�޷�������
			}
			tVal = tTmp;
		}
	}

	enum ErrCode
	{
		Compound_End = 1,//����
		AllOk = 0,//û������
		OutOfRange = -1,
		TypeError = -2,
		WhatTheFuck = -1145,//���������ܵĴ���
	};


	static int GetName(const std::string &data, size_t &szCurrent, std::string &sName)
	{
		//��ȡ2�ֽڵ��޷������Ƴ���
		uint16_t wNameLength = 0;//w->word=2*byte
		if (!ReadBigEndian(data, szCurrent, wNameLength))
		{
			return OutOfRange;
		}

		//�жϳ����Ƿ񳬹�
		if (szCurrent + wNameLength >= data.size())
		{
			return OutOfRange;
		}

		//����������
		 sName = { data.begin() + szCurrent, data.begin() + (szCurrent + wNameLength) };//�������Ϊ0����0���ַ������Ϸ���Ϊ
		 szCurrent += wNameLength;//�ƶ��±�
		 return AllOk;
	}


	template<typename T, bool bHasName = true>
	static int GetbuiltInType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//��ȡNBT��N�����ƣ�
		std::string sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(data, szCurrent, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ȡ����
		if constexpr (std::is_same<T, NBT_Node::NBT_Float>::value)//����������
		{
			uint32_t tTmpData = 0;
			if (!ReadBigEndian(data, szCurrent, tTmpData))
			{
				return OutOfRange;
			}

			if constexpr (bHasName)
			{
				//����-�ں����ݵĽڵ���뵱ǰ����ջ��ȵĸ��ڵ�
				nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ (*((NBT_Node::NBT_Float *)&tTmpData)) });//������������ת��
			}
			else
			{
				//������Ϊ�б�Ԫ�أ�ֱ���޸�nRoot
				nRoot = NBT_Node{ (*((NBT_Node::NBT_Float *)&tTmpData)) };
			}
			
		}
		else if constexpr (std::is_same<T, NBT_Node::NBT_Double>::value)//����������
		{
			uint64_t tTmpData = 0;
			if (!ReadBigEndian(data, szCurrent, tTmpData))
			{
				return OutOfRange;
			}

			if constexpr (bHasName)
			{
				//����-�ں����ݵĽڵ���뵱ǰ����ջ��ȵĸ��ڵ�
				nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ (*((NBT_Node::NBT_Double *)&tTmpData)) });//������������ת��
			}
			else
			{
				//������Ϊ�б�Ԫ�أ�ֱ���޸�nRoot
				nRoot = NBT_Node{ (*((NBT_Node::NBT_Double *)&tTmpData)) };
			}
		}
		else if constexpr (std::is_integral<T>::value)
		{
			T tTmpData = 0;
			if (!ReadBigEndian(data, szCurrent, tTmpData))
			{
				return OutOfRange;
			}

			if constexpr (bHasName)
			{
				//����-�ں����ݵĽڵ���뵱ǰ����ջ��ȵĸ��ڵ�
				nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ tTmpData });
			}
			else
			{
				nRoot = NBT_Node{ tTmpData };
			}
		}
		else
		{
			static_assert(false, "Not a legal type call!");//�׳��������
		}

		return AllOk;
	}

	template<typename T>
	struct is_std_vector : std::false_type
	{};

	template<typename T, typename Alloc>
	struct is_std_vector<std::vector<T, Alloc>> : std::true_type
	{};

	template<typename T, bool bHasName = true>
	static int GetArrayType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//��ȡNBT��N�����ƣ�
		std::string sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(data, szCurrent, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ȡ4�ֽ��з���������������Ԫ�ظ���
		int32_t dwElementCount = 0;//dw->double-word=4*byte
		if (!ReadBigEndian(data, szCurrent, dwElementCount))
		{
			return OutOfRange;
		}

		//�жϳ����Ƿ񳬹�
		if (szCurrent + dwElementCount * sizeof(T) >= data.size())//��֤�·����ð�ȫ
		{
			return OutOfRange;
		}

		//�ж��ǲ���vector<x>
		if constexpr (!is_std_vector<T>::value)
		{
			static_assert(false, "Not a legal type call!");//�׳��������
		}
		
		//���鱣��
		T tArray;
		tArray.reserve(dwElementCount);//��ǰ����

		//��ȡdElementCount��Ԫ��
		for (int32_t i = 0; i < dwElementCount; ++i)
		{
			typename T::value_type tTmpData;
			FastReadBigEndian(data, szCurrent, tTmpData);//������Ҫȷ����Χ��ȫ
			tArray.emplace_back(tTmpData);//��ȡһ������һ��
		}
		
		if constexpr (bHasName)
		{
			//��ɺ����
			nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ std::move(tArray) });
		}
		else//�����ƣ�Ϊ�б�Ԫ��
		{
			nRoot = NBT_Node{ std::move(tArray) };
		}
		return AllOk;
	}

	template<bool bHasName = true>
	static int GetCompoundType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//��ȡNBT��N�����ƣ�
		std::string sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(data, szCurrent, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ʼ�ݹ�
		NBT_Node nodeTemp{ NBT_Node::NBT_Compound{} };
		int iRet = GetNBT(data, szCurrent, nodeTemp);
		if (iRet < AllOk)
		{
			return iRet;
		}

		if constexpr (bHasName)
		{
			//�ݹ���ɣ������ӽڵ��ѵ�λ
			//ȡ��NBT_Compound�ҵ��Լ��������ƶ���
			nRoot.GetData<NBT_Node::NBT_Compound>().emplace(sName, NBT_Node{ std::move(nodeTemp.GetData<NBT_Node::NBT_Compound>()) });
		}
		else//�����ƣ�Ϊ�б�Ԫ��
		{
			nRoot = NBT_Node{ std::move(nodeTemp.GetData<NBT_Node::NBT_Compound>()) };
		}

		return AllOk;
	}

	template<bool bHasName = true>
	static int GetStringType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//��ȡNBT��N�����ƣ�
		std::string sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(data, szCurrent, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ȡ2�ֽڵ��޷������Ƴ���
		uint16_t wStrLength = 0;//w->word=2*byte
		if (!ReadBigEndian(data, szCurrent, wStrLength))
		{
			return OutOfRange;
		}

		//�жϳ����Ƿ񳬹�
		if (szCurrent + wStrLength >= data.size())
		{
			return OutOfRange;
		}

		if constexpr (bHasName)
		{
			//ԭλ����
			nRoot.GetData<NBT_Node::NBT_Compound>().emplace(std::move(sName), NBT_Node{ std::string{data.begin() + szCurrent, data.begin() + (szCurrent + wStrLength)} });
		}
		else//�б�Ԫ��ֱ�Ӹ�ֵ
		{
			nRoot = NBT_Node{ std::string{data.begin() + szCurrent, data.begin() + (szCurrent + wStrLength)} };
		}
		szCurrent += wStrLength;//�ƶ��±�
		
		return AllOk;
	}

	template<bool bHasName = true>
	static int GetListType(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)
	{
		//��ȡNBT��N�����ƣ�
		std::string sName{};
		if constexpr (bHasName)//�����������stringĬ��Ϊ��
		{
			int iRet = GetName(data, szCurrent, sName);
			if (iRet < AllOk)
			{
				return iRet;
			}
		}

		//��ȡ1�ֽڵ��б�Ԫ������
		uint8_t bListElementType = 0;//b=byte
		if (!ReadBigEndian(data, szCurrent, bListElementType))
		{
			return OutOfRange;
		}


		//��ȡ4�ֽڵ��з����б���
		int32_t dwListLength = 0;//dw=double-world=4*byte
		if (!ReadBigEndian(data, szCurrent, dwListLength))
		{
			return OutOfRange;
		}

		//����Ԫ�����ͣ���ȡn���б�
		NBT_Node::NBT_List tmpList;
		for (int32_t i = 0; i < dwListLength; ++i)
		{
			NBT_Node tmpNode{};//�б�Ԫ�ػ�ֱ�Ӹ�ֵ�޸�
			int iRet = SwitchNBT<false>(data, szCurrent, tmpNode, (NBT_Node::NBT_TAG)bListElementType);

			if (iRet != AllOk)
			{
				return iRet;
			}

			//ÿ��ȡһ���������һ��
			tmpList.emplace_back(std::move(tmpNode));
		}

		//�б��Ƕ�ף����Դ�����Ƕ���������
		if constexpr (bHasName)
		{
			nRoot.GetData<NBT_Node::NBT_Compound>().emplace(sName, NBT_Node{ std::move(tmpList) });
		}
		else//�б��е��б�ֱ�Ӹ�ֵ���������в���
		{
			nRoot = NBT_Node{ std::move(tmpList) };
		}

		return AllOk;
	}

	template<bool bHasName = true>
	static inline int SwitchNBT(const std::string &data, size_t &szCurrent, NBT_Node &nRoot, NBT_Node::NBT_TAG tag)
	{
		if (szCurrent >= data.size() && tag != NBT_Node::TAG_End)
		{
			return OutOfRange;
		}

		int iRet = AllOk;

		switch (tag)
		{
		case NBT_Node::TAG_End:
			{
				iRet = Compound_End;
			}
			break;
		case NBT_Node::TAG_Byte:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Byte, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Short:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Short, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Int:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Int, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Long:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Long, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Float:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Float, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Double:
			{
				iRet = GetbuiltInType<NBT_Node::NBT_Double, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Byte_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Byte_Array, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_String:
			{
				iRet = GetStringType<bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_List://��Ҫ�ݹ���ã��б�ͷ������ǩID�ͳ��ȣ�������Ϊһϵ��ͬ���ͱ�ǩ����Ч���أ��ޱ�ǩ ID �����ƣ�
			{//���
				iRet = GetListType<bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Compound://��Ҫ�ݹ����
			{
				iRet = GetCompoundType<bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Int_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Int_Array, bHasName>(data, szCurrent, nRoot);
			}
			break;
		case NBT_Node::TAG_Long_Array:
			{
				iRet = GetArrayType<NBT_Node::NBT_Long_Array, bHasName>(data, szCurrent, nRoot);
			}
			break;
		default:
			{
				iRet = WhatTheFuck;//�������ô���ֵģ�
			}
			break;
		}

		return iRet;
	}

	static int GetNBT(const std::string &data, size_t &szCurrent, NBT_Node &nRoot)//�ݹ���ö�ȡ����ӽڵ�
	{
		//�ڵ����ͼ�飺��֤��ǰnRoot��NBT_Node::NBT_Compound���ͣ�����ʧ��
		if (!nRoot.TypeHolds<NBT_Node::NBT_Compound>())//���ʹ���
		{
			return TypeError;
		}

		int iRet;
		do
		{
			iRet = SwitchNBT(data, szCurrent, nRoot, (NBT_Node::NBT_TAG)(uint8_t)data[szCurrent++]);
		} while (iRet == AllOk);
		
		return iRet;
	}
public:
	NBT_Tool(void) = default;
	NBT_Tool(const std::string &data)
	{
		GetNBT(data);
	}
	~NBT_Tool(void) = default;

	bool GetNBT(const std::string &data)
	{
		nRoot.Clear();//�������
		size_t szCurrent{ 0 };
		return GetNBT(data, szCurrent, nRoot) == Compound_End;
	}

	NBT_Node& GetRoot(void)
	{
		return nRoot;
	}

	const NBT_Node &GetRoot(void) const
	{
		return nRoot;
	}

	void Print(void) const
	{
		PrintSwitch(nRoot, 0);
	}

private:
	void PrintSwitch(const NBT_Node &nRoot, int iLevel) const
	{
		switch (nRoot.GetTag())
		{
		case NBT_Node::TAG_End:
			{
				printf("[Compound End]");
			}
			break;
		case NBT_Node::TAG_Byte:
			{
				printf("%db", nRoot.GetData<NBT_Node::NBT_Byte>());
			}
			break;
		case NBT_Node::TAG_Short:
			{
				printf("%ds", nRoot.GetData<NBT_Node::NBT_Short>());
			}
			break;
		case NBT_Node::TAG_Int:
			{
				printf("%d", nRoot.GetData<NBT_Node::NBT_Int>());
			}
			break;
		case NBT_Node::TAG_Long:
			{
				printf("%lldl", nRoot.GetData<NBT_Node::NBT_Long>());
			}
			break;
		case NBT_Node::TAG_Float:
			{
				printf("%ff", nRoot.GetData<NBT_Node::NBT_Float>());
			}
			break;
		case NBT_Node::TAG_Double:
			{
				printf("%lff", nRoot.GetData<NBT_Node::NBT_Double>());
			}
			break;
		case NBT_Node::TAG_Byte_Array:
			{
				auto &arr = nRoot.GetData<NBT_Node::NBT_Byte_Array>();
				printf("[B;");
				for (auto &it : arr)
				{
					printf("%d,", it);
				}
				if (arr.size() != 0)
				{
					printf("\b");
				}

				printf("]");
			}
			break;
		case NBT_Node::TAG_String:
			{
				printf("\"%s\"", nRoot.GetData<NBT_Node::NBT_String>().c_str());
			}
			break;
		case NBT_Node::TAG_List:
			{
				auto &list = nRoot.GetData<NBT_Node::NBT_List>();
				printf("[");
				for (auto &it : list)
				{
					PrintSwitch(it, ++iLevel);
					printf(",");
				}

				if (list.size() != 0)
				{
					printf("\b");
				}
				printf("]");
			}
			break;
		case NBT_Node::TAG_Compound:
			{
				auto &cpd = nRoot.GetData<NBT_Node::NBT_Compound>();
				printf("{");

				for (auto &it : cpd)
				{
					printf("\"%s\":", it.first.c_str());
					PrintSwitch(it.second, ++iLevel);
					printf(",");
				}

				if (cpd.size() != 0)
				{
					printf("\b");
				}
				printf("}");
			}
			break;
		case NBT_Node::TAG_Int_Array:
			{
				auto &arr = nRoot.GetData<NBT_Node::NBT_Int_Array>();
				printf("[I;");
				for (auto &it : arr)
				{
					printf("%d,", it);
				}

				if (arr.size() != 0)
				{
					printf("\b");
				}
				printf("]");
			}
			break;
		case NBT_Node::TAG_Long_Array:
			{
				auto &arr = nRoot.GetData<NBT_Node::NBT_Long_Array>();
				printf("[L;");
				for (auto &it : arr)
				{
					printf("%lld,", it);
				}

				if (arr.size() != 0)
				{
					printf("\b");
				}
				printf("]");
			}
			break;
		default:
			{
				printf("[Unknow Type]");
			}
			break;
		}
	}
};