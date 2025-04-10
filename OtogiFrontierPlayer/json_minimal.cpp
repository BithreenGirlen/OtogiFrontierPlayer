/*Minimal JSON extractor.*/

#include <string.h>
#include <malloc.h>

#include "json_minimal.h"

namespace json_minimal
{

/*JSON特性体の抽出*/
bool ExtractJsonObject(char** src, const char* name, char** dst)
{
	char* p = nullptr;
	char* pp = *src;
	char* q = nullptr;
	char* qq = nullptr;
	size_t nLen = 0;
	int iCount = 0;

	if (name != nullptr)
	{
		p = strstr(pp, name);
		if (p == nullptr)return false;

		pp = strchr(p, ':');
		if (pp == nullptr)return false;
	}
	else
	{
		p = strchr(pp, '{');
		if (p == nullptr)return false;
		++iCount;
		pp = p + 1;
	}

	for (;;)
	{
		q = strchr(pp, '}');
		if (q == nullptr)return false;

		qq = strchr(pp, '{');
		if (qq == nullptr)break;

		if (q < qq)
		{
			--iCount;
			pp = q + 1;
		}
		else
		{
			++iCount;
			pp = qq + 1;
		}

		if (iCount == 0)break;
	}

	for (; iCount > 0; ++q)
	{
		if (*q == '}')
		{
			--iCount;
		}
	}
	++q;

	nLen = q - p;
	char* pBuffer = static_cast<char*>(malloc(nLen + 1));
	if (pBuffer == nullptr)return false;

	memcpy(pBuffer, p, nLen);
	*(pBuffer + nLen) = '\0';
	*dst = pBuffer;
	*src = q;

	return true;
}
/*JSON配列の抽出*/
bool ExtractJsonArray(char** src, const char* name, char** dst)
{
	char* p = nullptr;
	char* pp = *src;
	char* q = nullptr;
	char* qq = nullptr;
	size_t nLen = 0;
	int iCount = 0;

	if (name != nullptr)
	{
		p = strstr(pp, name);
		if (p == nullptr)return false;

		pp = strchr(p, ':');
		if (pp == nullptr)return false;
	}
	else
	{
		p = strchr(pp, '[');
		if (p == nullptr)return false;
		++iCount;
		pp = p + 1;
	}

	for (;;)
	{
		q = strchr(pp, ']');
		if (q == nullptr)return false;

		qq = strchr(pp, '[');
		if (qq == nullptr)break;

		if (q < qq)
		{
			--iCount;
			pp = q + 1;
		}
		else
		{
			++iCount;
			pp = qq + 1;
		}

		if (iCount == 0)break;
	}

	for (; iCount > 0; ++q)
	{
		if (*q == ']')
		{
			--iCount;
		}
	}
	++q;

	nLen = q - p;
	char* pBuffer = static_cast<char*>(malloc(nLen + 1));
	if (pBuffer == nullptr)return false;

	memcpy(pBuffer, p, nLen);
	*(pBuffer + nLen) = '\0';
	*dst = pBuffer;
	*src = q;

	return true;
}
/*JSON区切り位置探索*/
char* FindJsonValueEnd(char* src)
{
	const char ref[] = ",}\"]";
	return strpbrk(src, ref);
}
/*JSON要素の値を取得*/
bool GetJsonElementValue(char* src, const char* name, char* dst, size_t nDstSize, int* iDepth, char** pEnd)
{
	char* p = nullptr;
	char* pp = src;
	size_t nLen = 0;

	p = strstr(pp, name);
	if (p == nullptr)return false;

	pp = strchr(p, ':');
	if (pp == nullptr)return false;
	++pp;

	p = FindJsonValueEnd(pp);
	if (p == nullptr)return false;
	if (*p == '"')
	{
		pp = p + 1;
		p = strchr(pp, '"');
		if (p == nullptr)return false;
	}
	else
	{
		for (; *pp == ' '; ++pp);
		for (char* q = p - 1;; --q)
		{
			if (*q != ' ' && *q != '\r' && *q != '\n' && *q != '\t')break;
			p = q;
		}
	}

	nLen = p - pp;
	if (nLen > nDstSize - 1)return false;
	memcpy(dst, pp, nLen);
	*(dst + nLen) = '\0';

	/*入れ子の要素であればiDepth > 0*/
	if (iDepth != nullptr && *pEnd != nullptr)
	{
		*pEnd = p + 1;

		char* q = nullptr;
		char* qq = nullptr;
		pp = src;

		for (;;)
		{
			q = strchr(pp, '}');
			if (q == nullptr)break;

			qq = strchr(pp, '{');
			if (qq == nullptr)break;

			if (q < qq)
			{
				--(*iDepth);
				pp = q + 1;
			}
			else
			{
				++(*iDepth);
				pp = qq + 1;
			}

			if (pp > p)break;
		}
	}

	return true;
}
/*JSON変数名開始位置探索*/
char* FindJsonNameStart(char* src)
{
	const char ref[] = " :{[,";
	for (char* p = src; p != nullptr; ++p)
	{
		bool b = false;
		/*終端除外*/
		for (size_t i = 0; i < sizeof(ref) - 1; ++i)
		{
			if (*p == ref[i])
			{
				b = true;
			}
		}
		if (!b)return p;
	}

	return nullptr;
}
/*JSON対要素読み取り*/
bool ReadNextKey(char** src, char* key, size_t nKeySize, char* value, size_t nValueSize)
{
	char* p = nullptr;
	char* pp = *src;
	size_t nLen = 0;

	p = FindJsonNameStart(pp);
	if (p == nullptr)return false;
	if (*p == '"')
	{
		++p;
		pp = strchr(p, '"');
		if (pp == nullptr)return false;
	}
	else
	{
		pp = strchr(p, ':');
		if (pp == nullptr)return false;
	}

	nLen = pp - p;
	if (nLen > nKeySize - 1)return false;
	memcpy(key, p, nLen);
	*(key + nLen) = '\0';

	++pp;
	p = FindJsonValueEnd(pp);
	if (p == nullptr)return false;
	if (*p == '"')
	{
		pp = p + 1;
		p = strchr(pp, '"');
		if (p == nullptr)return false;
	}

	nLen = p - pp;
	if (nLen > nValueSize - 1)return false;
	memcpy(value, pp, nLen);
	*(value + nLen) = '\0';
	*src = p + 1;

	return true;
}
/*次の配列要素読み取り*/
bool ReadNextArrayValue(char** src, char* dst, size_t nDstSize)
{
	char* p = nullptr;
	char* pp = *src;
	size_t nLen = 0;

	p = FindJsonNameStart(pp);
	if (p == nullptr)return false;
	if (*p == '"')
	{
		++p;
		pp = strchr(p, '"');
		if (pp == nullptr)return false;
	}
	else if (*p == ']' || *p == '\0')
	{
		return false;
	}
	else
	{
		pp = FindJsonValueEnd(p);
		if (pp == nullptr)return false;
	}

	nLen = pp - p;
	if (nLen > nDstSize - 1)return false;
	memcpy(dst, p, nLen);
	*(dst + nLen) = '\0';
	*src = *pp == '"' ? pp + 1: pp;

	return true;
}
/*名称終わり位置まで読み進め*/
bool ReadUpToNameEnd(char** src, const char* name, char* value, size_t nValueSize)
{
	char* p = nullptr;
	char* pp = *src;

	if (name != nullptr)
	{
		p = strstr(pp, name);
		if (p == nullptr)return false;
	}
	else
	{
		p = FindJsonNameStart(pp);
		if (p == nullptr)return false;
		++p;
	}

	pp = FindJsonValueEnd(p);
	if (pp == nullptr)return false;

	/*名称取得*/
	if (name == nullptr && value != nullptr && nValueSize != 0)
	{
		size_t nLen = pp - p;
		if (nLen > nValueSize - 1)return false;
		memcpy(value, p, nLen);
		*(value + nLen) = '\0';
	}

	*src = *pp == '"' ? pp + 1 : pp;

	return true;
}

} // namespace json_minimal
